# Plano de refatoração — CTRComposer

> Resposta ao code review feito após o feedback público de **biometrix76** (autor do
> Gen6CTRPluginFramework). Cada etapa é auto-contida: abra uma sessão nova, aponte para este
> arquivo, diga "execute a Etapa N". Não precisa do histórico da conversa que gerou o plano.

---

## O que o revisor disse, e o que foi confirmado

| Crítica | Verificado? |
|---|---|
| "foundational setup mashed into main.c" | **Sim.** 24 módulos lógicos num arquivo de 5.582 linhas. |
| "hard to navigate as a template" | **Sim.** O que o dev precisa editar está espalhado em 7 regiões, separadas por milhares de linhas. |
| "too much commenting (something AI typically does)" | **Parcialmente.** 12% de comentário puro é normal; o problema são 8 blocos de 10+ linhas (o maior com 27) e 353 comentários em fim de linha. |
| "can't say anything about stability/bugs" | **Ele não podia — mas há 1 bug real de travamento.** Ver Etapa 2. |

**Um ponto de justiça:** o problema **não é o tamanho do arquivo**. O `Search.cpp` do próprio
Gen6 tem 5.375 linhas. O problema é o que está *misturado*: motor e área-do-usuário no mesmo
arquivo, sem fronteira.

---

## A descoberta que torna isso seguro

A refatoração parecia arriscada (313 `static` compartilhados). **Não é** — se for feita por
`#include` em vez de módulos de compilação separados.

Isso foi **testado e provado** antes deste plano ser escrito: extrair uma seção do `main.c`
para `Sources/engine/toast.inc.c` e substituí-la por `#include "engine/toast.inc.c"` produziu um
`.3gx` **byte a byte idêntico**. Mesmas 441 funções, mesmos tamanhos.

Por quê: o pré-processador cola os arquivos de volta antes do compilador ver. Continua sendo
uma unidade de compilação só, os `static` continuam `static`, o compilador gera exatamente o
mesmo código. Você ganha navegabilidade com **risco zero**, e consegue *provar* isso com `cmp`.

O custo: é uma técnica menos convencional que headers + módulos separados. A Etapa 7 (opcional)
converte para o modelo tradicional, se algum dia fizer sentido.

---

## Etapa 0 — Rede de segurança *(fazer primeiro, sempre)*

**Modelo sugerido: Sonnet** — mecânico, sem julgamento.

Sem isto, nenhuma etapa seguinte é verificável.

1. Adicionar ao fim do `Makefile`:
   ```make
   .PRECIOUS: %.elf
   ```
   Sem isso o `.elf` é apagado no fim do build e não dá para inspecionar os símbolos.

2. Criar `Tools/fingerprint.sh`:
   ```sh
   #!/bin/sh
   # Impressao digital do binario: nome + tamanho de cada funcao, SEM enderecos.
   # Se ela nao muda, o compilador gerou o mesmo codigo.
   arm-none-eabi-nm --print-size --defined-only "$1" | awk '{print $NF, $2}' | sort
   ```

3. Guardar a referência antes de qualquer mudança:
   ```sh
   make clean && make
   cp CTRComposer-BlankTemplate.3gx /tmp/base.3gx
   sh Tools/fingerprint.sh CTRComposer-BlankTemplate.elf > /tmp/base.fp
   ```

**Como verificar depois de cada etapa:**
```sh
make clean && make
cmp CTRComposer-BlankTemplate.3gx /tmp/base.3gx && echo "IDENTICO"
diff /tmp/base.fp <(sh Tools/fingerprint.sh CTRComposer-BlankTemplate.elf)
```

> ⚠️ O binário deve ser **byte a byte idêntico** nas Etapas 3 e 4. Se não for, algo de verdade
> mudou — pare e investigue, não siga em frente.

---

## Etapa 1 — Bug de travamento por falta de memória 🔴

**Modelo sugerido: Sonnet** — correção pequena e bem especificada.
**Risco: baixo. Prioridade: máxima** (é o único bug que derruba o console).

No arranque (`ThreadMain`, ~L5468) o plugin reserva três blocos:

```c
gCompose = (u8 *)malloc(TOP_W * TOP_H * 3);   // 288 KB
savedBot = (u16 *)malloc(BOT_W * BOT_H * 2);  // 150 KB
savedTop = (u16 *)malloc(TOP_W * TOP_H * 2);  // 192 KB
```

Nenhum é checado. O `RunMenu` confere só o `gCompose`. Resultado: se o **segundo** falhar, o
`RunMenu` passa pela checagem, chama `BotGrab()`, e o `BotGrab` escreve 150 KB em
`savedBot[...]` — endereço nulo. **Travamento imediato.**

**Em linguagem simples:** o programa pede três caixas emprestadas, confere se a primeira chegou,
e depois enche as três. Se a segunda não veio, ele despeja o conteúdo no chão.

Isso só dispara com memória apertada — exatamente o cenário que existiu quando testamos
`MemorySize: 2MiB`.

**O que fazer:**
- Em `ThreadMain`, tratar a falha de qualquer um dos três: se algum for `NULL`, o menu não deve
  abrir (o plugin fica inerte em vez de derrubar o jogo). Um toast avisando seria ideal, mas o
  toast também desenha — então tem que ser degradação silenciosa.
- Em `BotGrab()`, `CaptureTopBackdrop()` e `RestoreTopBackdrop()`: confirmar guarda de `NULL`
  (os dois últimos já têm; `BotGrab` **não**).
- Revisar `ToastTick()`: desenha via `CPix` sem checar `gCompose`. Hoje é inalcançável (o único
  jeito de enfileirar um toast é pelo menu, que já checa), mas é frágil por construção.

**Verificação:** a impressão digital muda **apenas** nas funções tocadas. Nenhuma outra.

---

## Etapa 2 — Enxugar os comentários 🟡

**Modelo sugerido: Opus** — é julgamento, não mecânica. Decidir o que é essencial e o que é
ruído exige entender o código; um modelo mais fraco tende a cortar demais ou de menos.
**Risco: zero** (comentário não vira código). **Oráculo perfeito.**

Responde diretamente ao *"too much commenting and text (something that AI typically does)"*.

**Alvos, em ordem:**

1. **8 blocos de 10+ linhas seguidas** — os maiores estão em L4001 (27 linhas), L14 (19),
   L341 (18), L783 (15), L5485 (14), L1536 (13).
2. **30 blocos de 5+ linhas** (275 linhas no total).
3. **353 comentários em fim de linha de código** — muitos apenas repetem o que a linha diz.

**Critério de corte** (este é o julgamento que justifica o modelo forte):

- **Apagar:** narrativa histórica ("uma versão anterior tentava X e não funcionou porque Y").
  Isso é diário de desenvolvimento — pertence ao histórico do Git ou ao README, não ao código.
  O bloco do `EXIT_HANDSHAKE` (L5485) é o exemplo mais claro.
- **Apagar:** comentário que repete o código (`i++; // incrementa i`).
- **Manter:** restrição que o código não consegue mostrar sozinho. Exemplos que **devem
  sobreviver**: o alerta de que `?:` com as macros de cor gera expressão-vírgula; o "não chame
  `hidInit()`"; o "endereço 0 trava o console"; o "empacote com arredondamento, não truncamento".
  Esses são armadilhas reais, e apagá-los faz alguém repetir o erro.
- **Encurtar:** cabeçalhos de seção — manter o título, cortar o ensaio.

**Verificação — esta é a melhor de todas:**
```sh
cmp CTRComposer-BlankTemplate.3gx /tmp/base.3gx
```
Tem que ser **byte a byte idêntico**. Comentário não afeta a compilação (confirmado: o código
não usa `__LINE__` nem `__FILE__`). Se o binário mudar, código foi apagado por acidente.

---

## Etapa 3 — Separar a área do usuário 🔴 *(a etapa que responde à crítica central)*

**Modelo sugerido: Opus para decidir as fronteiras, depois Sonnet para executar.**
**Risco: zero, com verificação byte a byte.**

Hoje quem quer fazer um plugin precisa editar 7 regiões espalhadas:

| O que editar | Linha |
|---|---|
| Nome e versão | 42 |
| IDs dos cheats | 314 |
| Pasta do jogo (`PLUGIN_DIR`) | 359 |
| Implementação dos cheats | 825, 885 |
| Menus e pastas | 985–1021 |
| Ícones dos cheats | 1592 |
| Dados do tracker | 4039 |

**Objetivo:** tudo isso passa a viver em **um arquivo só**, `Sources/plugin.c`, incluído pelo
`main.c`. O desenvolvedor abre esse arquivo, edita, e nunca precisa olhar o resto.

É exatamente o que o CTRPluginFramework faz: no Gen6, o autor mexe em 5 arquivos (`Codes.cpp`,
`PKHeX.cpp`...) e jamais abre `Library/`.

**Restrição crítica:** a ordem dos `#include` no `main.c` **tem que ser a mesma ordem
top-to-bottom do arquivo original**. É isso que garante que as declarações venham antes dos usos.
Não reordene nada nesta etapa — mover é uma coisa, reordenar é outra.

**Como executar (uma seção por vez, verificando entre cada uma):**
1. Recortar a seção para `Sources/plugin.c`
2. Substituir por `#include "plugin.c"` na posição exata
3. `make` e `cmp` → tem que ser idêntico
4. Só então passar para a próxima

> O `Makefile` compila apenas `Sources/*.c`. Arquivos em `Sources/engine/` **não** são compilados
> sozinhos — é isso que faz o `#include` funcionar sem duplicar símbolos. Se um dia mover um
> `.inc.c` para `Sources/`, ele passa a ser compilado duas vezes e o link quebra.

---

## Etapa 4 — Dividir o motor em módulos 🔴

**Modelo sugerido: Sonnet** — depois que a Etapa 3 definiu o padrão, isto é recorte mecânico com
oráculo perfeito. **Risco: zero, com verificação byte a byte.**

Estrutura alvo:

```
Sources/
  main.c              ← só entrada + orquestração (~150 linhas)
  plugin.c            ← VOCÊ EDITA AQUI (cheats, menus, Title ID)
  engine/
    render.inc.c      ← framebuffer, compose, texto, sprites   (~660 linhas)
    theme.inc.c       ← temas, cores, auto-contraste
    menu.inc.c        ← modelo de itens, desenho, loop          (~950)
    tools.inc.c       ← Cheat Search, RAM Dumper, Hex Editor    (~1200)
    guide.inc.c       ← leitor de guias                         (~400)
    tracker.inc.c     ← tracker de progresso                    (~790)
    storage.inc.c     ← config, favoritos, tradução             (~320)
    platform.inc.c    ← LCD, HID, pause, toque
```

Guia de corte (os números vêm do mapa real do `main.c`):

| Seção atual | Linha | Tamanho | Vai para |
|---|---|---|---|
| LCD registers / compose / framebuffer | 83–306 | 223 | `platform` + `render` |
| Cheat IDs | 314 | 35 | **`plugin.c`** |
| Where files live / config | 341–419 | 78 | `storage` (+ `PLUGIN_DIR` → `plugin.c`) |
| Localization | 419 | 117 | `storage` |
| SD guides | 536 | 162 | `guide` |
| Quick menu hotkey | 698 | 85 | `menu` |
| Cheat implementations | 783 | 134 | **`plugin.c`** |
| Pickers | 917 | 34 | **`plugin.c`** |
| Menu model (folders) | 951 | 259 | **`plugin.c`** (tabelas) + `menu` (macros) |
| Rendering | 1210 | 326 | `render` + `theme` |
| Sprites | 1536 | 191 | `render` |
| Bottom screen / Toast | 1727 | 153 | `render` |
| Menu rendering | 1880 | 524 | `menu` |
| Tools | 2404 | 1200 | `tools` |
| Guides | 3604 | 397 | `guide` |
| Tracker | 4001 | 788 | `tracker` |
| Pause / Menu loop / Quick menu | 4789–5406 | 617 | `menu` |
| Thread / entry | 5406 | 177 | `main.c` |

---

## Etapa 5 — Repensar o `TOOLS_ONLY` 🟡

**Modelo sugerido: Opus** — decisão de design, sem resposta óbvia.
**Risco: médio** (mexe nas duas builds).

Hoje um `#define` liga/desliga blocos `#if` espalhados pelo arquivo inteiro. Funciona — a CI
compila as duas — mas obriga quem lê a simular o pré-processador mentalmente: você nunca vê o
programa que roda, vê os dois sobrepostos.

Depois da Etapa 4 há uma saída mais limpa: se cada ferramenta virou um arquivo, a variante
tools-only pode ser **uma lista diferente de `#include`** em vez de `#if` espalhado. O `main.c`
da build universal simplesmente não inclui `tracker.inc.c` nem `guide.inc.c`.

Fazer **depois** da Etapa 4, nunca antes.

---

## Etapa 6 — Limpezas cosméticas 🟢

**Modelo sugerido: Sonnet.** **Risco: baixo.**

- `PLUGIN_TAG` duplicado (`"1.0"` / `"T1.0"`) mantido à mão em dois lugares — já causou
  divergência entre CI e build local uma vez.
- Comentários que descrevem decisões já revertidas (vai ser em boa parte resolvido na Etapa 2).
- `main.c` guardando coisas que não são "main" (resolvido pelas Etapas 3–4).

---

## Etapa 7 — Módulos de compilação de verdade 🟢 *(opcional, provavelmente desnecessária)*

**Modelo sugerido: Opus.** **Risco: REAL — a única etapa sem oráculo perfeito.**

Converter os `.inc.c` em `.c` de verdade com headers e `extern`.

**Recomendação honesta: não fazer, a menos que alguém peça.** As Etapas 3–4 já resolvem a
crítica (navegabilidade e separação motor/usuário) com risco zero. Esta etapa troca risco por
pureza:

- Os 313 `static` viram globais → o compilador perde oportunidades de *inline*
- As 15 funções que ele hoje clona (`.isra`, `.constprop`) mudam ou somem
- **A impressão digital deixa de bater legitimamente**, então a verificação vira "mesmos nomes de
  função + tamanho parecido + teste manual no console" — bem mais fraca

Só vale se o projeto crescer a ponto de o tempo de compilação incomodar, ou se alguém for
contribuir e reclamar da convenção.

---

## Ordem recomendada

```
Etapa 0  (rede de segurança)   ─┐
Etapa 1  (bug de travamento)    ├─ fazer agora, são rápidas e independentes
Etapa 2  (comentários)         ─┘

        ↓  ← aqui: fazer o plugin do segundo plugin

Etapa 3  (separar plugin.c)    ─┐
Etapa 4  (dividir o motor)      ├─ depois do segundo plugin
Etapa 5  (TOOLS_ONLY)          ─┤
Etapa 6  (cosmético)           ─┘

Etapa 7  → provavelmente nunca
```

**Por que o segundo plugin no meio:** as Etapas 3–4 reorganizam a área que o desenvolvedor edita. Fazer um
plugin real *antes* mostra na prática o que incomoda — e a única metade do projeto que nunca foi
testada de verdade é justamente a de cheats (os exemplos são inertes por construção). O segundo plugin é o
primeiro teste real dela, e vai informar como `plugin.c` deve ficar.

**Alternativa válida:** fazer 3–4 antes do segundo plugin, e já desenvolver o segundo plugin na estrutura nova. Como
a verificação é byte a byte, o risco não é o argumento — o argumento é só não redesenhar o
`plugin.c` sem ter usado ele de verdade uma vez.

---

## Resumo dos modelos

| Etapa | Modelo | Por quê |
|---|---|---|
| 0 · Rede de segurança | **Sonnet** | Mecânico, especificado |
| 1 · Bug de memória | **Sonnet** | Correção pequena e clara |
| 2 · Comentários | **Opus** | Julgamento sobre o que é essencial |
| 3 · Fronteiras | **Opus** decide, **Sonnet** executa | Design vs. recorte |
| 4 · Dividir motor | **Sonnet** | Mecânico + oráculo perfeito |
| 5 · TOOLS_ONLY | **Opus** | Decisão de design |
| 6 · Cosmético | **Sonnet** | Trivial |
| 7 · Módulos reais | **Opus** | Único sem oráculo — se for fazer |

O critério: **onde existe um oráculo automático** (o `cmp` byte a byte), o trabalho é mecânico e
um modelo mais barato dá conta — se ele errar, a verificação pega. **Onde não existe oráculo**
(o que é comentário essencial, onde fica a fronteira), o julgamento é o produto, e vale o modelo
mais forte.

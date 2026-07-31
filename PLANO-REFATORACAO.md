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

## Etapa 5 — Repensar o `TOOLS_ONLY` 🟡 ✅ *(feita — com a premissa corrigida)*

**Modelo sugerido: Opus** — decisão de design, sem resposta óbvia.
**Risco real: baixo** (o oráculo byte a byte continuou valendo; a estimativa "médio" era pessimista).

Hoje um `#define` liga/desliga blocos `#if` espalhados pelo arquivo inteiro. Funciona — a CI
compila as duas — mas obriga quem lê a simular o pré-processador mentalmente: você nunca vê o
programa que roda, vê os dois sobrepostos.

### A proposta original não se sustentou

O plano dizia: *"a variante tools-only pode ser uma lista diferente de `#include` em vez de `#if`
espalhado"*. Ao executar, isso se mostrou **errado como estratégia geral**. Dos 25 pontos
condicionais que existiam, só ~6 eram blocos inteiros e auto-contidos. Os outros ~19 estão
**dentro** de uma construção que um `#include` não alcança:

- membros de `enum` (`F_EXAMPLES`, `T_TRACKER` — mudam o valor de `NUM_TOOLS`)
- `case` dentro de `switch` (`case T_GAMEGUIDE:`)
- elementos de inicializador de array (`kToolKeys[]`, as linhas do About)
- corpos de `#define` (`PLUGIN_NAME`)

Levada ao pé da letra, a proposta **adicionaria** uma condicional (a própria lista de includes)
mantendo 19 das antigas, e espalharia a decisão por dois lugares em vez de um. Seria pior.

### O que foi feito no lugar

Só o subconjunto que de fato funciona — tudo verificado byte a byte:

1. **`engine/guide.inc.c` (397 linhas, 4 `#if`) virou 5 arquivos com zero `#if`.** O leitor
   genérico (`guide_reader`), o Game Guide (`guide_game`), o Plugin Guide (`guide_plugin`) e o
   texto das páginas, agora selecionado por **uma única** condicional no `main.c`.
2. **O texto do guia foi para `Sources/plugin/guide_text.inc.c`** — correção de uma falha da
   Etapa 3: as páginas do Plugin Guide e a página de créditos são conteúdo que o autor do plugin
   *deve* editar, e estavam enterradas no motor.
3. **`ToolRun()` saiu do rabo de `tracker_ui.inc.c`** para `engine/tool_dispatch.inc.c`. Ele é o
   despachante de ferramentas, não código de tracker — era uma emenda ruim da Etapa 4.
4. **As 3 condicionais do tracker viraram 1**, em volta dos `#include` no `main.c`.
   `tracker.inc.c` e `tracker_ui.inc.c` passaram a ser código comum, sem `#if` nenhum.

Resultado: 25 → 21 pontos condicionais. O número caiu pouco, mas **nenhum arquivo do motor tem
mais um `#if` envolvendo o corpo inteiro**, e os dois maiores blocos de dados duplicados saíram
de `engine/`.

### Os 21 que ficaram, e por que ficam

São de 1 a 3 linhas cada, e estão exatamente onde a diferença é. Eliminá-los exigiria um
**registro de ferramentas em runtime** (cada tool se cadastra num `{nome, ícone, run}`, arrays
dimensionados em runtime, `switch` virando busca em tabela). Isso custaria ponteiros de função e
bytes num binário que é carregado dentro da memória do jogo, e **destruiria o oráculo de
verificação** — pela primeira vez uma etapa não poderia provar que não quebrou nada.
Desproporcional para resolver 19 linhas espalhadas. **Recomendação: não fazer.**

---

## Etapa 6 — Limpezas cosméticas 🟢 ✅ *(feita)*

**Modelo sugerido: Sonnet.** **Risco: baixo.**

- **`PLUGIN_TAG` duplicado** — corrigido. `identity.inc.c` agora define `PLUGIN_VER_MAJOR/MINOR/
  PATCH` como a única fonte da versão; `PLUGIN_VER` ("v1.0.1") e `PLUGIN_TAG` ("1.0"/"T1.0", sem
  o dígito de patch por falta de espaço na barra de título) são montados a partir desses três
  números via macro de stringize (`#x`), não mais digitados separadamente. Bump de versão agora é
  editar 1 lugar, não 2 — a divergência que já aconteceu entre CI e build local fica estruturalmente
  impossível, não só lembrada num comentário.
- **Comentário de narrativa histórica residual** — achado 1: `guide_reader.inc.c` ainda tinha
  *"The title used to be hardcoded to 'Game Guide', which meant..."*, sobre um bug corrigido
  depois que a Etapa 2 já tinha rodado. Cortado para só a restrição real (`GuideBottom` recebe o
  título porque é compartilhado pelos dois leitores). Busca ampla por outros padrões similares
  (`was hardcoded`, `used to be`, `had a bug`, `regression`...) não achou mais nenhum.
- **`main.c` guardando coisas que não são "main"** — já resolvido pelas Etapas 3–4; confirmado
  que `main.c` tem 250 linhas e contém só includes + `ThreadMain`/`main()`. Nada a fazer aqui.

Verificado byte a byte idêntico nas duas variantes (a mudança de comentário e a troca de string
literal por concatenação em tempo de compilação não alteram o binário).

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

        ↓  ← aqui: fazer um plugin novo de verdade

Etapa 3  (separar plugin.c)    ─┐
Etapa 4  (dividir o motor)      ├─ depois desse plugin
Etapa 5  (TOOLS_ONLY)          ─┤
Etapa 6  (cosmético)           ─┘
Etapa 8  (migração derivados)  ── feita ao fim, com a estrutura já estável

Etapa 7  → provavelmente nunca
```

**Por que um plugin novo no meio:** as Etapas 3–4 reorganizam a área que o desenvolvedor edita.
Fazer um plugin real *antes* mostra na prática o que incomoda — e a única metade do projeto que
nunca foi testada de verdade é justamente a de cheats (os exemplos são inertes por construção).
Um plugin novo é o primeiro teste real dela, e vai informar como `plugin.c` deve ficar.

**Alternativa válida:** fazer 3–4 antes, e já desenvolver o plugin novo na estrutura nova. Como
a verificação é byte a byte, o risco não é o argumento — o argumento é só não redesenhar o
`plugin.c` sem ter usado ele de verdade uma vez.

---

## Etapa 8 — Documento de migração para os plugins derivados 🟡 ✅ *(feita)*

**Modelo sugerido: Opus.** **Risco: nenhum aqui** (é documentação; o risco fica na execução
dentro de cada plugin).

**Entregue:** `MIGRACAO-PLUGINS-DERIVADOS.md` (canônico, neste repo), copiado como
`MIGRACAO-CTRCOMPOSER.md` na raiz de cada plugin para que a sessão de lá encontre localmente.

O guia é auto-contido — uma sessão nova abre o plugin, aponta para o arquivo e diz "execute a
migração", sem precisar de nenhum histórico.

**O ponto crítico do documento:** os dois **não** têm o mesmo motor que o template. Eles
divergiram, e o guia é *"reorganize na mesma estrutura"*, **nunca** *"substitua seu motor pelo
nosso"* — isso apagaria trabalho legítimo deles. A divergência foi **medida**, não suposta, e a
tabela está no documento: o OoT tem 13 headers e nenhum `TOOLS_ONLY`; o segundo tem 12 headers e
24 pontos condicionais; o template tem 8 headers. Ambos têm arte real (`sprites.h`, `topbg.h`,
`botbg.h`, `logo.h`) que o template não tem de propósito.

**Achados da inspeção que entraram no guia:**
- **O OoT não tem `DrawScaled()`** — a função simplesmente não existe nesse fork. Ele tem
  `DrawImg()` e `DrawSprite()` em posições diferentes. O guia manda casar **pela função, nunca
  pelo cabeçalho de seção**, e avisa que o OoT não terá um `engine/sprites.inc.c` equivalente.
- **`ARepeat()` está encravado dentro de `Cheat implementations` nos dois plugins** (L953 e L985)
  — a mesma armadilha que o template tinha. Recortar a seção inteira levaria motor
  junto para dentro do arquivo "que você edita". O guia manda tirá-lo primeiro, em passo separado.
- **Nomes de seção divergem**: o tracker do OoT se chama `Checklist 100%`, os pickers dele são
  `Pickers (bottle contents / inventory item)`, a janela é `(parchment window)`.
- **Os dois têm conteúdo de autor enterrado no motor** (`GUIDE_CREDITS`, `PLUGIN_PAGES`) — a mesma
  falha que a Etapa 5 corrigiu aqui.

**O que torna a migração viável:**
- O `Makefile` dos dois já compila só `sources/*.c` (`$(wildcard $(dir)/*.c)`), então o truque do
  `#include` funciona sem nenhuma alteração — **desde que** os `.inc.c` fiquem num subdiretório.
  O guia grifa essa armadilha.
- A verificação byte a byte funciona igual para eles. Nenhum dos dois tem `.PRECIOUS: %.elf` nem
  `Tools/fingerprint.sh` — a Etapa 0 do guia manda copiar exatamente esses dois itens, e **nada
  mais**, do CTRComposer.

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

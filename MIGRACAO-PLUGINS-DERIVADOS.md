# Migração para a estrutura CTRComposer — guia para os plugins derivados

> **Para quem é este arquivo:** uma sessão do Claude Code trabalhando dentro do
> `ZeldaOOTplugin` ou do `SegundoPluginDerivado`. É auto-contido: você não precisa do histórico da
> conversa que o gerou.
>
> **Como usar:** abra a sessão na pasta do plugin, aponte para este arquivo e diga
> *"execute a migração"*. Leia a seção **Regra de ouro** antes de tocar em qualquer coisa.

---

## O que aconteceu, e por que você está lendo isto

O `CTRComposer` (o template do qual os dois plugins descendem) recebeu uma crítica pública do
autor do Gen6CTRPluginFramework: *"foundational setup mashed into main.c"*, *"hard to navigate
as a template"*. A crítica procedia — um `main.c` de 5.582 linhas misturava o motor com a área
que o desenvolvedor edita, sem nenhuma fronteira.

Isso foi corrigido no template ao longo de 7 etapas. O `main.c` foi de **5.582 para 250 linhas**,
e o resto virou dois diretórios com significado:

```
sources/
  main.c        ← só a lista de #include + ThreadMain/main()
  plugin/       ← O QUE VOCÊ EDITA (cheats, menus, textos, dados do jogo)
  engine/       ← o motor (render, menu, tools, guias, tracker)
```

**Nenhum byte do binário mudou em nenhuma etapa.** Isso não é uma estimativa — é comparação
`cmp` byte a byte a cada passo. A técnica que permite isso está explicada abaixo, e é a mesma
que você vai usar.

Seu plugin ainda está no formato antigo. Este guia é como chegar ao mesmo layout **sem perder
nada do que você construiu**.

---

## ⛔ Regra de ouro: reorganizar, NUNCA substituir

**Você vai mover o SEU código para arquivos novos. Você não vai copiar nada do CTRComposer para
dentro do seu plugin.**

Isso não é excesso de cuidado. Os três projetos **divergiram de verdade**, e cada um tem trabalho
legítimo que o template não tem. Medido, não suposto:

| | CTRComposer | ZeldaOOTplugin | SegundoPluginDerivado |
|---|---|---|---|
| linhas no `main.c` | 250 (já migrado) | 6.172 | 6.616 |
| headers em `includes/` | 8 | **13** | **12** |
| arte própria | nenhuma | `sprites.h` `topbg.h` `botbg.h` `logo.h` `kbkeys.h` | `sprites.h` `topbg.h` `botbg.h` `logo.h` |
| `TOOLS_ONLY` | 21 pontos | **0 — não existe** | 24 pontos |
| `PLUGIN_DIR` | vazio | **não existe** | `00040000000XXXXX` |
| seção `Item sprites (from sprites.h)` | não tem | tem | tem |

O motor do template foi **esvaziado de propósito** (é um template em branco: sem sprites, sem
fundo, sem logo). O seu tem arte real, e funções de motor que você alterou para features que o
template não possui — teleporte, pickers com arte de item, temas próprios.

> Se em algum momento você se pegar pensando *"o arquivo do CTRComposer está mais limpo, vou usar
> ele"* — **pare**. Isso apaga trabalho seu. O objetivo é que o SEU `engine/menu_render.inc.c`
> contenha o SEU código de menu, só que agora num arquivo com nome.

O que você copia do CTRComposer: **exatamente dois arquivos de ferramenta** (`Tools/fingerprint.sh`
e uma linha do `Makefile`), listados na Etapa 0. Mais nada.

---

## Por que isto é seguro (e como provar)

O truque é usar `#include` em vez de módulos de compilação separados.

Quando você recorta um trecho do `main.c` para `sources/engine/render.inc.c` e põe
`#include "engine/render.inc.c"` no lugar exato, o pré-processador **cola o conteúdo de volta**
antes de o compilador ver qualquer coisa. Continua sendo **uma unidade de compilação só**:

- os `static` continuam `static` (nada vira global)
- o compilador enxerga exatamente o mesmo fluxo de tokens
- ele gera **exatamente o mesmo código**

Ou seja: você ganha navegabilidade com **risco zero**, e consegue *provar* isso com `cmp`.

**O `Makefile` dos dois plugins já está preparado para isso**, sem nenhuma alteração:

```make
SOURCES := sources
CFILES  := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
```

Ele compila só `sources/*.c` — arquivos em `sources/plugin/` e `sources/engine/` **não** são
compilados sozinhos. É isso que faz o `#include` funcionar sem símbolo duplicado.

> ⚠️ **Por isso os `.inc.c` TÊM que ficar num subdiretório.** Se você criar
> `sources/render.inc.c` (direto em `sources/`), o `wildcard *.c` vai pegá-lo, ele será compilado
> uma segunda vez, e o link quebra com centenas de "multiple definition". A extensão `.inc.c` é só
> convenção para o leitor; quem protege é a **pasta**.

---

## Etapa 0 — Rede de segurança *(obrigatória, faça primeiro)*

Sem isto nenhum passo seguinte é verificável, e você fica migrando no escuro.

**1. Confirme que o baseline compila, ANTES de tocar em qualquer coisa.**

```bash
make clean && make
```

Se não compilar agora, resolva isso primeiro. Não comece a migração com o build quebrado — você
perde a única referência que importa.

**2. Adicione ao fim do `Makefile`** (no bloco `else`, junto das outras regras):

```make
# Sem isto o make trata o .elf como intermediário descartável e o apaga assim que gera o .3gx.
# Tools/fingerprint.sh precisa do .elf para ler os símbolos.
.PRECIOUS: %.elf
```

**3. Copie `Tools/fingerprint.sh`** do repo do CTRComposer
([github.com/samaBR85/CTRComposer](https://github.com/samaBR85/CTRComposer), arquivo
`Tools/fingerprint.sh`) para a mesma pasta no seu plugin. Ele não tem nada específico do
template — é só `arm-none-eabi-nm` com normalização.

**4. Guarde a referência.** Descubra o nome do seu `.elf` (varia por plugin) e salve:

```bash
make clean && make
ls *.elf                       # OoT: RawPlugin.elf   segundo plugin: SegundoPlugin.elf
cp SEU.3gx /tmp/base.3gx
sh Tools/fingerprint.sh SEU.elf > /tmp/base.fp
```

**Depois de CADA recorte, verifique:**

```bash
make clean && make
cmp SEU.3gx /tmp/base.3gx && echo "IDENTICO"
diff /tmp/base.fp <(sh Tools/fingerprint.sh SEU.elf)
```

> O binário deve ser **byte a byte idêntico**. Se não for, algo de verdade mudou — **pare e
> investigue**, não siga em frente. A única exceção legítima é se você reordenar declarações de
> propósito: aí o `cmp` pode falhar mas o `diff` da impressão digital tem que continuar vazio
> (mesmas funções, mesmos tamanhos).

**5. Commite antes de começar.** Cada recorte deve virar um commit. É o seu ponto de reversão.

---

## A estrutura alvo

Os três projetos herdaram os **mesmos nomes de seção** (`// ===== Cheat implementations =====`),
então as fronteiras de corte são reconhecíveis no seu `main.c` também. Esta é a tabela de destino
— a coluna da esquerda é o que procurar no SEU arquivo:

### `sources/plugin/` — o que VOCÊ edita

| Seção no seu `main.c` | Vira |
|---|---|
| `TOOLS_ONLY`, `PLUGIN_VER`, `PLUGIN_NAME`, `PLUGIN_TAG`, `PLUGIN_DIR` | `plugin/identity.inc.c` |
| `Cheat IDs` (+ `Where this plugin keeps its files`, se separado) | `plugin/cheat_ids.inc.c` |
| `Cheat implementations` (+ `IsToggleCheat`) | `plugin/cheats.inc.c` |
| `Pickers ...` | `plugin/pickers.inc.c` |
| as **tabelas** de `Menu model (folders)` (`rootItems[]`, `folders[]`...) | `plugin/menu_tables.inc.c` |
| `SpriteKeyForCheat()` | `plugin/cheat_icons.inc.c` |
| os **dados** do tracker (`CHK_CATS` / equivalente) | `plugin/tracker_data.inc.c` |
| `GUIDE_CREDITS` + `PLUGIN_PAGES[]` | `plugin/guide_text.inc.c` |

### `sources/engine/` — o motor (seu, só que organizado)

| Seção no seu `main.c` | Vira |
|---|---|
| handle da thread, `R8`/`W8`..., `LCD registers` | `engine/platform.inc.c` |
| `RAM compose buffer` + `Framebuffer <-> compose` | `engine/render.inc.c` |
| `Config persistence` + `Localization` + `SD-loaded guides` + `Quick menu hotkey` | `engine/storage.inc.c` |
| `ARepeat()` (auto-repeat do D-pad) | `engine/input.inc.c` |
| os **tipos/enums/macros** de `Menu model` (`Item`, `Folder`, `F_*`, `T_*`, `IT_*`) | `engine/menu_model.inc.c` |
| `kToolKeys[]`, `ItemHidden`, `NavSkip`, `VisPos` | `engine/menu_nav.inc.c` |
| `LabelForCheat`, `CheatForLabel`, `FavSave`, `FavLoad` | `engine/favorites.inc.c` |
| `CTRPF-style rendering (... window)` | `engine/theme.inc.c` |
| `Sprites (RGBA4444 art)` — o blit genérico, `DrawScaled`, `SPRK_*` | `engine/sprites.inc.c` |
| ícones vetoriais desenhados em código + `DrawCheatIcon` | `engine/icons.inc.c` |
| `Bottom screen` | `engine/bottom_screen.inc.c` |
| `Toast` | `engine/toast.inc.c` |
| `Menu rendering` | `engine/menu_render.inc.c` |
| `Tools` | `engine/tools.inc.c` |
| `Game Guide / Plugin Guide` → o leitor compartilhado | `engine/guide_reader.inc.c` |
| → `ToolGameGuide()` + `GG_Cats()` | `engine/guide_game.inc.c` |
| → `PG_Pages()` + `ToolPluginGuide()` | `engine/guide_plugin.inc.c` |
| `Completion tracker` / `Checklist 100%` → tipos (`ChkItem`, `ChkCat`) | `engine/tracker.inc.c` |
| → estado, auto-fill e desenho | `engine/tracker_ui.inc.c` |
| `ToolRun()` | `engine/tool_dispatch.inc.c` |
| `Game pause` + `Menu loop` | `engine/menu_loop.inc.c` |
| `Quick menu (favorites, L+SELECT)` | `engine/quick_menu.inc.c` |
| `Thread / entry` | **fica no `main.c`** |

### Seções que você tem e o template não

Ambos os plugins têm `// ===== Item sprites (RGBA4444, from sprites.h) =====`, que não existe no
CTRComposer. **Isso não é problema — é o esperado.** Dê a ela o mesmo tratamento: um arquivo
próprio, no diretório que corresponder à natureza dela.

- É arte do jogo que você escolheu e mapeou → `plugin/item_sprites.inc.c`
- É maquinaria genérica de blit → `engine/item_sprites.inc.c`

Na dúvida, o teste é: *"se alguém fizesse um plugin de outro jogo com este motor, teria que
reescrever isto?"* Se sim → `plugin/`. Se não → `engine/`.

O mesmo vale para qualquer outra seção sua que não esteja na tabela.

---

## Como executar

**Uma seção por vez. Verificando entre cada uma.** Não faça os 25 recortes e compile no fim — se
quebrar, você não vai saber qual foi.

Para cada seção:

1. Recorte o trecho para o arquivo novo
2. Ponha `#include "engine/xxx.inc.c"` **na posição exata** de onde saiu
3. `make clean && make`
4. `cmp` → tem que ser idêntico
5. Commite
6. Só então passe para a próxima

**Ordem recomendada:** faça `plugin/` primeiro (é o que responde à crítica e o que te dá o
benefício imediato), depois `engine/` de cima para baixo.

### ⛔ A restrição que não pode ser violada

**A ordem dos `#include` no `main.c` tem que ser a mesma ordem top-to-bottom do arquivo
original.** É isso que garante que as declarações venham antes dos usos. Mover é uma coisa;
reordenar é outra — e reordenar quebra a compilação de formas confusas (`implicit declaration`,
`unknown type name`).

Se você *quiser* reordenar algo de propósito (juntar `PLUGIN_DIR` com `PLUGIN_VER`, por exemplo),
faça isso **num passo separado**, e aceite que aí o `cmp` pode falhar — a verificação passa a ser
o `diff` da impressão digital.

---

## As armadilhas (todas foram encontradas de verdade na migração do template)

### 1. `#if`/`#endif` têm que fechar DENTRO do mesmo arquivo

O GCC recusa (`unterminated #if` / `#endif without #if`) se um bloco condicional abre em um
`.inc.c` e fecha em outro — mesmo que no fim das contas tudo esteja balanceado.

Isso morde quando um `#if !TOOLS_ONLY` envolve uma seção inteira que você está partindo em dois
arquivos. **Duas saídas:**

- replicar o guard em cada arquivo que ele atravessa (abre e fecha em cada um), ou
- tirar o `#if` de dentro dos arquivos e pôr **em volta dos `#include`** no `main.c` — mais limpo,
  e foi o que o template acabou fazendo com o tracker

Cheque antes de compilar:

```bash
for f in sources/main.c sources/plugin/*.inc.c sources/engine/*.inc.c; do
  a=$(grep -cE '^\s*#\s*(if|ifdef|ifndef)\b' "$f")
  b=$(grep -cE '^\s*#\s*endif\b' "$f")
  [ "$a" -ne "$b" ] && echo "DESBALANCEADO: $f ($a/$b)"
done
```

*(Só se aplica ao segundo plugin. O OoT não tem `TOOLS_ONLY` nenhum.)*

### 2. Não ache o fim de uma função procurando `}` sozinho numa linha

Se você automatizar o recorte, **conte chaves de verdade**. Procurar a primeira linha que é só
`}` para no `}` que fecha um `switch` interno, e você corta no meio da função — o erro que sai
depois é uma cascata de `invalid storage class for function`, que não aponta para o lugar certo.

Ignore também chaves dentro de comentário de fim de linha.

### 3. `Cheat IDs` tem que vir cedo

O `enum CH_*` define `NUM_CHEATS`, que dimensiona `cheatState[]` e `favorite[]`. Ele precisa estar
antes de qualquer coisa que use esses arrays. Não mova esse `#include` para baixo.

### 3b. Tem motor encravado no meio da área do usuário — nos dois plugins

`ARepeat()` (auto-repeat do D-pad) é motor puro, mas nos **dois** plugins ele está **dentro** da
seção `Cheat implementations`, entre as suas funções de cheat:

| | seção `Cheat implementations` | `ARepeat()` |
|---|---|---|
| OoT | linhas 700–1037 | **953** |
| segundo plugin | linhas 807–1190 | **985** |

Se você recortar a seção inteira para `plugin/cheats.inc.c`, leva o `ARepeat` junto — e aí o
arquivo "que você edita" tem motor no meio. **Tire o `ARepeat` primeiro**, para
`engine/input.inc.c`, num passo separado; só depois recorte a seção de cheats. Foi exatamente
isso que o template fez.

> Este é o caso em que reordenar é **de propósito**: o `cmp` pode falhar, mas o `diff` da
> impressão digital tem que continuar vazio.

### 4. Conteúdo de autor escondido no motor

Ao migrar, você vai achar coisas que *parecem* motor mas são conteúdo seu — no template foram as
páginas do Plugin Guide e a página de créditos do guia, ambas enterradas na seção de guias. Se o
texto é algo que **outro plugin teria que reescrever**, ele vai para `plugin/`.

### 5. O `.3gx` na pasta errada

Depois de migrar, confira que o artefato final continua com o mesmo nome e vai para o mesmo lugar
de sempre. O `TARGET` do Makefile do OoT é derivado do nome da pasta (`$(notdir $(CURDIR))`) —
não renomeie a pasta `RawPlugin` no meio da migração.

---

## Notas por plugin

> ⚠️ **Os números de linha abaixo são de 31/07/2026 e vão envelhecer.** Estes plugins estão em
> desenvolvimento ativo em sessões paralelas — o `main.c` do segundo plugin já cresceu de 6.260 para 6.616
> linhas entre o planejamento e este documento. **Trate os números como "onde procurar", nunca
> como endereço.** Confirme sempre com `grep -n` antes de cortar. O que **não** envelhece são os
> nomes de função e de seção.

### ZeldaOOTplugin

- **Não tem `TOOLS_ONLY`.** Toda a armadilha nº 1 não se aplica; sua migração é mais simples.
- **Não tem `PLUGIN_DIR`.** Se ele não existe, o `identity.inc.c` fica só com nome/versão. Vale
  considerar adicionar (o template usa para separar `Settings.cfg` por jogo), mas isso é uma
  mudança de comportamento — **não** faça junto com a migração. Fica para depois, com teste.
- A seção do tracker se chama **`Checklist 100% (progression-ordered, mixed item types)`**, não
  `Completion tracker`. Case por conteúdo, não pelo título.
- A seção de pickers se chama **`Pickers (bottle contents / inventory item)`**.
- A janela é **`CTRPF-style rendering (parchment window)`** — o template diz `(themed window)`.
- Tem `kbkeys.h`, que nenhum dos outros dois tem. O teclado (`KbKey()`, **linha 2930**) fica
  **dentro da seção `Tools`**, não numa seção própria — ele vai junto para `engine/tools.inc.c`.
  Se quiser dar a ele um arquivo só seu (`engine/keypad.inc.c`), faça num passo separado.
- **Não tem a seção `Sprites (RGBA4444 art)` genérica, e não tem `DrawScaled()`.** Verificado: a
  função simplesmente não existe nesse fork. O que existe é `DrawImg()` (**1721**, dentro de
  `CTRPF-style rendering`) e `DrawSprite()` (**1852**, dentro de `Item sprites`). Ou seja: você
  **não terá** um `engine/sprites.inc.c` equivalente ao do template — e está tudo certo. Case pela
  função, nunca pelo cabeçalho de seção.
- `IsToggleCheat()` está na **linha 1679**, dentro da seção de rendering — não junto dos cheats
  como no template. Ele só depende do `enum CH_*`, então pode subir para `plugin/cheats.inc.c`
  (é lá que o comentário "mantenha em sincronia com ApplyCheats" faz sentido). Passo separado.
- `ToolRun()` está na **linha 5292**, logo antes de `Game pause` — mesma posição que o template
  tinha. Vale tirá-lo para `engine/tool_dispatch.inc.c`, como descrito na tabela.

### SegundoPluginDerivado

- **Tem `TOOLS_ONLY` (24 pontos condicionais).** A armadilha nº 1 vale para você. Vale a pena
  fazer o que o template fez: onde o `#if` envolve uma seção inteira, tire-o de dentro do arquivo
  e ponha em volta do `#include` no `main.c`.
- Estrutura **mais próxima do template** que o OoT: já tem `Where this plugin keeps its files`
  (com o Title ID `00040000000XXXXX` preenchido) e a seção `Sprites (RGBA4444 art)` genérica
  separada de `Item sprites`.
- `TARGET := SegundoPlugin` → o `.elf` é `SegundoPlugin.elf`.
- Tem **duas** seções de sprite, e tem `DrawScaled()` (**2076**) — ao contrário do OoT. Mantenha as
  duas separadas: `engine/sprites.inc.c` (blit genérico) e `plugin/item_sprites.inc.c` (a arte que
  você mapeou para os itens do segundo plugin).
- `PLUGIN_PAGES[]` aparece **duas vezes** (**4553** e **4611**): é o par `#if TOOLS_ONLY` /
  `#else`, idêntico ao que o template tinha. Ao migrar, a versão do template (a do `#else`) é
  conteúdo seu → `plugin/guide_text.inc.c`, junto do `GUIDE_CREDITS` (**4482**). A versão
  tools-only descreve a build universal e fica no motor.
- `IsToggleCheat()` (**1915**), `SpriteKeyForCheat()` (**2114**), `KbKey()` (**3166**, dentro de
  `Tools`), `ToolRun()` (**5793**, logo antes de `Game pause`).

---

## Checklist final

Ao terminar, confira:

- [ ] `make clean && make` compila limpo
- [ ] `cmp` do `.3gx` contra `/tmp/base.3gx` → **idêntico**
- [ ] `main.c` tem só `#include`s + `ThreadMain`/`main()`
- [ ] Nenhum `.inc.c` solto em `sources/` (todos em `plugin/` ou `engine/`)
- [ ] O balanço de `#if`/`#endif` fecha em cada arquivo
- [ ] **Testado no console de verdade** — abrir o menu, um cheat, uma tool, o guia, o tracker

> O último item não é opcional. A verificação byte a byte prova que o *compilador* gerou o mesmo
> código; ela não prova que você não esqueceu um arquivo fora do `#include`. Se um arquivo ficar
> órfão, o build quebra — mas se você duplicar um `#include`, pode compilar e se comportar
> diferente. Uma passada de 2 minutos no hardware fecha a conta.

---

## Referência

A estrutura final do template está em `Sources/` no repo do CTRComposer
([github.com/samaBR85/CTRComposer](https://github.com/samaBR85/CTRComposer)). Use como **modelo de
organização** — nunca como fonte de código para copiar.

O histórico completo de decisões (incluindo uma proposta que foi testada e **rejeitada** por não
funcionar) está em `PLANO-REFATORACAO.md`, no mesmo repo.

# Correções de motor para os plugins derivados

> **Para quem é este arquivo:** uma sessão do Claude Code trabalhando dentro de um plugin
> construído sobre o motor CTRComposer — `ZeldaOOTplugin` (OcarinaCTRComposer) ou
> `ZeldaMM3Dplugin`. É auto-contido: não precisa do histórico de nenhuma conversa.
>
> **Como usar:** abra a sessão na pasta do plugin, aponte para este arquivo e diga
> *"aplique estas correções"*.

Quatro defeitos encontrados no CTRComposer, **todos anteriores a qualquer reorganização** — eles
estão em todo plugin que herdou este motor. Corrigidos e **confirmados no console**.

| | Defeito | Gravidade |
|---|---|---|
| **1** | A tela de cima congela ao sair do plugin | 🔴 quebra o uso |
| **2** | Esperas por botão sem limite podem travar o console | 🟡 latente |
| **3** | Texto traduzido estoura buffer de pilha | 🟡 latente |
| **4** | `{D-Pad}` não é um token — o glifo vira texto literal | 🟢 cosmético |

Aplique na ordem. Cada um é independente; se algum não se aplicar ao seu fork, o próprio texto
diz como confirmar antes de mexer.

**Independente da migração para `plugin/` + `engine/`.** Funciona igual no `main.c` monolítico. Se
o plugin já foi migrado, os mesmos trechos estarão em arquivos diferentes — a seção *Onde o código
está* cobre os dois casos.

> ⚠️ **Estas correções MUDAM o binário, de propósito.** Se você usa a verificação byte a byte da
> migração (`cmp` contra um baseline), ela **tem que falhar** aqui. Não tente fazer bater. Por
> isso: migre primeiro, verifique idêntico, e só então aplique isto.

---

# 1 · A tela de cima congela ao sair 🔴

## O sintoma

Você sai do plugin e a **tela de cima fica congelada** na última imagem dele. A de baixo volta
para o jogo normalmente, o jogo continua rodando, o áudio toca — só o topo fica parado. Reabrir o
menu desenha um novo por cima da imagem travada, indefinidamente.

**Como reproduzir:** estrele uma **ferramenta** (Cheat Search / RAM Dumper / Hex Editor) como
favorita, abra o quick menu (L+SELECT), lance a ferramenta por ali e saia.

Só o atalho de **ferramenta** dispara — o de cheat não passa por `RunMenu()`. É por isso que o
defeito pode estar anos num plugin sem aparecer: se os favoritos são cheats, ninguém nunca anda
por esse caminho.

## A causa

`Present()` desenha no buffer escondido e depois troca qual dos dois o LCD exibe:

```c
REG32(LCD_TOP + LCD_SELECT) = sel ^ 1;
```

**Nada nunca devolve esse registrador.** Ao sair, o LCD segue varrendo o buffer do *plugin*. O
jogo volta a rodar de verdade por baixo — daí a tela de baixo retornar e o áudio continuar — mas
o topo fica parado no último frame do plugin.

E é exatamente por isso que só a tela de cima quebra: a de baixo escreve **direto no buffer
visível**, então restaurar os pixels (`BotRestoreBoth()`) basta. Só a de cima troca o registrador.

## Confirme que se aplica antes de mexer

```sh
grep -n "LCD_SELECT" sources/main.c sources/engine/render.inc.c 2>/dev/null
```

Você deve ver **uma única escrita** (`REG32(LCD_TOP + LCD_SELECT) = sel ^ 1;`), dentro de
`Present()`, e **nenhuma** restauração. As outras ocorrências são leituras (`& 1`) e o `#define`
do offset — essas são normais.

Se já existir um `TopRelease()` ou equivalente, a correção já foi aplicada. Pare aqui.

---

## Onde o código está

| Trecho | `main.c` monolítico | Depois da migração |
|---|---|---|
| `Present()` | `sources/main.c` | `sources/engine/render.inc.c` |
| `RunMenu()` | `sources/main.c` | `sources/engine/menu_loop.inc.c` |
| `QuickMenu()` | `sources/main.c` | `sources/engine/quick_menu.inc.c` |
| o despacho do quick menu em `ThreadMain` | `sources/main.c` | `sources/main.c` |

---

## Passo 1 — guardar e devolver o registrador

Logo **antes** de `Present()`:

```c
// Qual buffer a tela de cima exibia quando assumimos. Present() troca esse registrador para
// mostrar o NOSSO frame; ninguem mais o devolve, entao o plugin tem que devolver.
static u32 g_lcdSelSaved = 0;
static int g_lcdSelValid = 0;

// Chamar quando o overlay assume a tela de cima, ANTES do primeiro Present().
static void TopTakeOver(void)
{
    if (g_lcdSelValid) return;              // aninhado (quick menu -> menu): mantem o mais externo
    g_lcdSelSaved = REG32(LCD_TOP + LCD_SELECT) & 1;
    g_lcdSelValid = 1;
}

// Chamar ao devolver a tela para o jogo, logo antes de ResumeGame().
//
// POR QUE ISTO EXISTE: a tela de baixo desenha direto no buffer visivel, entao restaurar os
// pixels dela basta. A de cima NAO - Present() escreve no buffer escondido e troca este
// registrador. Deixe-o trocado e o LCD continua varrendo o nosso frame: o jogo roda (a tela de
// baixo volta, o audio toca) enquanto o topo fica congelado no menu.
static void TopRelease(void)
{
    if (!g_lcdSelValid) return;
    REG32(LCD_TOP + LCD_SELECT) = g_lcdSelSaved;
    g_lcdSelValid = 0;
}
```

> O guard `g_lcdSelValid` importa: o quick menu pode chamar `RunMenu()`, e sem ele o valor salvo
> pelo mais externo seria sobrescrito pelo mais interno — que já é o nosso próprio buffer.

## Passo 2 — os três pontos de chamada

**Em `RunMenu()`**, logo após o `PauseGame()`:

```c
    PauseGame();
    TopTakeOver();   // remember which buffer the game was showing, so we can hand it back
```

**Em `RunMenu()`**, no fim, junto do restore da tela de baixo:

```c
    BotRestoreBoth();
    TopRelease();   // hand the top screen back too, else it stays frozen on our last frame
    DrainButtons(...);
    ResumeGame();
```

**Em `QuickMenu()`**, logo após o `PauseGame()`:

```c
    PauseGame();
    TopTakeOver();
    GrabFb();
```

**Em `QuickMenu()`**, no fim — e aqui está a sutileza:

```c
    DrainButtons(BUTTON_B | BUTTON_SELECT | BUTTON_A);
    // Só devolve se formos mesmo para o jogo. Quando uma pasta ou ferramenta favorita foi
    // escolhida, RunMenu() assume em seguida - devolver aqui mostraria um frame do jogo só
    // para recapturá-lo, que é o piscar que este hand-off existe para evitar.
    if (g_openFolder < 0 && g_openTool < 0) TopRelease();
    ResumeGame();
```

---

## Passo 3 — o defeito vizinho: o quick menu vira o fundo do menu

Mesmo caminho, defeito diferente, e vale corrigir junto.

Vindo do quick menu, `RunMenu()` chama `GrabFb()` **microssegundos** depois do `ResumeGame()`. O
jogo não desenhou nada nesse intervalo, então o `GrabFb()` captura o **próprio quick menu** como
se fosse o frame do jogo — e o `CaptureTopBackdrop()` grava isso como fundo oficial. O menu passa
a ser desenhado sobre uma fotografia de si mesmo, e cada reabertura empilha outra camada.

**A flag**, junto das outras globais de estado do menu (`g_quitToGame`, `g_resumeTool`):

```c
// Setada quando RunMenu() e chamado direto do quick menu. Significa que o jogo NAO desenhou um
// frame desde a ultima vez que pintamos a tela, entao GrabFb() capturaria o nosso proprio painel
// e o assaria no fundo - o menu renderiza sobre uma foto de si mesmo, e cada reabertura empilha
// mais uma copia.
static int g_qmHandoff = 0;
```

**Em `RunMenu()`**, onde hoje está `GrabFb(); DimOutsideWindow(); CaptureTopBackdrop();`:

```c
    if (g_qmHandoff)
    {
        g_qmHandoff = 0;
        RestoreTopBackdrop();  // savedTop = o frame real do jogo, salvo antes do painel desenhar
    }
    else
    {
        GrabFb();
    }
    DimOutsideWindow();
    CaptureTopBackdrop();
```

**Em `ThreadMain`**, nos **dois** ramos que despacham o quick menu (`g_openFolder >= 0` e
`g_openTool >= 0`), antes do `RunMenu()`:

```c
                g_qmHandoff = 1;   // o quick menu ainda esta na tela: nao recapture como fundo
```

## Passo 4 — cosmético: cursor num separador

O ramo de **ferramenta** do `ThreadMain` deixa o cursor na linha 0 do HOME, que pode ser um
cabeçalho não-selecionável. O ramo de **pasta**, logo acima no mesmo bloco, já pula separadores.
Replique:

```c
                menuDepth = 0; menuFolder = 0; menuScroll = 0; menuCursor = 0;
                { const Folder *nf = &folders[F_ROOT]; // primeira linha selecionavel, nao um separador
                  while (menuCursor < nf->count && IS_SEP(&nf->items[menuCursor])) menuCursor++;
                  if (menuCursor >= nf->count) menuCursor = 0; }
```

---

## Como verificar o defeito 1

1. `make` compila limpo
2. **No console:** estrele uma ferramenta, lance pelo quick menu, saia — a tela de cima tem que
   voltar a mostrar o jogo, sem resquício do menu
3. Confirme que o caminho normal não regrediu: SELECT → menu → ferramenta → SELECT → jogo
4. E o quick menu comum: L+SELECT → alternar um cheat → B → jogo

Suba a versão antes de gravar no cartão, e confira na tela. É a sua prova de que o `.3gx` testado
é o que você acabou de compilar.

---

# 2 · Esperas por botão sem limite 🟡

## O sintoma

Nenhum, até acontecer: o plugin fica preso numa tela **com o jogo pausado**, o que se apresenta
como console morto. Só HOME ou o botão de força saem.

Não dá para reproduzir de propósito sem quebrar o controle — é por isso que é latente, não
teórico. Basta um botão preso ou um ombro encostado na case no momento errado.

## A causa

O motor já tem a função certa, e ela **é limitada**:

```c
static void DrainButtons(u32 mask)
{
    for (int i = 0; i < 125 && (HID_PAD & mask); ++i)   // ~2s, um pad preso nao trava
        svcSleepThread(16 * 1000 * 1000);
}
```

Mas quatro esperas cruas escapam dela:

| Onde | O quê |
|---|---|
| fim do info box | `while (HID_PAD) svcSleepThread(...);` |
| fim do About | `while (HID_PAD) svcSleepThread(...);` |
| entrada do menu | `while (HID_PAD & BUTTON_SELECT) svcSleepThread(...);` |
| entrada do quick menu | `while (HID_PAD & BUTTON_SELECT) svcSleepThread(...);` |

`HID_PAD` lê um registrador de hardware cru. Se ele nunca voltar limpo, esses laços giram para
sempre — e o jogo está pausado.

## Confirme e corrija

```sh
grep -rn "while (HID_PAD" sources/
```

Cada ocorrência vira uma chamada limitada:

```c
DrainButtons(~0u);            // no lugar de  while (HID_PAD) ...
DrainButtons(BUTTON_SELECT);  // no lugar de  while (HID_PAD & BUTTON_SELECT) ...
```

> ⚠️ **A busca por `while (HID_PAD)` não pega `while (HID_PAD & BOTAO)`.** Eu corrigi duas na
> primeira passada e só achei as outras duas ao reverificar. Use o `grep` acima, que pega as duas
> formas, e confirme que sobrou zero.

**Onde `DrainButtons` mora.** Se o info box e o About ficam *antes* do menu no seu arquivo, eles
não enxergam a função — que é por que tinham a própria espera crua. Mova `DrainButtons` para junto
do `ARepeat` (o bloco de auto-repeat do D-pad), que vem cedo e é onde ela pertence: é plumbing de
entrada, não do menu.

## Como verificar no console

Exercite **segurando** o botão em vez de tocar:

1. Abra o info box com `X`, **segure B** ao fechar por ~3s. O plugin espera no máximo ~2s e segue.
2. Mesma coisa saindo do About.
3. Abra o menu **segurando SELECT** por ~3s. Tem que abrir normalmente.

Nada pode prender. A abertura do menu continua instantânea — a espera roda na *saída*, para o jogo
não receber o botão que você ainda está segurando.

---

# 3 · Texto traduzido estoura buffer de pilha 🟡

## O sintoma

Travamento aparentemente aleatório depois de instalar um arquivo de tradução — e praticamente
impossível de rastrear até o arquivo de idioma, porque o estouro corrompe a pilha e o crash
acontece longe da causa.

## A causa

`T()` resolve para um `.txt` no cartão SD. Esse texto — de comprimento que o motor **não
controla** — cai em buffers de pilha de tamanho fixo, sem limite:

```c
char val[5][40];
siprintf(val[0], "%s", T(REGION_NAME[g_memRegion]));   // traducao > 39 chars = estouro
```

O pior caso do template era quatro traduções concatenadas em 96 bytes:

```c
char leg[96];
siprintf(leg, "%s  %s  %s  %s", T("{A} apply"), T("{B} cancel"), T("{L}/{R} page"), fmode);
```

Vale para rótulos de cheat e títulos de pasta escritos pelo autor também — o motor não sabe o
comprimento deles.

## A correção

Troque `siprintf` por `sniprintf` com o tamanho do destino, em **toda** chamada cujo formato
receba `T(...)` ou um rótulo do autor:

```c
sniprintf(val[0], sizeof val[0], "%s", T(REGION_NAME[g_memRegion]));
sniprintf(leg,    sizeof leg,    "%s  %s  %s  %s", ...);
```

Formatos que só recebem números podem ficar como estão.

> **`sniprintf`, não `snprintf`.** É a variante integer-only, mesma família do `siprintf` que o
> motor já usa. O `snprintf` puxaria o formatador de ponto flutuante para dentro do binário sem
> necessidade.

Para achar os pontos:

```sh
grep -rn "siprintf" sources/ | grep "T("
grep -rn "siprintf" sources/ | grep -E '"%s|%s\n'
```

No CTRComposer foram 15 chamadas. Confira o tamanho declarado de cada destino antes de usar
`sizeof` — precisa ser um array no escopo, não um ponteiro.

---

# 4 · `{D-Pad}` não é um token 🟢

## O sintoma

Um rodapé mostra `{D-Pad} scroll` como texto literal, com as chaves, enquanto outras telas
desenham o ícone do D-pad corretamente.

## A causa

O tokenizador casa **exatamente** `{DP}`:

```c
if (s[1] == 'D' && s[2] == 'P' && s[3] == '}') return GL_DP;
```

Qualquer outra grafia não é reconhecida e é desenhada como texto. **Falha silenciosa:** sem aviso
do compilador, sem erro em execução. Só aparece olhando a tela.

## Ache todas de uma vez

Em vez de procurar essa grafia específica, extraia **todo** `{token}` das strings do projeto e
compare com o que o `GlyphTok()` de fato aceita — `{DP}`, `{A}`, `{B}`, `{X}`, `{Y}`, `{L}`, `{R}`
e `{HK}` (esse último o `HkExpand()` troca antes de desenhar). Qualquer coisa fora dessa lista
está sendo desenhada como texto.

No CTRComposer havia exatamente uma, no rodapé do About — e ela tinha um **segundo** defeito na
mesma linha: era a única string de rodapé sem `T()`, ou seja, inalcançável para um tradutor.
Verifique isso também enquanto estiver ali.

---

# Verificação final

- [ ] `make` compila limpo
- [ ] Se ainda não usa `-Wall -Wextra`, ligue agora — o CTRComposer estava em zero avisos depois
      de tratar dois casos deliberados (`(void)arg` no `ThreadMain`, `-Wno-main` no Makefile)
- [ ] Suba a versão **antes** de gravar no cartão, e confira na tela
- [ ] Console: o roteiro de cada defeito acima
- [ ] Console, regressão: `SELECT` → menu → `SELECT` → jogo; menu → ferramenta → `B` → menu;
      `L+SELECT` → alternar um cheat → `B` → jogo

---

# Referência

Todos diagnosticados a partir de uso real no hardware, corrigidos e **confirmados no console** no
CTRComposer:

| Defeito | Commit | Release |
|---|---|---|
| 1 · tela congelada | `c6ee3fe` | v1.1.2 |
| 2 · esperas sem limite | `28745ef` | v1.1.4 |
| 3 · estouro de buffer | `8949822` | v1.1.4 |
| 4 · token do glifo | `28745ef` | v1.1.4 |

Nenhum é consequência de reorganização de código. Estão no motor desde a primeira versão, e em
todo plugin que o herdou.

# Correção: tela de cima travada ao sair do plugin 🔴

> **Para quem é este arquivo:** uma sessão do Claude Code trabalhando dentro de um plugin
> construído sobre o motor CTRComposer — `ZeldaOOTplugin` (OcarinaCTRComposer) ou
> `ZeldaMM3Dplugin`. É auto-contido: não precisa do histórico de nenhuma conversa.
>
> **Como usar:** abra a sessão na pasta do plugin, aponte para este arquivo e diga
> *"aplique esta correção"*.

**Independente da migração para `plugin/` + `engine/`.** Funciona igual no `main.c` monolítico.
Se o plugin já foi migrado, os mesmos trechos estarão em arquivos diferentes — a seção
*Onde o código está* cobre os dois casos.

---

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

## Como verificar

**O binário MUDA de propósito.** Se você usa a verificação byte a byte (`cmp` contra um baseline),
ela **tem que falhar** aqui — é o objetivo. Não tente fazer bater.

1. `make` compila limpo
2. **No console:** estrele uma ferramenta, lance pelo quick menu, saia — a tela de cima tem que
   voltar a mostrar o jogo, sem resquício do menu
3. Confirme que o caminho normal não regrediu: SELECT → menu → ferramenta → SELECT → jogo
4. E o quick menu comum: L+SELECT → alternar um cheat → B → jogo

Suba a versão antes de gravar no cartão, e confira na tela. É a sua prova de que o `.3gx` testado
é o que você acabou de compilar.

---

## Referência

Diagnosticado a partir de um relato de hardware, corrigido e **confirmado no console** no
CTRComposer — commit `c6ee3fe`, publicado na v1.1.2.

Os três defeitos existem desde a primeira versão do motor. Não são consequência de nenhuma
reorganização, e estão em todo plugin que herdou este `Present()`.

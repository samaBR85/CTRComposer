// ============================================================================================
// MOTOR - voce normalmente nao edita este arquivo.
//
// Incluido por Sources/main.c na posicao original, entao a ordem das declaracoes continua
// valendo. Nao e compilado sozinho: o Makefile so compila Sources/*.c, e este arquivo esta em
// Sources/engine/.
// ============================================================================================

// O modelo de dados do menu (Item/Folder), os enums F_*/T_* e as macros IT_*/IS_SEP
// usadas por Sources/plugin/menu_tables.inc.c para montar as pastas e linhas.

// ===================== Menu model (folders) =====================
// A layout-agnostic data model: DrawMenuItem() renders ONE row/cell wherever you put it,
// so HOME can be a plain scrolling list, a 2-column grid, or anything else. This template
// uses the 2-column grid for HOME and simple lists everywhere else.
typedef struct { const char *label; int cheat; int folder; int picker; const char *desc; int tool; u8 wide; } Item;
typedef struct { const char *title; const Item *items; int count; } Folder;

#if TOOLS_ONLY
enum { F_ROOT, F_SETTINGS, NUM_FOLDERS };
#else
enum { F_ROOT, F_EXAMPLES, F_TOOLS, F_SETTINGS, NUM_FOLDERS };
#endif

// tool screens. The tools-only build drops the two that are inherently per-game, so their
// code, their data tables and their menu rows all disappear from that binary.
#if TOOLS_ONLY
enum { T_SEARCH, T_RAMDUMP, T_HEXEDIT, T_ABOUT, T_PLUGINGUIDE, NUM_TOOLS };
#else
enum { T_SEARCH, T_RAMDUMP, T_HEXEDIT, T_ABOUT, T_GAMEGUIDE, T_PLUGINGUIDE, T_TRACKER, NUM_TOOLS };
#endif
static void ToolRun(int t); // fwd

#define IT_CHEAT(lbl, ch, d)   { lbl, ch, -1, -1, d, -1, 0 }
#define IT_FOLDER(lbl, fl)     { lbl, -1, fl, -1, NULL, -1, 0 }
#define IT_PICKER(lbl, pk, d)  { lbl, -1, -1, pk, d, -1, 0 }
#define IT_TOOL(lbl, tl, d)    { lbl, -1, -1, -1, d, tl, 0 }
#define IT_TOOL_WIDE(lbl, tl, d) { lbl, -1, -1, -1, d, tl, 1 } // HOME only: spans both columns, still selectable
#define IT_SEP(lbl)            { lbl, -2, -1, -1, NULL, -1, 0 } // non-selectable section header
#define IS_SEP(it)             ((it)->cheat == -2)

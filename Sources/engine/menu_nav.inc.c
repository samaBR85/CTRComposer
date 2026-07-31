// ============================================================================================
// MOTOR - voce normalmente nao edita este arquivo.
//
// Incluido por Sources/main.c na posicao original, entao a ordem das declaracoes continua
// valendo. Nao e compilado sozinho: o Makefile so compila Sources/*.c, e este arquivo esta em
// Sources/engine/.
// ============================================================================================

// Navegacao sobre o modelo de menu: quais linhas pular (separador/oculta) e a
// posicao visivel de um indice - usado pela rolagem.

static u8  folderFav[NUM_FOLDERS]; // folders starred for the quick menu (own Favorites lines, '#'-prefixed)
static u8  toolFav[NUM_TOOLS];     // tools starred for the quick menu (own Favorites lines, '&'-prefixed)
static int g_openFolder = -1;      // quick menu sets this to a folder id to open after it closes
static int g_openTool   = -1;      // quick menu sets this to a tool id to launch after it closes
// stable keys for tool favorites in Favorites.txt (index = tool id; order must match the T_* enum)
static const char *kToolKeys[NUM_TOOLS] = {
#if TOOLS_ONLY
    "Cheat Search", "RAM Dumper", "Hex Editor", "About", "Plugin Guide"
#else
    "Cheat Search", "RAM Dumper", "Hex Editor", "About", "Game Guide", "Plugin Guide", "Tracker"
#endif
};

// Hook for hiding rows dynamically (the reference build used it for a category filter).
// Nothing is hidden in the template; return 1 from here to skip a row in render+nav+scroll.
static int ItemHidden(int folderIdx, const Item *it)
{ (void)folderIdx; (void)it; return 0; }
// A row the cursor must skip over: a section header OR a filtered-out item.
static int NavSkip(int folderIdx, int c)
{ const Folder *f = &folders[folderIdx]; return IS_SEP(&f->items[c]) || ItemHidden(folderIdx, &f->items[c]); }
// Visible position (0-based) of a raw item index, skipping hidden items - used for scroll math.
static int VisPos(int folderIdx, int rawIdx)
{ int vp = 0; const Folder *f = &folders[folderIdx];
  for (int i = 0; i < f->count && i < rawIdx; ++i) if (!ItemHidden(folderIdx, &f->items[i])) vp++;
  return vp; }

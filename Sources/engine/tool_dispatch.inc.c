// ============================================================================================
// MOTOR - voce normalmente nao edita este arquivo.
//
// Incluido por Sources/main.c na posicao original, entao a ordem das declaracoes continua
// valendo. Nao e compilado sozinho: o Makefile so compila Sources/*.c, e este arquivo esta em
// Sources/engine/.
// ============================================================================================

// Despachante das ferramentas: traduz um id T_* na funcao que abre aquela tela.
// Fica depois de todas as Tool*() e antes de menu_loop.inc.c, o unico que o chama.

static void ToolRun(int t)
{
    if (t == T_SEARCH) ToolSearch();
    else if (t == T_RAMDUMP) ToolRamDump();
    else if (t == T_HEXEDIT) ToolHexEdit();
    else if (t == T_ABOUT) ToolAbout();
#if !TOOLS_ONLY
    else if (t == T_GAMEGUIDE) ToolGameGuide();
    else if (t == T_TRACKER)   ToolChecklist();
#endif
    else if (t == T_PLUGINGUIDE) ToolPluginGuide();
}

// ============================================================================================
// PARTE DO JOGO - voce edita este arquivo.
//
// Incluido por Sources/main.c na posicao original, entao a ordem das declaracoes continua
// valendo. Nao e compilado sozinho: o Makefile so compila Sources/*.c, e este arquivo esta em
// Sources/plugin/.
// ============================================================================================

// TODO o texto que este plugin traz na tela: as paginas do Plugin Guide (que explicam
// o SEU plugin) e a pagina de creditos do Game Guide. E so texto - o leitor que desenha
// tudo isso e generico e vive em engine/guide_reader.inc.c.
//
// Alternativa sem recompilar: <pasta do plugin>/guide/<Idioma>/{game,plugin}.txt no SD
// sobrescreve estas paginas em tempo de execucao (ver engine/storage.inc.c).

// Credits page for the Game Guide. If you ship someone else's walkthrough text,
// THIS is where you credit them - name the author, where it came from, and under
// what permission. Replace the placeholder below before you publish.
static const char *GUIDE_CREDITS =
    "Replace this page with credits for your guide content.\n"
    "\n"
    "If the walkthrough text is not yours, say so here:\n"
    "  - who wrote it\n"
    "  - where it came from\n"
    "  - that you have permission to redistribute it\n"
    "\n"
    "Do the same for anything else you build on: address maps, save-data\n"
    "research, art, translations. It costs one screen and it is the difference\n"
    "between a fan project and a rip.\n"
    "\n"
    "Game names and game content belong to their publisher.";

// ---- Plugin Guide (original content, explains this plugin) ----
// Estas paginas descrevem o SEU plugin para quem abrir o menu. As de fabrica falam do
// template - reescreva-as para o seu jogo.
static const GuidePage PLUGIN_PAGES[] = {
    { "Overview",
      "This plugin draws its own overlay on top of the running game.\n"
      "\n"
      "Press SELECT during the game to open the menu. The game pauses while the\n"
      "menu is open. Press SELECT again (from anywhere) to jump straight back to\n"
      "the game.\n"
      "\n"
      "Navigate with the D-Pad. A opens a folder or toggles a cheat. B goes back\n"
      "one level. X shows info about the selected item. Y stars a favorite." },
    { "Quick Menu & Favorites",
      "Star your most-used cheats with Y in the menu. Then hold L+SELECT (or\n"
      "R+SELECT) to open the Quick Menu: a compact list of just your favorites,\n"
      "without opening the full menu.\n"
      "\n"
      "The hotkey can be changed in Settings. Favorites, the toast toggle and the\n"
      "hotkey are saved to the SD card and survive a reboot." },
    { "Cheat Search",
      "Find the memory address of any value, then change it.\n"
      "\n"
      "Known Value: type a number you can see (e.g. your rupees), Search, then\n"
      "narrow the results as the value changes (Greater / Less / Changed...).\n"
      "\n"
      "Unknown Search: don't know the number? Take a snapshot, change the value\n"
      "in the game, then scan Increased / Decreased / Changed to close in on it.\n"
      "\n"
      "The real loop: Search, press SELECT to return to the game, change the\n"
      "value, SELECT to reopen (results are kept), scan again. Repeat until a few\n"
      "results remain. Press A on a result to poke a new value. L undoes a scan." },
    { "RAM Dumper",
      "Save a block of the game's memory to a .bin file on the SD card.\n"
      "\n"
      "Set a Start address (or press Y / From Search to pull the address you\n"
      "found in Cheat Search) and a Size, then Dump. Files are written to\n"
      "the plugin's own folder on the SD card, under dumps/.\n"
      "\n"
      "The tool only writes memory that is actually readable, so it never\n"
      "crashes on an unmapped address. Great for studying the bytes around a\n"
      "value you found." },
    { "Hex Editor",
      "Browse memory as a live hex grid and edit any byte on the spot.\n"
      "\n"
      "D-Pad moves the cursor (left/right one byte, up/down one row). L/R page\n"
      "up and down. X jumps to an address; Y jumps to your Cheat Search result.\n"
      "Press A to edit the byte under the cursor.\n"
      "\n"
      "Read-only regions are protected: editing there is refused instead of\n"
      "crashing. Unreadable bytes show as --." },
    { "Tips",
      "- SELECT is always 'back to the game', from any screen.\n"
      "- Reopening the menu returns you to where you were, even inside a tool.\n"
      "- Code-patch cheats are never auto-enabled on boot, by design.\n"
      "- Toast notifications can be turned off in Settings." },
};

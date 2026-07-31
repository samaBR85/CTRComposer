// ============================================================================================
// PARTE DO JOGO - voce edita este arquivo.
//
// Incluido por Sources/main.c na posicao original, entao a ordem das declaracoes continua
// valendo. Nao e compilado sozinho: o Makefile so compila Sources/*.c, e este arquivo esta em
// Sources/plugin/.
// ============================================================================================

// As pastas e linhas do menu. Puro dado - DrawMenuItem() desenha o que estiver aqui.
// O #if TOOLS_ONLY escolhe entre o HOME do template e o da build universal.

#if TOOLS_ONLY
// Universal build: the tools ARE the plugin, so they go straight on HOME with no folder to
// dig through. No cheats, no tracker, no game guide - none of those can mean anything when the
// same binary loads into every title on the system.
static const Item rootItems[] = {
    IT_SEP("MEMORY TOOLS"),
    IT_TOOL_WIDE("Cheat Search", T_SEARCH,  "Search this game's RAM for a value, then narrow it down (greater/less/changed...) to find its address. Poke results directly. Works on any title - it scans memory, it doesn't need to know the game."),
    IT_TOOL("RAM Dumper",   T_RAMDUMP, "Save a block of memory to a .bin on the SD card. Pick a start address and size, or pull the address from Cheat Search."),
    IT_TOOL("Hex Editor",   T_HEXEDIT, "Browse memory as a live hex grid and edit any byte on the spot. Read-only regions are protected."),
    IT_SEP("SYSTEM"),
    IT_TOOL("Plugin Guide", T_PLUGINGUIDE, "How to use this plugin: the menu, the quick menu, and the memory tools."),
    IT_TOOL("About",        T_ABOUT,   "Plugin info and credits."),
    IT_FOLDER("Settings",   F_SETTINGS),
};
#else
static const Item rootItems[] = {
    IT_SEP("CHEATS"),
    IT_FOLDER("Examples", F_EXAMPLES),
    IT_SEP("GUIDES"),
    IT_TOOL_WIDE("Tracker", T_TRACKER, "A general per-item progress tracker: each entry is untouched / auto / checked / cleared. Auto-fill syncs it from game memory. Ships with placeholder rows only - fill in CHK_CATS with your game's collectibles."),
    IT_TOOL("Game Guide",   T_GAMEGUIDE,   "A scrollable, categorized reader for your game's content. Ships with placeholder pages - replace them, or drop guide/English/game.txt on the SD card."),
    IT_TOOL("Plugin Guide", T_PLUGINGUIDE, "How to use this plugin: the menu, the quick menu, and the Cheat Search / RAM Dumper / Hex Editor tools."),
    IT_SEP("SYSTEM"),
    IT_FOLDER("Tools",    F_TOOLS),
    IT_FOLDER("Settings", F_SETTINGS),
};
#endif
#if !TOOLS_ONLY
static const Item toolsItems[] = {
    IT_TOOL("Cheat Search", T_SEARCH,  "Search the game's RAM for a value, then narrow it down (greater/less/changed...) to find its address. Poke results directly."),
    IT_TOOL("RAM Dumper",   T_RAMDUMP, "Save a block of the game's memory to a .bin file on the SD card. Pick a start address and size, or pull the address from Cheat Search."),
    IT_TOOL("Hex Editor",   T_HEXEDIT, "Browse memory as a live hex grid and edit any byte on the spot. Jump to an address, or to your Cheat Search result. Read-only regions are protected."),
    IT_TOOL("About",        T_ABOUT,   "Plugin info and credits."),
};

// EXAMPLE cheats - these demonstrate the shapes a cheat can take. Delete them and write your
// own; the descriptions are what the info box ({X}) shows.
// EVERY row here is INERT: EXAMPLE_ENABLED is 0, so toggling them writes nothing at all.
// They exist so you can walk the menu - navigation, auto-repeat, the {X} info box, {Y}
// favorites, toasts, the checkbox-vs-action distinction - before you have a single address.
static const Item exampleItems[] = {
    IT_SEP("CONTINUOUS (toggles)"),
    IT_CHEAT("Example: direct write",  CH_EX_DIRECT,
             "EXAMPLE - inert until you edit it. Writes a fixed 16-bit value to a fixed address every frame while it is on. The simplest kind of cheat: see EXAMPLE_ADDR_DIRECT in Sources/main.c."),
    IT_CHEAT("Example: byte write",    CH_EX_BYTE,
             "EXAMPLE - inert until you edit it. Same idea, but 8-bit. Match the write width (W8 / W16 / W32) to whatever the game actually stores at that address, or you will clobber the bytes next door."),
    IT_CHEAT("Example: 32-bit write",  CH_EX_WORD,
             "EXAMPLE - inert until you edit it. A 32-bit write, for counters and pointers that are a full word wide."),
    IT_CHEAT("Example: base + offset", CH_EX_BASEOFF,
             "EXAMPLE - inert until you edit it. Reads a pointer to the player struct, then writes a field at a fixed offset inside it. ALWAYS null-check the base before writing through it."),
    IT_CHEAT("Example: hold {HK}",     CH_EX_HOTKEY,
             "EXAMPLE - inert until you edit it. Only acts while you hold {HK} in game. Rebind that button in Settings - this text shows the live binding, because the token is swapped for the real glyph when the card opens."),
    IT_SEP("ONE-SHOT (actions)"),
    IT_CHEAT("Example: apply once",    CH_EX_ONESHOT,
             "EXAMPLE - inert until you edit it. Applied once, the moment you press {A}, instead of every frame. Use this for 'give me the item' style cheats. Note it gets a plain box, not a checkbox: it has no on/off state."),
    IT_CHEAT("Example: toggle a bit",  CH_EX_ONESHOT2,
             "EXAMPLE - inert until you edit it. A one-shot that flips a bit and then reads it back, so the flash says ADDED or REMOVED instead of just OK. Good for equipment-style cheats."),
    IT_SEP("PICKER"),
    IT_PICKER("Example: pick a value", PK_EXAMPLE,
              "EXAMPLE - inert until you edit it. Opens a list and writes the value you choose to one address. Because the address is still a placeholder, it will refuse the write and say so rather than poking address zero."),
};
#endif // !TOOLS_ONLY

static const Item settingsItems[] = {
    IT_SEP("GENERAL"),
#if TOOLS_ONLY
    IT_CHEAT("Change Theme", CH_CFG_THEME, "Recolor every menu live. Your pick is saved to the SD card."),
    IT_CHEAT("Language", CH_CFG_LANG, "Press {A} to cycle the menu language. Translations load from the plugin folder, under lang/. English is built in."),
#else
    IT_CHEAT("Change Theme", CH_CFG_THEME, "Recolor every menu live. The template ships one neutral theme; add your own to THEMES[] in Includes/themes.h. Your pick is saved."),
    IT_CHEAT("Language", CH_CFG_LANG, "Press {A} to cycle the menu language. Translations load from <plugin folder>/lang/. The template ships English only."),
#endif
    IT_CHEAT("Toggle notifications (toast)", CH_CFG_TOAST, "Shows a small notification in-game when something is toggled."),
#if !TOOLS_ONLY
    IT_CHEAT("Auto-fill Tracker on open", CH_CFG_AUTOFILL, "When on, the Tracker syncs itself from game memory every time you open it."),
#endif
    IT_SEP("IN-GAME HOTKEYS"),
    IT_CHEAT("Quick Menu hotkey", CH_CFG_QMKEY, "Press {A} to cycle the button combo that opens the quick menu in game."),
#if !TOOLS_ONLY
    IT_CHEAT("Example hotkey 1", CH_CFG_HK1, "Press {A} to cycle the button used by the 'hold' example cheat."),
    IT_CHEAT("Example hotkey 2", CH_CFG_HK2, "Press {A} to cycle a second in-game hotkey. Wire it to one of your own cheats."),
#endif
    IT_CHEAT("Reset hotkeys to default", CH_CFG_HKRESET, "Press {A} to restore the Quick Menu and example hotkeys to their defaults ({L}+SELECT / {Y} / {X})."),
};

#define FCOUNT(a) (int)(sizeof(a) / sizeof((a)[0]))
static const Folder folders[NUM_FOLDERS] = {
#if TOOLS_ONLY
    { "CTRComposer Tools",    rootItems,     FCOUNT(rootItems) },
    { "Settings",             settingsItems, FCOUNT(settingsItems) },
#else
    { "CTRComposer Template", rootItems,     FCOUNT(rootItems) },
    { "Examples",             exampleItems,  FCOUNT(exampleItems) },
    { "Tools",                toolsItems,    FCOUNT(toolsItems) },
    { "Settings",             settingsItems, FCOUNT(settingsItems) },
#endif
};

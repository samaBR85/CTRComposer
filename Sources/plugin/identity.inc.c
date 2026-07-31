// ============================================================================================
// PARTE DO JOGO - quem e este plugin e onde ele guarda os arquivos dele.
//
// Incluido no topo de Sources/main.c. Nao e compilado sozinho (o Makefile so compila
// Sources/*.c e este esta em Sources/plugin/).
// ============================================================================================

// 1 = the universal build for Luma's /luma/plugins/default.3gx slot, which loads into any title
// with no plugin folder of its own. Drops the per-game parts (example cheats, tracker, game
// guide) and puts the memory tools straight on HOME; the engine itself is unchanged.
// CAUTION: as default.3gx it loads into EVERYTHING - Home Menu, applets, homebrew - flipping
// each host process to RWX and pausing its threads. Much broader blast radius. Experimental.
#define TOOLS_ONLY 0

// Opt-in teardown on Luma's process-exit event. OFF because it was measured never to fire (see
// ThreadMain). Kept in case a future Luma delivers it: flipping this to 1 also writes a marker
// file at shutdown, so one run tells you.
#define EXIT_HANDSHAKE 0

// Bump EVERY build - the on-screen tag is your proof that the .3gx on the SD card is the one you
// just compiled. Edit the three numbers for real and check the result on screen; a blind sed can
// no-op silently. PLUGIN_TAG below is derived from these, not typed again - a stray hand-edit of
// just one of the two used to slip past the CI, which sedded TOOLS_ONLY/PLUGIN_NAME only.
#define PLUGIN_VER_MAJOR 1
#define PLUGIN_VER_MINOR 1
#define PLUGIN_VER_PATCH 2

#define PLUGIN_STR2(x) #x
#define PLUGIN_STR(x) PLUGIN_STR2(x)
#define PLUGIN_VER "v" PLUGIN_STR(PLUGIN_VER_MAJOR) "." PLUGIN_STR(PLUGIN_VER_MINOR) "." PLUGIN_STR(PLUGIN_VER_PATCH) // About screen and pause box
#define PLUGIN_VER_SHORT PLUGIN_STR(PLUGIN_VER_MAJOR) "." PLUGIN_STR(PLUGIN_VER_MINOR) // tag has no room for the patch digit

// Name and tag follow TOOLS_ONLY, so flipping that flag is the only edit the other build needs.
#if TOOLS_ONLY
#define PLUGIN_NAME "CTRComposer Tools"
#define PLUGIN_TAG  "T" PLUGIN_VER_SHORT   // compact tag - cramped menu title bar
#else
#define PLUGIN_NAME "CTRComposer"
#define PLUGIN_TAG  PLUGIN_VER_SHORT
#endif

// ===================== Where this plugin keeps its files =====================
// Holds Settings.cfg, Favorites.txt, Tracker.txt, lang/, guide/ and dumps/.
// Set it to your game's folder once you know the Title ID:
//     #define PLUGIN_DIR "/luma/plugins/0004000000033500/"
// Left empty everything lands in /luma/plugins/ itself, which works but is shared by every
// game - two plugins built from this template would fight over the same Settings.cfg.
#define PLUGIN_DIR ""

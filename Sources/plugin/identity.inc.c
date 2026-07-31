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
// just compiled. Edit it for real and check it on screen; a blind sed can no-op silently.
#define PLUGIN_VER "v1.0.1"           // About screen and pause box

// Name and tag follow TOOLS_ONLY, so flipping that flag is the only edit the other build needs.
#if TOOLS_ONLY
#define PLUGIN_NAME "CTRComposer Tools"
#define PLUGIN_TAG  "T1.0"              // compact tag - cramped menu title bar
#else
#define PLUGIN_NAME "CTRComposer"
#define PLUGIN_TAG  "1.0"
#endif

// ===================== Where this plugin keeps its files =====================
// Holds Settings.cfg, Favorites.txt, Tracker.txt, lang/, guide/ and dumps/.
// Set it to your game's folder once you know the Title ID:
//     #define PLUGIN_DIR "/luma/plugins/0004000000033500/"
// Left empty everything lands in /luma/plugins/ itself, which works but is shared by every
// game - two plugins built from this template would fight over the same Settings.cfg.
#define PLUGIN_DIR ""

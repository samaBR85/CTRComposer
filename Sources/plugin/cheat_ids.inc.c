// ============================================================================================
// PARTE DO JOGO - voce edita este arquivo.
//
// Incluido por Sources/main.c na posicao original, entao a ordem das declaracoes continua
// valendo. Nao e compilado sozinho: o Makefile so compila Sources/*.c, e este arquivo esta em
// Sources/plugin/.
// ============================================================================================

// Um CH_* por cheat. Precisa vir cedo: NUM_CHEATS dimensiona os arrays de config.

// ===================== Cheat IDs =====================
// Game-specific. One enum entry per cheat, then a row in a Folder below (IT_CHEAT) and an
// implementation in ApplyCheats() (continuous) or OneShot() (applied once).
// CH_CFG_* are not cheats - they are Settings rows reusing the same row-drawing code.
enum {
    // ---- EXAMPLE cheats: replace these with your game's ----
    CH_EX_DIRECT,     // continuous: direct u16 write to a fixed address
    CH_EX_BYTE,       // continuous: u8 write
    CH_EX_WORD,       // continuous: u32 write
    CH_EX_BASEOFF,    // continuous: base pointer + offset write
    CH_EX_HOTKEY,     // continuous: only while a rebindable hotkey is held
    CH_EX_ONESHOT,    // one-shot: applied once, when you select it
    CH_EX_ONESHOT2,   // one-shot with a custom result message
    // ---- Settings rows (not cheats) ----
    CH_CFG_TOAST, CH_CFG_AUTOFILL, CH_CFG_QMKEY, CH_CFG_HK1, CH_CFG_HK2,
    CH_CFG_HKRESET, CH_CFG_THEME, CH_CFG_LANG,
    NUM_CHEATS
};
static u8 cheatState[NUM_CHEATS];
static u8 favorite[NUM_CHEATS];

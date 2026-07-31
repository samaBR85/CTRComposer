// ============================================================================================
// PARTE DO JOGO - voce edita este arquivo.
//
// Incluido por Sources/main.c na posicao original, entao a ordem das declaracoes continua
// valendo. Nao e compilado sozinho: o Makefile so compila Sources/*.c, e este arquivo esta em
// Sources/plugin/.
// ============================================================================================

// Enderecos e escritas. ApplyCheats() roda todo frame com o menu fechado;
// OneShot() dispara uma vez quando voce aperta A na linha.

// ===================== Cheat implementations =====================
// The plugin runs inside the game's process, so writing game memory is just a pointer write
// (W8/W16/W32 above) - all you need are the addresses, from an existing code bank or from the
// engine's own Cheat Search.
//
// Addresses are ALWAYS region- and version-specific. Re-anchor when either changes, or the
// writes land somewhere random and crash the game.
//
// The EXAMPLE_* addresses below are fake and guarded: EXAMPLE_ENABLED is 0, so nothing is
// written until you set it to 1 with real addresses in place.
#define EXAMPLE_ENABLED 0

// EXAMPLE - replace with your game's address. A counter you want pinned to a value.
#define EXAMPLE_ADDR_DIRECT   0x00000000u
#define EXAMPLE_VALUE_DIRECT  0x03E7       // 999

// EXAMPLE - replace with your game's address. The classic base+offset pattern: a
// pointer to the player/entity struct, and a field at a fixed offset inside it.
#define EXAMPLE_ADDR_BASE     0x00000000u  // holds a pointer
#define EXAMPLE_OFF_FIELD     0x00u
#define EXAMPLE_VALUE_FIELD   0x00000000u

// EXAMPLE - replace with your game's address. Written once, when selected in the menu.
#define EXAMPLE_ADDR_ONESHOT  0x00000000u
#define EXAMPLE_VALUE_ONESHOT 0xFF

// Read the base pointer for base+offset cheats. Returns 0 when it looks unusable, so
// every caller can just check for 0 and skip - never write through a null base.
static u32 ExampleBase(void)
{
    if (!EXAMPLE_ENABLED) return 0;
    return R32(EXAMPLE_ADDR_BASE);
}

// One-shot cheats: applied instantly when selected in the menu, then the row flashes.
// Return 1 if `id` is a one-shot (so the menu knows not to treat it as a toggle).
// Set g_oneShotMsg to override the "OK" flash text - handy for ADDED/REMOVED toggles.
static int OneShot(int id)
{
    g_oneShotMsg = "OK"; // default flash text
    switch (id)
    {
        case CH_EX_ONESHOT:
            if (EXAMPLE_ENABLED) W8(EXAMPLE_ADDR_ONESHOT, EXAMPLE_VALUE_ONESHOT);
            else g_oneShotMsg = "EXAMPLE";  // nothing written: see EXAMPLE_ENABLED above
            return 1;

        // Same thing, but reporting a RESULT. A toggle-style one-shot (flip a bit, then read
        // it back) can say which way it went, so the flash is unambiguous instead of a bare OK.
        case CH_EX_ONESHOT2:
            if (EXAMPLE_ENABLED)
            {
                u8 v = (u8)(R8(EXAMPLE_ADDR_ONESHOT) ^ 0x01);
                W8(EXAMPLE_ADDR_ONESHOT, v);
                g_oneShotMsg = (v & 0x01) ? "ADDED" : "REMOVED";
            }
            else g_oneShotMsg = "EXAMPLE";
            return 1;

        // Add your one-shots here:
        //   case CH_MY_CHEAT: W16(0x00123456, 0x0064); return 1;
        //
        // For a CODE patch (an instruction rewrite in the read-only .text segment):
        //   svcControlProcess(CUR_PROCESS_HANDLE, PROCESSOP_SET_MMU_TO_RWX, 0, 0); // once
        //   ALWAYS save the original instruction first so the cheat can be switched off,
        //   then W32() the new one and flush:
        //   svcFlushEntireDataCache(); svcInvalidateEntireInstructionCache();
        // NEVER auto-enable a code patch on boot.
    }
    return 0;
}

// Continuous cheats: applied every tick while the menu is CLOSED (game running).
// Keep this cheap - it runs at game framerate.
static void ApplyCheats(void)
{
    // Guard, not #if: the example bodies below stay COMPILED (so they cannot silently rot
    // as the engine changes) while -Os folds them away entirely until you flip the flag.
    if (!EXAMPLE_ENABLED) return;

    u32 pad = HID_PAD;

    // EXAMPLE - direct write. Pins a value for as long as the cheat is on.
    // W8 / W16 / W32 pick the width; match whatever the game actually stores there.
    if (cheatState[CH_EX_DIRECT])
        W16(EXAMPLE_ADDR_DIRECT, EXAMPLE_VALUE_DIRECT);
    if (cheatState[CH_EX_BYTE])
        W8(EXAMPLE_ADDR_DIRECT, 0x63);           // 99, the classic "max this counter"
    if (cheatState[CH_EX_WORD])
        W32(EXAMPLE_ADDR_DIRECT, 0x0000270F);    // 9999

    // EXAMPLE - base+offset write. ALWAYS null-check the base before writing through it.
    if (cheatState[CH_EX_BASEOFF])
    {
        u32 base = ExampleBase();
        if (base) W32(base + EXAMPLE_OFF_FIELD, EXAMPLE_VALUE_FIELD);
    }

    // EXAMPLE - hold-to-act, using the player's rebindable hotkey instead of a fixed button.
    if (cheatState[CH_EX_HOTKEY] && (pad & hotKeys[hk1].mask))
    {
        u32 base = ExampleBase();
        if (base) W32(base + EXAMPLE_OFF_FIELD, EXAMPLE_VALUE_FIELD);
    }
}

// True only for genuine on/off toggles - the cheats ApplyCheats() holds continuously.
// Everything else is a one-shot action or a shortcut, and gets a plain tinted box instead
// of a checkbox, so the menu never shows an "off" checkbox next to something that isn't
// stateful. KEEP THIS IN SYNC with the cheatState[] uses in ApplyCheats().
static int IsToggleCheat(int id)
{
    switch (id)
    {
        case CH_EX_DIRECT: case CH_EX_BYTE: case CH_EX_WORD:
        case CH_EX_BASEOFF: case CH_EX_HOTKEY:
            return 1;
        default: return 0;
    }
}

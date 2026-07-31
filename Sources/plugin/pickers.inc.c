// ============================================================================================
// PARTE DO JOGO - voce edita este arquivo.
//
// Incluido por Sources/main.c na posicao original, entao a ordem das declaracoes continua
// valendo. Nao e compilado sozinho: o Makefile so compila Sources/*.c, e este arquivo esta em
// Sources/plugin/.
// ============================================================================================

// Uma linha de menu que abre uma lista e escreve o valor escolhido em um endereco.

// ===================== Pickers (choose a value from a list) =====================
// A picker is a menu row that opens a list and writes the chosen value to one address.
// Good for "which item is in this slot" style cheats where a toggle makes no sense.
typedef struct { const char *name; u8 val; } PickOpt;
typedef struct { const char *title; const PickOpt *opts; int count; u32 addr; } Picker;

// EXAMPLE - replace the options and the address with your game's.
static const PickOpt exampleOpts[] = {
    { "None",     0x00 }, { "Option A", 0x01 }, { "Option B", 0x02 },
    { "Option C", 0x03 }, { "Option D", 0x04 },
};

enum { PK_EXAMPLE, NUM_PICKERS };
static const Picker pickers[NUM_PICKERS] = {
    { "Example Slot", exampleOpts, (int)(sizeof(exampleOpts)/sizeof(exampleOpts[0])), EXAMPLE_ADDR_DIRECT },
};

// A picker points at a GAME address, and that address is a placeholder (0) until you fill it
// in - and even then it can be wrong, or unmapped in the current scene. NEVER dereference it
// blind: an unmapped read on the 3DS is a data abort that hard-freezes the console, with the
// menu still on screen. These two wrap every picker access.
static int PickerRead(const Picker *pk, u8 *out)
{
    if (!pk->addr || !MemReadable(pk->addr)) return 0;
    *out = R8(pk->addr);
    return 1;
}
static int PickerWrite(const Picker *pk, u8 v)
{
    if (!pk->addr || !MemWritable(pk->addr)) return 0;
    W8(pk->addr, v);
    return 1;
}

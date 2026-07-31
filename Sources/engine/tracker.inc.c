// ============================================================================================
// MOTOR - voce normalmente nao edita este arquivo.
//
// Incluido por Sources/main.c na posicao original, entao a ordem das declaracoes continua
// valendo. Nao e compilado sozinho: o Makefile so compila Sources/*.c, e este arquivo esta em
// Sources/engine/.
// ============================================================================================

// Tipos do tracker de progresso (ChkItem/ChkCat) e os kinds de deteccao
// automatica. Os dados de cada jogo vem de Sources/plugin/tracker_data.inc.c.
//
// O tracker inteiro (este arquivo + tracker_data.inc.c + tracker_ui.inc.c) so existe fora
// da build TOOLS_ONLY - por isso o #if abre aqui e so fecha no fim de tracker_ui.inc.c
// (main.c espelha o mesmo #if em volta do #include de plugin/tracker_data.inc.c).
#if !TOOLS_ONLY

// ===================== Completion tracker (per-item progress) =====================
// Each item is in one of four states:
//     0 untouched   1 auto-detected   2 you checked it   3 you cleared it
//
// AUTO-FILL IS A SYNC, NOT A TALLY: it re-reads every detectable item and both sets marks it
// finds and clears stale auto-marks it no longer finds, never touching a manual mark. That is
// what keeps it correct after a save reload.
//
// Detection kinds, all read-only, keyed off addresses you supply:
//   CK_MANUAL  - no memory signal; only you can tick it
//   CK_BIT     - (R8(addr) & mask) != 0        a flag bit inside a byte
//   CK_BYTEEQ  - R8(addr) == mask              an exact byte value
//   CK_NONZERO - R8(addr) != 0                 "the slot is filled"
// A new kind means extending this enum and ChecklistAutoFill() together.
//
// CHK_CATS below is a placeholder pointing at address 0 - replace it with your game's
// collectibles, or delete the Tracker row from rootItems[] if there is nothing to track.
//
// Progress persists KEYED BY THE `key` STRING, not by position, so items can be added, removed
// and reordered without invalidating saved progress. Never reuse a key for a different item.
enum { CK_MANUAL = 0, CK_BIT = 1, CK_BYTEEQ = 2, CK_NONZERO = 3 };
enum { CKI_HEART, CKI_SKULL, CKI_NOTE, CKI_KEYITEM, CKI_NONE };

typedef struct {
    const char *key;    // stable save-key, never shown (so item text can be edited freely later)
    const char *task, *hint, *loc; // loc = "" -> no location to reveal
    u8 iconKind; u16 iconArg; u8 kind;
    u32 addr; u8 mask;
} ChkItem;
typedef struct { const char *name; const ChkItem *items; int count; } ChkCat;

#endif // !TOOLS_ONLY

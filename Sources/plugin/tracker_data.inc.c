// ============================================================================================
// PARTE DO JOGO - voce edita este arquivo.
//
// Incluido por Sources/main.c na posicao original, entao a ordem das declaracoes continua
// valendo. Nao e compilado sozinho: o Makefile so compila Sources/*.c, e este arquivo esta em
// Sources/plugin/.
// ============================================================================================

// Os itens do tracker. Puro dado: a UI nao sabe quantas categorias ou itens existem.
// Se o jogo nao tem nada para rastrear, apague a linha Tracker de menu_tables.inc.c.

static const ChkItem CK_EXAMPLE[] = {
    // key            task                  hint                                   location   icon        arg  kind        addr  mask
    { "ex_manual",   "Example: manual only", "Nothing in memory tells us about this one, so you tick it yourself with {A}.", "", CKI_KEYITEM, 0, CK_MANUAL,  0x00000000, 0x00 },
    { "ex_bit",      "Example: flag bit",    "Auto-detected when a chosen bit is set in a chosen byte. Point addr/mask at your game.", "", CKI_NOTE,    0, CK_BIT,     0x00000000, 0x01 },
    { "ex_nonzero",  "Example: slot filled", "Auto-detected when a byte is anything other than zero - good for 'is this inventory slot used'.", "", CKI_SKULL, 0, CK_NONZERO, 0x00000000, 0x00 },
};

static const ChkCat CHK_CATS[] = {
    { "Examples", CK_EXAMPLE, (int)(sizeof(CK_EXAMPLE) / sizeof(CK_EXAMPLE[0])) },
};

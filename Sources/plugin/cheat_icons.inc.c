// ============================================================================================
// PARTE DO JOGO - voce edita este arquivo.
//
// Incluido por Sources/main.c na posicao original, entao a ordem das declaracoes continua
// valendo. Nao e compilado sozinho: o Makefile so compila Sources/*.c, e este arquivo esta em
// Sources/plugin/.
// ============================================================================================

// Qual icone ilustra cada linha de cheat. Os SPRK_* sao os icones vetoriais
// desenhados em codigo, definidos logo acima em main.c.

// Which icon illustrates each cheat row (-1 = none, which is fine for most rows).
// EXAMPLE mapping - repoint these at your own cheats, or just return -1 everywhere.
static int SpriteKeyForCheat(int ch)
{
    switch (ch)
    {
        case CH_EX_DIRECT:  return SPRK_PIN;
        case CH_EX_HOTKEY:  return SPRK_CLOCK;
    }
    return -1;
}

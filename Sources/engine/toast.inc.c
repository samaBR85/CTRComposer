// ============================================================================================
// MOTOR - voce normalmente nao edita este arquivo.
//
// Incluido por Sources/main.c na posicao original, entao a ordem das declaracoes continua
// valendo. Nao e compilado sozinho: o Makefile so compila Sources/*.c, e este arquivo esta em
// Sources/engine/.
// ============================================================================================

// O aviso rapido que aparece e some sozinho (ex: cheat ligado).

// ===================== Toast =====================
static char toastMsg[48];
static volatile int toastTicks;

static void QueueToastRaw(const char *label, const char *suffix)
{
    if (!cheatState[CH_CFG_TOAST]) return;
    int i = 0;
    while (label[i] && i < 38) { toastMsg[i] = label[i]; i++; }
    for (int j = 0; suffix[j] && i < 46; ++j) toastMsg[i++] = suffix[j];
    toastMsg[i] = 0;
    toastTicks = 625; // ~2.5s at the 4ms toast tick
}
static void QueueToast(const char *label, int on) { QueueToastRaw(label, on ? ": ON" : ": OFF"); }

static void ToastTick(void)
{
    // Unreachable today in practice: toastTicks only becomes >0 via QueueToastRaw(), and every
    // call site of that is inside RunMenu()/QuickMenu(), which already require gCompose before
    // getting this far. The check here is explicit anyway - CFill/CText6Btn below write through
    // gCompose, and that implicit chain is exactly the kind of thing a new toast call site added
    // later could break silently without this guard.
    if (!gCompose) return;
    if (toastTicks <= 0) return;
    toastTicks--;

    int w = C6BtnWidth(toastMsg) + 10, h = 14; // glyph-aware: cheat labels may carry {A}/{L} tokens
    int x0 = TOP_W - 6 - w, y0 = TOP_H - 6 - h;

    CFill(x0, y0, w, h, BG); // theme background: keeps text readable on light & dark themes
    CFill(x0, y0, w, 1, GOLD); CFill(x0, y0 + h - 1, w, 1, GOLD);
    CFill(x0, y0, 1, h, GOLD); CFill(x0 + w - 1, y0, 1, h, GOLD);
    CText6Btn(x0 + 5, y0 + 2, toastMsg, INK);

    FbInfo f = GetFb(0);
    BlitTopRect(&f, x0, y0, w, h);
}

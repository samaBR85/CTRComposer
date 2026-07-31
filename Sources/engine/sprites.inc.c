// ============================================================================================
// MOTOR - voce normalmente nao edita este arquivo.
//
// Incluido por Sources/main.c na posicao original, entao a ordem das declaracoes continua
// valendo. Nao e compilado sozinho: o Makefile so compila Sources/*.c, e este arquivo esta em
// Sources/engine/.
// ============================================================================================

// Blit generico de arte RGBA4444 (DrawScaled) e as chaves SPRK_* que
// Sources/plugin/cheat_icons.inc.c usa para escolher o icone de cada cheat.

// ===================== Sprites (RGBA4444 art) =====================
// No sprite sheet ships with the template - only the machinery: DrawImg() (1:1 blit),
// DrawScaled() (any source size onto any rect), and the code-drawn vector icons below.
//
// Adding real art: convert to an RGBA4444 array (v = R4<<12 | G4<<8 | B4<<4 | A4) and pack with
// ROUND-TO-NEAREST, clamp((c + 8) / 17, 0, 15). Truncating with c>>4 biases every channel up by
// as much as +15/255 and visibly brightens the art. Assets/gen_glyphs.py packs it correctly.

// Nearest-neighbour scaled blit of an arbitrary RGBA4444 sprite (sw x sh) into a dst rect
// (dw x dh) on the top-screen compose buffer, alpha-blended. dim (0..255) darkens toward
// black - used to grey out the hex keys on the keypad while it is in DEC mode. One routine
// handles every art size, so a single small source tile can fill a large key.
static void DrawScaled(int dx, int dy, int dw, int dh, const u16 *px, int sw, int sh, int dim)
{
    for (int yy = 0; yy < dh; ++yy)
    {
        int sy = yy * sh / dh; int Y = dy + yy;
        if ((unsigned)Y >= TOP_H) continue;
        for (int xx = 0; xx < dw; ++xx)
        {
            int sx = xx * sw / dw; int X = dx + xx;
            if ((unsigned)X >= TOP_W) continue;
            u16 v = px[sy * sw + sx];
            u32 a = (u32)(v & 0xF) * 17;
            if (!a) continue;
            u8 r = (u8)(((v >> 12) & 0xF) * 17);
            u8 g = (u8)(((v >> 8) & 0xF) * 17);
            u8 b = (u8)(((v >> 4) & 0xF) * 17);
            if (dim) { r = (u8)(r * (255 - dim) / 255); g = (u8)(g * (255 - dim) / 255); b = (u8)(b * (255 - dim) / 255); }
            u8 *p = CPix(X, Y);
            p[0] = (u8)((p[0] * (255 - a) + r * a) / 255);
            p[1] = (u8)((p[1] * (255 - a) + g * a) / 255);
            p[2] = (u8)((p[2] * (255 - a) + b * a) / 255);
        }
    }
}

// Pseudo-keys for the code-drawn vector icons below. There is no sprite sheet in the
// template, so every key here maps to a function, not to art. Add your own the same way.
#define SPRK_SUNRISE  0x1F0
#define SPRK_DAY      0x1F1
#define SPRK_SUNSET   0x1F2
#define SPRK_NIGHT    0x1F3
#define SPRK_CLOCK    0x1F4
#define SPRK_RAIN     0x1F5
#define SPRK_PIN      0x1F7
#define SPRK_PORTAL   0x1F8

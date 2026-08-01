# Instructions for Claude Code in this repository

## Comments: the bar is high, and it is enforced

This project was publicly criticised for *"too much commenting and text (something that AI
typically does)"*. That criticism was fair and it was acted on. **Do not undo it.**

Before writing a comment, decide which of these it is:

| | |
|---|---|
| ❌ **Delete** | Development narrative — *"a previous version tried X"*, *"this used to be broken"*, *"was missing"*. That belongs in the commit message, not the source. |
| ❌ **Delete** | Anything the code already says. `i++; // increment i` |
| ✅ **Keep** | A constraint the code **cannot show by itself** — a hardware quirk, a trap, a rule that will silently break something if violated. |
| ✂️ **Shorten** | Section headers. Keep the title, cut the essay. |

The traps below are the standard for what earns a comment. They are all real, all cost someone
hours, and none is visible from reading the code:

- the colour macros expand to three comma-separated arguments, so `on ? GREEN_ON : INK` compiles
  and silently draws the wrong channels
- never call `hidInit()` — the touch panel is mapped by hand
- an address of `0` in a picker hangs the console
- pack RGBA4444 with round-to-nearest, `clamp((c + 8) / 17, 0, 15)`, or the art comes out brighter

**Check yourself before finishing.** Explanatory comments should sit around **10–12%** of
non-blank lines:

```sh
sh Tools/comment-ratio.sh
```

If a change pushes it up, the new comments had better be traps, not narrative.

## Structure

```
Sources/
  main.c        the #include list, in build order, plus ThreadMain/main(). ~250 lines.
  plugin/       what a plugin author edits: cheats, menus, guide text, identity
  engine/       the engine: render, theme, menu, tools, guides, tracker, storage
```

Two rules that break the build if violated:

- **`.inc.c` files must stay in a subfolder.** The Makefile compiles `Sources/*.c`; one sitting
  loose in `Sources/` gets compiled a second time and the link dies in "multiple definition".
- **`#if` must balance inside each physical file.** A conditional that opens in one `.inc.c` and
  closes in another is a compile error, even though the translation unit is balanced overall.
  Put the guard around the `#include` in `main.c` instead.

## Building

The devkitPro toolchain cannot build from a path containing a space, and this checkout is at
`D:\Claude Code\...`. Use the script — it mirrors to a space-free path, builds **both** variants,
runs the same checks CI does, and copies the artifacts back to `releases/`:

```sh
sh Tools/build.sh
```

`-Wall -Wextra` is on and the tree is at **zero warnings**. Keep it there — a permanent warning
trains everyone to ignore warnings.

## Verifying a change

- **Pure reorganisation** (moving code between files) must produce a **byte-identical** `.3gx`.
  Prove it with `cmp`, or with `sh Tools/fingerprint.sh` if you deliberately reordered
  declarations. If it differs and you did not mean it to, stop and investigate.
- **Behaviour changes** have no oracle. They need a real console. Bump the version first
  (`Sources/plugin/identity.inc.c` **and** the `Version:` block in `CTRComposer.plgInfo` — two
  files, two formats, neither derivable from the other) so the on-screen version proves which
  build is on the card.

## After any structural change, check all four surfaces

Byte-identical verification says the compiler emitted the same code. It says nothing about
documentation or automation, and these have all drifted silently before:

1. **CI** — `.github/workflows/build.yml` `sed`s a flag by file path. Confirm the run went green;
   do not assume.
2. **README** — it names specific files and symbols.
3. **Site** — the `gh-pages` branch.
4. **Wiki** — a separate repo, `CTRComposer.wiki.git`.

## Related documents

- `CORRECOES-MOTOR.md` — four engine bugs and their patches, for forks predating v1.1.4
- `MIGRACAO-PLUGINS-DERIVADOS.md` — moving a fork to the `plugin/` + `engine/` layout
- `PLANO-REFATORACAO.md` — why the structure is what it is, including a proposal that was tested
  and rejected

Those three are in Portuguese, for the sessions maintaining the derived plugins. **Code and code
comments are in English.**

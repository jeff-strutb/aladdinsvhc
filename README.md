# Aladdin (SNES): bit-exact decompilation

**In progress: Phase 2 of the plan. The ROM rebuilds byte for byte from
source; one routine's C is proven behaviour exact.** Started October 28, 2026.
The goal is the same as the Stunt Race FX decomp's: every routine as readable
C proven to behave exactly like the original, and every routine as symbolic,
commented assembly that rebuilds the ROM byte for byte.

> **Behaviour exact, not cycle exact.** The C states no instruction timing:
> instruction counts are an emulator's bookkeeping and were never part of the
> original source. See [Timing](#timing-behaviour-exact).

| | Behaviour exact (M1) | Byte exact (M2) |
|---|---|---|
| **Whole ROM rebuilt from source** | | **yes** (0 of 1,310,720 B differ) |
| Routines certified | 1 (SoundCommandSend, $80:B463) | 0 |
| S-CPU (65816) code traced | 37,607 B | |
| SPC700 (sound) code traced | 2,268 B | |

- **The routine count is not yet known exactly.** The census puts it at
  about 923 routines (813 to 987): 594 named from the traces, 109 found
  statically, 220 orphans. The bars and treemap of the Stunt Race FX README
  come once the C lane is under way.
- **The first routine is proven.** SoundCommandSend's C is identical to the
  original on all 1,139 calls of the level 1 script under the per-call check
  (A5) and in lockstep (A7, 1,216 frames, 0 differing). Three planted bugs
  each fail both.
- **The whole ROM rebuilds byte for byte** from the raw intake; a planted
  byte fails the image gate.

The figures are from `catalog.py`, `census.py`, `codemap.py` and `image.py`
on the tree of October 28, 2026. Query the tree; do not trust a number in
this file.

**Maintainer:** Jeffrey Wilbur, Strut B, LLC\
**Contact:** [retro@strutb.com](mailto:retro@strutb.com)

## License, and no claim to the game

**I claim no ownership whatsoever of the decompiled source code.** Aladdin,
its code and its assets belong to its rights holders. The C and assembly in
this repository are a reconstruction of their work, recovered so that they
rebuild the original bytes. It is not mine to own, license or give away, and
nothing here says otherwise. No game data ships in this repository: every
build starts from your own copy of the cartridge.

This project exists for preservation and for love of this game, so that the
platformer Capcom made in 1993 can still be read, studied, built and played
long after the cartridges and hardware it shipped on are gone.

The work that is my own (the tools and the test harness) is dedicated to the
public domain under [CC0 1.0 Universal](LICENSE). Anyone may use it for any
purpose, with no permission or credit needed.

**Why CC0.** A preservation project should claim as little as possible. CC0
asserts nothing over the reconstructed game, and it gives everything that is
mine to the public domain, with a fallback licence for countries where
copyright cannot be waived outright. That fallback is why CC0 was chosen over
the Unlicense, which has the same intent but no fallback.

## The Decompilation Standard

The purpose of a decompilation, as opposed to a recompilation, is
preservation: recovering the game itself, as written, in a form that outlives
the original hardware and media. Playability and portability are welcome side
effects, not the direct goal. With a bit-exact decompilation, every function,
built with the original compiler, must reproduce the original machine code
byte for byte. A bit-exact match is 100% reliability that the source is
accurate to the original, ensuring preservation for future generations.

## Project Purpose

A decompilation of **Aladdin** (SNES, 1993): every routine rewritten as
readable C, documented with what it does and how, filed by what it is for,
and proven to behave exactly like the original; and the whole ROM rebuilt byte
for byte. Progress is measured only by those two proofs; playability is a
consequence, never a reason to reorder the work.

## Game background

Capcom's *Aladdin* for the SNES is a 1.25 MB LoROM FastROM cartridge with no
coprocessor and no cartridge RAM. Two processors run it: the SNES CPU (a
65816) runs the game, from the FastROM banks $80 up after its first
instructions at reset, and the SPC700 sound CPU plays the music and effects
from a driver, two sample sets and one block per song that the main CPU
uploads by index from a table at $85:8000. The ROM has no compiler footprint:
both programs were written by hand in assembly.

The game runs its screens and modes as **cooperative tasks**: eight slots,
each with its own stack and direct page, and a scheduler that switches
between them. A task waits for frames mid-routine, sometimes several calls
deep, and resumes later on its own stack. That shapes the decomp more than
anything else here; see [Tasks](#tasks).

## Two lanes, one game

When a game was compiled from C, the right C fed to the original compiler
gives back the original bytes, so one source carries both proofs. No compiler
produced these bytes, so no C can rebuild them. Each routine therefore has two
sources:

- **C, the decompilation.** Readable C per routine, certified behaviour exact
  against the running original (T3).
- **Assembly, the byte-exact companion.** Symbolic, commented assembly that
  rebuilds the ROM byte for byte (T4). It is the reading the C is written
  from, and the byte-exact rebuild proves that reading is right.

## Timing: behaviour exact

**The standard is behaviour exact, not cycle exact**, as in the Stunt Race FX
decomp. The C states no instruction time on either processor: statements of
how long the original's instructions took are an emulator's bookkeeping, never
in the original source, so a preservation decompilation does not carry them.
Each routine's inputs to outputs must be exactly the original's; how long it
took is the console's business, not the game's C.

## Tasks

The oracles judge a routine from its call to its return on one stack, and C
has no way to stop mid-function and continue frames later. A task that waits
breaks both. The design for this, for decision before Phase 3, is in
[docs/aladdin/TASKS.md](docs/aladdin/TASKS.md): tasks as coroutines (each
task's C on a native stack of its own, a wait suspending the whole chain, the
task stacks in work RAM kept byte for byte), with A5 judging a waiting call
in segments. Until that harness exists, only routines that cannot wait can
reach T3.

## Current Status

| Phase | State |
|---|---|
| 0. Bootstrap | done: toolchain pinned in the repo, harness carried over from Stunt Race FX and remapped for the LoROM board |
| 1. Raw byte-exact skeleton | done: the intake rebuilds the ROM with 0 differing bytes |
| 2. First proofs and coverage | A5 and A7 proven on SoundCommandSend; every level reached by password; task design written |
| 3. Routines bottom up | waits on the task design decision |

Coverage comes from 24 input scripts: attract mode, the options and password
screens, a level 1 walk, and seeded random play through the menus and from
the start of every level reached by password. Bosses, deaths, level ends and
the ending are not yet reached.

## Aladdin decompilation

Query the tree. Do not trust a number in this file.

```bash
python3 tools/aladdin/build.py              # trace, regenerate the raw intake, assemble
python3 tools/aladdin/image.py              # M2 gate: built ROM vs original, 0 differing bytes
python3 tools/aladdin/catalog.py            # routines and bytes the scripts have reached
python3 tools/aladdin/census.py             # every routine, traced or found statically
python3 tools/aladdin/codemap.py            # executed code bytes per processor
python3 tools/aladdin/hybrid.py             # build the C as the library the oracles load
python3 tools/aladdin/asmcontrol.py         # the assembler encodes every instruction form
```

A5 and A7 run in the headless runner on the hybrid library:

```bash
build/aladdin/box/snesbox --rom build/aladdin/baserom.sfc --script SCRIPT --out DIR \
  --check build/aladdin/hybrid/libaladdin.dylib      # A5: every call vs the original
build/aladdin/box/snesbox --rom build/aladdin/baserom.sfc --script SCRIPT --out DIR \
  --lockstep build/aladdin/hybrid/libaladdin.dylib   # A7: all C in place, lockstep
```

T1 = traced, raw intake only; T2 = C or assembly in the tree, not certified;
T3 = the C is certified behaviour exact; T4 = T3, and the routine's assembly is
symbolic, documented and byte-identical in place.

### How T3 is verified

"Behaviour exact" means same input, same output, proven against the running
original. Both oracles run the **original game** headless on a pinned,
accuracy-first emulator core (bsnes, with its speed shortcuts off), driven by
input scripts in `tools/aladdin/scripts/`.

- **A5: per call** (`snesbox --check`): at every real call of a routine in
  the running game, its C runs on the same inputs (memory as it was at entry,
  the same registers, the original's hardware reads replayed) and must leave
  the same registers, memory and hardware writes.
- **A7: whole image** (`snesbox --lockstep`): the game runs with the C in
  place, in lockstep with the untouched original, and work RAM, VRAM, OAM,
  CGRAM and sound RAM must match after every frame.

A7 exists because a per-call check cannot see timing or ordering effects.
Both gates are proven to have teeth on this game: a deliberately broken copy
of a certified routine fails them.

### Keeping it that way

A routine is not done until it says what it does and how, and lives in the
folder for what it is for. The pre-commit hook refuses a new `@routine`
without `WHAT IT DOES:` and `HOW:`, and refuses a file named for an address
range anywhere outside the raw intake. Every commit and commit message also
passes the provenance check.

### Architecture

    src/aladdin/                 the C decomp, by area; include/ holds the machine
                                 model (machine.h) and the RAM map (wram.h)
    asm/aladdin/                 the companion assembly, same areas as the C
    config/aladdin/              data the tools read (the SPC700 upload lists)
    tools/aladdin/               tracer, disassembler, build, gates, catalog, census
    tools/aladdin/snesbox/       the headless runner and both oracles
    tools/common/                git hooks, provenance check
    tools/toolchains/            pinned asar and bsnes (staged by setup.sh), local patches
    docs/aladdin/                plan, task design, handbook, dated notes
    reference/                   ROMs (not tracked)
    build/                       everything generated

### Building & Setup

- **`./setup.sh`**: stages the toolchain inside the repo (nothing installed on
  the host): asar 1.91 built from a pinned commit, and the bsnes core at a
  pinned commit with this project's patches (trace, oracle and lockstep hooks,
  and tracing of the cartridge's own ROM) applied, plus the headless runner.
- **`python3 tools/aladdin/build.py`**: runs the scripts with tracing (in
  parallel), regenerates the raw intake for every byte filed source does not
  own, and assembles the ROM. Data is included from your ROM at build time, so
  the tree carries none of it.

### Reference ROM (you supply; not tracked in git)

```
reference/aladdin/Aladdin (USA).sfc                          the target
    SHA-1  bd5fea18ce7456cc0231a39473927d8ea9c6589b     1,310,720 bytes
    MD5    8e00910312b30663768d907114d301c2
```

`build.py` checks the target's SHA-1 and refuses any other image.

## More

- [docs/aladdin/PLAN.md](docs/aladdin/PLAN.md): the plan, the standard, and what differs from Stunt Race FX.
- [docs/aladdin/TASKS.md](docs/aladdin/TASKS.md): the task scheduler and the design for decision.
- [docs/aladdin/HANDBOOK.md](docs/aladdin/HANDBOOK.md): what the work has learned, one rule per finding.

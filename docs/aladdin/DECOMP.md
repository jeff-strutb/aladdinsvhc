# Aladdin decomp: standard, tools, layout

## Standard

| | |
|---|---|
| T1 | traced; in the raw intake (address labels); reassembles exact |
| T2 | C written and filed, assembly named and filed; not yet certified |
| T3 | the C is certified behaviour exact: `t3.py --qualify` passes gates 0, C, A5, A7 |
| T4 | T3, and the routine's assembly is filed with the same header, symbolic, and byte-identical in place in the whole-image gate |
| EXCLUDED | beside the tiers: code the retail game provably never runs, proof in the routine header |
| M1 | every game routine T3 |
| M2 | every game routine T4, and the whole ROM rebuilt byte-exact |

A routine is done when it is T4: its C and its assembly land together.
Progress is code bytes by tier, from `tools/aladdin/progress.py`; query the
tree for counts, never a number written in a file.

Symbolic means no hexadecimal literal outside an immediate: an address is a
name (`asm/aladdin/include/*.inc`), a place inside a table or record is the
name plus a decimal index.

## Every routine

The header comment above each routine, the same in the C and the assembly:

```
@routine <bank>:<addr> <Name>
WHAT IT DOES: its purpose, in game terms
HOW: how it does it
Entry: M, X, DB, D, inputs.  Exit: outputs, flags, what it clobbers.
```

The pre-commit hook refuses a new `@routine` without `WHAT IT DOES:` and
`HOW:`, and refuses address-named files outside the raw intake. Routines are
filed by responsibility, never by address range, in the same areas in `src/`
and `asm/`.

Routines that wait for their task's next turn are C coroutines
([TASKS.md](TASKS.md)); so is actor code, which yields by BRK
([ACTORS.md](ACTORS.md)).

## Filing fast

The C starts as a draft: `cdraft.py ADDR...` writes each routine as its
executed instructions, one `ops65.h` step each at the width it ran with,
calls as calls to the C called, waits and yields as the coroutine calls.
Drafts are behaviour exact before anyone names anything (on 2026-11-01
the drafts of 184 certified routines, in place of their hand C, were
IDENTICAL on A5 and A7 on every script). What it cannot say exactly
carries a TODO, which gate 0 refuses: untraced code, code that takes
from its caller's frame or sets S (the task primitives), an instruction
that ran at both widths with different lengths.

1. `worklist.py` (or `blockers.py`) for what is ready, bottom up.
2. `cdraft.py` and `asmdraft.py` for a whole subsystem at once.
3. Name: addresses the headers know are named already; add the rest to
   the area's header. Write each header (WHAT IT DOES, HOW, Entry, Exit),
   the same in the C and the assembly. Restructure the C only where it
   reads better and the gates still pass.
4. Build, image gate, gate A5 and A7 once for the batch, land once.

## Layout

| | |
|---|---|
| `src/aladdin/<area>/` | the C decomp |
| `src/aladdin/include/` | C headers: machine model, RAM maps |
| `asm/aladdin/<area>/` | the companion assembly, same areas as the C, placed with `%at(addr)` |
| `asm/aladdin/include/` | hardware registers, RAM maps, ROM names, macros |
| `config/aladdin/` | SPC700 upload lists, verdict ledgers (`t3_live.csv`, `whole_image.csv`) |
| `tools/aladdin/` | build, gates, runner (`snesbox/`), tracer, disassembler, census |
| `tools/aladdin/scripts/`, `states/` | trace and gate scripts; state recipes |
| `tools/common/` | hooks, provcheck |
| `tools/toolchains/` | pinned asar and bsnes core (`setup.sh`), local patches |
| `docs/aladdin/` | this file, the plan, the handbook, dated notes in `decomp/` |
| `reference/aladdin/` | ROMs (not tracked) |
| `build/aladdin/` | everything generated |

## Tools

| | |
|---|---|
| `setup.sh` | stage asar and the patched bsnes core, build snesbox, install the hooks |
| `snesbox/` | the original ROM run headless; `--trace`, `--check` (A5), `--lockstep` (A7), unit calls |
| `build.py` | trace (in parallel), regenerate the raw intake for every byte no filed source claims, assemble |
| `image.py` | Gate I: 0 differing bytes or exit 1 |
| `asmcontrol.py`, `spc700.py --check` | assembler controls: every 65816 and SPC700 form round-trips |
| `catalog.py`, `spccat.py` | routine catalogs from the traces (task entries and resume points included) |
| `census.py` | code no script runs, found statically (task-call arguments kept as data) |
| `hybrid.py` | the C as the library the harness loads |
| `t3.py` | `--live` (A5), `--image` (A7), `--qualify ADDR` (every gate, the tier) |
| `gate.py` | A5 and A7 together from HEAD, the image gate and the progress refresh |
| `progress.py` | the README's bars and the progress map, by code bytes |
| `worklist.py`, `widths.py` | routines ready to work (callees done); entry widths |
| `asmdraft.py`, `asmfile.py` | a starting draft for filing a routine's assembly; drafts are intake, read and named by hand |
| `cdraft.py` | a routine's C drafted from its traced assembly, instruction by instruction (`ops65.h`): behaviour exact as drafted; `--proof DIR` re-drafts every routine with C, for the gates to judge |

```bash
python3 tools/aladdin/build.py
python3 tools/aladdin/image.py
python3 tools/aladdin/t3.py --qualify 80:B463
python3 tools/aladdin/gate.py
```

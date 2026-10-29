# Aladdin decomp: plan

Target: `Aladdin (USA).sfc`, 1,310,720 B, SHA-1
`bd5fea18ce7456cc0231a39473927d8ea9c6589b`. Header: LoROM FastROM (map
$30), cart type $00 (ROM only: no coprocessor, no cartridge RAM), 1.25 MB.
Reset $00:8000 (`SEI; CLC; XCE; JML $80:8007`): everything after runs in the
FastROM banks $80 up. NMI and IRQ vectors are in ROM. Hand-written assembly:
no C-compiler stack frames anywhere.

The method is the SNES decompilation playbook (nativeshvc,
docs/snes-decomp-learnings.md), carried over from the Stunt Race FX decomp
with its Super FX lane removed.

## 1. Standard

| | |
|---|---|
| ASM | symbolic source that rebuilds the ROM byte for byte (image gate) |
| C | behaviour exact: each routine's inputs to outputs exactly the original's (A5 + A7) |
| Enhanced | later, beside the C, never in place of it |

Cycle exact is not the standard (playbook section 15): the C carries no
instruction timing.

| Tier | Meaning |
|---|---|
| T1 | traced; in the raw intake with address labels; reassembles exact |
| T2 | C written and filed, assembly named and filed; not yet certified |
| T3 | C certified behaviour exact: gates 0, C, A5, A7 pass |
| T4 | T3, and its assembly is symbolic and byte identical in place in the whole image |

M1 = every routine T3. M2 = every routine T4 and the ROM rebuilt byte exact.
Progress is measured in code bytes, with code no script runs (the census)
in the denominator.

## 2. What is different from Stunt Race FX

| | |
|---|---|
| Processors | 65816 and SPC700 only; no Super FX lane |
| Mapping | LOROM board, 1.25 MB: banks past $A7 read the 256 KB tail's mirrors (the core's Bus::mirror, reproduced in the checker and codemap) |
| Run banks | FastROM mirror $80-$A7; intake addresses are written there |
| Tasks | a cooperative task system: each task has its own stack and direct page and yields mid-routine (section 4) |
| Inline arguments | the task-create call takes 5 bytes after its JSL and steps its return address over them |
| Sound | the driver and two sample sets, then one block per song to APU $0D20, uploaded by index from the table at $85:8000 (config/aladdin/spclists.csv) |

## 3. Status

Phase 0 (bootstrap) and Phase 1 (raw byte-exact skeleton) are in place:

- Toolchain pinned in the repo: asar 1.91 (65816 control: 268 forms; SPC700
  control: 256 opcodes; both round-trip with 0 differing bytes, and the
  65816 control fails on a planted operand size), bsnes at the Stunt Race FX
  pin with its 13 harness patches plus 0014 (trace the cartridge's own ROM).
- snesbox (the headless runner) with trace, unit calls, the A5 checker and
  the A7 lockstep, carried over; the checker's memory map rewritten for the
  LOROM board. A5 and A7 are not yet proven on this game (Phase 2).
- 10 trace scripts at first (attract, options, password, level 1, seeded
  random menus and play): 26,683 S-CPU code bytes and 2,257 SPC700 code
  bytes traced; no code runs from work RAM in these traces.
- Image gate: the intake rebuilds the ROM with 0 differing bytes, and fails
  on a planted byte.

## 4. Limits and open decisions (before Phase 3)

1. **Tasks.** The A5 and A7 oracles judge a routine from its call to its
   return on one stack. A task that waits (switches to the scheduler) and
   resumes frames later breaks that model. Design for decision:
   [TASKS.md](TASKS.md).
2. **Inline arguments** are handled by the trace-based intake (only traced
   bytes become instructions), but the census and every static walk must
   skip them and seed task entries from them.
3. **Coverage.** Scripts start every level by password but play each only
   at random; bosses, deaths, the ending and level ends need plays or unit
   calls.
4. **Unit-call stubs** sit at $1FE0 and $7F:FFE0 as in Stunt Race FX; not yet
   checked against this game's task stacks and RAM use.
5. **Code size** is unknown until the census; an outside static estimate is
   90-100 KB of S-CPU code against 26.7 KB traced so far.

## 5. Phase 2 (2026-10-28)

- A5 and A7 proven on SoundCommandSend ($80:B463): 1,139 of 1,139 calls
  identical; lockstep 1,216 frames compared, 0 differing. Three planted
  bugs (wrong port byte, flag left set, wrong queue byte) each fail both.
- Catalog from traces: 573 routines.
- Every level reached by password (the table at $80:FDD5) and played at
  random: 37,607 S-CPU code bytes traced.
- Task design written for decision: [TASKS.md](TASKS.md).

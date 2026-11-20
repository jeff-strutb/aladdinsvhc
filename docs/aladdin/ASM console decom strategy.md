# ASM console decomp strategy

What worked and what cost days on the Aladdin (SNES) decomp, written for the
master session of another hand-written-assembly console game. Aladdin ended at
M1 100% and M2 100% (90,269 / 90,269 B, 378 scripts identical, commit
`cffb00c`). Most of the lost time came from a short list of mistakes; they are
in section 9. Read that first if you read nothing else.

Aladdin's tools are named here as examples (`tools/aladdin/t3.py`,
`runner.py`, `progress.py`, `snesbox/check.c`, `docs/aladdin/PLAN.md`). The
method does not depend on the SNES.

## 1. The standard, fixed before any work

| | |
|---|---|
| ASM | symbolic source that rebuilds the ROM byte for byte |
| C | behaviour exact: each routine's inputs to outputs are the original's |
| Not the standard | cycle timing |

Tiers per routine: T1 traced, T2 C filed, T3 C certified, T4 T3 plus symbolic
byte-identical assembly. Two bars: M1 = bytes in T3+T4 routines, M2 = bytes in
T4 routines.

- **Measure bytes, not routines.** The routine count grows as entries are
  found; the byte total barely moves.
- **The denominator is every code byte**, run by a script or found statically
  (a census following calls, jumps, tables and handler pointers).
- **One question decides everything:** does it meet the standard? If yes, do
  it without asking the owner. Ask only for a new rule or a change of design.

## 2. The four gates

| Gate | What it proves | Aladdin's check |
|---|---|---|
| 0 | the routine's header is complete | header fields present |
| C | every byte of the routine's span was run by a script, or is exempt by proof | coverage from traces |
| A5 | each call of the C, alone, gives the original's outputs | per-call compare in the emulator |
| A7 | whole scripts run in lockstep, C against original, identical | every script that reaches the routine |

- A routine passes A7 only when **every** script reaching it is identical and
  measured. One red script blocks only the routines it reaches.
- **A5 passing is not enough.** A5 judges a call in isolation. Lockstep finds
  what A5 cannot: an interrupt landing in the middle of a C call and seeing
  state the original would not have had yet.

## 3. Access order: the largest single cause of late reds

The C must make every read, write, push and pull **where the assembly makes
it**, not merely end with the same values.

- A call that can reach a wait or be interrupted must push the original's
  return address at the original's place (Aladdin: `call_frame` for JSR,
  `jsl_frame` for JSL).
- Write this rule into the writers' procedure on day one. On Aladdin it was
  found late: 13 red scripts traced back to 5 causes, most of them this one,
  and 713 return frames had to be added by tool afterwards.
- A bulk fix by tool needs its own static audit. Lockstep stays identical when
  a frame names the wrong call site (the bytes sit below the stack pointer and
  are overwritten). Check each added frame's ROM byte is the call opcode to
  the routine called next, and scan for the same address framed twice.

## 4. The catalog can turn a passing script red

The routine catalog is rebuilt from all traces. A new script's trace can add
an entry **inside code that already has C**. Lockstep treats an entry with no
C of its own as original code, so scripts go red with no C change at all.

- Rule: when a script is added, rebuild the catalog with its trace and give
  every new entry its own routine tag and C before landing.
- Before blaming a commit for a red, build with the suspect catalog entries
  filtered out. I reverted a writer's correct commit over this and had to
  restore it.

## 5. Coverage for code nothing reaches

In order of preference:

1. **A script with real input.**
2. **Pokes** of work RAM for a guarded branch.
3. **A unit call at the routine's own entry** for a dead routine. Judge
   unit-call scripts piece by piece, not as one whole lockstep run (a false
   red appears a few frames after a state load otherwise).
4. **A resume-address poke** for code inside a waiting actor or task.
5. **Exempt by proof**, only for bytes that no input, no poke and no unit call
   at the routine's own entry can run sanely (a path that would run data, a
   branch needing ROM bytes that are not there). The proof goes in the
   routine's header with the exact byte range. Aladdin has 131 such bytes in
   twenty ranges. This is the owner's decision: ask once, then apply it.

Never force coverage with a call into the middle of a routine or a state that
crashes the original.

## 6. The gate runner

One process owns certification. Writers do not run gates.

- It takes each new commit, re-tests **only the routines whose compiled C
  changed** and only the scripts that reach them, and commits the ledgers and
  the progress report itself.
- **It reports failures and holds nothing.** Automatic holding of failing C
  was tried: C reaches waits and tasks by runtime jumps that a symbol closure
  misses, so holding one routine sent about 15 scripts divergent.
- **Harness changes do not make stored results stale**, or every checker fix
  re-runs the world.
- The whole-game run is off the critical path: only when asked.
- Known flakes and traps, each of which printed a wrong headline number:
  - A script can stall under a loaded batch and pass alone. Rerun the one
    script alone before believing a red. One such row read 71.2% where 88.5%
    was true.
  - "Stopped, no report" was a real divergence, not a crash: read the lockstep
    report's first DIFF line.
  - A first difference on a scheduler counter only means the clocks drifted:
    look for the earlier cause.
  - With new scripts on main and no result for them yet, the report printed
    0.0%. Never publish a report taken in that state.
  - Never run a forced gate job while the runner is up: they share a lock and
    every batch fails.
- **Build a private copy of the checker** for any run longer than a few
  minutes. The shared one was rebuilt mid-run once and the run mixed two
  checkers.

## 7. Running the sessions

- **The master thinks, briefs and certifies. It writes no game C.** Writers
  decompile and write coverage scripts.
- **Split by address range or by file** so two writers never touch the same
  files. Give each brief: the files, the rule documents, what done means, and
  "do not stop between items".
- **One rules file all writers read** (Aladdin: `build/aladdin/writers/TODAY.txt`
  and `PROCEDURE.txt`), amended with a dated section each time a new cause is
  found. Telling sessions one at a time does not hold.
- **A generated worklist**, not a remembered one: a script that prints every
  routine short of the bar, with the gate that fails and why (Aladdin:
  `worklist-remaining.py`, `worklist-m2.py`). Regenerate after every landing.
  The last 18 routines of M2 took two sessions one pass because the list named
  the exact failing lines.
- **Landing goes through one script under one lock**, with the pre-commit
  checks in it: every called C routine is defined in non-held C (a missing one
  broke the link for every gate and voided a 20-minute run), and the filed
  assembly assembles to the ROM's bytes, intake first.
- **Concurrent checks never share an output folder.** One helper overwrote a
  library mid-test and a red passed as identical.
- **Idle writers get the next item at once.** Check on every note.
- **A writer's question for the owner is asked by that writer**, as a prompt
  in its own session. Do not relay it.
- **A session's report is not proof.** Read the diff or run the gate before
  repeating its claim.
- Fresh sessions on a short written brief did better than long-lived ones
  carrying a day of context.

## 8. Predict before you measure

Trial and error is banned on this owner's projects, and it is also slower.

- Before a fix, state the expected result in numbers. Aladdin's what-if tool
  copies the script ledger, flips the scripts the fix should turn identical,
  and computes the bars. If the measured number misses the prediction, explain
  the miss from evidence before doing anything else.
- This also ranks the work. I told the owner one group of seven scripts was
  the biggest lever; the what-if showed three other scripts were worth 38
  points.
- For a red: get the report's first difference, rerun that one script alone
  with the checker's trace switches, find the exact access that differs, then
  change the C once.

## 9. What cost the most time

| Mistake | Cost | Rule that prevents it |
|---|---|---|
| Certifying routines known to be red | most of a day of void gate runs | never start a gate that cannot pass |
| Automatic holding of failing C | about 15 scripts divergent | the runner reports, it holds nothing |
| Access order not in the writers' rules from the start | 13 red scripts, 713 frames added later | section 3 on day one |
| Catalog entries inside code with C | reds with no C change, a wrong revert | section 4 |
| Believing a batch red or a 0.0% report | wrong numbers given to the owner | rerun alone; never publish an unmeasured state |
| Long tasks started without a prediction | an hour, then a failure | section 8; nothing long without a stated expected result |
| Shared checker or output folder between runs | a red passed as identical, runs redone | private builds, private folders |
| Guessing a tool's flags | one tool has no help and rewrites the report on any run | read the tool's source first |

## 10. Reporting to the owner

- Lead with the measured number and the commit it was measured at.
- Say what was not run. Never repeat a session's claim as a result.
- Short plain tables. No plan that contains "maybe".
- Publish the report and push only at a ledger commit the runner wrote.

## 11. Order of work that I would use again

1. Fix the standard, tiers and bars. Build the byte-for-byte image gate.
2. Build the per-call gate, the lockstep gate and the runner before mass
   decompiling, with the staleness rules of section 6.
3. Write the writers' procedure with access order and the catalog rule in it.
4. File everything with assembly and C by address range, many writers.
5. Generate the worklist; clear reds by cause, largest predicted gain first.
6. Coverage for the unreached, by section 5's order.
7. Symbolic assembly last: it changes no bytes and no behaviour, so it is
   quick once a list names the offending lines.

## 12. Design the decomp's C for the native target from the first day

Added after starting Aladdin's native conversion. This is the lesson that would
have saved the most work, and it applies before any C is written.

**What happened.** The decomp's C was shaped only by what the proof needed:
a CPU register file, memory read and written by address, return addresses
pushed where the assembly pushes them, and tasks and actors that pause in the
middle of a routine with their stack in work RAM. That C is certified, but it
is not native-shaped. Moving it onto a native engine then meant rewriting
every routine a second time. On Aladdin that was 86,000 lines, of which only
about 630 touch hardware; the rest is the rewrite.

**The record is still worth having.** Do not skip it and convert from the
assembly:
- It is the only proof of what each routine does. Native code has a different
  memory layout and cannot be run in step with the original, so each native
  routine is written from a certified description.
- It supplies the names (routines, addresses, tables) and the dead-code proofs.
- Its traces, recordings and scripts do real work in the conversion: they
  settle which bank a table is in, they are what the native actor checker
  compares against, and they are the replay inputs.

**What to do differently.** Decide the two steps together.
- Write the C against named, typed fields laid over the original memory
  layout (a struct whose members sit at the original addresses), not raw
  addresses, wherever the proof allows. It is still certifiable, since the
  accesses land on the same bytes in the same order.
- Give records their own types from the start: an actor record, a task
  record, the upload queue.
- Keep the register file and the pushed frames only where the proof needs
  them: at a routine's exits, at its pauses, and at calls that can be
  interrupted. Everything between is ordinary C.
- Read ROM tables through one data layer with typed readers, not by bank and
  address in the routine. The bank a table is read in then gets settled once,
  from traces. On Aladdin the comments naming a table's bank were wrong more
  than a dozen times, always found the same way: a routine running with the
  data bank at $80 commented as $81 or $83.
- Record, per routine, whether it can pause and how deep. That list is what
  the native step needs to turn pauses into explicit states.

**What still has to be rewritten whatever you do:** the hardware touches,
the pauses, and sound. Those are small next to the rest.

**If the second step is already under way,** what made it workable:
- One typed state file every area builds on, landed before anything else.
- One owner per unit of behaviour, not per source file. A kind of actor
  whose code is spread over four files is one piece of work. Splitting by
  file caused a day of ownership questions.
- A landing script that builds the game and runs its boot under the lock.
  Two landings that each built cleanly on their own tip broke main twice
  before this was added.
- Temporary stand-ins that print their name once, each in its owner's own
  file, so sessions can land in parallel and nothing is silent.
- Get one end-to-end path running early (boot, then one level) before
  converting widely. Code that compiles but is never reached is not tested,
  and a session working past that point is spending without proof.
- A message to a running session waits for its turn to end. A stop
  instruction that matters has to interrupt the turn.

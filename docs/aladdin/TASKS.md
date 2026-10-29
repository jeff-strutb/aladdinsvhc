# Tasks: design for the C and the oracles

For decision before Phase 3. Everything under "What the game does" is read
from the code and measured on the 24 trace scripts (2026-10-28).

## What the game does

Aladdin runs its screens and game modes as cooperative tasks.

| | |
|---|---|
| Slots | 8, of $18 bytes from $006F: state ($6F), wait count ($70), saved S ($71), stack top ($73), a parameter ($75); the rest is the task's own data |
| States | 0 free, 1 waiting, 4 ready, 8 running, $0C new (not yet started) |
| Scheduler $81:BC6D | runs on its own stack ($0326). Scans the slots in order for the first ready or new one; before slot 0 it reads the joypads. Switches to the task's stack, D = 0; a new task starts at its entry (RTL), a waiting one pulls P, Y, X, D, B and returns (RTL) after its wait call. Nothing ready: scans again (the NMI makes one ready) |
| Wait $81:BD31 (A = frames) | pushes B, D, X, Y, P on the task's stack, saves S, marks the slot waiting, back to the scheduler |
| NMI ($80:830B) | counts every waiting slot down; at 0 it is ready |
| Create $81:BCB8 | 5 bytes after the JSL: slot, parameter, entry (word, minus 1 on the stack) and bank; steps its return address over them |
| Exec $81:BCF6 | 3 bytes after the JSL: the current task starts again at that entry, its stack emptied |
| Exit $81:BD20 | frees the current slot |

Measured: 82 task entries started by the scheduler; 59 wait sites, 10 exec
sites and 32 create sites. A wait is not always in the task's body: 43 of
70 wait/exec/exit sites are; 27 sit inside called routines, one to three
calls deep, and they are the busiest. $80:AE9A, the gameplay loop's frame
end (commit the frame, wait one frame, return), alone ran 14,718 waits at
depth 7 (two JSRs and a JSL below the body).

## The problem

1. **The oracles judge a call from entry to return on one stack.** A
   routine that waits leaves for the scheduler, other tasks run (their
   routines are calls of their own), and it comes back frames later on its
   own stack. Today that is a call that never returns, then code entered by
   an RTL nobody recorded.
2. **C has no way to stop mid-function and continue later**, and the stop
   can be several calls deep, so every routine on the chain is suspended.
3. **Task stacks are memory the oracles compare.** A suspended task's live
   frames (return addresses, the pushed registers) are work RAM; C that
   calls C pushes nothing.
4. **Static tools** (catalog, census) must not read the inline bytes as
   code, must seed the entries from them, and must not take a resume point
   as a routine start.

## Option A: tasks as coroutines (recommended)

Each task's C runs on a native stack of its own. A wait in C suspends the
whole chain and the scheduler resumes it, so the C reads as the game was
written:

```c
void FrameEnd(void)          /* $80:AE9A */
{
    FrameCommit();
    task_wait(1);
}
```

- **Primitives** (machine.h, like call_routine): `task_wait(frames)`,
  `task_exec(entry)`, `task_exit()`, `task_create(slot, param, entry)`.
  Each makes exactly the original's memory effects (the pushes, the slot's
  state, wait count and saved S) and then switches coroutines.
- **Calls that can wait push their frames.** A C call of a routine that can
  reach a wait (known statically: the callers of the 70 sites) writes the
  return address the original's JSR or JSL writes, through one helper, so
  the task stack in work RAM is the original's byte for byte. Calls that
  cannot wait stay plain C calls, as now.
- **The scheduler in C** resumes a task's coroutine, or starts its entry.
- **Native (nativeshvc)**: the same coroutines; the NMI's countdown is
  unchanged game code. No console change is needed beyond a stack switch,
  which can be the game's own code.

Oracle work:

- **Resume entries.** The catalog records each wait's resume address (the
  byte after its JSL) and the slot. When a task is resumed there and its
  wait was made by C, the harness switches back into that coroutine.
  Mixed states are inevitable during Phase 3: C waiting and asm resuming
  it, asm waiting and C resuming it. Both directions are handled.
- **A5 judges segments.** A call that waits is judged in pieces: entry to
  wait, resume to the next wait, last resume to return. Each piece is
  checked against the original with its replayed reads; the checker keeps
  each task's replay coroutine across pieces.
- **A7** keeps comparing every frame; calls already may span frames. The
  "dead stack" rule widens from the running stack to every slot's stack
  below its saved S.

## Option B: split task code at its waits

Each resume address becomes a routine of its own, and C ends where the
original waits. Nothing new in the harness beyond resume entries. Rejected
as the main design: because waits sit up to three calls deep, a resume
continues through the tails of every routine on the chain. Each tail then
becomes an inner entry, so the C is a state machine, not the game's
structure. It is also the opposite of the plain C (playbook section 18)
the decomp ends in.

## Limits (stated before building)

- **The coroutine harness is new and the largest risk** of the decomp so
  far: resume detection, the per-slot A5 replay, and mixed C/asm
  transitions. It needs the same proofs as day one: planted bugs that turn
  A5 and A7 red, and "frames compared > 0" on every new piece.
- **Interrupts while a coroutine is suspended** are ordinary game code (the
  NMI), but the oracles' interrupt handling has only seen one stack. Expect
  traps there.
- **Until the harness exists, only routines that cannot wait can reach T3.**
  The rest are judged as asm.
- **Exec and exit throw a chain away**: the coroutine is abandoned, as
  `cpu_restart` abandons C frames today.

## Plan if A is approved

1. Catalog and census: inline bytes as data, task entries seeded from
   create and exec arguments, resume addresses recorded.
2. Primitives and the harness switch; first proof on $80:AE9A (FrameEnd,
   the busiest wait) with planted bugs: a wrong wait count, a dropped push,
   a wrong resume.
3. The scheduler, create, wait, exec and exit in C, proven the same way.
4. Meanwhile, Phase 3 goes on bottom up through routines that cannot wait
   (most of the code: objects, drawing, math, DMA).

# Returns past inline arguments in the oracles (2026-11-01)

TaskCreate ($81:BCB8) and TaskExec ($81:BCF6) read arguments from the bytes
after their JSL. TaskCreate then adds 5 to the return address on the stack
and returns after them. TaskExec never returns: it jumps to the scheduler.

## What the oracles did

- A5 ended a watch only at its return address, or at an RTS/RTL that took S
  above the exit level. TaskCreate's RTL lands at the exit level 5 bytes
  later, so the original side never saw the call end.
- A5 failed any C that set `cpu.unwound` while S was back at the exit
  level, so no C could say it had landed somewhere else at that level.
- The A7 hybrid returned a swapped C to the return address computed at
  entry, and set the RTL/RTS opcode only for C that had not unwound.

## The change (tools/aladdin/snesbox/check.c)

- A5 and lockstep: an RTS or RTL that brings S back to the exit level ends
  the call wherever it lands.
- A5: when the original's call ended away from its return address or exit
  level, the C must have unwound to the same S and PC; when it did not, the
  C must not have.
- Hybrid: a C that unwound to its exit level is taken as returned (RTL/RTS
  opcode), so where it lands is not taken as entered by the caller's JSL.

The C of TaskCreate does its own RTL: it pulls the stepped address, sets
`cpu.s` and `cpu.pc` and sets `cpu.unwound`.

## Proof (rand_level1_1)

| C | A5 | A7 lockstep |
|---|---|---|
| as filed | TaskCreate 8/8 and TaskExec 2/2 IDENTICAL | IDENTICAL, 4,019 frames, 0 differing |
| steps the return by 4 | DIVERGENT 8/8 (pc, stack byte) | DIVERGENT, 1,657 frames differ |
| does not unwind | DIVERGENT 8/8 (unwound=0/1) | DIVERGENT, 1,657 frames differ |
| TaskExec's entry not less one | wrong byte found, but rated inconclusive (an interrupt overlapped the call), UNVERIFIED | DIVERGENT, 1,511 frames differ |

TaskCreate is also IDENTICAL in attract (26 calls), rand_menus1 (16) and
rand_pw5_1 (5); TaskExec in rand_menus1 (2) and rand_pw5_1 (1).

## A task's stack reaches three bytes above its top

Found on the same day, when TileBehaviourLoad ($81:BEC5, called straight
from a task's own level) had its C running as a frame ended. A task's code
runs with S up to its stack top + 3: the scheduler's RTL has pulled the
three entry bytes above the top. The frame compare gave each task the
stack (top - $38, top], so a call whose exit S was $0163 (slot 0's top
$0162) matched no stack in use, and slot 0's dead bytes under S were judged
against its stale saved S: three bytes at $015B-$015D differed in every
script (options frame 1036, rand_level1_1 frames 2470 and 3671).

`stack_dead` now gives a task (top - $35, top + 3], still $38 bytes, and
the scheduler's stack starts above the last task's top + 3.

| C, rand_level1_1 unless named | A7 lockstep |
|---|---|
| as filed, options | IDENTICAL, 1,050 frames |
| as filed | IDENTICAL, 4,019 frames |
| TaskWait without its PHB push (the handbook's planted bug) | DIVERGENT from frame 0, every frame |

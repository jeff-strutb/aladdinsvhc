/*
 * actor_loop.c: the actor loop (docs/aladdin/ACTORS.md). Each frame the
 * player's record and then every other record is looked at: a free one is
 * skipped; a waiting one counts down, and goes back into its kind's code
 * when its wait runs out; one to start goes to its kind's start entry. The
 * kind's code returns its next wait (the original's BRK or COP) or that it
 * ended. A record whose flags have bit 7 set runs its kind's wait routine
 * each frame it waits, and after each turn of its code.
 */
#include <stdio.h>

#include "actor/actor_own.h"
#include "system/task.h"

#define ACTOR_EXEC (-100)       /* run_record: the actor's code made a TaskExec; the walk stops */

/* A record's turn of its code: the start entry its state picks ($0C-$7F,
   $80-$BF, $C0 up), or where it stopped. Returns ACTOR_ENDED, or the wait
   it set. */
static int run_record(Actor *a)
{
    uint8_t was = a->state;
    ActorCode code;
    int r;

    a->state = ACTOR_STATE_RUNNING;
    if (was >= ACTOR_STATE_NEW) {
        uint8_t start = !(was & 0x80) ? ACTOR_START : !(was & 0x40) ? ACTOR_START_SECOND : ACTOR_START_THIRD;

        a->resume = start;
        a->call_at = 0;                 /* no shared routine's place carried from the record's last actor */
        code = actor_kind_code(a->kind, start);
        a->code = code;                 /* it goes on in this code until it is handed to another */
    } else {
        code = a->code;                 /* where it stopped, in the code that owns its resume point */
        if (!code) {
            static uint8_t told[256];

            /* waiting, but never started by the loop (its state set by other code) */
            if (!told[a->kind]++)
                fprintf(stderr, "actor kind $%02X: waiting with no code to go on in\n", a->kind);
        }
    }
    if (!code) {
        /* no code for it yet (reported once): it stays, waiting a frame */
        a->state = ACTOR_STATE_WAITING;
        a->wait = 1;
        return 1;
    }
    {
        uint16_t resume_was = a->resume;
        ActorCode code_was = a->code;

        r = code(a);
        if (task_exec_pending()) {
            /* 81:BCF6 TaskExec from its code: the original never comes back
               (it empties the task's stack), so no yield saved a new place:
               the record keeps state 8 and goes on next time where its last
               yield left it. (A record that does it on its first turn has no
               yield behind it; the original resumes at whatever its +$02-$05
               held. Here it starts its entry again.) */
            a->resume = resume_was;
            a->code = code_was;
            return ACTOR_EXEC;
        }
    }
    if (r == ACTOR_ENDED)
        return ACTOR_ENDED;
    a->state = ACTOR_STATE_WAITING;
    a->wait = (uint8_t)r;               /* 256 frames: 0, which the countdown takes round */
    return r;
}

/* After a turn, or a frame of waiting: the wait routine, when bit 7 of its
   flags asks for it. */
static void wait_work(Actor *a)
{
    ActorWaitCode w;

    if (!(a->flags & 0x80))
        return;
    w = actor_kind_wait(a->kind);
    if (w)
        (void)w(a);                     /* a wait routine that ends its actor leaves it free: nothing follows either way */
}

/* One record's turn, as both loops take it. Returns 0 when a TaskExec from
   the actor's code or its wait routine stops the walk (the later records do
   not run: the original left the loop for the task scheduler). */
static int record_turn(Actor *a)
{
    int r;

    if (a->state == ACTOR_STATE_FREE)
        return 1;
    if (a->state == ACTOR_STATE_WAITING && --a->wait != 0) {
        wait_work(a);
        return !task_exec_pending();
    }
    r = run_record(a);
    if (r == ACTOR_EXEC)
        return 0;
    if (r != ACTOR_ENDED)
        wait_work(a);
    return !task_exec_pending();
}

/*
 * @routine 83:9475 PlayerLoop
 * WHAT IT DOES: The player's turn of the actor loop: its record run (its
 * kind's code) when its wait runs out or it starts, else its wait routine.
 * Also 83:948B PlayerLoopBack, the turn after its code yields.
 */
void PlayerLoop(void)
{
    (void)record_turn(&g.actors[0]);    /* a TaskExec: FrameStep stops after it (task_exec_pending) */
}

/*
 * @routine 83:94A4 ActorLoop
 * WHAT IT DOES: The other actors' turns: each of the 35 records after the
 * player's looked at in turn and run when due. With g.pause_block set, the
 * loop stops before the pool's records (actors[8] on).
 * Also 83:94BF ActorLoopBack and 83:94D3 ActorLoopNext, the loop's re-entries
 * after an actor yields or ends and its step to the next record.
 */
void ActorLoop(void)
{
    int i;

    for (i = 1; i < ACTOR_RECORDS; i++) {
        if (!record_turn(&g.actors[i]))
            return;                     /* a TaskExec: the walk stops here, FrameStep after it */
        if (i + 1 >= ACTOR_RECORDS - ACTOR_POOL_SIZE && g.pause_block)
            return;
    }
}

/*
 * ActorRunAs: hands the actor's record to `code`, which owns its resume
 * point from now on (the original's jump into another routine with the
 * record, whose saved PC then lies in that routine): a->resume = `resume`,
 * and `code` runs now. Returns what `code` returns: its wait, or
 * ACTOR_ENDED. A kind's own code calls it as `return ActorRunAs(...)`.
 */
int ActorRunAs(Actor *a, ActorCode code, uint16_t resume)
{
    a->code = code;
    a->resume = resume;
    return code(a);
}

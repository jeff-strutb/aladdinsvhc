/*
 * iris_close_task.c: a task that closes the screen down to a circle round
 * a point, by the windows: started by actor kind $24 and by the scene at
 * $80:B04E.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "wram.h"

void abi_CircleWipeStep(void);
void abi_TaskCreate(void);
void abi_TaskExit(void);
void abi_TaskWait(void);

#define IRIS_WHICH 0x00BDu      /* which iris: an index into the tables below */
#define IRIS_RADIUS 0x00BEu     /* the circle's radius, one down a frame to 0 */
#define IRIS_X 0x00BFu          /* its centre on the screen (words) */
#define IRIS_Y 0x00C1u
#define IRIS_RADII 0xDB87u      /* by IRIS_WHICH: the starting radius (bytes) */
#define IRIS_CENTRES 0xDB89u    /* by IRIS_WHICH * 4: x, then y (words) */
#define WINDOW_03A6 0x03A6u     /* the windows' settings, $03A6-$03AE */

/*
 * @routine 80:B263 IrisCloseTask
 * WHAT IT DOES: Closes the screen down to nothing round a point: starts
 * the window effect, masks layers 1 and 2 by the windows, and draws a
 * circle round the point of the iris IRIS_WHICH names whose radius shrinks
 * by one each frame from its start until it is 0, then ends.
 * HOW: TaskCreate (slot 6, parameter 4, WindowEffectTask); TMW and TSW 3,
 * $03A6 0, $03A7 $FF, $03AA $33, $03AB and WOBJSEL 0. IRIS_RADIUS =
 * IRIS_RADII by IRIS_WHICH; IRIS_X and IRIS_Y from IRIS_CENTRES by
 * IRIS_WHICH * 4. Each frame: CircleWipeStep with A = IRIS_RADIUS, X =
 * IRIS_X, Y = IRIS_Y (16-bit); MVN of the $200-byte window table from
 * $7F:5000 to $7E:F500, DB kept; M and X 8-bit; TaskWait 1; IRIS_RADIUS
 * down by one; at 0, TaskExit.
 * Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM and
 * whose $DB87 holds the tables (bank $80); a task's entry (TaskCreate slot
 * 3 at $84:8025, and at $80:B04E). Exit: never returns (TaskExit).
 */
void abi_IrisCloseTask(void)
{
    uint8_t which, r;

    jsl_frame(0x80B263u); abi_TaskCreate(); cpu.unwound = 0;
    a8_load(0x03);
    wr8(dba(TMW_SHADOW), 0x03);
    wr8(dba(TSW_SHADOW), 0x03);
    wr8(dba(WINDOW_03A6), 0);
    a8_load(0xFF);
    wr8(dba(WINDOW_03A6 + 1), 0xFF);
    a8_load(0x33);
    wr8(dba(WINDOW_03A6 + 4), 0x33);
    wr8(dba(WINDOW_03A6 + 5), 0);
    wr8(dba(WOBJSEL_SHADOW), 0);

    /* the iris's start: its radius and centre */
    which = rd8(dba(IRIS_WHICH));
    x8_load(which);
    r = rd8(dba((uint16_t)(IRIS_RADII + which)));
    a8_load(r);
    wr8(dba(IRIS_RADIUS), r);
    op_rep(0x20);
    cpu.x = (uint8_t)(which << 2);      /* TXA, ASL, ASL, TAX with X 8-bit */
    flag_set(P_C, 0);                   /* the second ASL's: bit 14 of the index */
    a16_load(rd16(dba((uint16_t)(IRIS_CENTRES + cpu.x))));
    wr16(dba(IRIS_X), cpu.a);
    a16_load(rd16(dba((uint16_t)(IRIS_CENTRES + 2 + cpu.x))));
    wr16(dba(IRIS_Y), cpu.a);

    /* each frame a step smaller */
    do {
        op_rep(0x30);
        a16_load((uint16_t)(rd16(dba(IRIS_RADIUS)) & 0x00FF));
        cpu.x = rd16(dba(IRIS_X));
        cpu.y = rd16(dba(IRIS_Y));
        nz16_of(cpu.y);
        abi_CircleWipeStep();
        phb();
        cpu.x = 0x5000;
        cpu.y = 0xF500;
        a16_load(0x01FF);
        mvn(0x7E, 0x7F);                   /* to $7E:F500 from $7F:5000 */
        plb();
        op_sep(0x30);
        a8_load(0x01);
        jsl_frame(0x80B2C6u);
        abi_TaskWait();
    } while (dec8_at(dba(IRIS_RADIUS)));
    abi_TaskExit();
}

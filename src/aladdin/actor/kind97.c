/*
 * kind97.c: actor kind $97, a dropper kept in place: by its $07, it
 * waits for the point actors are kept in range of to come into a window,
 * then drops a child (kind $37, from above that point) and waits for it to
 * end, or takes a free record and sends a carrier pair, and waits for that.
 * No script meets any of it: written from its bytes.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "actor.h"

void abi_ActorCreate(void);
void abi_ActorFreeListTake(void);
void abi_ActorSpawnPlace(void);
void abi_CarrierPairSend(void);
void abi_Kind97DropChild(void);
void abi_Kind97Start(void);

#define K97_TIMER 0x40u         /* frames left to wait (the first wait $1E, else 1) */
#define K97_BUSY 0x41u          /* nonzero while its child is out */
#define K97_TAKEN 0x4Au         /* the free record ActorFreeListTake gave (Y) */
#define K97_COUNT 0x4Bu         /* at 4 or more it waits (counted by its pairs) */
#define K97_CHILD 0x4Eu         /* its child's record (word) */
#define K97_CREATED 0x0020u     /* the record ActorCreate made (word) */
#define K97_RANGE_X 0x1A44u     /* the x of the point actors are kept in range of (word) */
#define K97_RANGE_Y 0x1A46u     /* its y (word) */
#define K97_DROP_LO 0xD8FEu     /* (data bank) the window of K97_RANGE_X it drops in: low, high (words) */
#define K97_DROP_HI 0xD900u
#define K97_PAIR_A_LO 0xD902u   /* (data bank) the window it sends a pair in, $07 AND $11 nonzero: low, high */
#define K97_PAIR_A_HI 0xD904u
#define K97_PAIR_B_LO 0xD906u   /* (data bank) the window for $07 AND $11 zero: low, high */
#define K97_PAIR_B_HI 0xD908u
#define K97_CHILD_KIND 0x37u    /* the kind of the child it drops */
#define K97_KIND 0x97u          /* its own kind, back in ACTOR_06 after the drop */

/*
 * @routine 83:93B5 Kind97Enter
 * @actor
 * WHAT IT DOES: Where actors of kind $97 go on from the actor loop in
 * its start state (the table at $83:8DAB): Kind97Start. No script meets
 * it: written from its bytes.
 * HOW: JML to Kind97Start ($84:9F4F).
 * Entry: D = the actor's record; reached by the actor loop's jump (JMP
 * (table,X)) by kind. Exit: never returns: as Kind97Start.
 */
void abi_Kind97Enter(void)
{
    abi_Kind97Start();
}

/* K97_RANGE_X within the window lo..hi (data bank words), as the original
   tests it: 16-bit, CMP lo, then (not below) hi CMP x; M 8-bit after. */
static int in_window(uint16_t lo_at, uint16_t hi_at)
{
    uint16_t x, lo, hi;
    int ok = 0;

    op_rep(0x20);
    x = rd16(dba(K97_RANGE_X));
    a16_load(x);
    lo = rd16(dba(lo_at));
    cmp16_of(x, lo);
    if (x >= lo) {
        hi = rd16(dba(hi_at));
        a16_load(hi);
        cmp16_of(hi, x);
        ok = hi >= x;
    }
    return ok;
}

/* BRK at `brk_at` with 16-bit X until its child's record is free; then
   K97_BUSY 0 and X 8-bit. */
static void await_child(uint32_t brk_at, int brk_first)
{
    uint16_t child;

    do {
        if (brk_first) {
            actor_yield(brk_at);
            op_rep(0x10);
        } else {
            op_rep(0x10);
            actor_yield(brk_at);
        }
        child = dp16(K97_CHILD);
        cpu.x = child;
        cpu.p = nz16(cpu.p, child);
        a8_load(rd8(dba((uint16_t)(ACTOR_STATE + child))));
    } while ((uint8_t)cpu.a);
    wr8(dpa(K97_BUSY), 0x00);
    op_sep(0x10);
}

/* $07 bit 0 clear: the dropper ($84:9F67 on). */
static void dropper(void)
{
    for (;;) {
        do
            actor_yield(0x849F67u);
        while (dec8_at(dpa(K97_TIMER)));
        for (;;) {
            actor_yield(0x849F6Du);
            a8_load(rd8(dpa(K97_BUSY)));
            if ((uint8_t)cpu.a)
                continue;
            if (in_window(K97_DROP_LO, K97_DROP_HI)) {
                op_sep(0x20);
                break;
            }
            op_sep(0x20);
        }
        abi_Kind97DropChild();
        await_child(0x849F8Cu, 0);
    }
}

/* $07 bit 0 set: the sender ($84:9F9B on). */
static void sender(void)
{
    uint8_t c;
    int ok;

    for (;;) {
        op_sep(0x20);
        actor_yield(0x849F9Du);
        c = rd8(dpa(K97_COUNT));
        a8_load(c);
        cmp8_of(c, 0x04);
        if (c >= 0x04)
            continue;
        a8_load(rd8(dpa(K97_BUSY)));
        if ((uint8_t)cpu.a)
            continue;
        if (dec8_at(dpa(K97_TIMER)))
            continue;
        (void)inc8_at(dpa(K97_TIMER));
        a8_load((uint8_t)(rd8(dpa(ACTOR_07)) & 0x11));
        if ((uint8_t)cpu.a)
            ok = in_window(K97_PAIR_A_LO, K97_PAIR_A_HI);
        else
            ok = in_window(K97_PAIR_B_LO, K97_PAIR_B_HI);
        if (!ok)
            continue;
        op_sep(0x20);
        a8_load(0x02);
        abi_ActorFreeListTake();
        if (!(cpu.p & P_C))
            continue;
        wr8(dpa(K97_TAKEN), (uint8_t)cpu.y);
        abi_CarrierPairSend();
        await_child(0x849FEAu, 1);
    }
}

/*
 * @routine 84:9F4F Kind97Start
 * @actor
 * WHAT IT DOES: Actor kind $97: with $07 bit 0 clear, a dropper: after 30
 * frames, each time it is idle and the point actors are kept in range of
 * lies within its window, it drops a child and waits until the child's
 * record is free. With bit 0 set, a sender: each frame its count is below
 * 4 and it is idle, once its timer runs out and the point lies within the
 * window its $07 picks, it takes a free record, sends a carrier pair and
 * waits until its child's record is free. No script meets it: written from
 * its bytes.
 * HOW: $07 bit 0 clear: K97_TIMER $1E, K97_BUSY 0; BRK until K97_TIMER
 * counts out; then each frame (BRK) while K97_BUSY or K97_RANGE_X outside
 * K97_DROP_LO..K97_DROP_HI, again; Kind97DropChild (JSR); 16-bit X, BRK
 * until K97_CHILD's state is 0; K97_BUSY 0, X 8-bit, to the timer wait.
 * Bit 0 set: K97_TIMER 1, K97_BUSY and K97_COUNT 0; each frame (BRK):
 * K97_COUNT 4 or more, K97_BUSY set, or K97_TIMER down to nonzero: again;
 * K97_TIMER 1; K97_RANGE_X within K97_PAIR_A ($07 AND $11 nonzero) or
 * K97_PAIR_B: ActorFreeListTake with A = 2, C set: K97_TAKEN = Y,
 * CarrierPairSend (JSR); BRK, 16-bit X, until K97_CHILD's state is 0;
 * K97_BUSY 0, X 8-bit; again.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $84; reached by
 * Kind97Enter's JML. Exit: never returns.
 */
void abi_Kind97Start(void)
{
    uint8_t v = rd8(dpa(ACTOR_07));

    a8_load((uint8_t)(v & 0x01));
    if (!(v & 0x01)) {
        a8_load(0x1E);
        wr8(dpa(K97_TIMER), 0x1E);
        wr8(dpa(K97_BUSY), 0x00);
        dropper();
    } else {
        a8_load(0x01);
        wr8(dpa(K97_TIMER), 0x01);
        wr8(dpa(K97_BUSY), 0x00);
        wr8(dpa(K97_COUNT), 0x00);
        sender();
    }
}

/*
 * @routine 84:9FFB Kind97WaitWork
 * WHAT IT DOES: What actors of kind $97 run each frame they wait, by JSL
 * from the kind's stub ($83:93B9): an RTS on either path ($07 bit 0 clear
 * or set). An RTS pulls only two of the JSL's three return bytes, so it
 * goes on at the word pulled + 1 in this bank ($84:93BD for the stub's
 * JSL) with the JSL's bank byte left on the stack. No script meets it:
 * written from its bytes.
 * HOW: LDA $07, AND #1, BNE to the second RTS; RTS.
 * Entry: M 8-bit; D = the actor's record; called by JSL. Exit: at the
 * word pulled + 1 in bank $84; A = $07 AND 1, Z from it.
 */
void abi_Kind97WaitWork(void)
{
    uint16_t ret;

    a8_load((uint8_t)(rd8(dpa(ACTOR_07)) & 0x01));
    ret = pull16();                             /* RTS: two of the JSL's three bytes */
    jump_routine(0x840000u | (uint16_t)(ret + 1));
}

/*
 * @routine 84:A003 Kind97DropChild
 * WHAT IT DOES: Kind $97's drop: a child of kind $37 made as if from 80
 * above the point actors are kept in range of, with $07 one more than its
 * own, placed and started; while it is out, K97_BUSY counts and
 * K97_TIMER is $78. No script meets it: written from its bytes.
 * HOW: 16-bit: its x and y pushed; x = K97_RANGE_X, y = K97_RANGE_Y - $50;
 * ACTOR_06 = K97_CHILD_KIND; ActorCreate; C clear (made): 16-bit X =
 * K97_CREATED, kept in K97_CHILD; the child's $07 = its $07 + 1; spawn
 * place $44; ActorSpawnPlace; the child's state $0C; K97_BUSY up,
 * K97_TIMER $78. X 8-bit; its x and y pulled back; ACTOR_06 = K97_KIND.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
 * $0000-$1FFF is work RAM; called by JSR from Kind97Start. Exit: M and X
 * 8-bit; A = K97_KIND.
 */
void abi_Kind97DropChild(void)
{
    uint16_t child, y, x;

    op_rep(0x20);
    a16_load(dp16(ACTOR_X + 1));
    push16(cpu.a);
    a16_load(dp16(ACTOR_Y + 1));
    push16(cpu.a);
    a16_load(rd16(dba(K97_RANGE_X)));
    dp16_set(ACTOR_X + 1, cpu.a);
    flag_set(P_C, 1);
    a16_load(op_sbc16(rd16(dba(K97_RANGE_Y)), 0x0050, &cpu.p));
    dp16_set(ACTOR_Y + 1, cpu.a);
    op_sep(0x20);
    a8_load(K97_CHILD_KIND);
    wr8(dpa(ACTOR_06), K97_CHILD_KIND);
    abi_ActorCreate();
    if (!(cpu.p & P_C)) {
        op_rep(0x10);
        child = rd16(dba(K97_CREATED));
        cpu.x = child;
        cpu.p = nz16(cpu.p, child);
        dp16_set(K97_CHILD, child);
        a8_load((uint8_t)(rd8(dpa(ACTOR_07)) + 1));
        wr8(dba((uint16_t)(ACTOR_07 + cpu.x)), (uint8_t)cpu.a);
        a8_load(0x44);
        wr8(dpa(ACTOR_SPAWN_AT), 0x44);
        abi_ActorSpawnPlace();
        a8_load(0x0C);
        wr8(dba((uint16_t)(ACTOR_STATE + cpu.x)), 0x0C);
        (void)inc8_at(dpa(K97_BUSY));
        a8_load(0x78);
        wr8(dpa(K97_TIMER), 0x78);
    }
    op_sep(0x10);
    op_rep(0x20);
    y = pull16();
    a16_load(y);
    dp16_set(ACTOR_Y + 1, y);
    x = pull16();
    a16_load(x);
    dp16_set(ACTOR_X + 1, x);
    op_sep(0x20);
    a8_load(K97_KIND);
    wr8(dpa(ACTOR_06), K97_KIND);
}

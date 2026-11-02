/*
 * thrown_shot.c: actor kinds $10 and $14-$17 of the second pool: a shot
 * that flies its course, splashing (kind $13) where it lands, and kind
 * $10's end when it hits (kind $11). Drafted by cdraft.py; named and
 * described by hand.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "wram.h"
#include "actor.h"
#include "actor_anim.h"
#include "wram_video.h"
#include "wram_player.h"

void abi_ActorAllocBLong(void);
void abi_ActorEnd(void);
void abi_ActorInGround(void);
void abi_ActorMotionPreset(void);
void abi_ActorMove(void);
void abi_AnimStart(void);
void abi_AnimStep(void);
void abi_SoundQueueAdd(void);
void abi_Spawn13AtYLong(void);

#define SHOT_16 0x16u  /* word: $C9FF */
#define SHOT_11 0x11u
#define SHOT_LAUNCH 0x40u  /* frames left before bit 7 of $08 is cleared */
#define V_1774 0x1774u  /* the high byte of the word at $1773 (the camera's x) */

/* The shot is gone: off the screen, or below the level's floor (levels
   8 and $10: past the map's bottom; level $0E before screen 7: at its
   top row). */
static int shot_gone(void)
{
    uint8_t level = rd8(dba(LEVEL)), v;

    a8_load(level);
    cmp8_of(level, 0x08);
    if (level != 0x08)
        cmp8_of(level, 0x10);
    if (level == 0x08 || level == 0x10) {
        v = rd8(dpa(ACTOR_Y + 2));
        a8_load(v);
        if (v & 0x80)
            return 1;
    } else {
        cmp8_of(level, 0x0E);
        if (level == 0x0E) {
            v = rd8(dba(V_1774));
            a8_load(v);
            cmp8_of(v, 0x07);
            if (v < 0x07) {
                v = rd8(dpa(ACTOR_Y + 2));
                a8_load(v);
                if (!v)
                    return 1;
            }
        }
    }
    a8_load(rd8(dpa(ACTOR_09)) & 0x40);
    return !(uint8_t)cpu.a;
}

/*
 * @routine 81:EABA ShotFly
 * @actor
 * WHAT IT DOES: A shot (kinds $10 and $14-$17) started new: it flies the
 * course its $07 picks until it lands in the ground, where it splashes (an
 * actor of kind $13 in its place) and ends; it ends at once when it can no
 * longer be hit, and, in levels 8 and $10, when it is above the level's
 * top, in level $0E, when it is at the top and the camera is left of x
 * $0700.
 * HOW: Position fractions cleared, $09 = $50, animation 7, $2C = 4,
 * ActorMotionPreset by $07; fractions cleared again, $16 = $C9FF, $11 and
 * $18 cleared, bit 7 of $08 set, $40 = 2. Each frame (BRK first): the
 * level tests (LEVEL 8 or $10: y's high byte negative; $0E: V_1774 below 7
 * and y's high byte 0) and bit 6 of $09 clear end it (ActorEnd); else
 * AnimStep, ActorMove; after 2 frames bit 7 of $08 cleared; ActorInGround
 * with C set and bit 6 of V_0012 clear: Spawn13AtYLong at its own record
 * (Y = D), then ActorEnd.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $81; reached by
 * the actor loop's jump for kinds $10, $14-$17 in state $0C (through the
 * stub at $83:920E). Exit: never returns; ends in ActorEnd.
 */
void abi_ShotFly(void)
{
    uint8_t ground;

    wr8(dpa(ACTOR_X), 0);
    wr8(dpa(ACTOR_Y), 0);
    a8_load(0x50);
    wr8(dpa(ACTOR_09), 0x50);
    a8_load(0x07);
    abi_AnimStart();
    a8_load(0x04);
    wr8(dpa(ACTOR_2C), 0x04);
    y8_load(rd8(dpa(ACTOR_07)));           /* its course */
    abi_ActorMotionPreset();
    wr8(dpa(ACTOR_X), 0);
    wr8(dpa(ACTOR_Y), 0);
    dp16_set(SHOT_16, 0xC9FF);
    cpu.a = 0xC9FF;
    wr8(dpa(SHOT_11), 0);
    wr8(dpa(ACTOR_18), 0);
    a8_load(0x80);
    tsb_at(dpa(ACTOR_08), 0x80);
    a8_load(0x02);
    wr8(dpa(SHOT_LAUNCH), 0x02);
    for (;;) {
        actor_yield(0x81EAEBu);
        if (shot_gone())
            break;
        abi_AnimStep();
        abi_ActorMove();
        a8_load(rd8(dpa(SHOT_LAUNCH)));
        if ((uint8_t)cpu.a) {
            if (dec8_at(dpa(SHOT_LAUNCH)))
                continue;
            a8_load((uint8_t)(rd8(dpa(ACTOR_08)) & 0x7F));
            wr8(dpa(ACTOR_08), (uint8_t)cpu.a);
        }
        /* into the ground (not one it passes through): a mark, and gone */
        abi_ActorInGround();
        if (!(cpu.p & P_C))
            continue;
        ground = rd8(dba(V_0012));
        bit8_of((uint8_t)cpu.a, ground);
        if (ground & 0x40)
            continue;
        op_rep(0x30);
        cpu.a = cpu.y = cpu.d;
        nz16_of(cpu.d);
        op_sep(P_M);
        abi_Spawn13AtYLong();
        break;
    }
    op_sep(0x30);
    abi_ActorEnd();
}


/*
 * @routine 81:EB44 ShotHit
 * @actor
 * WHAT IT DOES: Actor kind $10 in state $4C (it hit something): a frame
 * later, sound 8 and an actor of kind $11 in its place from the second
 * pool, then its end.
 * HOW: BRK; SoundQueueAdd 8; bit 7 of $08 set; ActorAllocBLong; a record
 * (N clear): state $0C, kind $11, $07 0, $08 $80, its x and y pixels; then
 * (also with none) ActorEnd, M and X 8-bit.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $81; reached by
 * the actor loop's jump for kind $10 in state $4C and kind $B9 in state
 * $0C (through the stub at $83:9212). Exit: never returns; ends in
 * ActorEnd.
 */
void abi_ShotHit(void)
{
    actor_yield(0x81EB44u);
    a8_load(0x08);
    abi_SoundQueueAdd();
    a8_load((uint8_t)(rd8(dpa(ACTOR_08)) | 0x80));
    wr8(dpa(ACTOR_08), (uint8_t)cpu.a);
    abi_ActorAllocBLong();
    if (!(cpu.p & P_N)) {                 /* a kind $11 mark where it hit */
        uint32_t mark = dba(0) + cpu.x;
        uint16_t y;

        a8_load(0x0C);
        wr8(mark + ACTOR_STATE, 0x0C);
        a8_load(0x11);
        wr8(mark + ACTOR_06, 0x11);
        wr8(mark + ACTOR_07, 0);
        a8_load(0x80);
        wr8(mark + ACTOR_08, 0x80);
        op_rep(P_M);
        wr16(mark + ACTOR_X + 1, dp16(ACTOR_X + 1));
        y = dp16(ACTOR_Y + 1);
        wr16(mark + ACTOR_Y + 1, y);
        a16_load(y);
    }
    op_sep(0x30);
    abi_ActorEnd();
}

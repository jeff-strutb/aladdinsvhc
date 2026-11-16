/*
 * kind73.c: actor kind $73's per-frame work while it waits: it follows a
 * path, shows the animation its controller picks, and is touched
 * (stomped, shots turned back) at a place offset from where it is drawn.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "actor.h"

void abi_AnimStart(void);
void abi_AnimStep(void);
void abi_ActorPictureSendFromKind(void);
void abi_ActorPathStep(void);
void abi_ActorStompedByPlayerLong(void);
void abi_ActorTurnsShotBackLong(void);

#define K73_ANIM 0x1B20u            /* the animation it should show */
#define K73_ANIM_SHOWN 0x1B21u      /* the animation it was last started on */
#define K73_KEPT_X 0x1B23u          /* its x (fraction, pixels word) kept over the touch tests */
#define K73_KEPT_X_PIXELS 0x1B24u
#define K73_KEPT_MID 0x1B25u        /* its x's high byte, then its y's fraction (word) */
#define K73_KEPT_Y_PIXELS 0x1B27u
#define K73_FLASH 0x1B2Au           /* frames left of its flash; at 0, $08 bits 4 and 7 cleared */
#define K73_UNTOUCHABLE 0x1B2Cu     /* nonzero: no touch tests */
#define K73_POSE 0x1B35u            /* index (by 4) of the touch place's offsets */
#define K73_TOUCH_DX 0xD7B6u        /* per pose: x offset (word), then y offset (word) */
#define K73_TOUCH_DY 0xD7B8u

/*
 * @routine 84:86F4 Kind73Work
 * WHAT IT DOES: What actors of kind $73 do each frame they wait: it
 * starts the animation K73_ANIM names when that changes, animates, shows
 * its picture from its kind's graphics, steps along its path, counts its
 * flash down, and is touched at a place offset from it by its pose:
 * Aladdin stomping it and shots turned back there, unless untouchable.
 * HOW: K73_ANIM against K73_ANIM_SHOWN: changed, kept and AnimStart.
 * AnimStep, ActorPictureSendFromKind, ActorPathStep. K73_FLASH nonzero:
 * down by one, at 0 ACTOR_08 AND $6F. Its x and y ($0B-$10) kept at
 * $1B23-$1B28 and moved by the pose's offsets (K73_TOUCH_DX/DY by
 * K73_POSE); K73_UNTOUCHABLE zero: ActorStompedByPlayerLong and
 * ActorTurnsShotBackLong; then $0B-$10 put back.
 * Entry: M 8-bit, X 8-bit; D = the actor; DB the bank of the offsets table
 * and of work RAM at $0000-$1FFF; called by JSL from the kind's stub
 * ($83:925E) through the actor loop's wait table at $83:8F4F. Exit: M
 * 8-bit, X 8-bit; A = its y's pixels word; X = its y's fraction; N and Z
 * from A; C as the touch tests or the y sum left it.
 */
void abi_Kind73Work(void)
{
    uint8_t anim = rd8(dba(K73_ANIM)), shown = rd8(dba(K73_ANIM_SHOWN));
    uint8_t flash, pose, untouchable;
    uint16_t at;

    a8_load(anim);
    cmp8_of(anim, shown);
    if (anim != shown) {
        wr8(dba(K73_ANIM_SHOWN), anim);
        abi_AnimStart();
    }
    abi_AnimStep();
    abi_ActorPictureSendFromKind();
    abi_ActorPathStep();

    flash = rd8(dba(K73_FLASH));
    a8_load(flash);
    if (flash && !dec8_at(dba(K73_FLASH)))
    {
        uint8_t v = (uint8_t)(rd8(dpa(ACTOR_08)) & 0x6F);

        a8_load(v);
        wr8(dpa(ACTOR_08), v);
    }

    /* moved to the touch place for the tests, its own place kept */
    op_rep(0x20);
    pose = rd8(dba(K73_POSE));
    x8_load(pose);
    at = dp16(ACTOR_X + 1);
    wr16(dba(K73_KEPT_X_PIXELS), at);
    flag_set(P_C, 0);
    cpu.a = op_adc16(at, rd16(dba((uint16_t)(K73_TOUCH_DX + pose))), &cpu.p);
    dp16_set(ACTOR_X + 1, cpu.a);
    at = dp16(ACTOR_Y + 1);
    wr16(dba(K73_KEPT_Y_PIXELS), at);
    flag_set(P_C, 0);
    cpu.a = op_adc16(at, rd16(dba((uint16_t)(K73_TOUCH_DY + pose))), &cpu.p);
    dp16_set(ACTOR_Y + 1, cpu.a);
    x8_load(rd8(dpa(ACTOR_X)));
    wr8(dba(K73_KEPT_X), (uint8_t)cpu.x);
    x8_load(rd8(dpa(ACTOR_Y)));
    wr8(dba(K73_KEPT_MID + 1), (uint8_t)cpu.x);
    op_sep(0x20);

    untouchable = rd8(dba(K73_UNTOUCHABLE));
    a8_load(untouchable);
    if (!untouchable) {
        abi_ActorStompedByPlayerLong();
        abi_ActorTurnsShotBackLong();
    }

    /* back where it is drawn */
    op_rep(0x20);
    dp16_set(ACTOR_X, rd16(dba(K73_KEPT_X)));
    dp16_set(ACTOR_X + 2, rd16(dba(K73_KEPT_MID)));
    at = rd16(dba(K73_KEPT_Y_PIXELS));
    a16_load(at);
    dp16_set(ACTOR_Y + 1, at);
    op_sep(0x20);
}

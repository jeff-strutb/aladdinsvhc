/*
 * kind73.c: actor kind $73's per-frame work while it waits: it follows a
 * path, shows the animation its controller picks, and is touched
 * (stomped, shots turned back) at a place offset from where it is drawn.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "actor.h"
#include "ops65.h"

void abi_AnimStart(void);
void abi_AnimStep(void);
void abi_ActorPictureSendFromKind(void);
void abi_ActorPathStep(void);
void abi_ActorStompedByPlayerLong(void);
void abi_ActorTurnsShotBackLong(void);
void abi_ActorStartBy40(void);
void abi_ActorAnimBy40(void);
void abi_PaletteSaveReversed(void);
void abi_ActorYieldForever(void);
void abi_ActorCreateChild8(void);
void abi_ActorLookFlash(void);
void abi_PaletteFlash(void);
void abi_ActorParityBitsSet(void);
void abi_ChainPieceEnd(void);

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
#define K73_HEALTH 0x11u            /* its health: 7, or $7F on a copier; below 0, beaten */
#define K73_28 0x28u                /* $FF */
#define K73_29 0x29u                /* word: $8919 */
#define K73_FRAMES 0x44u            /* frames left of a pause in its own turn */
#define K73_HITS 0x1B2Bu            /* counted up each time it is struck below 0 */
#define K73_HIT_FRAMES 0x1B29u      /* frames left of its show of being struck */
#define K73_SLOTS 0x7EFE60u         /* words 1-15 set to $FFFF at its start */
#define K73_COPIER_TEST 0x0000u     /* in bank $70 (by X = $80): cartridge RAM a copier has */
#define K73_COPIER_TEST_2 0x0080u
#define K73_COPIER_BANK 0x80u       /* less $10: the bank of the copier test */
#define CHAIN_GO 0x1B18u            /* nonzero once the chain is built (chain_start.c) */
#define CHAIN_ROUND_OR 0x1B1Eu      /* ORed with the round for the chain's pick lists */
#define HEAD_CUE 0x1B22u            /* set when its own turn is done (chain_start.c) */
#define V_0844 0x0844u              /* counted up at its start */

/* A pause of n frames (K73_FRAMES), BRK at brk_at each frame. */
static void k73_pause(uint8_t n, uint32_t brk_at)
{
    a8_load(n);
    wr8(dpa(K73_FRAMES), n);
    do
        actor_yield(brk_at);
    while (dec8_at(dpa(K73_FRAMES)));
}

/*
 * @routine 84:8629 Kind73Start
 * @actor
 * WHAT IT DOES: Actor kind $73, the head of the nine-piece chain: once the
 * chain is built it takes its health (7, or $7F on a copier, whose
 * cartridge RAM holds what is written there), starts and animates by its
 * number, clears its touch pose and the words at K73_SLOTS, saves the
 * palette reversed, shows no animation yet, and waits for good (its turns
 * come from the chain's eighth piece, which writes its resume address).
 * HOW: Each frame (BRK) until CHAIN_GO. With DB $70 (PHB, $80 less $10,
 * PHA, PLB): $67 written at $0080 and $89 at $0100 and read back, both
 * equal: A = $7F, else 7; PLB; K73_HEALTH = A; ACTOR_SPAWN_AT $88;
 * K73_UNTOUCHABLE up; K73_POSE 0; ActorStartBy40, ActorAnimBy40; K73_29
 * = $8919; K73_SLOTS words $1E down to 2 = $FFFF; PaletteSaveReversed 2;
 * V_0844 up; K73_28 $FF; K73_ANIM 0, K73_ANIM_SHOWN $FF;
 * ActorYieldForever.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
 * $0000-$1FFF is work RAM; reached by the actor loop's jump for kind $73
 * in state $0C. Exit: never returns.
 */
void abi_Kind73Start(void)
{
    uint8_t v;

    do {
        actor_yield(0x848629u);
        a8_load(rd8(dba(CHAIN_GO)));
    } while (!(uint8_t)cpu.a);

    /* the copier test, in bank $70 */
    x8_load(0x80);
    phb();
    flag_set(P_C, 1);
    a8_load(K73_COPIER_BANK);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | op_sbc8((uint8_t)cpu.a, 0x10, &cpu.p));
    pha8();
    plb();
    a8_load(0x67);
    wr8(dba((uint16_t)(K73_COPIER_TEST + cpu.x)), 0x67);
    cmp8_of(0x67, rd8(dba((uint16_t)(K73_COPIER_TEST + cpu.x))));
    v = 0x07;
    if (cpu.p & P_Z) {
        a8_load(0x89);
        wr8(dba((uint16_t)(K73_COPIER_TEST_2 + cpu.x)), 0x89);
        cmp8_of(0x89, rd8(dba((uint16_t)(K73_COPIER_TEST_2 + cpu.x))));
        if (cpu.p & P_Z)
            v = 0x7F;
    }
    a8_load(v);
    plb();
    wr8(dpa(K73_HEALTH), v);

    a8_load(0x88);
    wr8(dpa(ACTOR_SPAWN_AT), 0x88);
    (void)inc8_at(dba(K73_UNTOUCHABLE));
    wr8(dba(K73_POSE), 0);
    abi_ActorStartBy40();
    call_frame(0x8666u, abi_ActorAnimBy40);
    op_rep(0x20);
    a16_load(0x8919);
    dp16_set(K73_29, 0x8919);
    x8_load(0x1E);
    a16_load(0xFFFF);
    do {
        wr16(K73_SLOTS + cpu.x, 0xFFFF);
        x8_load((uint8_t)(cpu.x - 1));
        x8_load((uint8_t)(cpu.x - 1));
    } while (cpu.x);
    op_sep(0x20);
    a8_load(0x02);
    abi_PaletteSaveReversed();
    (void)inc8_at(dba(V_0844));
    a8_load(0xFF);
    wr8(dpa(K73_28), 0xFF);
    a8_load(0x00);
    wr8(dba(K73_ANIM), 0x00);
    a8_load(0xFF);                          /* DEC A */
    wr8(dba(K73_ANIM_SHOWN), 0xFF);
    abi_ActorYieldForever();
}

/*
 * @routine 84:8696 Kind73OwnTurn
 * @actor
 * WHAT IT DOES: Kind $73's own turn, where the chain's eighth piece sends
 * it (its resume address): twice animation 4, a child of hit box $A1, a
 * pause; then HEAD_CUE set for the piece, and it waits for good.
 * HOW: K73_ANIM 4, $14 frames (BRK, K73_FRAMES down), ActorCreateChild8
 * $A1, $1F frames; K73_ANIM 0, $18 frames; K73_ANIM 4, $14 frames,
 * ActorCreateChild8 $A1, $1F frames; K73_ANIM 0, $81 frames; HEAD_CUE up;
 * ActorYieldForever.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
 * $0000-$1FFF is work RAM; reached by the actor loop's RTI at the resume
 * address chain_start.c writes (HEAD_OWN_CODE). Exit: never returns.
 */
void abi_Kind73OwnTurn(void)
{
    a8_load(0x04);
    wr8(dba(K73_ANIM), 0x04);
    k73_pause(0x14, 0x84869Fu);
    a8_load(0xA1);
    abi_ActorCreateChild8();
    k73_pause(0x1F, 0x8486AFu);
    wr8(dba(K73_ANIM), 0);
    k73_pause(0x18, 0x8486BCu);
    a8_load(0x04);
    wr8(dba(K73_ANIM), 0x04);
    k73_pause(0x14, 0x8486CBu);
    a8_load(0xA1);
    abi_ActorCreateChild8();
    k73_pause(0x1F, 0x8486DBu);
    wr8(dba(K73_ANIM), 0);
    k73_pause(0x81, 0x8486E8u);
    (void)inc8_at(dba(HEAD_CUE));
    abi_ActorYieldForever();
}

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

/*
 * @routine 84:8766 Kind73Hit
 * @actor
 * WHAT IT DOES: Kind $73 struck: below 0 health counted in K73_HITS; it
 * shows being struck (animation 2, flashing) for $1F frames. Still
 * standing: it flashes for $5F frames more (K73_FLASH), the chain's pick
 * lists go to their second set (CHAIN_ROUND_OR 4) once its health is
 * below 3, its animation is started again, and it goes back to what it
 * was doing (ChainPieceEnd). Beaten (health negative): animation 3, its
 * $09 bit 7 set, and it stays for good.
 * HOW: K73_HEALTH negative: K73_HITS up. AnimStart 2,
 * ActorPictureSendFromKind; K73_HIT_FRAMES $1F; each frame (BRK)
 * K73_HIT_FRAMES down, ActorLookFlash, PaletteFlash, ActorParityBitsSet,
 * AnimStep, ActorPictureSendFromKind until it is 0. K73_HEALTH positive:
 * K73_FLASH $5F; below 3, CHAIN_ROUND_OR 4; K73_ANIM_SHOWN $FF; on into
 * ChainPieceEnd. Negative: K73_ANIM 3, ACTOR_09 OR $80, BRK for good.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
 * $0000-$1FFF is work RAM; reached by the actor loop's jump for kind $73
 * when struck. Exit: never returns.
 */
void abi_Kind73Hit(void)
{
    uint8_t h, v;

    h = rd8(dpa(K73_HEALTH));
    a8_load(h);
    if (h & 0x80)
        (void)inc8_at(dba(K73_HITS));
    a8_load(0x02);
    jsl_frame(0x84876Fu);
    abi_AnimStart();
    cpu.s = (uint16_t)(cpu.s + 3);
    jsl_frame(0x848773u);
    abi_ActorPictureSendFromKind();
    cpu.s = (uint16_t)(cpu.s + 3);
    a8_load(0x1F);
    wr8(dba(K73_HIT_FRAMES), 0x1F);
    do {
        actor_yield(0x84877Cu);
        (void)dec8_at(dba(K73_HIT_FRAMES));
        abi_ActorLookFlash();
        abi_PaletteFlash();
        abi_ActorParityBitsSet();
        abi_AnimStep();
        jsl_frame(0x84878Eu);
        abi_ActorPictureSendFromKind();
        cpu.s = (uint16_t)(cpu.s + 3);
        a8_load(rd8(dba(K73_HIT_FRAMES)));
    } while ((uint8_t)cpu.a);

    h = rd8(dpa(K73_HEALTH));
    a8_load(h);
    if (!(h & 0x80)) {
        a8_load(0x5F);
        wr8(dba(K73_FLASH), 0x5F);
        h = rd8(dpa(K73_HEALTH));
        a8_load(h);
        cmp8_of(h, 0x03);
        if (!(cpu.p & P_C)) {
            a8_load(0x04);
            wr8(dba(CHAIN_ROUND_OR), 0x04);
        }
        a8_load(0xFF);
        wr8(dba(K73_ANIM_SHOWN), 0xFF);
        abi_ChainPieceEnd();
        return;
    }
    a8_load(0x03);
    wr8(dba(K73_ANIM), 0x03);
    v = (uint8_t)(rd8(dpa(ACTOR_09)) | 0x80);
    a8_load(v);
    wr8(dpa(ACTOR_09), v);
    for (;;)
        actor_yield(0x8487BEu);
}

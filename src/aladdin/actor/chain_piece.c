/*
 * chain_piece.c: what the pieces of the nine-piece chain (kinds $74 and
 * $75, behind its head of kind $73) do each frame they wait.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "wram.h"
#include "actor.h"

void abi_AnimStep(void);
void abi_ActorPathStep(void);
void abi_ActorsFollowIf8(void);
void abi_ActorHurtsPlayerOnTouchLong(void);
void abi_ActorTurnsShotBackLong(void);
void abi_ActorLookFlash(void);
void abi_ActorParityBitsSet(void);
void abi_ActorYieldForever(void);
void abi_ActorListStart(void);
void abi_ActorListFindIdle(void);
void abi_TaskCreate(void);
void abi_TaskExec(void);
void abi_SongPlay(void);
void abi_SoundQueueAdd(void);
void abi_StepCountDown(void);
void abi_StepCountUp(void);
void abi_ChainPieceEnd(void);

#define CHAIN_HEAD 0x1B14u          /* the head's record (word; child_chain.c) */
#define CHAIN_UNTOUCHABLE 0x1B2Cu   /* nonzero: no touch tests (the head's K73_UNTOUCHABLE) */
#define V_032D 0x032Du              /* the high byte of the word at $032C */
#define STATE_GONE 0x8Cu            /* the head's state once it is going (and so the pieces') */
#define PIECE_RESUME 0x35u          /* word: where the piece resumes, kept from ACTOR_03 */
#define HEAD_HIT_FRAMES 0x1B29u     /* frames left of the head's show of being struck (kind73.c) */
#define HEAD_HEALTH 0x11u           /* the head's health (by its record); negative: beaten */
#define HEAD_ANIM 0x1B20u           /* the head's animation (kind73.c's K73_ANIM) */
#define PIECE_NUMBER 0x40u          /* the piece's place in the chain (8: the one that ends the fight) */
#define PIECE_FRAMES 0x44u          /* frames left of a pause */
#define FIGHT_WON 0x03FAu           /* counted up by each piece as the fight ends */
#define V_1A16 0x1A16u              /* 8 as the scene after the fight starts */
#define SCENE_WAIT 0x0087u          /* nonzero while the scene's fade runs */
#define V_005D 0x005Du              /* nonzero on a PAL console (RegionCheck): the other scene after */
#define PLAYER_HEALTH 0x0367u       /* the player's health */
#define PLAYER_DIR_SET 0x1428u      /* nonzero: the player's direction is set (wram_player.h's V_1428) */
#define PLAYER_CODE_AFTER 0xE154u   /* the player's code once the scene has him */
#define RISE_V 0x1B19u              /* $D4, counted up to 0 as the piece rises */
#define RISE_SOUNDED 0x1B31u        /* set once the head has risen past y $B0 (sound $49) */
#define RISE_SPEED 0x1B2Du          /* 24-bit, fraction first: the piece's rise, down by $20 a frame */
#define RISE_1B30 0x1B30u           /* $20 */
#define CLOSE_COUNT 0x1B32u         /* counted up as the closing list starts */
#define HEAD_Y_PIXELS 0x0Fu         /* (by its record) the head's y pixels word */

/*
 * @routine 84:8A78 ChainPieceWork
 * WHAT IT DOES: What the chain's pieces (kinds $74 and $75) do each frame
 * they wait: animate, follow their path and the pieces after them, and,
 * unless the chain is untouchable, hurt Aladdin on touch on odd counts of
 * V_032D or turn his shots back on even ones; when the head goes into
 * state $8C the piece goes with it; its $08 bits 4 and 7 follow the
 * head's.
 * HOW: AnimStep, ActorPathStep, ActorsFollowIf8; CHAIN_UNTOUCHABLE zero:
 * V_032D bit 0 set ActorHurtsPlayerOnTouchLong, clear
 * ActorTurnsShotBackLong. With X 16-bit = CHAIN_HEAD: the head's state
 * $8C: the piece's state $8C, PIECE_RESUME = its $03 word, $09 bit 7
 * cleared (ASL, LSR). SCRATCH = the head's $08 AND $90; the piece's $08 =
 * its $08 AND $6F OR SCRATCH.
 * Entry: M 8-bit, X 8-bit; D = the piece's record; DB a bank whose
 * $0000-$1FFF is work RAM; called by JSL from the kinds' stubs ($83:926B,
 * $83:9278) through the actor loop's wait table. Exit: M 8-bit, X 8-bit;
 * A = the new $08, N and Z from it; X = the low byte of CHAIN_HEAD's
 * record; Y as the touch tests left it, or the low byte of $03 when the
 * head went; C clear when the head went, else from comparing $8C with the
 * head's state.
 */
void abi_ChainPieceWork(void)
{
    uint16_t head;
    uint8_t untouchable, state, bits, v;

    jsl_frame(0x848A78u);
    abi_AnimStep();
    cpu.s = (uint16_t)(cpu.s + 3);
    call_frame(0x8A7E, abi_ActorPathStep);
    call_frame(0x8A81, abi_ActorsFollowIf8);
    untouchable = rd8(dba(CHAIN_UNTOUCHABLE));
    a8_load(untouchable);
    if (!untouchable) {
        a8_load((uint8_t)(rd8(dba(V_032D)) & 0x01));
        if (cpu.a & 0x01)
            { jsl_frame(0x848A8Eu); abi_ActorHurtsPlayerOnTouchLong(); cpu.s = (uint16_t)(cpu.s + 3); }
        else
            { jsl_frame(0x848A94u); abi_ActorTurnsShotBackLong(); cpu.s = (uint16_t)(cpu.s + 3); }
    }

    op_rep(0x10);
    head = rd16(dba(CHAIN_HEAD));
    cpu.x = head;
    a8_load(STATE_GONE);
    state = rd8(dba(head));
    cmp8_of(STATE_GONE, state);
    if (state == STATE_GONE) {
        uint16_t resume = dp16(ACTOR_03);

        wr8(dpa(ACTOR_STATE), STATE_GONE);
        cpu.y = resume;
        dp16_set(PIECE_RESUME, resume);
        wr8(dpa(ACTOR_09), (uint8_t)(rd8(dpa(ACTOR_09)) << 1));
        wr8(dpa(ACTOR_09), (uint8_t)(rd8(dpa(ACTOR_09)) >> 1));
        flag_set(P_C, 0);                   /* the LSR's: bit 0 of a doubled byte */
    }
    bits = (uint8_t)(rd8(dba((uint16_t)(head + ACTOR_08))) & 0x90);
    wr8(dba(SCRATCH), bits);
    v = (uint8_t)((rd8(dpa(ACTOR_08)) & 0x6F) | bits);
    a8_load(v);
    wr8(dpa(ACTOR_08), v);
    op_sep(0x10);
}

/* ActorListStart with the list at $84:hi lo and the step a. */
static void list_start(uint8_t lo, uint8_t hi, uint8_t a)
{
    x8_load(lo);
    y8_load(hi);
    a8_load(a);
    abi_ActorListStart();
}

/* Each frame (BRK at brk_at) until ActorListFindIdle finds one idle. */
static void until_idle(uint32_t brk_at)
{
    do {
        actor_yield(brk_at);
        abi_ActorListFindIdle();
    } while (!(cpu.p & P_C));
}

/* LDA at; ADC with the byte at m (CLC made by the caller); STA at. */
static void adc_mem(uint32_t at, uint32_t m)
{
    uint8_t v = rd8(at);

    v = op_adc8(v, rd8(m), &cpu.p);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    wr8(at, v);
}

/* LDA at; ADC #imm; STA at. */
static void adc_imm(uint32_t at, uint8_t imm)
{
    uint8_t v = rd8(at);

    v = op_adc8(v, imm, &cpu.p);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    wr8(at, v);
}

/* TaskCreate by the JSL at site, its five bytes after it. */
static void task_create(uint32_t site)
{
    jsl_frame(site);
    abi_TaskCreate();
    cpu.unwound = 0;
}

/* The fight won ($84:8AF6, in ChainPieceEnd's code): reached from
   ChainPieceGone when the head's health is negative. */
static void fight_won(void)
{
    uint8_t v;
    uint16_t head;

    (void)inc8_at(dba(FIGHT_WON));
    v = (uint8_t)(rd8(dpa(ACTOR_09)) | 0x80);
    a8_load(v);
    wr8(dpa(ACTOR_09), v);
    v = rd8(dpa(PIECE_NUMBER));
    a8_load(v);
    cmp8_of(v, 0x08);
    if (v != 0x08) {
        abi_ActorYieldForever();
        return;
    }
    list_start(0xCD, 0x89, 0x04);
    until_idle(0x848B11u);
    list_start(0xBB, 0x89, 0x10);
    a8_load(0x08);
    wr8(dba(V_1A16), 0x08);
    task_create(0x848B26u);                 /* slot 1: ShakeTask, 4 */
    task_create(0x848B2Fu);                 /* slot 2: SoundWhileSlot1Task */
    task_create(0x848B38u);                 /* slot 3: RiserCoursesTask */
    task_create(0x848B41u);                 /* slot 4: $81:A2FF */
    do {                                    /* down half a pixel a frame through the fade */
        actor_yield(0x848B4Au);
        flag_set(P_C, 0);
        adc_imm(dpa(ACTOR_Y), 0x80);
        adc_imm(dpa(ACTOR_Y + 1), 0x00);
        adc_imm(dpa(ACTOR_Y + 2), 0x00);
        a8_load(rd8(dba(SCENE_WAIT)));
    } while ((uint8_t)cpu.a);
    a8_load(rd8(dba(PLAYER_HEALTH)));
    if (!(uint8_t)cpu.a)
        for (;;)
            actor_yield(0x848B69u);         /* Aladdin is gone: it stays */
    do {
        actor_yield(0x848B6Du);
        a8_load(rd8(dba(PLAYER_DIR_SET)));
    } while (!(uint8_t)cpu.a);

    /* the player's code from here; his $09 bit 7 and $08 bit 4 cleared */
    a8_load((uint8_t)PLAYER_CODE_AFTER);
    wr8(dba((uint16_t)(PLAYER + ACTOR_03)), (uint8_t)PLAYER_CODE_AFTER);
    a8_load((uint8_t)(PLAYER_CODE_AFTER >> 8));
    wr8(dba((uint16_t)(PLAYER + ACTOR_04)), (uint8_t)(PLAYER_CODE_AFTER >> 8));
    v = rd8(dba((uint16_t)(PLAYER + ACTOR_09)));
    flag_set(P_C, v & 0x80);
    v = (uint8_t)(v << 1);
    nz8_of(v);
    wr8(dba((uint16_t)(PLAYER + ACTOR_09)), v);
    v = rd8(dba((uint16_t)(PLAYER + ACTOR_09)));
    flag_set(P_C, v & 0x01);
    v = (uint8_t)(v >> 1);
    nz8_of(v);
    wr8(dba((uint16_t)(PLAYER + ACTOR_09)), v);
    v = (uint8_t)(rd8(dba((uint16_t)(PLAYER + ACTOR_08))) & 0xEF);
    a8_load(v);
    wr8(dba((uint16_t)(PLAYER + ACTOR_08)), v);
    a8_load(0xBD);
    actor_wait(0x848B8Eu);
    actor_yield(0x848B90u);
    a8_load(0x36);
    jsl_frame(0x848B94u);
    abi_SongPlay();
    cpu.s = (uint16_t)(cpu.s + 3);
    a8_load(0xFF);
    actor_wait(0x848B9Au);
    a8_load(0xFF);
    actor_wait(0x848B9Eu);

    /* it rises, slower each frame, until its speed turns */
    a8_load(0xD4);
    wr8(dba(RISE_V), 0xD4);
    wr8(dba(RISE_SOUNDED), 0);
    op_rep(0x30);
    a16_load(0x02C8);
    dp16_set(ACTOR_X + 1, 0x02C8);
    a16_load(0x0150);
    dp16_set(ACTOR_Y + 1, 0x0150);
    op_sep(0x30);
    a8_load(0x47);
    wr8(dba(RISE_SPEED), 0x47);
    a8_load(0xFA);
    wr8(dba(RISE_SPEED + 1), 0xFA);
    a8_load(0xFF);
    wr8(dba(RISE_SPEED + 2), 0xFF);
    a8_load(0x20);
    wr8(dba(RISE_1B30), 0x20);
    do {
        actor_yield(0x848BCAu);
        flag_set(P_C, 0);
        adc_mem(dpa(ACTOR_Y), dba(RISE_SPEED));
        adc_mem(dpa(ACTOR_Y + 1), dba(RISE_SPEED + 1));
        adc_mem(dpa(ACTOR_Y + 2), dba(RISE_SPEED + 2));
        flag_set(P_C, 0);
        adc_imm(dba(RISE_SPEED), 0x20);
        adc_imm(dba(RISE_SPEED + 1), 0x00);
        adc_imm(dba(RISE_SPEED + 2), 0x00);
        a8_load(rd8(dba(RISE_SOUNDED)));
        if (!(uint8_t)cpu.a) {
            uint16_t y;

            op_rep(0x30);
            head = rd16(dba(CHAIN_HEAD));
            cpu.x = head;
            nz16_of(head);
            y = rd16(dba((uint16_t)(head + HEAD_Y_PIXELS)));
            a16_load(y);
            cmp16_of(y, 0x00B0);
            op_sep(0x30);
            if (!(cpu.p & P_C)) {
                (void)inc8_at(dba(RISE_SOUNDED));
                a8_load(0x49);
                abi_SoundQueueAdd();
            }
        }
        a8_load(rd8(dba(RISE_V)));
        if ((uint8_t)cpu.a)
            (void)inc8_at(dba(RISE_V));
        a8_load(rd8(dba(RISE_SPEED + 2)));
    } while (cpu.a & 0x80);

    /* the closing lists */
    wr8(dba(HEAD_ANIM), 0);
    a8_load(0x01);
    wr8(dpa(ACTOR_18), 0x01);
    list_start(0xD1, 0x89, 0x01);
    until_idle(0x848C35u);
    a8_load(0x01);
    wr8(dba(HEAD_ANIM), 0x01);
    list_start(0xC9, 0x89, 0x01);
    do {
        actor_yield(0x848C4Au);
        abi_StepCountDown();
    } while (!(cpu.p & P_Z));
    until_idle(0x848C51u);
    a8_load(0x3F);
    wr8(dpa(PIECE_FRAMES), 0x3F);
    do
        actor_yield(0x848C5Cu);
    while (dec8_at(dpa(PIECE_FRAMES)));
    a8_load(0x06);
    wr8(dba(HEAD_ANIM), 0x06);
    list_start(0x2E, 0x8A, 0x10);
    a8_load(0x49);
    abi_SoundQueueAdd();
    (void)inc8_at(dba(CLOSE_COUNT));
    do {
        actor_yield(0x848C79u);
        abi_StepCountUp();
    } while (!(cpu.p & P_Z));
    until_idle(0x848C80u);
    a8_load(0x06);
    wr8(dba(HEAD_ANIM), 0x06);
    task_create(0x848C8Cu);                 /* slot 1: BrightnessDownTask, 4 */
    do {
        actor_yield(0x848C95u);
        a8_load(rd8(dba(SCENE_WAIT)));
    } while ((uint8_t)cpu.a);
    a8_load(rd8(dba(V_005D)));
    if ((uint8_t)cpu.a) {
        jsl_frame(0x848CA1u);               /* TaskExec $84:AFCC */
        abi_TaskExec();
        return;
    }
    jsl_frame(0x848CA8u);                   /* TaskExec $80:AF42 */
    abi_TaskExec();
}

/*
 * @routine 84:8AC2 ChainPieceGone
 * @actor
 * WHAT IT DOES: A piece of the chain while the head shows being struck:
 * it flashes with the head; then, with the head still standing, goes back
 * to what it was doing (ChainPieceEnd); with the head beaten, the fight is
 * won: FIGHT_WON up, and all but the last piece wait for good; the last
 * (PIECE_NUMBER 8) runs the scene after the fight: two lists of children,
 * the shake, sound and riser tasks, the fade waited on (moving down half a
 * pixel a frame), then, with Aladdin still alive, his code set to
 * PLAYER_CODE_AFTER and a wait; the song $36; it rises at a speed that
 * slows (RISE_SPEED) until it turns, sound $49 once the head is above y
 * $B0; three closing lists and the brightness task; once the fade is done
 * TaskExec to the scene that follows ($80:AF42, or $84:AFCC on a PAL
 * console).
 * HOW: Each frame (BRK) ActorLookFlash, ActorParityBitsSet,
 * ActorsFollowIf8 until HEAD_HIT_FRAMES 0. The head's HEAD_HEALTH (X
 * 16-bit = CHAIN_HEAD) positive: on into ChainPieceEnd. Negative: FIGHT_WON
 * up; ACTOR_09 OR $80; PIECE_NUMBER not 8: ActorYieldForever. 8:
 * ActorListStart $89CD (4), BRK until ActorListFindIdle; ActorListStart
 * $89BB ($10); V_1A16 8; TaskCreate slots 1-4 (ShakeTask 4,
 * SoundWhileSlot1Task, RiserCoursesTask, $81:A2FF); each frame (BRK) its y
 * + $80 (24-bit) until SCENE_WAIT 0; health 0: BRK for good; BRK until
 * PLAYER_DIR_SET; the player's code word PLAYER_CODE_AFTER, his $09 bit 7
 * and $08 bit 4 cleared; COP $BD, BRK, SongPlay $36, COP $FF twice; RISE_V
 * $D4, RISE_SOUNDED 0, x $02C8, y $0150, RISE_SPEED $FFFA47, RISE_1B30
 * $20; each frame (BRK) y + RISE_SPEED, RISE_SPEED + $20, the head above y
 * $B0 the first time: sound $49; RISE_V nonzero: up; until RISE_SPEED's
 * top byte is not negative. HEAD_ANIM 0, ACTOR_18 1, ActorListStart $89D1
 * (1), BRK until idle; HEAD_ANIM 1, ActorListStart $89C9 (1), BRK until
 * StepCountDown gives 0, BRK until idle; $3F frames; HEAD_ANIM 6,
 * ActorListStart $8A2E ($10), sound $49, CLOSE_COUNT up, BRK until
 * StepCountUp gives 0, BRK until idle; HEAD_ANIM 6; TaskCreate slot 1
 * BrightnessDownTask 4; BRK until SCENE_WAIT 0; TaskExec.
 * Entry: M 8-bit, X 8-bit; D = the piece's record; DB a bank whose
 * $0000-$1FFF is work RAM and that holds the lists (bank $84); reached by
 * the actor loop's jump for the piece in state $8C (ChainPieceWork puts it
 * there with the head). Exit: never returns.
 */
void abi_ChainPieceGone(void)
{
    uint16_t head;
    uint8_t h;

    do {
        actor_yield(0x848AC2u);
        call_frame(0x8AC6, abi_ActorLookFlash);
        call_frame(0x8AC9, abi_ActorParityBitsSet);
        call_frame(0x8ACCu, abi_ActorsFollowIf8);
        a8_load(rd8(dba(HEAD_HIT_FRAMES)));
    } while ((uint8_t)cpu.a);
    op_rep(0x10);
    head = rd16(dba(CHAIN_HEAD));
    cpu.x = head;
    nz16_of(head);
    h = rd8(dba((uint16_t)(head + HEAD_HEALTH)));
    a8_load(h);
    op_sep(0x10);
    if (h & 0x80) {
        fight_won();
        return;
    }
    abi_ChainPieceEnd();
}

/*
 * @routine 84:8ADE ChainPieceEnd
 * @actor
 * WHAT IT DOES: A piece of the chain (or its head) goes back to what it
 * was doing before it was struck: its resume address from PIECE_RESUME,
 * its $08 bit 4 and $09 bit 7 set, and the actor loop goes on.
 * HOW: ACTOR_03/ACTOR_04 = PIECE_RESUME; ACTOR_08 OR $10; ACTOR_09 OR
 * $80; JML ActorLoopNext.
 * @unreachable 84:8CA1-8CA7 The PAL console's path: taken only with $005D
 * set, which RegionCheck (84:AFAE) does only on a PAL console (STAT78
 * bit 4); its TaskExec to 84:AFCC runs into a JSL to $37:460B at 84:AFFF,
 * data, so the game then runs garbage (Jeff, 2026-11-19: exempt).
 * Entry: M 8-bit, X 8-bit; D = the actor's record; reached from
 * ChainPieceGone and Kind73Hit, and by the actor loop's jump. Exit: never
 * returns (into the actor loop).
 */
void abi_ChainPieceEnd(void)
{
    uint8_t v;

    v = rd8(dpa(PIECE_RESUME));
    a8_load(v);
    wr8(dpa(ACTOR_03), v);
    v = rd8(dpa(PIECE_RESUME + 1));
    a8_load(v);
    wr8(dpa(ACTOR_04), v);
    v = (uint8_t)(rd8(dpa(ACTOR_08)) | 0x10);
    a8_load(v);
    wr8(dpa(ACTOR_08), v);
    v = (uint8_t)(rd8(dpa(ACTOR_09)) | 0x80);
    a8_load(v);
    wr8(dpa(ACTOR_09), v);
    actor_leave(0x8394D3u);                 /* JML ActorLoopNext */
}

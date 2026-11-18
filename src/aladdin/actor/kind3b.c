/*
 * kind3b.c: actor kind $3B, by its $07 a switch, a marker of a level
 * flag, or a platform that carries the player: across, up and down, or
 * along a track drawn in the map. Drafted by cdraft.py; named and
 * described by hand.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "actor.h"
#include "wram_player.h"

void abi_ActorCarryPlayer(void);
void abi_ActorFacePlayer(void);
void abi_ActorMoveX(void);
void abi_ActorMoveY(void);
void abi_ActorSpeedXSet(void);
void abi_ActorSpeedYSet(void);
void abi_ActorStepCarry(void);
void abi_ActorStepToTargetCarry(void);
void abi_ActorZoneHasTestPointLong(void);
void abi_AnimStart(void);
void abi_Kind3BPart0(void);
void abi_Kind3BPart1(void);
void abi_Kind3BPart2(void);
void abi_LevelFlagFromMask(void);
void abi_LevelFlagSpotFind(void);
void abi_MapQuarterAt(void);
void abi_PlayerSpotBelowActor(void);

#define K3B_LEVEL_FLAGS 0x7EF260u /* eight level flags (bytes); bit 7 set by actors */
#define K3B_PARTS 0xC058u         /* in bank $83: the platform parts' code (words) */
#define K3B_FRAMES 0x40u          /* frames left */
#define K3B_RUN 0x41u             /* frames of a run after the first */
#define K3B_TOGGLED 0x42u         /* the switch: nonzero once it toggled; the marker: the flag shown */
#define K3B_STATE 0x44u           /* the switch: 0 up, positive down, negative coming back up */
#define K3B_QUARTER 0x45u         /* the track: the quarter's high byte, then the direction after a spot */
#define K3B_TURN 0x46u            /* the track: added to the direction at each step to a spot */
#define K3B_REST_Y 0x4Au          /* the switch: its y pixels at rest (word) */
#define K3B_TARGET_X 0x1Au        /* the track: the spot's target (words) */
#define K3B_TARGET_Y 0x1Eu
#define K3B_AHEAD 0xD222u         /* in bank $80: by direction times 4, x and y pixels ahead (words) */
#define K3B_TURNS 0xD232u         /* in bank $80: the direction after a turn, by the quarter's bits */
#define K3B_SPOT_DIRS 0xD23Au     /* in bank $80: the direction out of a spot */
#define K3B_SPOT_TURNS 0xD242u    /* in bank $80: the turn at each step to a spot */
#define K3B_PROBE_X 0x000Cu       /* the point MapQuarterAt reads (words) */
#define K3B_PROBE_Y 0x000Eu
#define K3B_V_1427 0x1427u        /* zero: the switch comes back up */
#define K3B_PLAYER_FACING (PLAYER + ACTOR_FACING)
#define K3B_PLAYER_RESUME (PLAYER + ACTOR_03) /* the player's resume address (word) */
#define K3B_SWITCH_CODE 0xD9C5    /* where the switch sends the player's code */

/* The flag of $07's low 3 bits: X = it, A = its byte. */
static void level_flag(void)
{
    lda8(ea_dp(ACTOR_07));
    and8_v(0x07);
    tax8();
    lda8(ea_longx(K3B_LEVEL_FLAGS));
}

/* `n` frames, a BRK at `brk_at` each. */
static void rest(uint8_t n, uint32_t brk_at)
{
    lda8_v(n);
    sta8(ea_dp(K3B_FRAMES));
    do {
        actor_yield(brk_at);
        MEM8(dec, ea_dp(K3B_FRAMES));
    } while (!IF_Z);
}

/* 8 frames (BRK at brk_at first) of ActorStepCarry. */
static void step8(uint32_t brk_at)
{
    lda8_v(0x08);
    sta8(ea_dp(K3B_FRAMES));
    do {
        actor_yield(brk_at);
        abi_ActorStepCarry();
        MEM8(dec, ea_dp(K3B_FRAMES));
    } while (!IF_Z);
}

static void switch_part(void)
{
    lda8_v(0x00);
    abi_AnimStart();
    op_rep(P_M);
    lda16(ea_dp(ACTOR_Y + 1));
    sta16(ea_dp(K3B_REST_Y));
    op_sep(P_M);
    stz8(ea_dp(K3B_STATE));
    stz8(ea_dp(K3B_FRAMES));
    for (;;) {
        lda8(ea_dp(K3B_FRAMES));
        if (IF_Z)
            stz8(ea_dp(K3B_TOGGLED));
        actor_yield(0x83BFBAu);
        lda8(ea_dp(K3B_STATE));
        if (IF_Z) {                       /* up: the player comes into its zone */
            lda8(ea_abs(V_141C));
            if (IF_Z)
                continue;
            ldy8_v(0x70);
            abi_ActorZoneHasTestPointLong();
            if (IF_C)
                continue;
            op_rep(P_M);
            lda16_v(K3B_SWITCH_CODE);
            sta16(ea_abs(K3B_PLAYER_RESUME));
            op_sep(P_M);
            lda8_v(0x03);
            abi_PlayerSpotBelowActor();
        } else if (!IF_N) {               /* down */
            lda8(ea_abs(K3B_V_1427));
            if (IF_Z) {
                lda8_v(0xFC);
                sta8(ea_dp(K3B_STATE));
                continue;
            }
            lda8(ea_dp(K3B_TOGGLED));
            if (!IF_Z)
                continue;
            lda8(ea_dp(K3B_FRAMES));
            cmp8_v(0x0F);
            if (!IF_C)
                continue;
            level_flag();
            if (IF_N)
                continue;
            eor8_v(0x01);
            sta8(ea_longx(K3B_LEVEL_FLAGS));
            MEM8(inc, ea_dp(K3B_TOGGLED));
            continue;
        }
        MEM8(inc, ea_dp(K3B_STATE));
    }
}

static void marker_part(void)
{
    lda8_v(0xFF);
    sta8(ea_dp(K3B_TOGGLED));
    lda8(ea_dp(ACTOR_08));
    ora8_v(0x03);
    sta8(ea_dp(ACTOR_08));
    for (;;) {
        level_flag();
        if (IF_N) {
            lda8_v(0x10);
            actor_wait(0x83C036u);
            level_flag();
            and8_v(0x0F);
            sta8(ea_longx(K3B_LEVEL_FLAGS));
        } else {
            cmp8_v(rd8(ea_dp(K3B_TOGGLED)));
            if (!IF_Z) {
                sta8(ea_dp(K3B_TOGGLED));
                ACC8(inc);
                ACC8(inc);
                abi_AnimStart();
            }
        }
        actor_yield(0x83C019u);
    }
}

/*
 * @routine 83:BF83 Kind3BStart
 * @actor
 * WHAT IT DOES: Actor kind $3B, by its $07: with bit 4 a switch the player
 * works by coming into its zone (his code sent to $D9C5 and he is placed
 * under it; the level flag of its $07 low bits toggled when it is down);
 * with bit 3 a marker that shows that level flag (pose 2 plus the flag)
 * and clears it 16 frames after an actor marks it; else a platform the
 * player rides, one of three parts by its $07 low bits: Kind3BPart0,
 * Kind3BPart1, Kind3BPart2.
 * HOW: ACTOR_08 bit 1 and ACTOR_09 bits $90 set; facing, flip and
 * K3B_STATE 0. The switch: AnimStart 0; K3B_REST_Y = its y pixels;
 * K3B_FRAMES 0; each frame (BRK first; K3B_FRAMES 0 clears K3B_TOGGLED):
 * K3B_STATE 0: on when V_141C is set and ActorZoneHasTestPointLong (Y =
 * $70) says C clear: the player's resume address = $D9C5,
 * PlayerSpotBelowActor 3, K3B_STATE up. K3B_STATE negative: up by one.
 * Positive: V_1427 0 sets it to $FC; else with K3B_TOGGLED 0, K3B_FRAMES
 * at least $0F and the flag not negative, the flag'"'"'s bit 0 flipped and
 * K3B_TOGGLED up. The marker: K3B_TOGGLED $FF, ACTOR_08 bits 0-1 set; each
 * frame (BRK after the first): the flag negative: COP $10 and its high
 * bits cleared; else when it differs from K3B_TOGGLED, kept there and
 * AnimStart with it + 2. The platforms: AnimStart 4; JMP through
 * K3B_PARTS by ($07 times 2) AND 7. No script runs the switch past its
 * first frames: written from its bytes.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
 * the actor loop's jump for kind $3B in state $0C (the table at $83:8CAB).
 * Exit: never returns.
 */
void abi_Kind3BStart(void)
{
    lda8(ea_dp(ACTOR_08));
    ora8_v(0x02);
    sta8(ea_dp(ACTOR_08));
    lda8(ea_dp(ACTOR_09));
    ora8_v(0x90);
    sta8(ea_dp(ACTOR_09));
    stz8(ea_dp(ACTOR_FACING));
    stz8(ea_dp(ACTOR_FLIP));
    stz8(ea_dp(K3B_STATE));
    lda8(ea_dp(ACTOR_07));
    bit8_i(0x10);
    if (!IF_Z) {
        switch_part();
        return;
    }
    bit8_i(0x08);
    if (!IF_Z) {
        marker_part();
        return;
    }
    lda8_v(0x04);
    abi_AnimStart();
    lda8(ea_dp(ACTOR_07));
    ACC8(asl);
    and8_v(0x07);
    tax8();
    switch (cpu.x) {
    case 0: abi_Kind3BPart0(); break;
    case 2: abi_Kind3BPart1(); break;
    case 4: abi_Kind3BPart2(); break;
    default: jump_routine(0x830000u | rd16(0x830000u | (uint16_t)(K3B_PARTS + cpu.x))); break;
    }
}

/*
 * @routine 83:C05E Kind3BPart0
 * @actor
 * WHAT IT DOES: Kind $3B's platform part 0: after a frame it moves away
 * from the player'"'"'s facing for 32 frames carrying him, rests 12 frames,
 * turns, and again for 64 frames, back and forth for good.
 * HOW: BRK; facing = the player'"'"'s facing EOR 1; K3B_RUN = $40;
 * ActorSpeedXSet with entry $0C; K3B_FRAMES = $20; each frame (BRK first)
 * ActorMoveX and ActorCarryPlayer until K3B_FRAMES runs out; facing
 * flipped; 12 BRKs; K3B_FRAMES = K3B_RUN, again. No script runs past its
 * first BRK: written from its bytes.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80 (the speed
 * table's bank); reached by a jump from Kind3BStart. Exit: never returns.
 */
void abi_Kind3BPart0(void)
{
    actor_yield(0x83C05Eu);
    lda8(ea_abs(K3B_PLAYER_FACING));
    eor8_v(0x01);
    sta8(ea_dp(ACTOR_FACING));
    lda8_v(0x40);
    sta8(ea_dp(K3B_RUN));
    ldy8_v(0x0C);
    jsl_frame(0x83C06Du); abi_ActorSpeedXSet(); cpu.s = (uint16_t)(cpu.s + 3);
    lda8_v(0x20);
    sta8(ea_dp(K3B_FRAMES));
    for (;;) {
        do {
            actor_yield(0x83C075u);
            jsl_frame(0x83C077u); abi_ActorMoveX(); cpu.s = (uint16_t)(cpu.s + 3);
            call_frame(0xC07D, abi_ActorCarryPlayer);
            MEM8(dec, ea_dp(K3B_FRAMES));
        } while (!IF_Z);
        lda8(ea_dp(ACTOR_FACING));
        eor8_v(0x01);
        sta8(ea_dp(ACTOR_FACING));
        rest(0x0C, 0x83C08Cu);
        lda8(ea_dp(K3B_RUN));
        sta8(ea_dp(K3B_FRAMES));
    }
}

/*
 * @routine 83:C098 Kind3BPart1
 * @actor
 * WHAT IT DOES: Kind $3B's platform part 1: after a frame it moves up and
 * down, carrying the player: 32 frames one way first, then 64 frames each
 * way, resting 12 frames at each end.
 * HOW: BRK; K3B_RUN = $40; ActorSpeedYSet with entry $1E; K3B_FRAMES =
 * $20; each frame (BRK first) ActorMoveY and ActorCarryPlayer until
 * K3B_FRAMES runs out; the speed turned: its low word negated, its top
 * byte EOR $FF; 12 BRKs; K3B_FRAMES = K3B_RUN, again.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80 (the speed
 * table's bank); reached by a jump from Kind3BStart. Exit: never returns.
 */
void abi_Kind3BPart1(void)
{
    actor_yield(0x83C098u);
    lda8_v(0x40);
    sta8(ea_dp(K3B_RUN));
    ldy8_v(0x1E);
    jsl_frame(0x83C0A0u); abi_ActorSpeedYSet(); cpu.s = (uint16_t)(cpu.s + 3);
    lda8_v(0x20);
    sta8(ea_dp(K3B_FRAMES));
    for (;;) {
        do {
            actor_yield(0x83C0A8u);
            jsl_frame(0x83C0AAu); abi_ActorMoveY(); cpu.s = (uint16_t)(cpu.s + 3);
            call_frame(0xC0B0, abi_ActorCarryPlayer);
            MEM8(dec, ea_dp(K3B_FRAMES));
        } while (!IF_Z);
        op_rep(P_M);
        lda16(ea_dp(ACTOR_SPEED_Y));
        eor16_v(0xFFFF);
        ACC16(inc);
        sta16(ea_dp(ACTOR_SPEED_Y));
        op_sep(P_M);
        lda8(ea_dp(ACTOR_SPEED_Y + 2));
        eor8_v(0xFF);
        sta8(ea_dp(ACTOR_SPEED_Y + 2));
        rest(0x0C, 0x83C0CBu);
        lda8(ea_dp(K3B_RUN));
        sta8(ea_dp(K3B_FRAMES));
    }
}

/*
 * @routine 83:C0D7 Kind3BPart2
 * @actor
 * WHAT IT DOES: Kind $3B's platform part 2: follows a track drawn in the
 * map, carrying the player: at each step it reads the map quarter ahead
 * in its direction and by its low bits goes on 8 steps, goes on and turns
 * back, goes on and turns by the quarter'"'"'s high bits, waits a frame, or
 * at a flagged spot marks the level flag there and travels to the spot'"'"'s
 * target in 5 steps of three frames, turning as it goes.
 * HOW: ActorFacePlayer; the direction ($14) doubled. Each step: $000C and
 * $000E = its x and y pixels plus the word pair of K3B_AHEAD by the
 * direction times 4; MapQuarterAt; its low 3 bits times 2 pick the case
 * (the table at $83:C103), its high byte kept in K3B_QUARTER. 0, 1: 8
 * frames (BRK first) of ActorStepCarry. 4, 5: the same, the direction EOR
 * 2, 8 BRKs. 2: the same 8 steps, then the direction from K3B_TURNS by
 * K3B_QUARTER'"'"'s top two bits and the direction'"'"'s bit 0 (shifted out), 8
 * BRKs. 3: K3B_TARGET_X and _Y = $000C, $000E until LevelFlagSpotFind
 * finds the spot (C clear); the flag'"'"'s bit 7 set (LevelFlagFromMask, ORA
 * $80); by its bit 0 (the flag'"'"'s old value'"'"'s bit 1) and the direction:
 * K3B_TURN from K3B_SPOT_TURNS, K3B_QUARTER from K3B_SPOT_DIRS, the target
 * moved by K3B_AHEAD by that direction times 4; the direction times 4; 5
 * times: three frames (BRK first) of ActorStepToTargetCarry, then the
 * direction plus K3B_TURN, AND $0F; the x and y fractions cleared; the
 * direction = K3B_QUARTER. 6, 7: a BRK. Then the next step. No script
 * runs cases 6 and 7: written from their bytes.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by a
 * jump from Kind3BStart. Exit: never returns.
 */
void abi_Kind3BPart2(void)
{
    call_frame(0xC0D9, abi_ActorFacePlayer);
    MEM8(asl, ea_dp(ACTOR_FACING));
    for (;;) {
        /* the map quarter ahead */
        lda8(ea_dp(ACTOR_FACING));
        ACC8(asl);
        ACC8(asl);
        tax8();
        op_rep(P_M);
        clc();
        lda16(ea_absx(K3B_AHEAD));
        adc16_v(rd16(ea_dp(ACTOR_X + 1)));
        sta16(ea_abs(K3B_PROBE_X));
        clc();
        lda16(ea_absx(K3B_AHEAD + 2));
        adc16_v(rd16(ea_dp(ACTOR_Y + 1)));
        sta16(ea_abs(K3B_PROBE_Y));
        jsl_frame(0x83C0F5u); abi_MapQuarterAt(); cpu.s = (uint16_t)(cpu.s + 3);
        and8_v(0x07);
        ACC8(asl);
        tax8();
        xba();
        sta8(ea_dp(K3B_QUARTER));
        switch (cpu.x) {
        case 0: case 2:                   /* on */
            step8(0x83C11Cu);
            break;
        case 8: case 10:                  /* on, then back */
            step8(0x83C12Cu);
            lda8(ea_dp(ACTOR_FACING));
            eor8_v(0x02);
            sta8(ea_dp(ACTOR_FACING));
            rest(0x08, 0x83C13Fu);
            break;
        case 4:                           /* on, then a turn */
            step8(0x83C14Cu);
            lda8_v(0x00);
            MEM8(asl, ea_dp(K3B_QUARTER));
            ACC8(rol);
            MEM8(asl, ea_dp(K3B_QUARTER));
            ACC8(rol);
            MEM8(lsr, ea_dp(ACTOR_FACING));
            ACC8(rol);
            tax8();
            lda8(ea_absx(K3B_TURNS));
            sta8(ea_dp(ACTOR_FACING));
            rest(0x08, 0x83C16Au);
            break;
        case 6:                           /* a flagged spot */
            do {
                op_rep(P_M);
                lda16(ea_abs(K3B_PROBE_X));
                sta16(ea_dp(K3B_TARGET_X));
                lda16(ea_abs(K3B_PROBE_Y));
                sta16(ea_dp(K3B_TARGET_Y));
                op_sep(P_M);
                call_frame(0xC183, abi_LevelFlagSpotFind);
            } while (IF_C);
            lda8(ea_longx(K3B_LEVEL_FLAGS));
            pha8();
            call_frame(0xC18D, abi_LevelFlagFromMask);
            ora8_v(0x80);
            sta8(ea_longx(K3B_LEVEL_FLAGS));
            pla8();
            and8_v(0x0F);
            ACC8(lsr);
            lda8(ea_dp(ACTOR_FACING));
            ACC8(rol);
            tax8();
            lda8(ea_absx(K3B_SPOT_TURNS));
            sta8(ea_dp(K3B_TURN));
            lda8(ea_absx(K3B_SPOT_DIRS));
            sta8(ea_dp(K3B_QUARTER));
            ACC8(asl);
            ACC8(asl);
            tax8();
            op_rep(P_M);
            clc();
            lda16(ea_absx(K3B_AHEAD));
            adc16_v(rd16(ea_dp(K3B_TARGET_X)));
            sta16(ea_dp(K3B_TARGET_X));
            clc();
            lda16(ea_absx(K3B_AHEAD + 2));
            adc16_v(rd16(ea_dp(K3B_TARGET_Y)));
            sta16(ea_dp(K3B_TARGET_Y));
            op_sep(P_M);
            MEM8(asl, ea_dp(ACTOR_FACING));
            MEM8(asl, ea_dp(ACTOR_FACING));
            lda8_v(0x05);
            sta8(ea_dp(K3B_FRAMES));
            do {
                actor_yield(0x83C1C5u);
                call_frame(0xC1C9, abi_ActorStepToTargetCarry);
                actor_yield(0x83C1CAu);
                call_frame(0xC1CE, abi_ActorStepToTargetCarry);
                actor_yield(0x83C1CFu);
                call_frame(0xC1D3, abi_ActorStepToTargetCarry);
                clc();
                lda8(ea_dp(ACTOR_FACING));
                adc8_v(rd8(ea_dp(K3B_TURN)));
                and8_v(0x0F);
                sta8(ea_dp(ACTOR_FACING));
                MEM8(dec, ea_dp(K3B_FRAMES));
            } while (!IF_Z);
            stz8(ea_dp(ACTOR_X));
            stz8(ea_dp(ACTOR_Y));
            lda8(ea_dp(K3B_QUARTER));
            sta8(ea_dp(ACTOR_FACING));
            break;
        case 12: case 14:                 /* a frame */
            actor_yield(0x83C113u);
            break;
        default:
            jump_routine(0x830000u | rd16(0x830000u | (uint16_t)(0xC103u + cpu.x)));
            return;
        }
    }
}

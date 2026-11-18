/*
 * kind20.c: actor kind $20, a guard that patrols, stops to look about,
 * and charges when it sees Aladdin.
 *
 * It walks its beat ahead of it (one pixel a frame) until the floor
 * ends, a wall ($70) is ahead or it has gone its range; then stands,
 * looks about (turning every 17 frames for 64) and walks on. Each frame
 * it may see Aladdin (ActorSeesPlayer): then it braces and charges,
 * speeding up and turning to keep after him, until the floor ahead ends;
 * then it halts and looks about again. Its spotting first writes to
 * cartridge RAM at $70:0023: a cartridge that keeps the byte (a copier)
 * hangs the game there. Drafted by cdraft.py; named, completed and
 * described by hand.
 */
#include "machine.h"
#include "ops65.h"
#include "wram.h"
#include "actor.h"

void abi_Actor44SetBy07(void);
void abi_Actor44Set40And80(void);
void abi_ActorDecelerateX(void);
void abi_ActorFly(void);
void abi_ActorOutOfGround(void);
void abi_ActorMapAhead7Down28(void);
void abi_ActorMapAhead7Down36(void);
void abi_ActorMapAtOffset(void);
void abi_ActorMoveX(void);
void abi_ActorOutOfWallProbe4(void);
void abi_ActorSeesPlayer(void);
void abi_ActorSideOfPlayer(void);
void abi_ActorSnapToGround(void);
void abi_ActorSpeedXSet1(void);
void abi_ActorTileAheadIs70(void);
void abi_ActorXTo40(void);
void abi_Kind20Patrol(void);
void abi_AnimStart(void);
void abi_SoundQueueAdd(void);

#define KIND20_16 0x16u            /* word: $D05A */
#define KIND20_29 0x29u            /* word: $890F */
#define PLACE 0x07u                /* its placement's byte: bit 0 its facing, bit 3 the faster charge, bit 4 no patrol */
#define BEAT_FROM 0x40u            /* its beat: where it starts (word) ... */
#define BEAT_LENGTH 0x46u          /* ... and how far it walks (word) */
#define COUNT 0x42u                /* frames left of a step */
#define TURNS 0x43u                /* frames to the next turn while it looks about; while charging, the last surface's sound */
#define ACTOR_X_PIXELS 0x0Cu       /* its x, whole pixels (word) */
#define SPED_UP 0x4Eu              /* nonzero once its charge is up to speed */
#define CHARGE_FRAMES 0x4Cu        /* counts its charge's frames */
#define SURFACE 0x27u              /* the class of the ground under it */
#define V_0013 0x0013u             /* $0F: the map test's last block was a ledge */
#define COPIER_TEST 0x700023u      /* cartridge RAM, which a retail cartridge has none of */
#define NO_PATROL 0x839AC9u        /* its code for bit 4 of PLACE, which no script reaches */

/* How a charging frame stopped: 0 by a branch into Kind20Halt (or running
   on into it), HALT_BY_JMP by a JMP to it. */
#define HALT_BY_JMP 2

void abi_Kind20Halt(void);

/* From 83:998C: while charging,
   the charge's sound on surfaces 9 and $0C, then: slowing down when
   COUNT is out; else on, turned to face the player again within 16
   frames. Returns 1 to go on charging (the JMP back to 83:9900),
   HALT_BY_JMP when the charge stops (its JMP to Kind20Halt). */
static int charge_on(void)
{
    lda8(ea_dp(SURFACE));
    cmp8_v(0x09);
    if (IF_Z || (cmp8_v(0x0C), IF_Z)) {
        cmp8_v(rd8(ea_dp(TURNS)));
        if (!IF_Z) {
            sta8(ea_dp(TURNS));
            lda8_v(0x13);
            abi_SoundQueueAdd();
        }
    }
    lda8(ea_dp(COUNT));
    if (!IF_Z) {
        MEM8(dec, ea_dp(COUNT));
        abi_ActorSideOfPlayer();
        cmp8_v(rd8(ea_dp(ACTOR_FACING)));
        if (IF_Z)
            return 1;
        lda8(ea_dp(COUNT));
        cmp8_v(0x10);
        if (!IF_C)
            return 1;
        lda8_v(0x10);
        sta8(ea_dp(COUNT));
        return 1;
    }
    abi_ActorDecelerateX();
    lda8(ea_dp(SPED_UP));
    if (IF_Z) {
        op_rep(0x20);
        lda16(ea_dp(ACTOR_SPEED_X));
        cmp16_v(0x0100);
        op_sep(0x20);
        if (!IF_C) {
            MEM8(inc, ea_dp(SPED_UP));
            lda8_v(0x02);
            abi_AnimStart();
        }
    }
    MEM8(inc, ea_dp(CHARGE_FRAMES));
    lda8(ea_dp(ACTOR_SPEED_X));
    if (!IF_Z)
        return 1;
    lda8(ea_dp(ACTOR_SPEED_X + 1));
    if (!IF_Z)
        return 1;
    return HALT_BY_JMP;
}

/* From 83:997C (no script runs it): in the air, a frame at a time
   (BRK), until it lands; then on as charge_on. */
static int airborne(void)
{
    do {
        actor_yield(0x83997Cu);
        abi_ActorFly();
        abi_ActorOutOfWallProbe4();
        abi_ActorOutOfGround();
    } while (IF_Z);
    return charge_on();
}

/* From 83:9906: a charging frame's ground and probes; 1 to go on
   charging, 0 when it stops by a branch into Kind20Halt (or running on
   into it at 83:9956), HALT_BY_JMP by the JMP to it; the caller goes on
   into Kind20Halt (halt_after). */
static int charge_frame(void)
{
    abi_ActorSnapToGround();
    lda8(ea_abs(V_0013));
    cmp8_v(0x0F);
    if (IF_Z) {
        /* on a ledge (no script runs this): the lower probe instead */
        abi_ActorMapAhead7Down36();
        if (!IF_Z)
            goto ahead;
        goto beyond;
    }
    abi_ActorMapAhead7Down28();
    if (!IF_Z)
        goto ahead;
beyond:
    /* no floor just ahead: is there one further on? */
    stz8(ea_abs(SCRATCH));
    lda8_v(0x2B);
    sta8(ea_abs(SCRATCH + 2));
    stz8(ea_abs(SCRATCH + 3));
    abi_ActorMapAtOffset();
    if (IF_Z)
        return 0;
    /* there is (no script runs this): it runs off, falling */
    stz8(ea_dp(ACTOR_SPEED_Y));
    stz8(ea_dp(ACTOR_SPEED_Y + 1));
    stz8(ea_dp(ACTOR_SPEED_Y + 2));
    lda8_v(0x60);
    sta8(ea_dp(ACTOR_ACCEL_Y));
    return airborne();
ahead:
    abi_ActorTileAheadIs70();
    if (!IF_Z)
        return charge_on();
    /* a wall: only the faster guard, not yet up to speed, may leap it
       (no script runs these) */
    lda8(ea_dp(PLACE));
    and8_v(0x08);
    if (IF_Z)
        return 0;
    lda8(ea_dp(SPED_UP));
    if (!IF_Z)
        return 0;
    stz8(ea_abs(SCRATCH));
    stz8(ea_abs(SCRATCH + 2));
    stz8(ea_abs(SCRATCH + 3));
    abi_ActorMapAtOffset();
    if (!IF_Z)
        return 0;
    lda8_v(0x60);
    sta8(ea_dp(ACTOR_SPEED_Y));
    lda8_v(0xFE);
    sta8(ea_dp(ACTOR_SPEED_Y + 1));
    lda8_v(0xFF);
    sta8(ea_dp(ACTOR_SPEED_Y + 2));
    lda8_v(0x10);
    sta8(ea_dp(ACTOR_ACCEL_Y));
    return airborne();
}

/* On into Kind20Halt once a charging frame stopped: by the JMP
   (HALT_BY_JMP) as a jump to it, which opens its piece (@actor) as the
   original's does; by a branch, or running on at 83:9956, its C goes on
   here as the original's code does, with no jump. */
static void halt_after(int on)
{
    if (on == HALT_BY_JMP)
        jump_routine(0x839956u);
    else
        abi_Kind20Halt();
}

/*
 * @routine 83:97AF Kind20Start
 * @actor
 * WHAT IT DOES: Actor kind $20's start: a guard set up to walk its beat
 * the way its placement faces.
 * HOW: ACTOR_08 bit 1 and ACTOR_09 bits $90 set; KIND20_16 $D05A,
 * KIND20_29 $890F; with PLACE $1F (the ending's scene), AnimStart 0, a
 * 32-frame wait, then PLACE 3; with PLACE bit 4, on at NO_PATROL (no
 * script runs that); Actor44SetBy07, AnimStart 2, facing and flip PLACE
 * bit 0, ActorXTo40, ActorSpeedXSet1; on into Kind20Patrol at its BRK.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
 * $0000-$1FFF is work RAM; reached by the actor loop's jump for kind $20
 * (the table at $83:8CAB). Exit: never returns.
 */
void abi_Kind20Start(void)
{
    lda8_v(0x02);
    tsb8(ea_dp(ACTOR_08));
    lda8_v(0x90);
    tsb8(ea_dp(ACTOR_09));
    op_rep(0x20);
    lda16_v(0xD05A);
    sta16(ea_dp(KIND20_16));
    lda16_v(0x890F);
    sta16(ea_dp(KIND20_29));
    op_sep(0x20);
    lda8(ea_dp(PLACE));
    cmp8_v(0x1F);
    if (IF_Z) {
        lda8_v(0x00);
        abi_AnimStart();
        lda8_v(0x20);
        actor_wait(0x8397D3u);
        lda8_v(0x03);
        sta8(ea_dp(PLACE));
    }
    lda8(ea_dp(PLACE));
    bit8_i(0x10);
    if (!IF_Z) {
        jump_routine(NO_PATROL);
        return;
    }
    abi_Actor44SetBy07();
    lda8_v(0x02);
    abi_AnimStart();
    lda8(ea_dp(PLACE));
    and8_v(0x01);
    sta8(ea_dp(ACTOR_FACING));
    sta8(ea_dp(ACTOR_FLIP));
    abi_ActorXTo40();
    abi_ActorSpeedXSet1();
    abi_Kind20Patrol();         /* on into its BRK */
}

/*
 * @routine 83:97F9 Kind20Patrol
 * @actor
 * WHAT IT DOES: The guard walks its beat, a frame at a time, until it
 * sees Aladdin (Kind20Spotted) or must stop (Kind20LookAbout): the floor
 * ends ahead, a wall ($70) is ahead, or it has walked BEAT_LENGTH from
 * BEAT_FROM.
 * HOW: BRK each frame; ActorSeesPlayer C clear: Kind20Spotted. Else
 * ActorMoveX, ActorSnapToGround; on a ledge (V_0013 $0F, no script) the
 * lower probe first; ActorMapAhead7Down28 or ActorTileAheadIs70 Z, or
 * BEAT_FROM + ACTOR_44 - its x at least BEAT_LENGTH: Kind20LookAbout.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
 * $0000-$1FFF is work RAM; reached by the JMP from Kind20LookAbout's end
 * or from Kind20Start running on. Exit: never returns.
 */
void abi_Kind20Patrol(void)
{
L_patrol:
    actor_yield(0x8397F9u);
    abi_ActorSeesPlayer();
    if (!IF_C) {
        jump_routine(0x839894u);
        return;
    }
    abi_ActorMoveX();
    abi_ActorSnapToGround();
    lda8(ea_abs(V_0013));
    cmp8_v(0x0F);
    if (IF_Z) {
        abi_ActorMapAhead7Down36();
        if (IF_Z) {
            jump_routine(0x839832u);
            return;
        }
        goto L_wall;
    }
    abi_ActorMapAhead7Down28();
    if (IF_Z) {
        jump_routine(0x839832u);
        return;
    }
L_wall:
    abi_ActorTileAheadIs70();
    if (IF_Z) {
        jump_routine(0x839832u);
        return;
    }
    op_rep(0x21);
    lda16(ea_dp(BEAT_FROM));
    adc16_v(rd16(ea_dp(ACTOR_44)));
    sec();
    sbc16_v(rd16(ea_dp(ACTOR_X_PIXELS)));
    cmp16_v(rd16(ea_dp(BEAT_LENGTH)));
    op_sep(0x20);
    if (!IF_C)
        goto L_patrol;
    jump_routine(0x839832u);
}

/*
 * @routine 83:9832 Kind20LookAbout
 * WHAT IT DOES: The guard stands and looks about: 16 frames still, 64
 * turning round every 17, 16 still, then walks on the way it last
 * looked; seeing Aladdin at any frame, Kind20Spotted.
 * HOW: ActorOutOfWallProbe4; AnimStart 0, COUNT 16 frames (BRK, then
 * ActorSeesPlayer C clear: Kind20Spotted); AnimStart 1, COUNT $40 and
 * TURNS $11, its facing flipped each time TURNS runs out; AnimStart 0,
 * 16 frames; AnimStart 2, its flip = its facing; Kind20Patrol.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
 * $0000-$1FFF is work RAM; reached by Kind20Patrol's jumps and by
 * Kind20Halt's. Exit: never returns.
 */
void abi_Kind20LookAbout(void)
{
    abi_ActorOutOfWallProbe4();
    lda8_v(0x00);
    abi_AnimStart();
    lda8_v(0x10);
    sta8(ea_dp(COUNT));
    do {
        actor_yield(0x839840u);
        abi_ActorSeesPlayer();
        if (!IF_C) {
            jump_routine(0x839894u);
            return;
        }
        MEM8(dec, ea_dp(COUNT));
    } while (!IF_Z);
    lda8_v(0x01);
    abi_AnimStart();
    lda8_v(0x40);
    sta8(ea_dp(COUNT));
    lda8_v(0x11);
    sta8(ea_dp(TURNS));
    do {
        actor_yield(0x839859u);
        abi_ActorSeesPlayer();
        if (!IF_C) {
            jump_routine(0x839894u);
            return;
        }
        MEM8(dec, ea_dp(TURNS));
        if (IF_Z) {
            lda8_v(0x11);
            sta8(ea_dp(TURNS));
            lda8(ea_dp(ACTOR_FACING));
            eor8_v(0x01);
            sta8(ea_dp(ACTOR_FACING));
        }
        MEM8(dec, ea_dp(COUNT));
    } while (!IF_Z);
    lda8_v(0x00);
    abi_AnimStart();
    lda8_v(0x10);
    sta8(ea_dp(COUNT));
    do {
        actor_yield(0x83987Cu);
        abi_ActorSeesPlayer();
        if (!IF_C) {
            jump_routine(0x839894u);
            return;
        }
        MEM8(dec, ea_dp(COUNT));
    } while (!IF_Z);
    lda8_v(0x02);
    abi_AnimStart();
    lda8(ea_dp(ACTOR_FACING));
    sta8(ea_dp(ACTOR_FLIP));
    jump_routine(0x8397F9u);
}

/*
 * @routine 83:9894 Kind20Spotted
 * WHAT IT DOES: The guard has seen Aladdin: unless the floor ahead ends,
 * it braces (12 frames, then 4) and charges, at $20 a frame's speed
 * ($80 with PLACE bit 3, no script), up to 128 frames. First it writes
 * $5A to COPIER_TEST: if that reads back (cartridge RAM, a copier), $43
 * too, and if that also reads back the game hangs (no script runs it).
 * HOW: The copier test; ActorTileAheadIs70 Z: Kind20Halt (no script);
 * ActorMapAhead7Down28 Z and ActorMapAtOffset (0, $2B) Z: Kind20Halt.
 * Else AnimStart 0, COP 12, AnimStart 3, COP 4; speed ($19) $20 or $80,
 * $1A 1, $1B 0, ACTOR_ACCEL_X 8, COUNT $80, SPED_UP 0; on into
 * Kind20Charge past its BRK and move.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
 * $0000-$1FFF is work RAM; reached by the jumps of Kind20Patrol and
 * Kind20LookAbout. Exit: never returns.
 */
void abi_Kind20Spotted(void)
{
    int on;

    lda8_v(0x5A);
    sta8(COPIER_TEST);
    cmp8_v(rd8(COPIER_TEST));
    if (IF_Z) {
        lda8_v(0x43);
        sta8(COPIER_TEST);
        cmp8_v(rd8(COPIER_TEST));
        if (IF_Z) {
            cpu_loop_forever(0x8398ACu);
            return;
        }
    }
    abi_ActorTileAheadIs70();
    if (IF_Z) {
        jump_routine(0x839956u);
        return;
    }
    abi_ActorMapAhead7Down28();
    if (IF_Z) {
        stz8(ea_abs(SCRATCH));
        lda8_v(0x2B);
        sta8(ea_abs(SCRATCH + 2));
        stz8(ea_abs(SCRATCH + 3));
        abi_ActorMapAtOffset();
        if (IF_Z) {
            jump_routine(0x839956u);
            return;
        }
    }
    lda8_v(0x00);
    abi_AnimStart();
    lda8_v(0x0C);
    actor_wait(0x8398D6u);
    lda8_v(0x03);
    abi_AnimStart();
    lda8_v(0x04);
    actor_wait(0x8398E0u);
    ldy8_v(0x20);
    lda8(ea_dp(PLACE));
    and8_v(0x08);
    if (!IF_Z)
        ldy8_v(0x80);
    sty8(ea_dp(ACTOR_SPEED_X));
    lda8_v(0x01);
    sta8(ea_dp(ACTOR_SPEED_X + 1));
    stz8(ea_dp(ACTOR_SPEED_X + 2));
    lda8_v(0x08);
    sta8(ea_dp(ACTOR_ACCEL_X));
    lda8_v(0x80);
    sta8(ea_dp(COUNT));
    stz8(ea_dp(SPED_UP));
    /* on into Kind20Charge past its BRK and move; its JMP back to
       Kind20Charge goes on in this same piece of the actor */
    on = charge_frame();
    if (on == 1)
        jump_routine(0x839900u);
    else
        halt_after(on);
}

/*
 * @routine 83:9900 Kind20Charge
 * WHAT IT DOES: The guard charges: a frame at a time it runs on, slowing
 * once COUNT is out, turned again toward Aladdin within 16 frames when he
 * gets behind it, with a sound on some surfaces; it halts (Kind20Halt)
 * when the floor ahead ends or it has stopped.
 * HOW: BRK; ActorMoveX; ActorSnapToGround; probes as Kind20Patrol's
 * (ActorMapAhead7Down28 Z and ActorMapAtOffset (0, $2B) Z, or a wall
 * ahead without PLACE bit 3: Kind20Halt); on SURFACE 9 or $0C a new one,
 * sound $13; COUNT nonzero: counted down, ActorSideOfPlayer not its
 * facing with COUNT $10 or more makes it $10; COUNT 0: ActorDecelerateX,
 * AnimStart 2 once its speed is under $100, CHARGE_FRAMES counted, and
 * at speed 0, Kind20Halt. The paths over a further floor or a wall are
 * not run by a script.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
 * $0000-$1FFF is work RAM; reached by its own and Kind20Spotted's jumps.
 * Exit: never returns.
 */
void abi_Kind20Charge(void)
{
    int on;

    do {
        actor_yield(0x839900u);
        abi_ActorMoveX();
        on = charge_frame();
    } while (on == 1);
    halt_after(on);
}

/*
 * @routine 83:9956 Kind20Halt
 * @actor
 * WHAT IT DOES: The guard halts: stands 24 frames, then takes up its beat
 * again from here and looks about.
 * HOW: AnimStart 0, COP $18; ActorSpeedXSet1, ActorXTo40,
 * Actor44Set40And80; Kind20LookAbout.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
 * $0000-$1FFF is work RAM; reached by the JMPs of Kind20Spotted,
 * Kind20Charge and Kind20Dazed, Kind20Charge's branches and its running on
 * at 83:9956; never by a call. Exit: never returns.
 */
void abi_Kind20Halt(void)
{
    lda8_v(0x00);
    abi_AnimStart();
    lda8_v(0x18);
    actor_wait(0x83995Eu);
    abi_ActorSpeedXSet1();
    abi_ActorXTo40();
    abi_Actor44Set40And80();
    jump_routine(0x839832u);
}

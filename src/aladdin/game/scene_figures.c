/*
 * scene_figures.c: scripted figures: actor kind $93, which walks in once
 * the scene's cue $00C2 is given, the player started with $07 = 15,
 * who stands at (128, 176) for a scene, and the player started with $07
 * = 14, who rides the carpet across the ending, the player started with
 * $07 = 9, who comes down to the ground and waits for a cue, and actor
 * kind $72,
 * which rides along at the player's place in his moves until he takes
 * move 5, then flies off. Drafted by cdraft.py; named and
 * described by hand.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "wram.h"
#include "actor.h"
#include "actor_anim.h"
#include "wram_player.h"
#include "wram_video.h"

void abi_ActorMoveX(void);
void abi_ActorSpeedXSet(void);
void abi_AnimStart(void);
void abi_PlayerMoveStart(void);
void abi_PlayerMoveChange(void);
void abi_ActorBobY(void);
void abi_ActorSpeedYClear(void);
void abi_ActorCreateChild8(void);
void abi_ActorMotionPreset(void);
void abi_ActorFly(void);
void abi_ActorEndPlaced(void);
void abi_ActorSnapToGround(void);
void abi_ActorMoveY(void);
void abi_ActorOutOfGround(void);
void abi_TaskCreate(void);
void abi_TaskExec(void);

#define SCENE_CUE_C2 0x00C2u  /* counted up by a scripted actor when it has waited; this one waits for it */
#define FIGURE_29 0x29u  /* word: its picture list ($891F, $8907) */
#define V_145B 0x145Bu
#define V_1430 0x1430u
/* The ride's screens: the play field's x screen (its scroll x's high
   byte), which it waits on. */
#define PLAY_SCREEN (LAYER0 + LAYER_SCROLL_X + 1)
#define RIDE_TIMER ACTOR_40   /* frames of its first stretch */
#define RIDE_COUNT ACTOR_46   /* frames of its third stretch (word) */
#define RIDE_41 ACTOR_41      /* 1 through the third stretch, else 0 */
#define VIEW_X 0x1773u        /* the view's left edge (pixels, word) */
#define VIEW_SCREEN_Y 0x1778u /* the view's top edge's high byte: its row of screens */
#define V_1A10 0x1A10u
#define V_1A12 0x1A12u
#define V_1B14 0x1B14u
void abi_PlayerEndingRideOn(void);
void abi_ActorAccelerateX(void);
void abi_ActorStopY(void);

/* The player started with $07 = 9 */
#define CUE_BE 0x0000BEu      /* the camera trigger task's second cue: cleared as it starts */
/* Not yet filed (no script reaches it): the player's part after the cue. */
#define SCENE_STEP_A 0x00BDu    /* nonzero: the walk-off figure goes on */
#define V_035D 0x035Du
#define V_147B 0x147Bu
#define V_036C 0x036Cu          /* nonzero: on at PlayerSceneFigure, not to the next part */
void abi_PlayerSceneFigure(void);
#define TASK2_STATE 0x009Fu     /* task slot 2's state: 0 when free */

/* Kind $72's own fields and tables */
#define ALONG_16 0x16u        /* word: $D6B5 */
#define ALONG_MOVE 0x48u      /* the player's move it last took up */
#define ALONG_08_BY_41 0xD6B3u /* in the bank DB names: its $08 by the player's $41 */
#define ALONG_LEAVE_MOVE 5    /* the player's move at which it flies off */
#define KIND_7A 0x7A          /* the child it starts as it flies off */
#define ALONG_FLY_MOTION 0x54 /* the motion preset it flies off on */

/* A JSL at `site` to fn, its return address on the stack while fn runs, as
   the original's (frame_step.c's model). */
static void jsl_call(uint32_t site, void (*fn)(void))
{
    jsl_frame(site);
    fn();
    cpu.s = (uint16_t)(cpu.s + 3);
}

/*
 * @routine A5:F3F0 Kind93WalkIn
 * @actor
 * WHAT IT DOES: Actor kind $93: stands facing left until the scene's cue
 * (SCENE_CUE_C2), then walks left until its x is below 162, takes its
 * second pose and stays.
 * HOW: $09 bits $90 set, $2C = 2, redraw marked, $29 = $891F, facing and
 * flip 1, animation 0; BRK until SCENE_CUE_C2; ActorSpeedXSet $72; each
 * frame ActorMoveX while its x pixels' low byte is 162 or more; animation
 * 1; BRK for good.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $A5; reached by
 * the actor loop's jump for kind $93 in state $0C (through the stub at
 * $83:9389). Exit: never returns.
 */
void abi_Kind93WalkIn(void)
{
    uint8_t v;

    a8_load(0x90);
    tsb_at(dpa(ACTOR_09), 0x90);
    a8_load(0x02);
    wr8(dpa(ACTOR_2C), 0x02);
    a8_load(0xFF);
    wr8(dpa(ACTOR_REDRAW), 0xFF);
    dp16_set(FIGURE_29, 0x891F);
    cpu.a = 0x891F;
    a8_load(0x01);
    wr8(dpa(ACTOR_FACING), 0x01);
    wr8(dpa(ACTOR_FLIP), 0x01);
    a8_load(0x00);
    jsl_call(0xA5F40Du, abi_AnimStart);
    do {                                  /* until the scene's cue */
        actor_yield(0xA5F411u);
        v = rd8(dba(SCENE_CUE_C2));
        a8_load(v);
    } while (!v);
    y8_load(0x72);
    jsl_call(0xA5F41Au, abi_ActorSpeedXSet);
    do {                                  /* left to x 162 */
        actor_yield(0xA5F41Eu);
        jsl_call(0xA5F420u, abi_ActorMoveX);
        v = rd8(dpa(ACTOR_X + 1));
        a8_load(v);
        cmp8_of(v, 0xA2);
    } while (v >= 0xA2);
    a8_load(0x01);
    jsl_call(0xA5F42Cu, abi_AnimStart);
    for (;;)
        actor_yield(0xA5F430u);
}


/*
 * @routine 81:E23E PlayerSceneStand
 * @actor
 * WHAT IT DOES: The player started with $07 = 15: Aladdin set at (128,
 * 176) facing right for a scene, standing; after $20 frames V_142D is
 * counted up, and he stays.
 * HOW: $29 = $8907, x 128, y 176; V_145B = 1; facing and flip 0;
 * PlayerMoveStart 8; V_1430 up; $09 = $D0, redraw marked, $06 cleared,
 * TASK_PARAM cleared; COP $20; V_142D up; BRK for good.
 * Entry: M 8-bit, X 8-bit; D = the player's record; DB = $81; reached by
 * the jump through the player's start table by $07 at $81:CB61 (81:CB56).
 * Exit: never returns.
 */
void abi_PlayerSceneStand(void)
{
    dp16_set(FIGURE_29, 0x8907);
    dp16_set(ACTOR_X + 1, 0x0080);
    dp16_set(ACTOR_Y + 1, 0x00B0);
    cpu.a = 0x00B0;
    a8_load(0x01);
    wr8(dba(V_145B), 0x01);
    wr8(dpa(ACTOR_FACING), 0);
    wr8(dpa(ACTOR_FLIP), 0);
    a8_load(0x08);
    call_frame(0xE25E, abi_PlayerMoveStart);
    inc8_at(dba(V_1430));
    a8_load(0xD0);
    wr8(dpa(ACTOR_09), 0xD0);
    a8_load(0xFF);
    wr8(dpa(ACTOR_REDRAW), 0xFF);
    wr8(dpa(ACTOR_06), 0);
    wr8(dba(TASK_PARAM), 0);
    a8_load(0x20);
    actor_wait(0x81E271u);
    inc8_at(dba(V_142D));
    for (;;)
        actor_yield(0x81E276u);
}


/* Each frame of the ride (BRK first): bobbing, moving, until the play
   field's screen is `screen`. */
static void ride_bobbing_to(uint32_t brk_at, uint8_t screen)
{
    uint8_t now;

    do {
        actor_yield(brk_at);
        abi_ActorBobY();
        abi_ActorMoveX();
        now = rd8(dba(PLAY_SCREEN));
        a8_load(now);
        cmp8_of(now, screen);
    } while (now != screen);
}

/*
 * @routine 81:E27A PlayerEndingRide
 * @actor
 * WHAT IT DOES: The player started with $07 = 14: Aladdin on the carpet
 * across the ending. He flies right at 2 pixels a frame for $57 frames,
 * then bobs up and down at 3/4 of a pixel a frame as the play field
 * scrolls: in move 0 to screen 5, move 3 for $1B0 frames, move 4 to
 * screen 12, and on (PlayerEndingRideOn).
 * HOW: $09 = $D0, $0A = $40, picture list $8917, redraw marked; flip 0;
 * speed $0200; RIDE_41 0; PlayerMoveStart 0; PLAYER_MOVE 0 again. Each
 * frame (BRK first) ActorMoveX, $57 times. Speed $00C0, ACTOR_49 1 (the
 * bob going up), ActorSpeedYClear, ACTOR_ACCEL_Y 1, PlayerMoveChange 0;
 * each frame ActorBobY and ActorMoveX until PLAY_SCREEN is 5. RIDE_41 1,
 * PlayerMoveChange 3, RIDE_COUNT $1B0: each frame ACTOR_49 cleared,
 * ActorBobY and ActorMoveX, until the count is down to 0. RIDE_41 0,
 * PlayerMoveChange 4: each frame ActorBobY and ActorMoveX until
 * PLAY_SCREEN is 12; then on at PlayerEndingRideOn (it falls into it).
 * Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; reached by
 * the jump through the player's start table by $07 at $81:CB61 (81:CB56).
 * Exit: never returns.
 */
void abi_PlayerEndingRide(void)
{
    uint8_t frames;
    uint16_t count;

    dp16_set(ACTOR_09, 0x40D0);
    dp16_set(FIGURE_29, 0x8917);
    cpu.a = 0x8917;
    wr8(dpa(ACTOR_REDRAW), 0xFF);
    wr8(dpa(ACTOR_FLIP), 0);
    /* right at 2 pixels a frame for $57 frames */
    wr8(dpa(ACTOR_SPEED_X), 0x00);
    wr8(dpa(ACTOR_SPEED_X + 1), 0x02);
    wr8(dpa(ACTOR_SPEED_X + 2), 0x00);
    wr8(dpa(RIDE_41), 0);
    a8_load(0x00);
    call_frame(0xE29C, abi_PlayerMoveStart);
    wr8(dba(PLAYER_MOVE), 0);
    frames = 0x57;
    a8_load(frames);
    wr8(dpa(RIDE_TIMER), frames);
    do {
        actor_yield(0x81E2A4u);
        abi_ActorMoveX();
        frames = (uint8_t)(rd8(dpa(RIDE_TIMER)) - 1);
        wr8(dpa(RIDE_TIMER), frames);
        nz8_of(frames);
    } while (frames);
    /* then 3/4 of a pixel a frame, bobbing, in move 0 to screen 5 */
    wr8(dpa(ACTOR_SPEED_X), 0xC0);
    wr8(dpa(ACTOR_SPEED_X + 1), 0x00);
    wr8(dpa(ACTOR_49), 1);
    a8_load(0x01);
    jsl_frame(0x81E2B8u);
    abi_ActorSpeedYClear();
    cpu.s = (uint16_t)(cpu.s + 3);
    a8_load(0x01);
    wr8(dpa(ACTOR_ACCEL_Y), 1);
    wr8(dpa(RIDE_41), 0);
    a8_load(0x00);
    abi_PlayerMoveChange();
    ride_bobbing_to(0x81E2C7u, 0x05);
    /* move 3 for $1B0 frames, the bob held going down */
    wr8(dpa(RIDE_41), 1);
    a8_load(0x03);
    abi_PlayerMoveChange();
    a8_load(0x01);
    dp16_set(RIDE_COUNT, 0x01B0);
    do {
        actor_yield(0x81E2E9u);
        wr8(dpa(ACTOR_49), 0);
        abi_ActorBobY();
        abi_ActorMoveX();
        count = (uint16_t)(rd16(dpa(RIDE_COUNT)) - 1);
        wr16(dpa(RIDE_COUNT), count);
        nz16_of(count);
    } while (count);
    /* move 4 to screen 12, and on */
    wr8(dpa(RIDE_41), 0);
    a8_load(0x04);
    abi_PlayerMoveChange();
    ride_bobbing_to(0x81E304u, 0x0C);
    abi_PlayerEndingRideOn();
}

/*
 * @routine 83:F40D Kind72RideAlong
 * @actor
 * WHAT IT DOES: Actor kind $72: rides along at the player's place, taking
 * his move as its animation, until he takes move 5; then it faces right,
 * starts a child of kind $7A, and flies off on motion $54 until it is
 * off the screen, and ends.
 * HOW: ACTOR_08 bit 0 and ACTOR_09 bits $90 set; ALONG_16 = $D6B5; $29 =
 * $890D; redraw marked; animation 0; ALONG_MOVE 0. Each frame (BRK
 * first): its x and y pixels the player's; with PLAYER_MOVE unlike
 * ALONG_MOVE: move 5 leaves the loop, else ALONG_MOVE and the animation
 * take it, and ACTOR_08 = ALONG_08_BY_41 by the player's $41. Leaving:
 * facing, flip and $21 cleared; ActorCreateChild8 $7A; animation 5;
 * ActorMotionPreset $54; each frame (BRK first) ActorFly while ACTOR_09
 * bit 6 is set; then ActorEndPlaced (JML, 83:F483; run by
 * cov_kind72_flight_end, the flier put out of view).
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $83; reached by
 * the actor loop's jump for kind $72 in state $0C (the table at
 * $83:8CAB). Exit: never returns.
 */
void abi_Kind72RideAlong(void)
{
    uint8_t move;

    wr8(dpa(ACTOR_08), (uint8_t)(rd8(dpa(ACTOR_08)) | 0x01));
    wr8(dpa(ACTOR_09), (uint8_t)(rd8(dpa(ACTOR_09)) | 0x90));
    dp16_set(ALONG_16, 0xD6B5);
    dp16_set(FIGURE_29, 0x890D);
    wr8(dpa(ACTOR_REDRAW), 0xFF);
    cpu.a = 0x8900;
    nz8_of(0x00);
    jsl_frame(0x83F42Du); abi_AnimStart(); cpu.s = (uint16_t)(cpu.s + 3);
    wr8(dpa(ALONG_MOVE), 0);
    /* at his place each frame; his move, when it changes, its animation */
    for (;;) {
        uint16_t y;

        actor_yield(0x83F433u);
        dp16_set(ACTOR_X + 1, rd16(dba(PLAYER + ACTOR_X + 1)));
        y = rd16(dba(PLAYER + ACTOR_Y + 1));
        dp16_set(ACTOR_Y + 1, y);
        move = rd8(dba(PLAYER_MOVE));
        cpu.a = (uint16_t)((y & 0xFF00) | move);
        cmp8_of(move, rd8(dpa(ALONG_MOVE)));
        if (move == rd8(dpa(ALONG_MOVE)))
            continue;
        cmp8_of(move, ALONG_LEAVE_MOVE);
        if (move == ALONG_LEAVE_MOVE)
            break;
        wr8(dpa(ALONG_MOVE), move);
        jsl_frame(0x83F451u); abi_AnimStart(); cpu.s = (uint16_t)(cpu.s + 3);
        x8_load(rd8(dba(PLAYER + ACTOR_41)));
        uint8_t bits = rd8(dba((uint16_t)(ALONG_08_BY_41 + cpu.x)));
        a8_load(bits);
        wr8(dpa(ACTOR_08), bits);
    }
    /* he takes move 5: it faces right and flies off with a kind $7A */
    wr8(dpa(ACTOR_FACING), 0);
    wr8(dpa(ACTOR_FLIP), 0);
    wr8(dpa(ACTOR_SPAWN_AT), 0);
    a8_load(KIND_7A);
    jsl_frame(0x83F467u);
    abi_ActorCreateChild8();
    cpu.s = (uint16_t)(cpu.s + 3);
    a8_load(0x05);
    jsl_frame(0x83F46Du); abi_AnimStart(); cpu.s = (uint16_t)(cpu.s + 3);
    y8_load(ALONG_FLY_MOTION);
    jsl_frame(0x83F473u);
    abi_ActorMotionPreset();
    cpu.s = (uint16_t)(cpu.s + 3);
    for (;;) {
        actor_yield(0x83F477u);
        jsl_frame(0x83F479u);
        abi_ActorFly();
        cpu.s = (uint16_t)(cpu.s + 3);
        uint8_t flags = rd8(dpa(ACTOR_09));
        a8_load(flags);
        flag_set(P_Z, !(flags & 0x40));
        if (!(flags & 0x40))
            break;
    }
    abi_ActorEndPlaced();
}

/*
 * @routine 81:DFBB PlayerSceneLand
 * @actor
 * WHAT IT DOES: The player started with $07 = 9: Aladdin faces right and,
 * unless there is ground just under him, falls 3 pixels a frame in move
 * $0E until he is out of the ground; then he stands (move 0) and waits
 * for the cue CUE_BE.
 * HOW: Facing and flip 0; ActorSnapToGround: on ground (Z clear) on to
 * standing. Else PlayerMoveStart $0E, vertical speed $0300 (3 pixels),
 * each frame (BRK first) ActorMoveY and ActorOutOfGround until it leaves
 * Z clear. Standing: V_145B 4, PlayerMoveStart 0; each frame (BRK first)
 * until CUE_BE is nonzero. Cued ($81:DFEF): the flip up for $80 frames
 * (COP) and back for $80, SCENE_STEP_A up, PlayerMoveStart 3, speed across
 * $0200 and $80 frames of ActorMoveX; V_035D and V_147B up; V_036C nonzero:
 * on at PlayerSceneFigure (81:CC31); else TaskCreate
 * (slot 2, parameter 4, BrightnessDownTask), ActorMoveX each frame while
 * slot 2's task runs, then TaskExec NextPartTask.
 * Entry: M 8-bit, X 8-bit; D = the player's record; DB = $81; reached by
 * the jump through the player's start table by $07 at $81:CB61 (81:CB56).
 * Exit: never returns.
 */
void abi_PlayerSceneLand(void)
{
    wr8(dpa(ACTOR_FACING), 0);
    wr8(dpa(ACTOR_FLIP), 0);
    jsl_frame(0x81DFBFu); abi_ActorSnapToGround(); cpu.s = (uint16_t)(cpu.s + 3);
    if (cpu.p & P_Z) {
        /* no ground under him: he falls 3 pixels a frame till he is in it */
        a8_load(0x0E);
        call_frame(0xDFC9, abi_PlayerMoveStart);
        a8_load(0x03);
        wr8(dpa(ACTOR_SPEED_Y + 1), 0x03);
        wr8(dpa(ACTOR_SPEED_Y), 0);
        wr8(dpa(ACTOR_SPEED_Y + 2), 0);
        do {
            actor_yield(0x81DFD2u);
            jsl_frame(0x81DFD4u); abi_ActorMoveY(); cpu.s = (uint16_t)(cpu.s + 3);
            jsl_frame(0x81DFD8u); abi_ActorOutOfGround(); cpu.s = (uint16_t)(cpu.s + 3);
        } while (cpu.p & P_Z);
    }
    a8_load(0x04);
    wr8(dba(V_145B), 0x04);
    a8_load(0x00);
    call_frame(0xDFE7, abi_PlayerMoveStart);
    for (;;) {
        actor_yield(0x81DFE8u);
        uint8_t cue = rd8(dba(CUE_BE));
        a8_load(cue);
        if (cue)
            break;
    }
    /* cued ($81:DFEF): he turns for $80 frames and back for $80, then
       walks off right at 2 pixels a frame for $80 frames */
    MEM8(inc, ea_dp(ACTOR_FLIP));
    lda8_v(0x80);
    actor_wait(0x81DFF3u);
    stz8(ea_dp(ACTOR_FLIP));
    lda8_v(0x80);
    actor_wait(0x81DFF9u);
    MEM8(inc, ea_abs(SCENE_STEP_A));
    lda8_v(0x03);
    call_frame(0xE002, abi_PlayerMoveStart);
    lda8_v(0x02);
    sta8(ea_dp(ACTOR_SPEED_X + 1));
    stz8(ea_dp(ACTOR_SPEED_X));
    stz8(ea_dp(ACTOR_SPEED_X + 2));
    lda8_v(0x80);
    sta8(ea_dp(ACTOR_40));
    do {
        actor_yield(0x81E00Fu);
        jsl_frame(0x81E011u); abi_ActorMoveX(); cpu.s = (uint16_t)(cpu.s + 3);
        MEM8(dec, ea_dp(ACTOR_40));
    } while (!IF_Z);
    MEM8(inc, ea_abs(V_035D));
    MEM8(inc, ea_abs(V_147B));
    lda8(ea_abs(V_036C));
    if (!IF_Z) {
        abi_PlayerSceneFigure();                 /* 81E024 jmp */
        return;
    }
    /* BrightnessDownTask in slot 2 (parameter 4); he walks on while it runs */
    jsl_frame(0x81E027u); abi_TaskCreate(); cpu.unwound = 0;
    do {
        actor_yield(0x81E030u);
        jsl_frame(0x81E032u); abi_ActorMoveX(); cpu.s = (uint16_t)(cpu.s + 3);
        lda8(ea_abs(TASK2_STATE));
    } while (!IF_Z);
    jsl_frame(0x81E03Bu); abi_TaskExec();        /* to NextPartTask: never returns */
}

/*
 * @routine 81:E315 PlayerEndingRideOn
 * @actor
 * WHAT IT DOES: The rest of Aladdin's ride across the ending, from screen
 * 12 (PlayerEndingRide falls into it): $149 frames bobbing in move 1, in
 * move 2 until the scroll is at screen $17, $1BA frames in move 3, in
 * move 4 until screen $1E (with V_1A12 set while the view is in its row 2
 * of screens), then move 1 with V_1A10 set and the speed climbing until
 * his x is more than $140 past the view's left edge, when he is put off the
 * screen above and waits for good. cov_ending_ride_on pokes his resume address to it in
 * the ending (frame 10600) and plays on.
 * HOW: RIDE_41 0; PlayerMoveChange 1; RIDE_COUNT $0149; each frame (BRK
 * first): actor_49 1, ActorBobY, ActorMoveX, RIDE_COUNT down to 0. Move 2:
 * each frame ActorBobY, ActorMoveX until PLAY_SCREEN is $17. RIDE_41 1,
 * move 3, RIDE_COUNT $01BA: each frame actor_49 0, ActorBobY, ActorMoveX
 * until 0. RIDE_41 0, move 4: each frame ActorBobY, ActorMoveX, VIEW_SCREEN_Y
 * 2 sets V_1A12, until PLAY_SCREEN is $1E. V_1A12 0, move 1, V_1A10 1,
 * V_145B 4, accelerations (actor_accel_x, actor_accel_y) 8: each frame
 * actor_49 1, ActorBobY, ActorAccelerateX, ActorMoveX while VIEW_X + $140
 * is not below his x (16-bit). Then V_1B14 up, x's high byte $22, y's high byte $FE,
 * ActorStopY, actor_08 $80, COP $FF, V_1A12 and V_1A10 1, BRK for ever.
 * Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; reached by
 * the fall from PlayerEndingRide (screen 12). Exit: never returns.
 */
void abi_PlayerEndingRideOn(void)
{
    stz8(ea_dp(RIDE_41));
    lda8_v(0x01);
    call_frame(0xE31B, abi_PlayerMoveChange);
    lda8_v(0x49);
    sta8(ea_dp(RIDE_COUNT));
    lda8_v(0x01);
    sta8(ea_dp(RIDE_COUNT + 1));
    do {
        actor_yield(0x81E324u);
        lda8_v(0x01);
        sta8(ea_dp(ACTOR_49));
        jsl_call(0x81E32Au, abi_ActorBobY);
        jsl_call(0x81E32Eu, abi_ActorMoveX);
        op_rep(P_M);
        MEM16(dec, ea_dp(RIDE_COUNT));
        op_sep(P_M);
    } while (!IF_Z);
    lda8_v(0x02);
    call_frame(0xE33E, abi_PlayerMoveChange);
    do {
        actor_yield(0x81E33Fu);
        jsl_call(0x81E341u, abi_ActorBobY);
        jsl_call(0x81E345u, abi_ActorMoveX);
        lda8(ea_abs(PLAY_SCREEN));
        cmp8_v(0x17);
    } while (!IF_Z);
    lda8_v(0x01);
    sta8(ea_dp(RIDE_41));
    lda8_v(0x03);
    call_frame(0xE358, abi_PlayerMoveChange);
    lda8_v(0xBA);
    sta8(ea_dp(RIDE_COUNT));
    lda8_v(0x01);
    sta8(ea_dp(RIDE_COUNT + 1));
    do {
        actor_yield(0x81E361u);
        stz8(ea_dp(ACTOR_49));
        jsl_call(0x81E365u, abi_ActorBobY);
        jsl_call(0x81E369u, abi_ActorMoveX);
        op_rep(P_M);
        MEM16(dec, ea_dp(RIDE_COUNT));
        op_sep(P_M);
    } while (!IF_Z);
    stz8(ea_dp(RIDE_41));
    lda8_v(0x04);
    call_frame(0xE37B, abi_PlayerMoveChange);
    do {
        actor_yield(0x81E37Cu);
        jsl_call(0x81E37Eu, abi_ActorBobY);
        jsl_call(0x81E382u, abi_ActorMoveX);
        lda8(ea_abs(VIEW_SCREEN_Y));
        cmp8_v(0x02);
        if (IF_Z) {
            lda8_v(0x01);
            sta8(ea_abs(V_1A12));
        }
        lda8(ea_abs(PLAY_SCREEN));
        cmp8_v(0x1E);
    } while (!IF_Z);
    stz8(ea_abs(V_1A12));
    lda8_v(0x01);
    call_frame(0xE3A0, abi_PlayerMoveChange);
    lda8_v(0x01);
    sta8(ea_abs(V_1A10));
    lda8_v(0x04);
    sta8(ea_abs(V_145B));
    lda8_v(0x08);
    sta8(ea_dp(ACTOR_ACCEL_X));
    sta8(ea_dp(ACTOR_ACCEL_Y));
    do {
        actor_yield(0x81E3B1u);
        lda8_v(0x01);
        sta8(ea_dp(ACTOR_49));
        jsl_call(0x81E3B7u, abi_ActorBobY);
        jsl_call(0x81E3BBu, abi_ActorAccelerateX);
        jsl_call(0x81E3BFu, abi_ActorMoveX);
        op_rep(P_M);
        lda16(ea_abs(VIEW_X));
        clc();
        adc16_v(0x0140);
        cmp16_v(rd16(ea_dp(ACTOR_X + 1)));
        op_sep(P_M);
    } while (IF_C);
    MEM8(inc, ea_abs(V_1B14));
    lda8_v(0x22);
    sta8(ea_dp(ACTOR_X + 2));
    lda8_v(0xFE);
    sta8(ea_dp(ACTOR_Y + 2));
    jsl_call(0x81E3DDu, abi_ActorStopY);
    lda8_v(0x80);
    sta8(ea_dp(ACTOR_08));
    lda8_v(0xFF);
    actor_wait(0x81E3E7u);
    lda8_v(0x01);
    sta8(ea_abs(V_1A12));
    sta8(ea_abs(V_1A10));
    for (;;)
        actor_yield(0x81E3F1u);
}

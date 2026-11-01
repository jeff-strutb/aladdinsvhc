/*
 * motion.c: an actor's movement: its speeds and gravity, and its position
 * stepped by them.
 *
 * Every routine here works on the actor whose record the direct page is
 * (actor.h). Positions and speeds are 24-bit; the horizontal speed is a
 * size along the actor's facing, the vertical one is signed (down is
 * positive). Each 24-bit step is done a byte at a time with the carry, so
 * A and the flags end as the last byte's ADC or SBC leaves them.
 */
#include "machine.h"
#include "cpu_ops.h"
#include "actor.h"

/* The direct-page byte `off` (bank 0, wrapping in 16 bits). */
static uint32_t dp(uint8_t off)
{
    return (uint16_t)(cpu.d + off);
}

/* The byte at `addr` in the data bank, indexed (abs,Y: may cross into the
   next bank). */
static uint32_t abs_y(uint16_t addr)
{
    return ((uint32_t)cpu.db << 16) + addr + cpu.y;
}

static void set_a8(uint8_t v)
{
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
}

/* LDA dst / ADC src / STA dst (or SBC), one byte of a 24-bit sum. */
static void adc_byte(uint8_t dst, uint8_t m)
{
    uint8_t a = op_adc8(rd8(dp(dst)), m, &cpu.p);

    wr8(dp(dst), a);
    set_a8(a);
}

static void sbc_byte(uint8_t dst, uint8_t m)
{
    uint8_t a = op_sbc8(rd8(dp(dst)), m, &cpu.p);

    wr8(dp(dst), a);
    set_a8(a);
}

/* The 24-bit field at dst plus (minus) the one at src: CLC (SEC), then
   byte by byte. Each byte reads dst, then src, then writes dst. */
static void add24(uint8_t dst, uint8_t src)
{
    int i;

    cpu.p &= (uint8_t)~P_C;
    for (i = 0; i < 3; i++) {
        uint8_t a = rd8(dp(dst + i));

        a = op_adc8(a, rd8(dp(src + i)), &cpu.p);
        wr8(dp(dst + i), a);
        set_a8(a);
    }
}

static void sub24(uint8_t dst, uint8_t src)
{
    int i;

    cpu.p |= P_C;
    for (i = 0; i < 3; i++) {
        uint8_t a = rd8(dp(dst + i));

        a = op_sbc8(a, rd8(dp(src + i)), &cpu.p);
        wr8(dp(dst + i), a);
        set_a8(a);
    }
}

/* The 24-bit field at dst plus (minus) the byte at src: the byte into the
   fraction, the carry (borrow) on through the whole part. */
static void add24_byte(uint8_t dst, uint8_t src)
{
    uint8_t a;

    cpu.p &= (uint8_t)~P_C;
    a = rd8(dp(dst));
    a = op_adc8(a, rd8(dp(src)), &cpu.p);
    wr8(dp(dst), a);
    set_a8(a);
    adc_byte(dst + 1, 0);
    adc_byte(dst + 2, 0);
}

static void sub24_byte(uint8_t dst, uint8_t src)
{
    uint8_t a;

    cpu.p |= P_C;
    a = rd8(dp(dst));
    a = op_sbc8(a, rd8(dp(src)), &cpu.p);
    wr8(dp(dst), a);
    set_a8(a);
    sbc_byte(dst + 1, 0);
    sbc_byte(dst + 2, 0);
}

/* LDA abs,Y / STA dp. */
static void copy_byte(uint16_t table, uint8_t dst)
{
    uint8_t v = rd8(abs_y(table));

    wr8(dp(dst), v);
    set_a8(v);
    cpu.p = nz8(cpu.p, v);
}

/* The motion presets: per preset (Y), the horizontal speed's three bytes
   and the vertical speed's three bytes and gravity, each in a table of its
   own, in the data bank. */
#define MOTION_PRESET_SPEED_X_HI 0xBF3Fu
#define MOTION_PRESET_SPEED_X_MID 0xBFCCu
#define MOTION_PRESET_SPEED_X_FRAC 0xC059u
#define MOTION_PRESET_SPEED_Y_HI 0xC0E6u
#define MOTION_PRESET_SPEED_Y_MID 0xC173u
#define MOTION_PRESET_SPEED_Y_FRAC 0xC200u
#define MOTION_PRESET_GRAVITY 0xC28Du
/* Horizontal and vertical speeds, three bytes per entry (Y the entry's
   byte offset), in the data bank. */
#define SPEED_X_TABLE 0xBAABu
#define SPEED_Y_TABLE 0xBB20u

/*
 * @routine 81:A3CE ActorMotionReset
 * WHAT IT DOES: Stops the actor: no speed either way, no horizontal
 * acceleration, and gravity A.
 * HOW: Clears the horizontal speed and acceleration and the vertical speed
 * with word stores, then stores A as the gravity.
 * Entry: M 8-bit; D = the actor; A low byte = the gravity.
 * Exit: A, X, Y and the flags unchanged; M 8-bit.
 */
void abi_ActorMotionReset(void)
{
    op_rep(P_M);
    wr16(dp(ACTOR_SPEED_X), 0);
    wr16(dp(ACTOR_SPEED_X + 2), 0);
    wr16(dp(ACTOR_SPEED_Y), 0);
    op_sep(P_M);
    wr8(dp(ACTOR_SPEED_Y + 2), 0);
    wr8(dp(ACTOR_ACCEL_Y), (uint8_t)cpu.a);
}

/*
 * @routine 81:A3EC ActorMotionPresetY
 * WHAT IT DOES: Gives the actor the vertical speed and gravity of motion
 * preset Y.
 * HOW: Copies the vertical speed's three bytes and the gravity from the
 * preset tables, fraction first.
 * Entry: M 8-bit, X 8-bit; D = the actor; DB = $81; Y = the preset.
 * Exit: A low byte = the gravity; N, Z from it; X, Y, C, V unchanged.
 */
void abi_ActorMotionPresetY(void)
{
    copy_byte(MOTION_PRESET_SPEED_Y_FRAC, ACTOR_SPEED_Y);
    copy_byte(MOTION_PRESET_SPEED_Y_MID, ACTOR_SPEED_Y + 1);
    copy_byte(MOTION_PRESET_SPEED_Y_HI, ACTOR_SPEED_Y + 2);
    copy_byte(MOTION_PRESET_GRAVITY, ACTOR_ACCEL_Y);
}

/*
 * @routine 81:A3DD ActorMotionPreset
 * WHAT IT DOES: Gives the actor the speeds and gravity of motion preset Y:
 * a jump or a throw's arc.
 * HOW: Copies the horizontal speed's three bytes from the preset tables,
 * fraction first, then goes on into ActorMotionPresetY.
 * Entry: M 8-bit, X 8-bit; D = the actor; DB = $81; Y = the preset.
 * Exit: as ActorMotionPresetY.
 */
void abi_ActorMotionPreset(void)
{
    copy_byte(MOTION_PRESET_SPEED_X_FRAC, ACTOR_SPEED_X);
    copy_byte(MOTION_PRESET_SPEED_X_MID, ACTOR_SPEED_X + 1);
    copy_byte(MOTION_PRESET_SPEED_X_HI, ACTOR_SPEED_X + 2);
    abi_ActorMotionPresetY();
}

/*
 * @routine 81:A413 ActorSpeedXSet
 * WHAT IT DOES: Sets the actor's horizontal speed from the speed table.
 * HOW: Copies the three bytes of the entry at byte offset Y.
 * Entry: M 8-bit, X 8-bit; D = the actor; DB = $81; Y = the entry's offset.
 * Exit: A low byte = the speed's high byte; N, Z from it; X, Y, C, V
 * unchanged.
 */
void abi_ActorSpeedXSet(void)
{
    copy_byte(SPEED_X_TABLE, ACTOR_SPEED_X);
    copy_byte(SPEED_X_TABLE + 1, ACTOR_SPEED_X + 1);
    copy_byte(SPEED_X_TABLE + 2, ACTOR_SPEED_X + 2);
}

/*
 * @routine 81:A425 ActorSpeedYSet
 * WHAT IT DOES: Sets the actor's vertical speed from the speed table.
 * HOW: Copies the three bytes of the entry at byte offset Y.
 * Entry: M 8-bit, X 8-bit; D = the actor; DB = $81; Y = the entry's offset.
 * Exit: A low byte = the speed's high byte; N, Z from it; X, Y, C, V
 * unchanged.
 */
void abi_ActorSpeedYSet(void)
{
    copy_byte(SPEED_Y_TABLE, ACTOR_SPEED_Y);
    copy_byte(SPEED_Y_TABLE + 1, ACTOR_SPEED_Y + 1);
    copy_byte(SPEED_Y_TABLE + 2, ACTOR_SPEED_Y + 2);
}

/*
 * @routine 81:A423 ActorSpeedYSetGravity
 * WHAT IT DOES: Sets the actor's gravity to A and its vertical speed from
 * the speed table.
 * HOW: Stores A as the gravity, then goes on into ActorSpeedYSet.
 * Entry: M 8-bit, X 8-bit; D = the actor; DB = $81; A low byte = the
 * gravity; Y = the entry's offset. Exit: as ActorSpeedYSet.
 */
void abi_ActorSpeedYSetGravity(void)
{
    wr8(dp(ACTOR_ACCEL_Y), (uint8_t)cpu.a);
    abi_ActorSpeedYSet();
}

/*
 * @routine 81:A437 ActorSpeedXClear
 * WHAT IT DOES: Stops the actor's horizontal movement.
 * HOW: Clears the three bytes of its horizontal speed.
 * Entry: M 8-bit; D = the actor. Exit: every register and flag unchanged.
 */
void abi_ActorSpeedXClear(void)
{
    wr8(dp(ACTOR_SPEED_X), 0);
    wr8(dp(ACTOR_SPEED_X + 1), 0);
    wr8(dp(ACTOR_SPEED_X + 2), 0);
}

/*
 * @routine 81:A435 ActorStopX
 * WHAT IT DOES: Stops the actor's horizontal movement and its horizontal
 * acceleration.
 * HOW: Clears the acceleration, then goes on into ActorSpeedXClear.
 * Entry: M 8-bit; D = the actor. Exit: every register and flag unchanged.
 */
void abi_ActorStopX(void)
{
    wr8(dp(ACTOR_ACCEL_X), 0);
    abi_ActorSpeedXClear();
}

/*
 * @routine 81:A440 ActorSpeedYClear
 * WHAT IT DOES: Stops the actor's vertical movement.
 * HOW: Clears the three bytes of its vertical speed.
 * Entry: M 8-bit; D = the actor. Exit: every register and flag unchanged.
 */
void abi_ActorSpeedYClear(void)
{
    wr8(dp(ACTOR_SPEED_Y), 0);
    wr8(dp(ACTOR_SPEED_Y + 1), 0);
    wr8(dp(ACTOR_SPEED_Y + 2), 0);
}

/*
 * @routine 81:A43E ActorStopY
 * WHAT IT DOES: Stops the actor's vertical movement and takes its gravity
 * away.
 * HOW: Clears the gravity, then goes on into ActorSpeedYClear.
 * Entry: M 8-bit; D = the actor. Exit: every register and flag unchanged.
 */
void abi_ActorStopY(void)
{
    wr8(dp(ACTOR_ACCEL_Y), 0);
    abi_ActorSpeedYClear();
}

/*
 * @routine 81:A4E3 ActorStepY
 * WHAT IT DOES: Moves the actor down by its vertical speed (up when the
 * speed is negative). The same steps as ActorMoveY, a routine of its own.
 * HOW: Adds the 24-bit vertical speed to the 24-bit y position.
 * Entry: M 8-bit; D = the actor. Exit: A low byte = the position's high
 * byte; N, V, Z, C from its ADC; X, Y unchanged.
 */
void abi_ActorStepY(void)
{
    add24(ACTOR_Y, ACTOR_SPEED_Y);
}

/*
 * @routine 81:A4F7 ActorStepYBack
 * WHAT IT DOES: Moves the actor against its vertical speed (up when the
 * speed is positive).
 * HOW: Takes the 24-bit vertical speed from the 24-bit y position.
 * Entry: M 8-bit; D = the actor. Exit: A low byte = the position's high
 * byte; N, V, Z, C from its SBC; X, Y unchanged.
 */
void abi_ActorStepYBack(void)
{
    sub24(ACTOR_Y, ACTOR_SPEED_Y);
}

/*
 * @routine 81:A560 ActorMoveY
 * WHAT IT DOES: Moves the actor by its vertical speed.
 * HOW: Adds the 24-bit vertical speed to the 24-bit y position.
 * Entry: M 8-bit; D = the actor. Exit: A low byte = the position's high
 * byte; N, V, Z, C from its ADC; X, Y unchanged.
 */
void abi_ActorMoveY(void)
{
    add24(ACTOR_Y, ACTOR_SPEED_Y);
}

/*
 * @routine 81:A5B3 ActorMoveRightNear
 * WHAT IT DOES: Moves the actor right by its horizontal speed.
 * HOW: Adds the 24-bit horizontal speed to the 24-bit x position.
 * Entry: M 8-bit; D = the actor; called by JSR. Exit: A low byte = the
 * position's high byte; N, V, Z, C from its ADC; X, Y unchanged.
 */
void abi_ActorMoveRightNear(void)
{
    add24(ACTOR_X, ACTOR_SPEED_X);
}

/*
 * @routine 81:A5AF ActorMoveXNear
 * WHAT IT DOES: Moves the actor by its horizontal speed the way it faces.
 * HOW: Facing right, adds the 24-bit horizontal speed to the 24-bit x
 * position (ActorMoveRightNear); facing left, takes it away.
 * Entry: M 8-bit; D = the actor; called by JSR. Exit: A low byte = the
 * position's high byte; N, V, Z, C from its ADC or SBC; X, Y unchanged.
 */
void abi_ActorMoveXNear(void)
{
    uint8_t facing = rd8(dp(ACTOR_FACING));

    set_a8(facing);
    cpu.p = nz8(cpu.p, facing);
    if (facing == 0)
        abi_ActorMoveRightNear();
    else
        sub24(ACTOR_X, ACTOR_SPEED_X);
}

/*
 * @routine 81:A5A7 ActorMoveX
 * WHAT IT DOES: Moves the actor by its horizontal speed the way it faces.
 * HOW: ActorMoveXNear, for callers in other banks.
 * Entry: as ActorMoveXNear; called by JSL. Exit: as ActorMoveXNear.
 */
void abi_ActorMoveX(void)
{
    abi_ActorMoveXNear();
}

/*
 * @routine 81:A5AB ActorMoveRight
 * WHAT IT DOES: Moves the actor right by its horizontal speed, whichever
 * way it faces.
 * HOW: ActorMoveRightNear, for callers in other banks.
 * Entry: as ActorMoveRightNear; called by JSL. Exit: as
 * ActorMoveRightNear.
 */
void abi_ActorMoveRight(void)
{
    abi_ActorMoveRightNear();
}

/*
 * @routine 81:A55D ActorMove
 * WHAT IT DOES: Moves the actor by its speeds: horizontally the way it
 * faces, and vertically.
 * HOW: ActorMoveXNear, then ActorMoveY.
 * Entry: M 8-bit; D = the actor. Exit: as ActorMoveY.
 */
void abi_ActorMove(void)
{
    abi_ActorMoveXNear();
    abi_ActorMoveY();
}

/*
 * @routine 81:A50B ActorFall
 * WHAT IT DOES: One frame of a fall with no horizontal movement: gravity
 * pulls the vertical speed down, and the actor moves by it.
 * HOW: Adds the gravity to the 24-bit vertical speed, then ActorMoveY.
 * Entry: M 8-bit; D = the actor. Exit: as ActorMoveY.
 */
void abi_ActorFall(void)
{
    add24_byte(ACTOR_SPEED_Y, ACTOR_ACCEL_Y);
    abi_ActorMoveY();
}

/*
 * @routine 81:A54A ActorFly
 * WHAT IT DOES: One frame of flight: gravity pulls the vertical speed
 * down, and the actor moves by its speeds.
 * HOW: Adds the gravity to the 24-bit vertical speed, then ActorMove.
 * Entry: M 8-bit; D = the actor. Exit: as ActorMoveY.
 */
void abi_ActorFly(void)
{
    add24_byte(ACTOR_SPEED_Y, ACTOR_ACCEL_Y);
    abi_ActorMove();
}

/*
 * @routine 81:A535 ActorFlyAgainst
 * WHAT IT DOES: One frame of flight with the vertical pull reversed: the
 * acceleration is taken from the vertical speed, and the actor moves by
 * its speeds.
 * HOW: Takes the gravity from the 24-bit vertical speed, then ActorMove.
 * Entry: M 8-bit; D = the actor. Exit: as ActorMoveY.
 */
void abi_ActorFlyAgainst(void)
{
    sub24_byte(ACTOR_SPEED_Y, ACTOR_ACCEL_Y);
    abi_ActorMove();
}

/*
 * @routine 81:A5FF ActorAccelerateX
 * WHAT IT DOES: Speeds the actor up horizontally by its acceleration.
 * HOW: Adds the acceleration to the 24-bit horizontal speed.
 * Entry: M 8-bit; D = the actor. Exit: A low byte = the speed's high byte;
 * N, V, Z, C from its ADC; X, Y unchanged.
 */
void abi_ActorAccelerateX(void)
{
    add24_byte(ACTOR_SPEED_X, ACTOR_ACCEL_X);
}

/*
 * @routine 81:A613 ActorAccelerateY
 * WHAT IT DOES: Gravity pulls the actor's vertical speed down.
 * HOW: Adds the gravity to the 24-bit vertical speed.
 * Entry: M 8-bit; D = the actor. Exit: A low byte = the speed's high byte;
 * N, V, Z, C from its ADC; X, Y unchanged.
 */
void abi_ActorAccelerateY(void)
{
    add24_byte(ACTOR_SPEED_Y, ACTOR_ACCEL_Y);
}

/*
 * @routine 81:A63B ActorDecelerateX
 * WHAT IT DOES: Slows the actor down horizontally by its acceleration,
 * stopping it rather than turning it round.
 * HOW: Takes the acceleration from the 24-bit horizontal speed; a negative
 * result becomes 0.
 * Entry: M 8-bit; D = the actor. Exit: A low byte = the high byte the SBC
 * made; N, V, Z, C from it; X, Y unchanged.
 */
void abi_ActorDecelerateX(void)
{
    sub24_byte(ACTOR_SPEED_X, ACTOR_ACCEL_X);
    if (cpu.p & P_N) {
        wr8(dp(ACTOR_SPEED_X), 0);
        wr8(dp(ACTOR_SPEED_X + 1), 0);
        wr8(dp(ACTOR_SPEED_X + 2), 0);
    }
}

/*
 * @routine 81:A66B ActorDecelerateY
 * WHAT IT DOES: Takes the actor's gravity from its vertical speed,
 * stopping at 0: a rise that slows to a halt.
 * HOW: Takes the gravity from the 24-bit vertical speed; a negative result
 * becomes 0.
 * Entry: M 8-bit; D = the actor. Exit: A low byte = the high byte the SBC
 * made; N, V, Z, C from it; X, Y unchanged.
 */
void abi_ActorDecelerateY(void)
{
    sub24_byte(ACTOR_SPEED_Y, ACTOR_ACCEL_Y);
    if (cpu.p & P_N) {
        wr8(dp(ACTOR_SPEED_Y), 0);
        wr8(dp(ACTOR_SPEED_Y + 1), 0);
        wr8(dp(ACTOR_SPEED_Y + 2), 0);
    }
}

/* Vectors in bank $82: per vector, its x fraction, x pixels low and high,
   y fraction, y pixels low and high, each byte in a table of its own
   ($409 vectors). A direction set's first vector is a word at
   VECTOR_SETS + X (data bank); the facing picks the next for left. */
#define VECTOR_X_FRAC 0x82C000u
#define VECTOR_X_MID 0x82C409u
#define VECTOR_X_HI 0x82C812u
#define VECTOR_Y_FRAC 0x82CC1Bu
#define VECTOR_Y_MID 0x82D024u
#define VECTOR_Y_HI 0x82D42Du
#define VECTOR_SETS 0xBBB9u

/* The vector's six bytes added to the 24-bit fields at fx and fy. */
static void add_vector(uint16_t v, uint8_t fx, uint8_t fy)
{
    cpu.p &= (uint8_t)~P_C;
    adc_byte(fx, rd8(VECTOR_X_FRAC + v));
    adc_byte(fx + 1, rd8(VECTOR_X_MID + v));
    adc_byte(fx + 2, rd8(VECTOR_X_HI + v));
    cpu.p &= (uint8_t)~P_C;
    adc_byte(fy, rd8(VECTOR_Y_FRAC + v));
    adc_byte(fy + 1, rd8(VECTOR_Y_MID + v));
    adc_byte(fy + 2, rd8(VECTOR_Y_HI + v));
}

/*
 * @routine 81:A854 ActorMoveByVector
 * WHAT IT DOES: Moves the actor by a vector from direction set X, the
 * set's left vector when it faces left.
 * HOW: The set's first vector (a word at $BBB9 + X) plus the facing is
 * the vector; its 24-bit x and y parts are added to the position.
 * Entry: M 8-bit, X 8-bit; D = the actor; the sets at $BBB9 in the data
 * bank; X = the set's byte offset. Exit: M and X 8-bit; X = the vector's low byte; A low byte =
 * the y position's high byte; N, V, Z, C from its ADC; Y unchanged.
 */
void abi_ActorMoveByVector(void)
{
    uint8_t facing = rd8(dp(ACTOR_FACING));

    cpu.a = (uint16_t)((cpu.a & 0xFF00) | facing);
    cpu.p = nz8(cpu.p, facing);
    op_rep(P_M | P_X);
    cpu.p &= (uint8_t)~P_C;
    cpu.a = op_adc16((uint16_t)(cpu.a & 0x00FF), rd16(((uint32_t)cpu.db << 16) + VECTOR_SETS + cpu.x), &cpu.p);
    cpu.x = cpu.a;
    cpu.p = nz16(cpu.p, cpu.x);
    op_sep(P_M);
    add_vector(cpu.x, ACTOR_X, ACTOR_Y);
    op_sep(P_X);
}

/*
 * @routine 81:A8DA ActorSpeedAddVector
 * WHAT IT DOES: Adds a vector from direction set X to the actor's speeds,
 * the set's left vector when it faces left.
 * HOW: The set's first vector (a word at $BBB9 + X) plus the facing is
 * the vector; its 24-bit x and y parts are added to the horizontal and
 * vertical speeds.
 * Entry: M 8-bit, X 8-bit; D = the actor; the sets at $BBB9 in the data
 * bank; X = the set's byte offset. Exit: M and X 8-bit; X = the vector's low byte; A low byte =
 * the vertical speed's high byte; N, V, Z, C from its ADC; Y unchanged.
 */
void abi_ActorSpeedAddVector(void)
{
    op_rep(P_M | P_X);
    cpu.p &= (uint8_t)~P_C;
    cpu.a = rd16(dp(ACTOR_FACING));
    cpu.a &= 0x00FF;
    cpu.a = op_adc16(cpu.a, rd16(((uint32_t)cpu.db << 16) + VECTOR_SETS + cpu.x), &cpu.p);
    cpu.x = cpu.a;
    cpu.p = nz16(cpu.p, cpu.x);
    op_sep(P_M);
    add_vector(cpu.x, ACTOR_SPEED_X, ACTOR_SPEED_Y);
    op_sep(P_X);
}

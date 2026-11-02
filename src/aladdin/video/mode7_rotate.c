/*
 * mode7_rotate.c: mode 7's matrix set for a rotation by an angle, from
 * the sine table in bank $82; and a task zooming the picture in.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
void abi_Mode7Rotate(void);
void abi_RotateSine(void);
void abi_Mode7ZoomInTask(void);
void abi_TaskWait(void);
void abi_TaskExit(void);

#define M7A_SHADOW 0x038Du         /* mode 7's matrix as the NMI writes it: A and D (words) */
#define M7B_SHADOW 0x038Fu
#define M7C_SHADOW 0x0391u
#define M7D_SHADOW 0x0393u
#define M7_CENTRE_Y 0x0397u        /* mode 7's centre, its y (word) */
#define V_146B 0x146Bu


/* A negated, N and Z from it. */
static void negate(void)
{
    a16_load((uint16_t)((cpu.a ^ 0xFFFF) + 1));
}

/* The other entry (Y) of the sine table into A. */
static void cosine(void)
{
    cpu.x = cpu.y;
    nz8_of((uint8_t)cpu.x);
    abi_RotateSine();
}

/*
 * @routine 81:B1CD Mode7Rotate
 * WHAT IT DOES: Sets mode 7's matrix to a rotation by angle A (256 to a
 * turn): A and D the cosine, B the sine and C its negative.
 * HOW: The angle's place in its quarter gives the table entry of one value
 * (X) and of the other (Y, from the quarter's far end); the quarter (A's
 * top two bits) picks which is the sine and which signs they take.
 * Entry: M 8-bit, X 8-bit; A = the angle; DB = $00-$3F or $80-$BF; called
 * by JSL. Exit: M 8-bit; A as the last store left it (16 bits); X, Y the
 * two entries.
 */
void abi_Mode7Rotate(void)
{
    uint8_t angle = (uint8_t)cpu.a, at;
    int quadrant;

    /* the sine at the angle within its quadrant (X), and at its
       complement (Y): 64 steps a quadrant, 2 bytes an entry */
    pha8();
    at = (uint8_t)((angle & 0x3F) << 1);
    cpu.x = at;
    cpu.y = (uint8_t)((at ^ 0x7F) + 1);
    pla8();
    cmp8_of(angle, 0xC0);
    quadrant = 3;
    if (angle < 0xC0) {
        cmp8_of(angle, 0x80);
        quadrant = 2;
        if (angle < 0x80) {
            cmp8_of(angle, 0x40);
            quadrant = angle >= 0x40 ? 1 : 0;
        }
    }
    op_rep(P_M);
    abi_RotateSine();
    switch (quadrant) {
    case 0:                               /* A = D = cos, B = sin, C = -sin */
        wr16(dba(M7B_SHADOW), cpu.a);
        negate();
        wr16(dba(M7C_SHADOW), cpu.a);
        cosine();
        wr16(dba(M7A_SHADOW), cpu.a);
        wr16(dba(M7D_SHADOW), cpu.a);
        break;
    case 1:
        negate();
        wr16(dba(M7A_SHADOW), cpu.a);
        wr16(dba(M7D_SHADOW), cpu.a);
        cosine();
        wr16(dba(M7B_SHADOW), cpu.a);
        negate();
        wr16(dba(M7C_SHADOW), cpu.a);
        break;
    case 2:
        wr16(dba(M7C_SHADOW), cpu.a);
        negate();
        wr16(dba(M7B_SHADOW), cpu.a);
        cosine();
        negate();
        wr16(dba(M7A_SHADOW), cpu.a);
        wr16(dba(M7D_SHADOW), cpu.a);
        break;
    default:
        wr16(dba(M7A_SHADOW), cpu.a);
        wr16(dba(M7D_SHADOW), cpu.a);
        cosine();
        wr16(dba(M7C_SHADOW), cpu.a);
        negate();
        wr16(dba(M7B_SHADOW), cpu.a);
        break;
    }
    op_sep(P_M);
}

/*
 * @routine 81:B25B RotateSine
 * WHAT IT DOES: Entry X of the quarter-turn sine table, divided by 16.
 * HOW: The word at $82:87D3 + X, shifted right four times.
 * Entry: M 16-bit; X = the entry's byte offset; called by JSR. Exit: A the
 * value; N clear, Z from it; C the last bit shifted out.
 */
void abi_RotateSine(void)
{
    uint16_t v = rd16(0x8287D3u + cpu.x);

    flag_set(P_C, (v >> 3) & 1);
    a16_load((uint16_t)(v >> 4));
}

/* The task's next frame (returning at `site`), the centre's y stepped by
   `dy` and the scale's low byte down $20: C clear when it went below 0. */
static void zoom_step(uint32_t site, uint8_t dy)
{
    uint8_t v;

    a8_load(0x01);
    jsl_frame(site);
    abi_TaskWait();
    flag_set(P_C, 0);
    wr8(dba(M7_CENTRE_Y), op_adc8(rd8(dba(M7_CENTRE_Y)), dy, &cpu.p));
    flag_set(P_C, 1);
    v = op_sbc8(rd8(dba(M7A_SHADOW)), 0x20, &cpu.p);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    wr8(dba(M7A_SHADOW), v);
    wr8(dba(M7D_SHADOW), v);
}

/*
 * @routine 81:B157 Mode7ZoomInTask
 * WHAT IT DOES: A task that zooms the mode 7 picture in from a speck to
 * full size, the centre drifting up and back, then ends.
 * HOW: After 24 frames, A and D of the matrix (the scale) = $0FF0 and the
 * centre's y = $80; each frame the scale drops by $20 and the centre's y
 * by 1, until the scale's high byte is 1; then each frame the scale drops
 * by $20 and the centre's y rises by 1, until the scale is below $0100;
 * then the scale = $0100 (full size), the centre's y = $40, $146B = $80
 * (purpose not yet established), and TaskExit.
 * Entry: the task's start: M 8-bit; DB a bank whose $0000-$1FFF is work
 * RAM. Exit: none: the task ends.
 */
void abi_Mode7ZoomInTask(void)
{
    uint8_t high;

    a8_load(0x18);
    jsl_frame(0x81B159u);
    abi_TaskWait();
    /* from scale $0FF0, centre y 128: up a line and in $20 a frame to
       scale 1.0; then down a line a frame to 0 */
    wr16(dba(M7A_SHADOW), 0x0FF0);
    wr16(dba(M7D_SHADOW), 0x0FF0);
    wr16(dba(M7_CENTRE_Y), 0x0080);
    cpu.a = 0x0080;
    for (;;) {
        zoom_step(0x81B172u, 0xFF);
        if (cpu.p & P_C)
            continue;
        high = (uint8_t)(rd8(dba(M7A_SHADOW + 1)) - 1);
        a8_load(high);
        wr8(dba(M7A_SHADOW + 1), high);
        wr8(dba(M7D_SHADOW + 1), high);
        cmp8_of(high, 0x01);
        if (high == 0x01)
            break;
    }
    do
        zoom_step(0x81B19Fu, 0x01);
    while (cpu.p & P_C);
    wr8(dba(M7A_SHADOW), 0);
    wr8(dba(M7D_SHADOW), 0);
    a8_load(0x40);
    wr8(dba(M7_CENTRE_Y), 0x40);
    a8_load(0x80);
    wr8(dba(V_146B), 0x80);
    abi_TaskExit();
}

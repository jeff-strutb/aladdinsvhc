/*
 * mode7_scale.c: a Mode 7 matrix element scaled, through the PPU's
 * multiplier, for the matrix the set-up at $80:85E2 writes.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
void abi_Mode7MatrixWrite(void);
#include "hw.h"

#define MODE7_MATRIX 0x038Du       /* the matrix to write: A, B, C, D (words) */
#define MODE7_CENTRE 0x0395u       /* the centre to write: x, y (words) */
#define MODE7_MATRIX_MODE 0x0829u  /* 0: none; 1: scaled by MODE7_SCALES; else as it is */
#define MODE7_SCALES 0x0825u       /* by element, its scale (bytes) */

/*
 * @routine 80:867A Mode7ElementScale
 * WHAT IT DOES: Scales a Mode 7 matrix element: A times X / 256, for the
 * caller to write to a matrix register (low byte Y, then high byte X).
 * HOW: The PPU's signed multiplier: the element negated goes to M7A (low
 * byte, then high), X negated to M7B; the product's middle and high bytes
 * are read back into Y and X (both factors negated, so X = $80 counts as
 * +128). The high byte is written to M7B once more.
 * Entry: M 16-bit, X 8-bit; A = the element; X = the scale; DB a bank
 * whose $2100-$213F are the PPU's. Exit: A = X negated (16 bits); Y = the
 * product's middle byte, X = its high byte; N and Z from X; M 16-bit, X
 * 8-bit; C, V and the other flags unchanged.
 */
void abi_Mode7ElementScale(void)
{
    uint16_t element = (uint16_t)((cpu.a ^ 0xFFFF) + 1);
    uint16_t scale = (uint16_t)(((uint8_t)cpu.x ^ 0xFFFF) + 1);

    /* the PPU's multiplier: the element negated by the scale negated */
    wr8(dba((uint16_t)M7A), (uint8_t)element);
    wr8(dba((uint16_t)M7A), (uint8_t)(element >> 8));
    cpu.a = scale;
    wr8(dba((uint16_t)M7B), (uint8_t)scale);
    cpu.y = rd8(dba((uint16_t)MPYM));
    x8_load(rd8(dba((uint16_t)MPYH)));
    wr8(dba((uint16_t)M7B), (uint8_t)cpu.x);
}

/* Element `i` of the matrix scaled (Mode7ElementScale by its scale). */
static void element_scaled(int i)
{
    cpu.a = rd16(dba((uint16_t)(MODE7_MATRIX + 2 * i)));
    x8_load(rd8(dba((uint16_t)(MODE7_SCALES + i))));
    abi_Mode7ElementScale();
}

/*
 * @routine 80:85E2 Mode7MatrixWrite
 * WHAT IT DOES: Writes the Mode 7 matrix and centre to the PPU, each
 * element scaled by its own scale first when the NMI's mode is 1; nothing
 * when it is 0.
 * HOW: nmi_ppu_mode 0: returns. 1: Mode7ElementScale for D, C, B and A
 * with their scales ($0825-$0828), the results (low, then high byte) to
 * M7D, M7C, M7A and M7B. Else the matrix shadow ($038D) as it is, a byte
 * pair to each of M7A-M7D. Then the centre ($0395) to M7X and M7Y.
 * Entry: M 8-bit, X 8-bit; DB a bank whose $2100-$213F are the PPU's and
 * $0000-$1FFF work RAM; called by JSR from the NMI. Exit: M 8-bit, X
 * 8-bit; written: A low byte = the centre's last byte.
 */
void abi_Mode7MatrixWrite(void)
{
    uint8_t mode = rd8(dba(MODE7_MATRIX_MODE)), v = 0;

    a8_load(mode);
    if (!mode)
        return;
    a8_load((uint8_t)(mode - 1));
    if (mode != 1) {                      /* the matrix as it is, each a byte at a time */
        for (int i = 0; i < 8; i++) {
            v = rd8(dba((uint16_t)(MODE7_MATRIX + i)));
            wr8(dba((uint16_t)(M7A + i / 2)), v);
        }
        a8_load(v);
    } else {                              /* each element scaled: D, C, B (kept), A */
        op_rep(P_M);
        element_scaled(3);
        wr8(dba((uint16_t)M7D), (uint8_t)cpu.y);
        wr8(dba((uint16_t)M7D), (uint8_t)cpu.x);
        element_scaled(2);
        wr8(dba((uint16_t)M7C), (uint8_t)cpu.y);
        wr8(dba((uint16_t)M7C), (uint8_t)cpu.x);
        element_scaled(1);
        phy8();
        phx8();
        element_scaled(0);
        wr8(dba((uint16_t)M7A), (uint8_t)cpu.y);
        wr8(dba((uint16_t)M7A), (uint8_t)cpu.x);
        plx8();
        ply8();
        wr8(dba((uint16_t)M7B), (uint8_t)cpu.y);
        wr8(dba((uint16_t)M7B), (uint8_t)cpu.x);
    }
    op_sep(P_M);
    for (int i = 0; i < 4; i++) {         /* the centre */
        v = rd8(dba((uint16_t)(MODE7_CENTRE + i)));
        wr8(dba((uint16_t)(M7X + i / 2)), v);
    }
    a8_load(v);
}

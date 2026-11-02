/*
 * palette_steps.c: one palette set at a time: from the ROM's table of
 * palettes, to white, or back to its kept colours.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
void abi_ActorPaletteFromRom(void);
void abi_PaletteFromRom(void);
void abi_PaletteWhite(void);
void abi_PaletteRestoreReversed(void);
void abi_PaletteOffset(void);
#include "wram.h"
#include "actor.h"

/* Palettes in bank $84 for actors: 16 colours (32 bytes) each. */
#define ACTOR_PALETTES 0xB000u

/* 16 colours from `from` plus Y (in the data bank) to the long address
   `to` plus X, stepping both. */
static void copy_colours(uint16_t from, uint32_t to)
{
    a16_load(0x0010);
    wr16(dba(SCRATCH), 0x0010);
    do {
        cpu.a = rd16(dba(from) + cpu.y);
        wr16((to + cpu.x) & 0xFFFFFF, cpu.a);
        cpu.x = (uint16_t)(cpu.x + 2);
        cpu.y = (uint16_t)(cpu.y + 2);
    } while (dec16_at(dba(SCRATCH)));
    op_sep(0x30);
    inc8_at(dba(PALETTE_UPLOAD));
    plb();
}

/*
 * @routine 81:9903 ActorPaletteFromRom
 * WHAT IT DOES: Sets the actor's sprite palette to palette Y of the bank
 * $84 table, and has the NMI upload the palettes.
 * HOW: The palette is bits 1-3 of the actor's sprite attributes' high
 * byte; with DB = $84, 16 words from $84:B000 + Y * 32 go to the palette
 * buffer at sprite palette (8 +) that one; then palette_upload steps up.
 * Entry: M 8-bit, X 8-bit; D = the actor; Y low byte = the ROM palette;
 * called by JSL. Exit: M and X 8-bit; DB as at entry, N and Z from it
 * (PLB); $0C = 0; X, Y past the copy (low bytes).
 */
void abi_ActorPaletteFromRom(void)
{
    uint8_t palette = (uint8_t)((rd8(dpa(ACTOR_LOOK + 1)) >> 1) & 0x07);

    cpu.x = palette;                      /* its palette (sprite palettes) */
    phb();
    a8_load(0x84);
    pha8();
    plb();
    op_rep(0x30);
    cpu.y = (uint16_t)((uint8_t)cpu.y << 5);
    cpu.x = (uint16_t)(palette << 5);
    flag_set(P_C, 0);                     /* the last ASL's: bit 11 of the byte, 0 */
    copy_colours(ACTOR_PALETTES, PALETTE_BUFFER + 0x100);
}

/*
 * @routine 81:9941 PaletteFromRom
 * WHAT IT DOES: Sets palette X (counted from palette 4) to palette A of
 * the ROM's table, and has the NMI upload the palettes.
 * HOW: With DB = $87: 16 words from $87:FF40 + A * 32 to the palette
 * buffer at palette 4 + X; then palette_upload steps up.
 * Entry: A low byte = the ROM palette; X low byte = the palette; called by
 * JSL. Exit: M 8-bit, X 8-bit; DB as at entry; $0C = 0; X, Y past the
 * copy.
 */
void abi_PaletteFromRom(void)
{
    uint8_t rom = (uint8_t)cpu.a;

    phb();
    y8_load(0x87);
    phy8();
    plb();
    op_rep(0x30);
    cpu.y = (uint16_t)(rom << 5);
    cpu.x = (uint16_t)((uint8_t)cpu.x << 5);
    flag_set(P_C, 0);
    copy_colours(0xFF40, PALETTE_BUFFER + 0x80);
}

/*
 * @routine 81:9CD7 PaletteWhite
 * WHAT IT DOES: Sets palette A to white and has the NMI upload the
 * palettes.
 * HOW: PaletteOffset, then 16 words of $FFFF into the palette buffer there;
 * palette_upload steps up.
 * Entry: A = the palette (low 4 bits); DB = $00-$3F or $80-$BF; called by
 * JSL. Exit: M 8-bit, X 8-bit; A low byte = $FF; Y = $FF.
 */
void abi_PaletteWhite(void)
{
    op_rep(0x30);
    abi_PaletteOffset();
    cpu.y = 0x000F;
    cpu.a = 0xFFFF;
    do {                                  /* its 16 colours white */
        wr16((PALETTE_BUFFER + cpu.x) & 0xFFFFFF, 0xFFFF);
        cpu.x = (uint16_t)(cpu.x + 2);
        cpu.y = (uint16_t)(cpu.y - 1);
    } while (!(cpu.y & 0x8000));
    op_sep(0x30);
    inc8_at(dba(PALETTE_UPLOAD));
}

/*
 * @routine 81:9CF1 PaletteRestoreReversed
 * WHAT IT DOES: Sets palette A back to the colours PaletteSaveReversed
 * kept, and has the NMI upload the palettes.
 * HOW: PaletteOffset; the kept colours (the first 16 words of
 * palette_saved, last first) go into the palette buffer there;
 * palette_upload steps up.
 * Entry: A = the palette (low 4 bits); DB = $00-$3F or $80-$BF; called by
 * JSL. Exit: M 8-bit, X 8-bit; Y = $FE.
 */
void abi_PaletteRestoreReversed(void)
{
    op_rep(0x30);
    abi_PaletteOffset();
    cpu.y = 0x001E;
    do {                                  /* the saved colours, last first */
        cpu.a = rd16(dba(PALETTE_SAVED) + cpu.y);
        wr16((PALETTE_BUFFER + cpu.x) & 0xFFFFFF, cpu.a);
        cpu.x = (uint16_t)(cpu.x + 2);
        cpu.y = (uint16_t)(cpu.y - 2);
    } while (!(cpu.y & 0x8000));
    op_sep(0x30);
    inc8_at(dba(PALETTE_UPLOAD));
}

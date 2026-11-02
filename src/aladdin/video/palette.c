/*
 * palette.c: loading and keeping palettes.
 *
 * CGRAM's image is kept at PALETTE_BUFFER (16 palettes of 16 colours, a
 * word each); setting PALETTE_UPLOAD has the NMI copy it to CGRAM. Colours
 * a fade works from are kept at PALETTE_SAVED.
 */
#include "machine.h"
#include "ops65.h"
#include "wram.h"
#include "wram_video.h"

void abi_PaletteLoad(void);
void abi_TaskWait(void);
void abi_TaskExit(void);

/* PaletteSequenceTask's six palette sets and the frames each is shown. */
#define PALETTE_SEQUENCE_SETS 0xBA6Au
#define PALETTE_SEQUENCE_FRAMES 0xBA70u

static uint32_t abs_db(uint16_t addr)
{
    return ((uint32_t)cpu.db << 16) | addr;
}

/* Palette loads in bank $84: 6 bytes each (source offset from $84:C000,
   word count, byte offset in PALETTE_BUFFER), and the colours they copy. */
#define PALETTE_LOADS 0x84C000u
#define PALETTE_DATA 0xC000u
#define PALETTE_BANK 0x84

/*
 * @routine 81:98C5 PaletteLoad
 * WHAT IT DOES: Loads palette set A into the palette buffer and has it
 * uploaded.
 * HOW: Load A's entry (A * 6) gives where its colours are, how many and
 * where they go; they are copied a word at a time, counting in scratch
 * $0C, with the data bank at $84; then the upload request is set.
 * Entry: A low byte = the set; DB, D any. Exit: M and X 8-bit; A = the
 * last colour copied (16 bits); X, Y = the low bytes of the offsets past
 * the last colour; DB as at entry, N and Z from it (PLB); C set, V clear
 * (the entry's offset).
 */
void abi_PaletteLoad(void)
{
    uint16_t entry, count, src, dst;
    uint8_t v;

    op_rep(P_M | P_X);
    entry = (uint16_t)((cpu.a & 0x00FF) * 2);
    wr16(abs_db(SCRATCH), entry);
    cpu.p |= P_C;
    entry = op_sbc16((uint16_t)(entry * 4), rd16(abs_db(SCRATCH)), &cpu.p);
    op_sep(P_M);
    op_push8(cpu.db);                         /* PHB, then the data bank = $84 */
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | PALETTE_BANK);
    op_push8(PALETTE_BANK);
    cpu.db = op_pull8();
    cpu.p = nz8(cpu.p, cpu.db);
    op_rep(P_M);
    count = rd16(PALETTE_LOADS + 2 + entry);
    wr16((PALETTE_BANK << 16) | SCRATCH, count);
    src = rd16(PALETTE_LOADS + entry);
    dst = rd16(PALETTE_LOADS + 4 + entry);
    do {
        cpu.a = rd16(((uint32_t)PALETTE_BANK << 16) + PALETTE_DATA + src);
        wr16(PALETTE_BUFFER + dst, cpu.a);
        dst += 2;
        src += 2;
        count = (uint16_t)(rd16_rmw((PALETTE_BANK << 16) | SCRATCH) - 1);
        wr16_rmw((PALETTE_BANK << 16) | SCRATCH, count);
    } while (count != 0);
    cpu.x = dst;
    cpu.y = src;
    op_sep(P_M | P_X);
    v = (uint8_t)(rd8_rmw((PALETTE_BANK << 16) | PALETTE_UPLOAD) + 1);
    wr8((PALETTE_BANK << 16) | PALETTE_UPLOAD, v);
    cpu.db = op_pull8();                      /* PLB */
    cpu.p = nz8(cpu.p, cpu.db);
}

/*
 * @routine 81:9DAA PaletteSave
 * WHAT IT DOES: Keeps Y palettes from palette A on, for a fade to work
 * from.
 * HOW: Notes where they start, then copies Y * 16 colours from the
 * palette buffer to the same places in the saved colours.
 * Entry: M 8-bit, X 8-bit; A low byte = the first palette; Y = how many;
 * DB a bank whose $0000-$1FFF is work RAM. Exit: M and X 8-bit; A = the
 * last colour (16 bits); X = the low byte of the offset past it; Y = 0;
 * Z set, N clear; C = bit 12 of the old Y (its last shift); V unchanged.
 */
void abi_PaletteSave(void)
{
    uint16_t x, y;

    op_rep(P_M | P_X);
    x = (uint16_t)((cpu.a & 0x00FF) << 5);
    wr16(abs_db(PALETTE_SAVED_AT), x);
    cpu.p = (uint8_t)((cpu.p & ~P_C) | ((cpu.y >> 12) & 1));
    y = (uint16_t)(cpu.y << 4);
    do {
        cpu.a = rd16(PALETTE_BUFFER + x);
        wr16(abs_db((uint16_t)(PALETTE_SAVED + x)), cpu.a);
        x += 2;
        y--;
    } while (y != 0);
    cpu.x = x;
    cpu.y = 0;
    cpu.p = nz16(cpu.p, 0);
    op_sep(P_M | P_X);
}

/*
 * @routine 81:9CBE PaletteOffset
 * WHAT IT DOES: The byte offset of palette A in the palette buffer.
 * HOW: (A & 15) * 32, into A and X.
 * Entry: M 16-bit, X 16-bit; called by JSR. Exit: A = X = the offset;
 * N clear, Z from it; C clear; Y, V unchanged.
 */
void abi_PaletteOffset(void)
{
    cpu.a = (uint16_t)((cpu.a & 0x000F) << 5);
    cpu.p &= (uint8_t)~P_C;
    cpu.x = cpu.a;
    cpu.p = nz16(cpu.p, cpu.x);
}

/*
 * @routine 81:9CA6 PaletteSaveReversed
 * WHAT IT DOES: Keeps palette A's 16 colours in the first saved palette,
 * last colour first.
 * HOW: PaletteOffset, then copies the palette's colours from first to
 * last into the saved colours' first palette from its end down.
 * Entry: M 8-bit, X 8-bit; A low byte = the palette; DB a bank whose
 * $0000-$1FFF is work RAM. Exit: M and X 8-bit; A = the last colour (16
 * bits); X = the low byte of the offset past it; Y = $FE; N set, Z clear;
 * C clear; V unchanged.
 */
void abi_PaletteSaveReversed(void)
{
    int y;

    op_rep(P_M | P_X);
    abi_PaletteOffset();
    for (y = 0x1E; y >= 0; y -= 2) {
        cpu.a = rd16(PALETTE_BUFFER + cpu.x);
        wr16(abs_db((uint16_t)(PALETTE_SAVED + y)), cpu.a);
        cpu.x += 2;
    }
    cpu.y = 0xFFFE;
    cpu.p = nz16(cpu.p, 0xFFFE);
    op_sep(P_M | P_X);
}

/* Per level, the offset (from LEVEL_PALETTES) of its palette list in bank
   $81: a count, then the palette sets to load. */
#define LEVEL_PALETTES 0xB812u

/*
 * @routine 81:86BF LevelPalettesLoad
 * WHAT IT DOES: Loads the level's palettes.
 * HOW: The level's list gives a count and the sets; each is loaded with
 * PaletteLoad, X and Y kept on the stack across it.
 * Entry: M 8-bit, X 8-bit; DB = $81; called by JSR. Exit: X = the list's
 * offset past the last set; Y = 0; Z set, N clear; with sets, A and C, V
 * as the last PaletteLoad leaves them; DB as at entry.
 */
void abi_LevelPalettesLoad(void)
{
    uint8_t lv = rd8(abs_db(LEVEL)), x, y;

    cpu.y = lv;
    x = rd8(abs_db((uint16_t)(LEVEL_PALETTES + lv)));
    cpu.x = x;
    y = rd8(abs_db((uint16_t)(LEVEL_PALETTES + x)));
    cpu.y = y;
    cpu.p = nz8(cpu.p, y);
    while (y != 0) {
        op_push8(x);
        op_push8(y);
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | rd8(abs_db((uint16_t)(LEVEL_PALETTES + 1 + x))));
        abi_PaletteLoad();
        y = op_pull8();
        x = op_pull8();
        x++;
        y--;
        cpu.x = x;
        cpu.y = y;
        cpu.p = nz8(cpu.p, y);
    }
}


/*
 * @routine 81:9E75 PaletteSequenceTask
 * WHAT IT DOES: A task that shows six palette sets in turn, each for its
 * own frames, then ends.
 * HOW: For Y from 5 down to 0: PaletteLoad of the set at $BA6A + Y, then
 * TaskWait for the frames at $BA70 + Y; then TaskExit.
 * Entry: the task's start: M 8-bit, X 8-bit; DB = $81. Exit: none: the
 * task ends.
 */
void abi_PaletteSequenceTask(void)
{
    ldy8_v(0x05);  /* 819E75 ldy */
L_819E77:
    lda8(ea_absy(PALETTE_SEQUENCE_SETS));  /* 819E77 lda */
    phy8();  /* 819E7A phy */
    abi_PaletteLoad();  /* 819E7B jsl */
    ply8();  /* 819E7F ply */
    lda8(ea_absy(PALETTE_SEQUENCE_FRAMES));  /* 819E80 lda */
    jsl_frame(0x819E83u); abi_TaskWait();  /* 819E83 jsl */
    dey8();  /* 819E87 dey */
    if (!IF_N) goto L_819E77;  /* 819E88 bpl */
    abi_TaskExit(); return;  /* 819E8A jml */
}

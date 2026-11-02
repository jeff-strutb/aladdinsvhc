/*
 * level_tables.c: values the level takes from per-level tables.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "cpu_ops.h"
#include "native.h"
void abi_LevelExitCheck(void);
void abi_LevelStartPlace(void);
void abi_LevelCompanionStart(void);
void abi_LevelSongPlay(void);
void abi_RestartPointCheck(void);
void abi_PaletteLoad(void);
void abi_SongPlay(void);
#include "wram.h"
#include "wram_video.h"
#include "wram_player.h"

static uint32_t abs_db(uint16_t addr)
{
    return ((uint32_t)cpu.db << 16) | addr;
}

/* Per level: two bytes for LEVEL_035C (bank $81, word per level). */
#define LEVEL_035C_TABLE 0xB933u
/* Per level, the offset of its trigger-spot list from LEVEL_SPOT_TABLE;
   the list is a count, then 6-byte spots. */
#define LEVEL_SPOT_TABLE 0xEC1Eu

/*
 * @routine 81:878B Level035CSet
 * WHAT IT DOES: Sets $035C-$035D from the level's entry in the table at
 * $81:B933.
 * HOW: The level * 2 indexes the table; its two bytes are copied.
 * Entry: M 8-bit, X 8-bit; DB = $81. Exit: A low byte = the second byte;
 * N, Z from it; X = the level * 2; C = the level's bit 7; Y, V unchanged.
 */
void abi_Level035CSet(void)
{
    uint8_t lv = rd8(abs_db(LEVEL)), v;

    cpu.p = (uint8_t)((cpu.p & ~P_C) | (lv >> 7));
    cpu.x = (uint8_t)(lv << 1);
    v = rd8(abs_db((uint16_t)(LEVEL_035C_TABLE + cpu.x)));
    wr8(abs_db(LEVEL_035C), v);
    v = rd8(abs_db((uint16_t)(LEVEL_035C_TABLE + 1 + cpu.x)));
    wr8(abs_db(LEVEL_035C + 1), v);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = nz8(cpu.p, v);
}

/*
 * @routine 81:8716 LevelSpotsSet
 * WHAT IT DOES: Finds the level's trigger spots: how many, and where their
 * list is.
 * HOW: The level's word in the spot table is the list's offset; its first
 * byte is the count.
 * Entry: M 8-bit, X 8-bit; DB = $81; called by JSR. Exit: M and X 8-bit;
 * A low byte = the count, high byte = the level word * 2's; X = the low
 * byte of the list's offset; Y = the level * 2 (low byte); N, Z from the
 * count; C = bit 15 of the level word; V unchanged.
 */
void abi_LevelSpotsSet(void)
{
    uint16_t lv, x;
    uint8_t n;

    op_rep(P_M | P_X);
    lv = rd16(abs_db(LEVEL));
    cpu.p = (uint8_t)((cpu.p & ~P_C) | (lv >> 15));
    cpu.a = (uint16_t)(lv << 1);
    cpu.y = cpu.a;
    x = rd16(abs_db((uint16_t)(LEVEL_SPOT_TABLE + cpu.y)));
    cpu.x = x;
    cpu.p = nz16(cpu.p, x);
    op_sep(P_M);
    n = rd8(abs_db((uint16_t)(LEVEL_SPOT_TABLE + x)));
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | n);
    cpu.p = nz8(cpu.p, n);
    wr8(abs_db(LEVEL_SPOT_COUNT), n);
    wr16(abs_db(LEVEL_SPOTS), x);
    op_sep(P_X);
}

/* A word of the data bank. */
static uint16_t db16(uint16_t addr) { return rd16(dba(addr)); }
static void db16_set(uint16_t addr, uint16_t v) { wr16(dba(addr), v); }

/*
 * @routine 81:84B2 LevelStartPlace
 * WHAT IT DOES: Puts the player and the camera at the level's start (its
 * restart point $0360): the player's position and facing, both layers'
 * scroll and the point actors are kept in range of.
 * HOW: $1738 cleared; the level's start list (a word at $80:ED9D + level
 * * 2, plus $0360 * 4) gives the player's x (bit 15: facing left, $08EF =
 * $0101) and y; the camera list ($80:EF15) gives layer 0's scroll x and
 * y, also its last and drawn positions; layer 1 takes them halved (x
 * whole on level $10 or with $083F set; y whole on levels 0, 3, $0D,
 * $14, $1A); the range point = the scroll + $80 each way; $1A40 = 0,
 * $1A42 = $7F00.
 * Entry: M 8-bit, X 8-bit; DB = $80; called by JSR. Exit: M and X 8-bit;
 * A = $7F00 (16 bits); X, Y as the tables left them (low bytes);
 * scratch $0C = $0360 * 4.
 */
void abi_LevelStartPlace(void)
{
    uint16_t y, x, v;

    wr8(dba((uint16_t)LAYER0), 0);
    /* the level's entry for the restart point ($0360, 4 bytes each) */
    y = (uint8_t)(rd8(dba(LEVEL)) << 1);
    wr8(dba(SCRATCH), (uint8_t)(rd8(dba(0x0360)) << 2));
    wr8(dba(0x000D), 0);
    op_rep(0x30);
    /* the player's place: x (bit 15 set: facing left) and y */
    flag_set(P_C, 0);
    x = op_adc16(db16((uint16_t)(0xED9D + y)), db16(SCRATCH), &cpu.p);
    v = db16((uint16_t)(0xED9D + x));
    db16_set(0x08E7, v & 0x7FFF);
    db16_set(0x08EF, v & 0x8000 ? 0x0101 : 0x0000);
    db16_set(0x08EA, db16((uint16_t)(0xED9F + x)));
    /* the camera's: layer 0's scroll, the backdrop's half of it in the plain set-up
       except on level $10 */
    flag_set(P_C, 0);
    x = op_adc16(db16((uint16_t)(0xEF15 + y)), db16(SCRATCH), &cpu.p);
    v = db16(x);
    db16_set(0x1773, v);
    db16_set((uint16_t)SCROLL_X_BEFORE, v);
    db16_set(0x176C, v);
    cpu.y = db16(LEVEL);
    if (cpu.y != 0x0010) {
        cpu.y = db16(0x083F);
        if (!cpu.y)
            v >>= 1;
    }
    db16_set(0x18C9, v);
    db16_set(0x18C2, v);
    v = db16((uint16_t)(SOUND_SEQ + x));
    db16_set(0x1777, v);
    db16_set((uint16_t)SCROLL_Y_BEFORE, v);
    db16_set(0x176E, v);
    cpu.y = db16(LEVEL);
    if (cpu.y != 0x0000 && cpu.y != 0x0003 && cpu.y != 0x000D && cpu.y != 0x0014 && cpu.y != 0x001A)
        v >>= 1;
    db16_set(0x18CD, v);
    db16_set(0x18C4, v);
    cpu.x = x;
    flag_set(P_C, 0);
    db16_set(0x1A44, op_adc16(db16(0x1773), 0x0080, &cpu.p));
    flag_set(P_C, 0);
    db16_set(0x1A46, op_adc16(db16(0x1777), 0x0080, &cpu.p));
    db16_set(0x1A40, 0);
    a16_load(0x7F00);
    db16_set(0x1A42, 0x7F00);
    op_sep(0x30);
}

/*
 * @routine 81:8623 LevelCompanionStart
 * WHAT IT DOES: On a level with a companion actor (the table at $80:B7BB),
 * starts it at the player's place, and loads its palettes.
 * HOW: The level's byte, less one, is the companion's $07; the record at
 * $097B is made new ($0C) with hit box $72, look $0240 and the player's
 * position; palette set $3A, and on level $12 set $48 too (PaletteLoad).
 * Entry: M 8-bit, X 8-bit; DB = $80; called by JSR. Exit: as PaletteLoad,
 * or with no companion A low byte = 0, Z set.
 */
void abi_LevelCompanionStart(void)
{
    uint8_t v;

    x8_load(rd8(dba(LEVEL)));
    a8_load(rd8(dba(0xB7BB) + cpu.x));
    if (!(uint8_t)cpu.a)
        return;
    /* the companion's record: kind, state, its time and place (the
       player's) */
    wr8(dba(0x0982), (uint8_t)(cpu.a - 1));
    wr8(dba(0x097B), 0x0C);
    wr8(dba(0x0981), 0x72);
    db16_set(0x09A6, 0x0240);
    db16_set(0x0987, db16(0x08E7));
    cpu.a = db16(0x08EA);
    db16_set(0x098A, cpu.a);
    a8_load(0x3A);
    abi_PaletteLoad();
    a8_load(v = rd8(dba(LEVEL)));
    cmp8_of(v, 0x12);
    if (v != 0x12)
        return;
    a8_load(0x48);
    abi_PaletteLoad();
}


/*
 * @routine 81:86F0 RestartPointCheck
 * WHAT IT DOES: Sets the level's restart point once the camera has
 * passed it: from there a lost life starts the level at its second start.
 * HOW: With no restart point yet ($0360 = 0) and $1423 clear, the level's
 * word in the table at $80:F015 (negative: none) below layer 0's scroll x
 * sets $0360 = 1.
 * Entry: M 8-bit, X 8-bit; DB = $80; called by JSR. Exit: M and X 8-bit;
 * A = the last word read (16 bits); X as the test left it.
 */
void abi_RestartPointCheck(void)
{
    uint16_t at;

    op_rep(P_M);
    x8_load(rd8(dba(0x0360)));
    if ((uint8_t)cpu.x)
        goto out;
    a16_load(db16(0x1423));
    if (cpu.a)
        goto out;
    /* the level's restart x: passed by the camera, the point is set */
    cpu.a = (uint16_t)((db16(LEVEL) & 0x00FF) << 1);
    flag_set(P_C, 0);
    x8_load((uint8_t)cpu.a);
    a16_load(at = db16((uint16_t)(0xF015 + cpu.x)));
    if (at & 0x8000)
        goto out;
    cmp16_of(at, db16(0x1773));
    if (at >= db16(0x1773))
        goto out;
    x8_load(0x01);
    wr8(dba(0x0360), 0x01);
out:
    op_sep(0x30);
}

/*
 * @routine 81:872E LevelExitCheck
 * WHAT IT DOES: Ends the level when the player reaches its exit spot (on
 * some levels, when the camera passes it): sets the next stage and starts
 * the player's exit walk.
 * HOW: Not while $141C, $1423 or $141B is set, nor with no spots. Levels
 * marked in the table at $80:B96B: the first spot's x below layer 0's
 * scroll x. Others: each spot in turn, the player within 8 pixels of its
 * x, then of the first spot's y. Reached ($81:879D on): the spot's byte
 * at +6 (low 7 bits) is the stage ($035D), 0 moving the sequence on
 * through the table at $80:B988; the player's code = $E730 (marked
 * levels) or $CBF9, its facing from the byte's bit 7; $0347 bit 7 set;
 * $1423 and $1454 up by one; the restart point cleared.
 * Entry: M 8-bit, X 8-bit; DB = $80; called by JSR. Exit: M and X 8-bit;
 * A, X, Y as the tests left them.
 */
void abi_LevelExitCheck(void)
{
    uint16_t d;
    uint8_t to;

    a8_load(rd8(dba(V_141C)) | rd8(dba(0x1423)) | rd8(dba(0x141B)));
    if ((uint8_t)cpu.a)
        return;
    a8_load(rd8(dba(LEVEL_SPOT_COUNT)));
    if (!(uint8_t)cpu.a)
        return;
    y8_load((uint8_t)cpu.a);
    x8_load(rd8(dba(LEVEL)));
    a8_load(rd8(dba(0xB96B) + cpu.x));
    op_rep(0x30);
    cpu.x = db16(LEVEL_SPOTS);
    if ((uint8_t)cpu.a) {
        /* scrolling levels: the exit once the camera is past its x */
        flag_set(P_C, 1);
        cpu.a = op_sbc16(db16((uint16_t)(0xEC1F + cpu.x)), db16(0x1773), &cpu.p);
        if (cpu.p & P_C)
            goto out;
        goto leave;
    }
    /* the others: the player within 8 of a spot, each 6 bytes (the y test
       reads the first spot's, as the original) */
    for (;;) {
        flag_set(P_C, 1);
        d = op_sbc16(db16((uint16_t)(0xEC1F + cpu.x)), db16(0x08E7), &cpu.p);
        flag_set(P_C, 0);
        cpu.a = d = op_adc16(d, 0x0008, &cpu.p);
        cmp16_of(d, 0x0010);
        if (d < 0x0010) {
            cpu.x = db16(LEVEL_SPOTS);
            flag_set(P_C, 1);
            d = op_sbc16(db16((uint16_t)(0xEC21 + cpu.x)), db16(0x08EA), &cpu.p);
            flag_set(P_C, 0);
            cpu.a = d = op_adc16(d, 0x0008, &cpu.p);
            cmp16_of(d, 0x0010);
            if (d < 0x0010)
                goto leave;
        }
        flag_set(P_C, 0);
        cpu.x = cpu.a = op_adc16(cpu.x, 0x0006, &cpu.p);
        cpu.y--;
        nz16_of(cpu.y);
        if (!cpu.y)
            goto out;
    }
leave:
    /* where the exit leads, and the player's facing there */
    op_sep(P_M);
    cpu.x = db16(LEVEL_SPOTS);
    to = rd8(dba(0xEC24) + cpu.x);
    wr8(dba(0x035D), to & 0x7F);
    if (!(to & 0x7F)) {
        cpu.x = db16(LEVEL_035C);
        wr8(dba(LEVEL_035C), rd8(dba(0xB988) + cpu.x));
    }
    cpu.x = to >> 7;
    flag_set(P_C, to & 0x80);
    cpu.y = db16(LEVEL);
    cpu.y = rd8(dba(0xB96B) + cpu.y) ? 0xE730 : 0xCBF9;
    db16_set(0x08DE, cpu.y);
    a8_load(0x80);
    tsb_at(dba(0x0347), 0x80);
    op_sep(0x30);
    wr8(dba(0x08F0), (uint8_t)cpu.x);
    wr8(dba(0x08EF), (uint8_t)cpu.x);
    inc8_at(dba(0x1423));
    inc8_at(dba(0x1454));
    wr8(dba(0x0360), 0);
    return;
out:
    op_sep(0x30);
}

/*
 * anim_sets.c: which animation sets a level's actors need.
 *
 * Each actor type uses one animation set. Loading a level marks the sets
 * of the types it has (the always-present types and the level's own) and
 * lists each set once, so each is loaded once.
 */
#include "machine.h"
#include "ops65.h"
void abi_AnimSetsLoad(void);
#include "wram.h"
#include "wram_video.h"
#include "actor_anim.h"

void abi_AnimSetsListEnd(void);
void abi_AnimSetsPlaces(void);

static void set_a8(uint8_t v)
{
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = nz8(cpu.p, v);
}

#define NO_SET 0xFF
/* Each type's animation set, 256 bytes in bank $81; the first 32 types
   are present in every level. */
#define TYPE_ANIM_SETS 0x81EF6Fu
#define ALWAYS_TYPES 32
/* Per level, the offset (from LEVEL_ACTORS) of its actor list in bank
   $82: a count, then 6-byte entries whose first byte is the type. */
#define LEVEL_ACTORS 0x8A7Au
#define LEVEL_ACTOR_SIZE 6

/*
 * @routine 81:9D4C AnimSetsFill
 * WHAT IT DOES: Sets every actor type's animation set to A.
 * HOW: Stores A in all 256 bytes, the first, then the last down.
 * Entry: M 8-bit, X 8-bit; A low byte = the value. Exit: X = 0; Z set,
 * N clear; A, Y, C, V unchanged.
 */
void abi_AnimSetsFill(void)
{
    uint8_t x = 0;

    do {
        wr8(ANIM_SET_OF_TYPE + x, (uint8_t)cpu.a);
        x--;
    } while (x != 0);
    cpu.x = 0;
    cpu.p = nz8(cpu.p, 0);
}

/*
 * @routine 81:9D4A AnimSetsClear
 * WHAT IT DOES: Marks every actor type as needing no animation set.
 * HOW: AnimSetsFill with $FF.
 * Entry: M 8-bit, X 8-bit. Exit: A low byte = $FF; X = 0; Z set, N clear;
 * Y, C, V unchanged.
 */
void abi_AnimSetsClear(void)
{
    set_a8(NO_SET);
    abi_AnimSetsFill();
}

/*
 * @routine 81:906B AnimSetsAlways
 * WHAT IT DOES: Marks the animation sets of the 32 types every level has.
 * HOW: Copies the first 32 bytes of the type table, last first.
 * Entry: M 8-bit, X 8-bit; called by JSR. Exit: A low byte = type 0's set;
 * X = $FF; N set, Z clear; Y, C, V unchanged.
 */
void abi_AnimSetsAlways(void)
{
    int x;

    for (x = ALWAYS_TYPES - 1; x >= 0; x--) {
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | rd8(TYPE_ANIM_SETS + x));
        wr8(ANIM_SET_OF_TYPE + x, (uint8_t)cpu.a);
    }
    cpu.x = 0xFF;
    cpu.p = nz8(cpu.p, 0xFF);
}

/*
 * @routine 81:9D0C AnimSetsOfLevel
 * WHAT IT DOES: Marks the animation sets of the actor types the level has.
 * HOW: With the data bank at $82: the level's actor list; for each of its
 * entries, the type's set is copied from the type table, counting in
 * scratch $0C.
 * Entry: M 8-bit, X 8-bit. Exit: M and X 8-bit; DB as at entry, N and Z
 * from it (PLB); with entries, A = the entry offset past the last (low
 * byte; C, V from its ADC), X = the last type, Y = the low byte of that
 * offset; with none, A = 0, Y = the list's offset, X, C, V unchanged.
 */
void abi_AnimSetsOfLevel(void)
{
    const uint32_t bank = 0x820000u;
    uint16_t y, n;

    op_push8(cpu.db);                         /* PHB, then the data bank = $82 */
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x82);
    op_push8(0x82);
    cpu.db = op_pull8();
    cpu.p = nz8(cpu.p, cpu.db);
    op_rep(P_M | P_X);
    y = (uint16_t)(rd16(bank | LEVEL) << 1);
    y = rd16(bank + LEVEL_ACTORS + y);
    cpu.a = (uint16_t)(rd16(bank + LEVEL_ACTORS + y) & 0x00FF);
    if (cpu.a != 0) {
        wr16(bank | SCRATCH, cpu.a);
        y++;
        do {
            uint8_t type = (uint8_t)rd16(bank + LEVEL_ACTORS + y);

            cpu.x = type;
            wr8(ANIM_SET_OF_TYPE + type, rd8(TYPE_ANIM_SETS + type));
            cpu.p &= (uint8_t)~P_C;
            cpu.a = op_adc16(y, LEVEL_ACTOR_SIZE, &cpu.p);
            y = cpu.a;
            n = (uint16_t)(rd16_rmw(bank | SCRATCH) - 1);
            wr16_rmw(bank | SCRATCH, n);
        } while (n != 0);
    }
    cpu.y = y;
    op_sep(P_M | P_X);
    cpu.db = op_pull8();                      /* PLB */
    cpu.p = nz8(cpu.p, cpu.db);
}

/*
 * @routine 81:902C AnimSetsList
 * WHAT IT DOES: Lists each animation set the level's types use, once.
 * HOW: With the data bank at $7F: for each type with a set, looks for the
 * set in the list; if it is not there, it takes the end's place, with a 0
 * after it and a new end mark.
 * Entry: M 8-bit, X 8-bit; called by JSR. Exit: X = 0; A low byte and Y
 * as the last type's search leaves them; C from its last compare; DB as at
 * entry, N and Z from it (PLB); V unchanged.
 */
void abi_AnimSetsList(void)
{
    const uint32_t bank = 0x7F0000u;
    uint8_t x = 0, y = 0, a;

    op_push8(cpu.db);                         /* PHB, then the data bank = $7F */
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x7F);
    op_push8(0x7F);
    cpu.db = op_pull8();
    cpu.p = nz8(cpu.p, cpu.db);
    a = NO_SET;
    wr8(ANIM_SETS_USED, a);
    do {
        a = rd8(bank | (uint16_t)(ANIM_SET_OF_TYPE + x));
        cpu.p = cmp8_flags(cpu.p, a, NO_SET);
        if (a != NO_SET) {
            y = 0;
            for (;;) {
                a = rd8(bank | (uint16_t)(ANIM_SET_OF_TYPE + x));
                cpu.p = cmp8_flags(cpu.p, a, rd8(bank | (uint16_t)(ANIM_SETS_USED + y)));
                if (cpu.p & P_Z)
                    break;
                a = rd8(bank | (uint16_t)(ANIM_SETS_USED + y));
                y = (uint8_t)(y + 2);
                cpu.p = cmp8_flags(cpu.p, a, NO_SET);
                if (cpu.p & P_Z) {
                    a = rd8(bank | (uint16_t)(ANIM_SET_OF_TYPE + x));
                    y = (uint8_t)(y - 2);
                    wr8(bank | (uint16_t)(ANIM_SETS_USED + y), a);
                    y++;
                    a = 0;
                    wr8(bank | (uint16_t)(ANIM_SETS_USED + y), a);
                    y++;
                    a = NO_SET;
                    wr8(bank | (uint16_t)(ANIM_SETS_USED + y), a);
                    break;
                }
            }
        }
        x++;
    } while (x != 0);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | a);
    cpu.x = 0;
    cpu.y = y;
    cpu.db = op_pull8();                      /* PLB */
    cpu.p = nz8(cpu.p, cpu.db);
}

/*
 * @routine 81:9079 AnimSetsListEnd
 * WHAT IT DOES: Finds the end of the level's list of animation sets.
 * HOW: Steps through the list a pair at a time until the $FF end mark.
 * Entry: M 8-bit, X 8-bit; called by JSR. Exit: X = the end mark's offset
 * + 2 (twice the sets listed, + 2); A low byte = $FF; Z set, C set, N
 * clear; Y, V unchanged.
 */
void abi_AnimSetsListEnd(void)
{
    uint8_t x = 0, a;

    do {
        a = rd8(ANIM_SETS_USED + x);
        x = (uint8_t)(x + 2);
        cpu.p = cmp8_flags(cpu.p, a, NO_SET);
    } while (a != NO_SET);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | a);
    cpu.x = x;
}

/*
 * @routine 81:9086 AnimSetsPlaces
 * WHAT IT DOES: For each actor type, finds where its animation set is in
 * the level's list of sets.
 * HOW: With the data bank at $7F: AnimSetsListEnd gives the sets listed;
 * for each of the 256 types the place is 4 (not listed) unless its set is
 * found in the list, when it is the set's place (offset / 2); the count
 * of sets left to look at is kept in scratch $0E.
 * Entry: M 8-bit, X 8-bit; called by JSR. Exit: Y = 0; X as the last
 * type's search left it; $0C = the sets listed; DB as at entry, N and Z
 * from it (PLB).
 */
void abi_AnimSetsPlaces(void)
{
    phb();  /* 819086 phb */
    lda8_v(0x7F);  /* 819087 lda */
    pha8();  /* 819089 pha */
    plb();  /* 81908A plb */
    abi_AnimSetsListEnd();  /* 81908B jsr */
    txa8();  /* 81908E txa */
    ACC8(lsr);  /* 81908F lsr */
    ACC8(dec);  /* 819090 dec */
    sta8(ea_abs(SCRATCH));  /* 819091 sta */
    ldy8_v(0x00);  /* 819094 ldy */
L_819096:
    lda8(ea_abs(SCRATCH));  /* 819096 lda */
    sta8(ea_abs(SCRATCH + 2));  /* 819099 sta */
    lda8_v(0x04);  /* 81909C lda */
    sta8(ea_absy((uint16_t)ANIM_SET_PLACE_OF_TYPE));  /* 81909E sta */
    ldx8_v(0x00);  /* 8190A1 ldx */
L_8190A3:
    lda8(ea_absy((uint16_t)ANIM_SET_OF_TYPE));  /* 8190A3 lda */
    cmp8_v(rd8(ea_absx((uint16_t)ANIM_SETS_USED)));  /* 8190A6 cmp */
    if (!IF_Z) goto L_8190B2;  /* 8190A9 bne */
    txa8();  /* 8190AB txa */
    ACC8(lsr);  /* 8190AC lsr */
    sta8(ea_absy((uint16_t)ANIM_SET_PLACE_OF_TYPE));  /* 8190AD sta */
    goto L_8190B9;  /* 8190B0 bra */
L_8190B2:
    inx8();  /* 8190B2 inx */
    inx8();  /* 8190B3 inx */
    MEM8(dec, ea_abs(SCRATCH + 2));  /* 8190B4 dec */
    if (!IF_Z) goto L_8190A3;  /* 8190B7 bne */
L_8190B9:
    iny8();  /* 8190B9 iny */
    if (!IF_Z) goto L_819096;  /* 8190BA bne */
    plb();  /* 8190BC plb */
    return;  /* 8190BD rts */
}

/*
 * @routine 81:8F47 AnimSetsLoad
 * WHAT IT DOES: Loads the level's animation sets: marks and lists the sets
 * its actor types use, and copies each set's picture list and animation
 * tables into work RAM.
 * HOW: With DB = $A6: AnimSetsClear, AnimSetsOfLevel, AnimSetsAlways,
 * AnimSetsList, AnimSetsListEnd. For each set listed: its picture list's
 * place ($7E:2000 + X) goes to anim_set_gfx; its entry in the table at
 * $A6:8000 (set * 3: offset from $8000, bank from $A6) gives a byte count
 * and the bytes, copied through the long pointer at $18 (each step past
 * $FFFF going on at $8000 of the next bank). Then with DB = $A5: for each
 * set, its table's place ($7E:B000 + X) goes to anim_set_table, and the
 * words of its table (offset at $A5:C000 + set * 2; a count / 2 + 1) are
 * copied. Last, AnimSetsPlaces. Scratch $0C counts the sets, $10 indexes
 * the lists, $16 counts bytes or words. The step at $81:8FAD (a count at
 * a bank's last two bytes) is in no set and no script runs it.
 * Entry: DB, P kept on the stack; D = 0; called by JSL. Exit: P and DB as
 * at entry; A, X, Y as AnimSetsPlaces leaves them.
 */
void abi_AnimSetsLoad(void)
{
    op_sep(0x30);  /* 818F47 sep */
    phb();  /* 818F49 phb */
    php();  /* 818F4A php */
    lda8_v(0xA6);  /* 818F4B lda */
    pha8();  /* 818F4D pha */
    plb();  /* 818F4E plb */
    abi_AnimSetsClear();  /* 818F4F jsl */
    abi_AnimSetsOfLevel();  /* 818F53 jsl */
    abi_AnimSetsAlways();  /* 818F57 jsr */
    abi_AnimSetsList();  /* 818F5A jsr */
    abi_AnimSetsListEnd();  /* 818F5D jsr */
    txa8();  /* 818F60 txa */
    op_rep(0x30);  /* 818F61 rep */
    and16_v(0x00FF);  /* 818F63 and */
    ACC16(lsr);  /* 818F66 lsr */
    ACC16(dec);  /* 818F67 dec */
    sta16(ea_dp(0x0C));  /* 818F68 sta */
    stz16(ea_dp(0x10));  /* 818F6A stz */
    ldx16_v(0x0000);  /* 818F6C ldx */
L_818F6F:
    phx16();  /* 818F6F phx */
    txa16();  /* 818F70 txa */
    ldx16(ea_dp(0x10));  /* 818F71 ldx */
    MEM16(inc, ea_dp(0x10));  /* 818F73 inc */
    MEM16(inc, ea_dp(0x10));  /* 818F75 inc */
    sta16(ea_longx(ANIM_SET_GFX));  /* 818F77 sta */
    lda16(ea_longx(ANIM_SETS_USED));  /* 818F7B lda */
    ACC16(asl);  /* 818F7F asl */
    clc();  /* 818F80 clc */
    adc16_v(rd16(ea_longx(ANIM_SETS_USED)));  /* 818F81 adc */
    tay16();  /* 818F85 tay */
    plx16();  /* 818F86 plx */
    lda16(ea_absy(0x8000));  /* 818F87 lda */
    pha16();  /* 818F8A pha */
    iny16();  /* 818F8B iny */
    iny16();  /* 818F8C iny */
    stz16(ea_dp(0x18));  /* 818F8D stz */
    lda16(ea_absy(0x8000));  /* 818F8F lda */
    and16_v(0x00FF);  /* 818F92 and */
    clc();  /* 818F95 clc */
    adc16_v(0x00A6);  /* 818F96 adc */
    sta16(ea_dp(0x1A));  /* 818F99 sta */
    pla16();  /* 818F9B pla */
    clc();  /* 818F9C clc */
    adc16_v(0x8000);  /* 818F9D adc */
    tay16();  /* 818FA0 tay */
    lda16(ea_dpily(0x18));  /* 818FA1 lda */
    sta16(ea_dp(0x16));  /* 818FA3 sta */
    iny16();  /* 818FA5 iny */
    iny16();  /* 818FA6 iny */
    clc();  /* 818FA7 clc */
    cpy16_v(0x8000);  /* 818FA8 cpy */
    if (IF_C) goto L_818FB5;  /* 818FAB bcs */
    /* 818FAD-818FB4: no script runs these (no set's byte count sits at
       a bank's last two bytes); the same steps as 818FC7-818FCD. */
    tya16();  /* 818FAD tya */
    clc();  /* 818FAE clc */
    adc16_v(0x8000);  /* 818FAF adc */
    tay16();  /* 818FB2 tay */
    MEM16(inc, ea_dp(0x1A));  /* 818FB3 inc */
L_818FB5:
    op_sep(0x20);  /* 818FB5 sep */
    lda8(ea_dpily(0x18));  /* 818FB7 lda */
    sta8(ea_longx(PICTURE_LIST));  /* 818FB9 sta */
    inx16();  /* 818FBD inx */
    iny16();  /* 818FBE iny */
    clc();  /* 818FBF clc */
    cpy16_v(0x8000);  /* 818FC0 cpy */
    if (IF_C) goto L_818FCF;  /* 818FC3 bcs */
    op_rep(0x20);  /* 818FC5 rep */
    tya16();  /* 818FC7 tya */
    clc();  /* 818FC8 clc */
    adc16_v(0x8000);  /* 818FC9 adc */
    tay16();  /* 818FCC tay */
    MEM16(inc, ea_dp(0x1A));  /* 818FCD inc */
L_818FCF:
    op_rep(0x20);  /* 818FCF rep */
    MEM16(dec, ea_dp(0x16));  /* 818FD1 dec */
    if (!IF_Z) goto L_818FB5;  /* 818FD3 bne */
    MEM16(dec, ea_dp(0x0C));  /* 818FD5 dec */
    if (!IF_Z) goto L_818F6F;  /* 818FD7 bne */
    op_sep(0x30);  /* 818FD9 sep */
    lda8_v(0xA5);  /* 818FDB lda */
    pha8();  /* 818FDD pha */
    plb();  /* 818FDE plb */
    abi_AnimSetsListEnd();  /* 818FDF jsr */
    op_rep(0x30);  /* 818FE2 rep */
    txa16();  /* 818FE4 txa */
    and16_v(0x00FF);  /* 818FE5 and */
    ACC16(lsr);  /* 818FE8 lsr */
    ACC16(dec);  /* 818FE9 dec */
    sta16(ea_dp(0x0C));  /* 818FEA sta */
    stz16(ea_dp(0x10));  /* 818FEC stz */
    ldx16_v(0x0000);  /* 818FEE ldx */
L_818FF1:
    phx16();  /* 818FF1 phx */
    txa16();  /* 818FF2 txa */
    ldx16(ea_dp(0x10));  /* 818FF3 ldx */
    MEM16(inc, ea_dp(0x10));  /* 818FF5 inc */
    MEM16(inc, ea_dp(0x10));  /* 818FF7 inc */
    sta16(ea_longx(ANIM_SET_TABLE));  /* 818FF9 sta */
    lda16(ea_longx(ANIM_SETS_USED));  /* 818FFD lda */
    ACC16(asl);  /* 819001 asl */
    tay16();  /* 819002 tay */
    plx16();  /* 819003 plx */
    lda16(ea_absy(0xC000));  /* 819004 lda */
    tay16();  /* 819007 tay */
    lda16(ea_absy(0xC000));  /* 819008 lda */
    ACC16(lsr);  /* 81900B lsr */
    ACC16(inc);  /* 81900C inc */
    sta16(ea_dp(0x16));  /* 81900D sta */
    iny16();  /* 81900F iny */
    iny16();  /* 819010 iny */
L_819011:
    lda16(ea_absy(0xC000));  /* 819011 lda */
    sta16(ea_longx(ANIM_DATA));  /* 819014 sta */
    iny16();  /* 819018 iny */
    iny16();  /* 819019 iny */
    inx16();  /* 81901A inx */
    inx16();  /* 81901B inx */
    MEM16(dec, ea_dp(0x16));  /* 81901C dec */
    if (!IF_Z) goto L_819011;  /* 81901E bne */
    MEM16(dec, ea_dp(0x0C));  /* 819020 dec */
    if (!IF_Z) goto L_818FF1;  /* 819022 bne */
    op_sep(0x30);  /* 819024 sep */
    abi_AnimSetsPlaces();  /* 819026 jsr */
    plp();  /* 819029 plp */
    plb();  /* 81902A plb */
    return;  /* 81902B rtl */
}

/*
 * kind30.c: actor kind $30, the pickups: by type, one with only its
 * sound, a once-only item, a counted item, a health mark, health filled
 * or one more, and the tens of a BCD count. Its wait is Kind30Wait
 * ($83:B94A).
 */
#include "machine.h"
#include "native.h"
#include "ops65.h"
#include "wram.h"
#include "wram_state.h"
#include "actor.h"

void abi_ActorCreateChild8(void);
void abi_ActorEndPlaced(void);
void abi_ActorFly(void);
void abi_ActorHitsTestBoxLong(void);
void abi_ActorMotionPreset(void);
void abi_ActorOutOfGround(void);
void abi_ActorOutOfWallProbe4(void);
void abi_AnimStart(void);
void abi_SoundQueueAdd(void);

#define K30_16 0x16u              /* word: $D17A */
#define K30_SHOT 0x43u            /* nonzero once a shot has hit it (type 7's wait sets it) */
#define K30_COUNT 0x0364u         /* counted up by each type 2 taken, to at most 100 */
#define K30_TENS 0x0369u          /* BCD; types 6 and 7 add $10, to at most $99 */
#define K30_ONCE 0x036Eu          /* set by a type 1 taken; then type 1 starts as type 5 */
#define K30_LEVEL_MARKS 0x0358u   /* a bit per level whose health mark (type 3) is taken */
#define K30_LEVEL_BITS 0xD182u    /* the level's bit in K30_LEVEL_MARKS, by LEVEL_035C (bytes, bank DB) */
#define K30_FALL_MOTIONS 0xD188u  /* types 6 and 7: the first drop's motion preset, by ACTOR_SPAWN_AT (bank DB) */
#define PLACED_FLAGS 0x7EC600u    /* a byte per placement: bit 7 set, never placed again */

/* Every JSL below puts its return address on the stack while the callee
   runs (jsl_frame), as the original's does, and steps S back after. */

/*
 * @routine 83:B684 Kind30Start
 * @actor
 * WHAT IT DOES: Actor kind $30, the pickups. Its type (bits 0-2 of $07)
 * picks its picture and what taking it does; bits 3-4 how it appears:
 * none set, it pops up and drops twice (ActorMotionPreset $27, then $29;
 * types 6 and 7 the first by ACTOR_SPAWN_AT); $08, it turns round and
 * drops once ($2C); $10, it is there at once. Then it waits until
 * Aladdin touches it with health left, plays its sound and ends; types
 * 0-5 are not placed again. Type 0: nothing more (sound $0B). Type 1:
 * K30_ONCE set (sound 9); once that is set, a type 1 starts as type 5.
 * Type 2: AnimStart 0 and palette 1 (look bits 1-3); taken, K30_COUNT
 * up, to at most 100; below 100 also $07 = $10, ACTOR_SPAWN_AT 0 and
 * ActorCreateChild8 with A = $85 (sound 9; sound $0B at 100). Type 3: a
 * health mark, once a level (the level's bit in K30_LEVEL_MARKS):
 * HEALTH_MARKS up, to at most 10, and HEALTH filled to them (sound 9);
 * the level's bit already set, it starts as type 4. Type 4: HEALTH
 * filled to HEALTH_MARKS (sound $0B). Type 5: HEALTH up by one, to at
 * most HEALTH_MARKS (sound $0B). Types 6 and 7: K30_TENS up by $10 (BCD
 * tens), to at most $99 (sound $0B), and they may be placed again. Type
 * 6 that has dropped drops again; type 7 waits (type 6 too with $10),
 * and once a shot has hit it (K30_SHOT, set by type 7's wait) it hops
 * ($2C, then $29), turning at a wall.
 * HOW: K30_16 = $D17A, ACTOR_LOOK = $0400 (words); ACTOR_08 = 3;
 * ACTOR_09 bits $90 set; AnimStart with the type; ACTOR_FLIP 0; JMP
 * (.starts,X) by type * 2. Appearing: bits $18 clear: ActorMotionPreset
 * Y = $27 (types 6, 7: K30_SHOT cleared, Y from K30_FALL_MOTIONS by
 * ACTOR_SPAWN_AT), each frame (BRK) ActorFly, ActorOutOfWallProbe4,
 * ActorOutOfGround until on the ground; then Y = $29 the same; bit 3
 * only: ACTOR_FACING up by one, Y = $2C the same; then JMP (.waits,X) by
 * type * 2. Waiting: each frame (BRK) ActorHitsTestBoxLong until C clear
 * with HEALTH nonzero; then the type's effect, SoundQueueAdd,
 * PLACED_FLAGS by ACTOR_34 = $80 (not for types 6, 7), and JML
 * ActorEndPlaced. Waiting at .tens (type 7, or 6 with $10) not touched
 * (C set) with K30_SHOT nonzero: ACTOR_FLIP 0; Y = $2C, each frame
 * ActorFly, ActorOutOfWallProbe4 (C set: ACTOR_FACING bit 0 turned),
 * ActorOutOfGround until on the ground; then Y = $29 the same (no turn);
 * K30_SHOT cleared. Kind $30's second state is the JML at .second.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
 * the actor loop's jump for kind $30 in its start state (the table at
 * $83:8CAB). Exit: never returns.
 */
void abi_Kind30Start(void)
{
    op_rep(0x20);  /* 83B684 rep */
    lda16_v(0xD17A);  /* 83B686 lda */
    sta16(ea_dp(K30_16));  /* 83B689 sta */
    lda16_v(0x0400);  /* 83B68B lda */
    sta16(ea_dp(ACTOR_LOOK));  /* 83B68E sta */
    op_sep(0x20);  /* 83B690 sep */
    lda8_v(0x03);  /* 83B692 lda */
    sta8(ea_dp(ACTOR_08));  /* 83B694 sta */
    lda8_v(0x90);  /* 83B696 lda */
    tsb8(ea_dp(ACTOR_09));  /* 83B698 tsb */
    lda8(ea_dp(ACTOR_07));  /* 83B69A lda */
    and8_v(0x07);  /* 83B69C and */
    jsl_frame(0x83B69Eu); abi_AnimStart(); cpu.s = (uint16_t)(cpu.s + 3);  /* 83B69E jsl */
    stz8(ea_dp(ACTOR_FLIP));  /* 83B6A2 stz */
    lda8(ea_dp(ACTOR_07));  /* 83B6A4 lda */
    and8_v(0x07);  /* 83B6A6 and */
    ACC8(asl);  /* 83B6A8 asl */
    tax8();  /* 83B6A9 tax */
    switch (cpu.x) {  /* 83B6AA jmp (B6AD,x) */
    case 0: goto L_83B6BD;
    case 2: goto L_83B6EB;
    case 4: goto L_83B732;
    case 6: goto L_83B788;
    case 8: goto L_83B7EC;
    case 10: goto L_83B820;
    case 12: goto L_83B859;
    case 14: goto L_83B859;
    }
L_83B6BD:
    lda8(ea_dp(ACTOR_07));  /* 83B6BD lda */
    and8_v(0x18);  /* 83B6BF and */
    if (!IF_Z) goto L_83B6C6;  /* 83B6C1 bne */
    goto L_83B8A8;  /* 83B6C3 jmp */
L_83B6C6:
    and8_v(0x10);  /* 83B6C6 and */
    if (!IF_Z) goto L_83B6CD;  /* 83B6C8 bne */
    goto L_83B8D6;  /* 83B6CA jmp */
L_83B6CD:
    actor_yield(0x83B6CDu);  /* 83B6CD brk */
    jsl_frame(0x83B6CFu); abi_ActorHitsTestBoxLong(); cpu.s = (uint16_t)(cpu.s + 3);  /* 83B6CF jsl */
    if (IF_C) goto L_83B6CD;  /* 83B6D3 bcs */
    lda8(ea_abs(HEALTH));  /* 83B6D5 lda */
    if (IF_Z) goto L_83B6CD;  /* 83B6D8 beq */
    lda8_v(0x0B);  /* 83B6DA lda */
    jsl_frame(0x83B6DCu); abi_SoundQueueAdd(); cpu.s = (uint16_t)(cpu.s + 3);  /* 83B6DC jsl */
    ldx8(ea_dp(ACTOR_34));  /* 83B6E0 ldx */
    lda8_v(0x80);  /* 83B6E2 lda */
    sta8(ea_longx(PLACED_FLAGS));  /* 83B6E4 sta */
    goto L_83B946;  /* 83B6E8 jmp */
L_83B6EB:
    lda8(ea_abs(K30_ONCE));  /* 83B6EB lda */
    if (IF_Z) goto L_83B6FF;  /* 83B6EE beq */
    lda8(ea_dp(ACTOR_07));  /* 83B6F0 lda */
    ora8_v(0x04);  /* 83B6F2 ora */
    sta8(ea_dp(ACTOR_07));  /* 83B6F4 sta */
    and8_v(0x07);  /* 83B6F6 and */
    jsl_frame(0x83B6F8u); abi_AnimStart(); cpu.s = (uint16_t)(cpu.s + 3);  /* 83B6F8 jsl */
    goto L_83B820;  /* 83B6FC jmp */
L_83B6FF:
    lda8(ea_dp(ACTOR_07));  /* 83B6FF lda */
    and8_v(0x18);  /* 83B701 and */
    if (!IF_Z) goto L_83B708;  /* 83B703 bne */
    goto L_83B8A8;  /* 83B705 jmp */
L_83B708:
    and8_v(0x10);  /* 83B708 and */
    if (!IF_Z) goto L_83B70F;  /* 83B70A bne */
    goto L_83B8D6;  /* 83B70C jmp */
L_83B70F:
    actor_yield(0x83B70Fu);  /* 83B70F brk */
    jsl_frame(0x83B711u); abi_ActorHitsTestBoxLong(); cpu.s = (uint16_t)(cpu.s + 3);  /* 83B711 jsl */
    if (IF_C) goto L_83B70F;  /* 83B715 bcs */
    lda8(ea_abs(HEALTH));  /* 83B717 lda */
    if (IF_Z) goto L_83B70F;  /* 83B71A beq */
    lda8_v(0x01);  /* 83B71C lda */
    sta8(ea_abs(K30_ONCE));  /* 83B71E sta */
    lda8_v(0x09);  /* 83B721 lda */
    jsl_frame(0x83B723u); abi_SoundQueueAdd(); cpu.s = (uint16_t)(cpu.s + 3);  /* 83B723 jsl */
    ldx8(ea_dp(ACTOR_34));  /* 83B727 ldx */
    lda8_v(0x80);  /* 83B729 lda */
    sta8(ea_longx(PLACED_FLAGS));  /* 83B72B sta */
    goto L_83B946;  /* 83B72F jmp */
L_83B732:
    lda8(ea_dp(ACTOR_LOOK + 1));  /* 83B732 lda */
    and8_v(0xF1);  /* 83B734 and */
    ora8_v(0x02);  /* 83B736 ora */
    sta8(ea_dp(ACTOR_LOOK + 1));  /* 83B738 sta */
    lda8_v(0x00);  /* 83B73A lda */
    jsl_frame(0x83B73Cu); abi_AnimStart(); cpu.s = (uint16_t)(cpu.s + 3);  /* 83B73C jsl */
    lda8(ea_dp(ACTOR_07));  /* 83B740 lda */
    and8_v(0x18);  /* 83B742 and */
    if (!IF_Z) goto L_83B749;  /* 83B744 bne */
    goto L_83B8A8;  /* 83B746 jmp */
L_83B749:
    and8_v(0x10);  /* 83B749 and */
    if (!IF_Z) goto L_83B750;  /* 83B74B bne */
    goto L_83B8D6;  /* 83B74D jmp */
L_83B750:
    actor_yield(0x83B750u);  /* 83B750 brk */
    jsl_frame(0x83B752u); abi_ActorHitsTestBoxLong(); cpu.s = (uint16_t)(cpu.s + 3);  /* 83B752 jsl */
    if (IF_C) goto L_83B750;  /* 83B756 bcs */
    lda8(ea_abs(HEALTH));  /* 83B758 lda */
    if (IF_Z) goto L_83B750;  /* 83B75B beq */
    lda8(ea_abs(K30_COUNT));  /* 83B75D lda */
    cmp8_v(0x64);  /* 83B760 cmp */
    if (IF_Z) goto L_83B777;  /* 83B762 beq */
    MEM8(inc, ea_abs(K30_COUNT));  /* 83B764 inc */
    lda8_v(0x10);  /* 83B767 lda */
    sta8(ea_dp(ACTOR_07));  /* 83B769 sta */
    stz8(ea_dp(ACTOR_SPAWN_AT));  /* 83B76B stz */
    lda8_v(0x85);  /* 83B76D lda */
    jsl_frame(0x83B76Fu); abi_ActorCreateChild8(); cpu.s = (uint16_t)(cpu.s + 3);  /* 83B76F jsl */
    lda8_v(0x09);  /* 83B773 lda */
    goto L_83B779;  /* 83B775 bra */
L_83B777:
    lda8_v(0x0B);  /* 83B777 lda */
L_83B779:
    jsl_frame(0x83B779u); abi_SoundQueueAdd(); cpu.s = (uint16_t)(cpu.s + 3);  /* 83B779 jsl */
    ldx8(ea_dp(ACTOR_34));  /* 83B77D ldx */
    lda8_v(0x80);  /* 83B77F lda */
    sta8(ea_longx(PLACED_FLAGS));  /* 83B781 sta */
    goto L_83B946;  /* 83B785 jmp */
L_83B788:
    ldx8(ea_abs(LEVEL_035C));  /* 83B788 ldx */
    lda8(ea_absx(K30_LEVEL_BITS));  /* 83B78B lda */
    and8_v(rd8(ea_abs(K30_LEVEL_MARKS)));  /* 83B78E and */
    if (IF_Z) goto L_83B7A5;  /* 83B791 beq */
    lda8_v(0x04);  /* 83B793 lda */
    jsl_frame(0x83B795u); abi_AnimStart(); cpu.s = (uint16_t)(cpu.s + 3);  /* 83B795 jsl */
    lda8(ea_dp(ACTOR_07));  /* 83B799 lda */
    and8_v(0xF8);  /* 83B79B and */
    clc();  /* 83B79D clc */
    adc8_v(0x04);  /* 83B79E adc */
    sta8(ea_dp(ACTOR_07));  /* 83B7A0 sta */
    goto L_83B7EC;  /* 83B7A2 jmp */
L_83B7A5:
    lda8(ea_dp(ACTOR_07));  /* 83B7A5 lda */
    and8_v(0x18);  /* 83B7A7 and */
    if (!IF_Z) goto L_83B7AE;  /* 83B7A9 bne */
    goto L_83B8A8;  /* 83B7AB jmp */
L_83B7AE:
    and8_v(0x10);  /* 83B7AE and */
    if (!IF_Z) goto L_83B7B5;  /* 83B7B0 bne */
    goto L_83B8D6;  /* 83B7B2 jmp */
L_83B7B5:
    actor_yield(0x83B7B5u);  /* 83B7B5 brk */
    jsl_frame(0x83B7B7u); abi_ActorHitsTestBoxLong(); cpu.s = (uint16_t)(cpu.s + 3);  /* 83B7B7 jsl */
    if (IF_C) goto L_83B7B5;  /* 83B7BB bcs */
    lda8(ea_abs(HEALTH));  /* 83B7BD lda */
    if (IF_Z) goto L_83B7B5;  /* 83B7C0 beq */
    lda8(ea_abs(HEALTH_MARKS));  /* 83B7C2 lda */
    cmp8_v(0x0A);  /* 83B7C5 cmp */
    if (IF_Z) goto L_83B7CC;  /* 83B7C7 beq */
    MEM8(inc, ea_abs(HEALTH_MARKS));  /* 83B7C9 inc */
L_83B7CC:
    ldx8(ea_abs(LEVEL_035C));  /* 83B7CC ldx */
    lda8(ea_absx(K30_LEVEL_BITS));  /* 83B7CF lda */
    tsb8(ea_abs(K30_LEVEL_MARKS));  /* 83B7D2 tsb */
    lda8(ea_abs(HEALTH_MARKS));  /* 83B7D5 lda */
    sta8(ea_abs(HEALTH));  /* 83B7D8 sta */
    lda8_v(0x09);  /* 83B7DB lda */
    jsl_frame(0x83B7DDu); abi_SoundQueueAdd(); cpu.s = (uint16_t)(cpu.s + 3);  /* 83B7DD jsl */
    ldx8(ea_dp(ACTOR_34));  /* 83B7E1 ldx */
    lda8_v(0x80);  /* 83B7E3 lda */
    sta8(ea_longx(PLACED_FLAGS));  /* 83B7E5 sta */
    goto L_83B946;  /* 83B7E9 jmp */
L_83B7EC:
    lda8(ea_dp(ACTOR_07));  /* 83B7EC lda */
    and8_v(0x18);  /* 83B7EE and */
    if (!IF_Z) goto L_83B7F5;  /* 83B7F0 bne */
    goto L_83B8A8;  /* 83B7F2 jmp */
L_83B7F5:
    and8_v(0x10);  /* 83B7F5 and */
    if (!IF_Z) goto L_83B7FC;  /* 83B7F7 bne */
    goto L_83B8D6;  /* 83B7F9 jmp */
L_83B7FC:
    actor_yield(0x83B7FCu);  /* 83B7FC brk */
    jsl_frame(0x83B7FEu); abi_ActorHitsTestBoxLong(); cpu.s = (uint16_t)(cpu.s + 3);  /* 83B7FE jsl */
    if (IF_C) goto L_83B7FC;  /* 83B802 bcs */
    lda8(ea_abs(HEALTH));  /* 83B804 lda */
    if (IF_Z) goto L_83B7FC;  /* 83B807 beq */
    lda8(ea_abs(HEALTH_MARKS));  /* 83B809 lda */
    sta8(ea_abs(HEALTH));  /* 83B80C sta */
    lda8_v(0x0B);  /* 83B80F lda */
    jsl_frame(0x83B811u); abi_SoundQueueAdd(); cpu.s = (uint16_t)(cpu.s + 3);  /* 83B811 jsl */
    ldx8(ea_dp(ACTOR_34));  /* 83B815 ldx */
    lda8_v(0x80);  /* 83B817 lda */
    sta8(ea_longx(PLACED_FLAGS));  /* 83B819 sta */
    goto L_83B946;  /* 83B81D jmp */
L_83B820:
    lda8(ea_dp(ACTOR_07));  /* 83B820 lda */
    and8_v(0x18);  /* 83B822 and */
    if (!IF_Z) goto L_83B829;  /* 83B824 bne */
    goto L_83B8A8;  /* 83B826 jmp */
L_83B829:
    and8_v(0x10);  /* 83B829 and */
    if (!IF_Z) goto L_83B830;  /* 83B82B bne */
    goto L_83B8D6;  /* 83B82D jmp */
L_83B830:
    actor_yield(0x83B830u);  /* 83B830 brk */
    jsl_frame(0x83B832u); abi_ActorHitsTestBoxLong(); cpu.s = (uint16_t)(cpu.s + 3);  /* 83B832 jsl */
    if (IF_C) goto L_83B830;  /* 83B836 bcs */
    lda8(ea_abs(HEALTH));  /* 83B838 lda */
    if (IF_Z) goto L_83B830;  /* 83B83B beq */
    lda8(ea_abs(HEALTH_MARKS));  /* 83B83D lda */
    cmp8_v(rd8(ea_abs(HEALTH)));  /* 83B840 cmp */
    if (IF_Z) goto L_83B848;  /* 83B843 beq */
    MEM8(inc, ea_abs(HEALTH));  /* 83B845 inc */
L_83B848:
    lda8_v(0x0B);  /* 83B848 lda */
    jsl_frame(0x83B84Au); abi_SoundQueueAdd(); cpu.s = (uint16_t)(cpu.s + 3);  /* 83B84A jsl */
    ldx8(ea_dp(ACTOR_34));  /* 83B84E ldx */
    lda8_v(0x80);  /* 83B850 lda */
    sta8(ea_longx(PLACED_FLAGS));  /* 83B852 sta */
    goto L_83B946;  /* 83B856 jmp */
L_83B859:
    stz8(ea_dp(K30_SHOT));  /* 83B859 stz */
    lda8(ea_dp(ACTOR_07));  /* 83B85B lda */
    and8_v(0x18);  /* 83B85D and */
    if (!IF_Z) goto L_83B86D;  /* 83B85F bne */
    ldx8(ea_dp(ACTOR_SPAWN_AT));  /* 83B861 ldx */
    ldy8(ea_absx(K30_FALL_MOTIONS));  /* 83B863 ldy */
    jsl_frame(0x83B866u); abi_ActorMotionPreset(); cpu.s = (uint16_t)(cpu.s + 3);  /* 83B866 jsl */
    goto L_83B8AA;  /* 83B86A jmp */
L_83B86D:
    and8_v(0x10);  /* 83B86D and */
    if (!IF_Z) goto L_83B874;  /* 83B86F bne */
    goto L_83B8D6;  /* 83B871 jmp */
L_83B874:
    actor_yield(0x83B874u);  /* 83B874 brk */
    jsl_frame(0x83B876u); abi_ActorHitsTestBoxLong(); cpu.s = (uint16_t)(cpu.s + 3);  /* 83B876 jsl */
    if (!IF_C) goto L_83B87F;  /* 83B87A bcc */
    goto L_83B907;  /* 83B87C jmp */
L_83B87F:
    lda8(ea_abs(HEALTH));  /* 83B87F lda */
    if (IF_Z) goto L_83B874;  /* 83B882 beq */
    lda8(ea_abs(K30_TENS));  /* 83B884 lda */
    cmp8_v(0x99);  /* 83B887 cmp */
    if (IF_Z) goto L_83B89F;  /* 83B889 beq */
    clc();  /* 83B88B clc */
    lda8(ea_abs(K30_TENS));  /* 83B88C lda */
    adc8_v(0x10);  /* 83B88F adc */
    sta8(ea_abs(K30_TENS));  /* 83B891 sta */
    and8_v(0xF0);  /* 83B894 and */
    cmp8_v(0xA0);  /* 83B896 cmp */
    if (!IF_Z) goto L_83B89F;  /* 83B898 bne */
    lda8_v(0x99);  /* 83B89A lda */
    sta8(ea_abs(K30_TENS));  /* 83B89C sta */
L_83B89F:
    lda8_v(0x0B);  /* 83B89F lda */
    jsl_frame(0x83B8A1u); abi_SoundQueueAdd(); cpu.s = (uint16_t)(cpu.s + 3);  /* 83B8A1 jsl */
    goto L_83B946;  /* 83B8A5 jmp */
L_83B8A8:
    ldy8_v(0x27);  /* 83B8A8 ldy */
L_83B8AA:
    jsl_frame(0x83B8AAu); abi_ActorMotionPreset(); cpu.s = (uint16_t)(cpu.s + 3);  /* 83B8AA jsl */
L_83B8AE:
    actor_yield(0x83B8AEu);  /* 83B8AE brk */
    jsl_frame(0x83B8B0u); abi_ActorFly(); cpu.s = (uint16_t)(cpu.s + 3);  /* 83B8B0 jsl */
    jsl_frame(0x83B8B4u); abi_ActorOutOfWallProbe4(); cpu.s = (uint16_t)(cpu.s + 3);  /* 83B8B4 jsl */
    jsl_frame(0x83B8B8u); abi_ActorOutOfGround(); cpu.s = (uint16_t)(cpu.s + 3);  /* 83B8B8 jsl */
    if (IF_Z) goto L_83B8AE;  /* 83B8BC beq */
    ldy8_v(0x29);  /* 83B8BE ldy */
    jsl_frame(0x83B8C0u); abi_ActorMotionPreset(); cpu.s = (uint16_t)(cpu.s + 3);  /* 83B8C0 jsl */
L_83B8C4:
    actor_yield(0x83B8C4u);  /* 83B8C4 brk */
    jsl_frame(0x83B8C6u); abi_ActorFly(); cpu.s = (uint16_t)(cpu.s + 3);  /* 83B8C6 jsl */
    jsl_frame(0x83B8CAu); abi_ActorOutOfWallProbe4(); cpu.s = (uint16_t)(cpu.s + 3);  /* 83B8CA jsl */
    jsl_frame(0x83B8CEu); abi_ActorOutOfGround(); cpu.s = (uint16_t)(cpu.s + 3);  /* 83B8CE jsl */
    if (IF_Z) goto L_83B8C4;  /* 83B8D2 beq */
    goto L_83B8EE;  /* 83B8D4 bra */
L_83B8D6:
    MEM8(inc, ea_dp(ACTOR_FACING));  /* 83B8D6 inc */
    ldy8_v(0x2C);  /* 83B8D8 ldy */
    jsl_frame(0x83B8DAu); abi_ActorMotionPreset(); cpu.s = (uint16_t)(cpu.s + 3);  /* 83B8DA jsl */
L_83B8DE:
    actor_yield(0x83B8DEu);  /* 83B8DE brk */
    jsl_frame(0x83B8E0u); abi_ActorFly(); cpu.s = (uint16_t)(cpu.s + 3);  /* 83B8E0 jsl */
    jsl_frame(0x83B8E4u); abi_ActorOutOfWallProbe4(); cpu.s = (uint16_t)(cpu.s + 3);  /* 83B8E4 jsl */
    jsl_frame(0x83B8E8u); abi_ActorOutOfGround(); cpu.s = (uint16_t)(cpu.s + 3);  /* 83B8E8 jsl */
    if (IF_Z) goto L_83B8DE;  /* 83B8EC beq */
L_83B8EE:
    lda8(ea_dp(ACTOR_07));  /* 83B8EE lda */
    and8_v(0x07);  /* 83B8F0 and */
    ACC8(asl);  /* 83B8F2 asl */
    tax8();  /* 83B8F3 tax */
    switch (cpu.x) {  /* 83B8F4 jmp (B8F7,x) */
    case 0: goto L_83B6CD;
    case 2: goto L_83B70F;
    case 4: goto L_83B750;
    case 6: goto L_83B7B5;
    case 8: goto L_83B7FC;
    case 10: goto L_83B830;
    case 12: goto L_83B859;
    case 14: goto L_83B874;
    }
L_83B907:
    lda8(ea_dp(K30_SHOT));  /* 83B907 lda */
    if (IF_Z) goto L_83B943;  /* 83B909 beq */
    stz8(ea_dp(ACTOR_FLIP));  /* 83B90B stz */
    ldy8_v(0x2C);  /* 83B90D ldy */
    jsl_frame(0x83B90Fu); abi_ActorMotionPreset(); cpu.s = (uint16_t)(cpu.s + 3);  /* 83B90F jsl */
L_83B913:
    actor_yield(0x83B913u);  /* 83B913 brk */
    jsl_frame(0x83B915u); abi_ActorFly(); cpu.s = (uint16_t)(cpu.s + 3);  /* 83B915 jsl */
    jsl_frame(0x83B919u); abi_ActorOutOfWallProbe4(); cpu.s = (uint16_t)(cpu.s + 3);  /* 83B919 jsl */
    if (!IF_C) goto L_83B925;  /* 83B91D bcc */
    lda8(ea_dp(ACTOR_FACING));  /* 83B91F lda */
    eor8_v(0x01);  /* 83B921 eor */
    sta8(ea_dp(ACTOR_FACING));  /* 83B923 sta */
L_83B925:
    jsl_frame(0x83B925u); abi_ActorOutOfGround(); cpu.s = (uint16_t)(cpu.s + 3);  /* 83B925 jsl */
    if (IF_Z) goto L_83B913;  /* 83B929 beq */
    ldy8_v(0x29);  /* 83B92B ldy */
    jsl_frame(0x83B92Du); abi_ActorMotionPreset(); cpu.s = (uint16_t)(cpu.s + 3);  /* 83B92D jsl */
L_83B931:
    actor_yield(0x83B931u);  /* 83B931 brk */
    jsl_frame(0x83B933u); abi_ActorFly(); cpu.s = (uint16_t)(cpu.s + 3);  /* 83B933 jsl */
    jsl_frame(0x83B937u); abi_ActorOutOfWallProbe4(); cpu.s = (uint16_t)(cpu.s + 3);  /* 83B937 jsl */
    jsl_frame(0x83B93Bu); abi_ActorOutOfGround(); cpu.s = (uint16_t)(cpu.s + 3);  /* 83B93B jsl */
    if (IF_Z) goto L_83B931;  /* 83B93F beq */
    stz8(ea_dp(K30_SHOT));  /* 83B941 stz */
L_83B943:
    goto L_83B874;  /* 83B943 jmp */
L_83B946:
    abi_ActorEndPlaced();  /* 83B946 jml */
    return;
}

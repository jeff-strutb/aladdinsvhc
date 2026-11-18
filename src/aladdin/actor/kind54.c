/*
 * kind54.c: actor kinds $54 and $55, things that leap from a spot (kind
 * $54 straight up, kind $55 in an arc), trailing copies of themselves,
 * and come back down to leap again; with $07 bit 3 set, one that only
 * drops and ends. Kind $54 notes a copier's cartridge RAM, and then holds
 * Aladdin's y at its own as it turns to fall. The steps are as drafted
 * from the traced assembly (cdraft.py), one per instruction; the fields
 * are named in kind54.asm.
 */
#include "machine.h"
#include "native.h"
#include "ops65.h"

void abi_ActorEndPlaced(void);
void abi_ActorFall(void);
void abi_ActorFly(void);
void abi_ActorMotionReset(void);
void abi_ActorSpawn5E(void);
void abi_ActorSpawnCopy(void);
void abi_ActorSpeedXSet(void);
void abi_ActorSpeedYSet(void);
void abi_AnimStart(void);
void abi_SoundQueueAdd(void);

/* Kinds $54 and $55 with $07 bit 3 set ($83:D7A5, Kind54Drop): they drop
   14 frames and end (ActorEndPlaced, by the JML at $83:D7F7). */
static void kind54_drop(void)
{
    lda8_v(0x03);  /* 83D7A5 lda */
    tsb8(ea_dp(0x08));  /* 83D7A7 tsb */
    lda8_v(0x08);  /* 83D7A9 lda */
    jsl_frame(0x83D7ABu); abi_ActorMotionReset(); cpu.s = (uint16_t)(cpu.s + 3);  /* 83D7AB jsl */
    lda8_v(0x0E);  /* 83D7AF lda */
    sta8(ea_dp(0x40));  /* 83D7B1 sta */
    lda8_v(0x07);  /* 83D7B3 lda */
    actor_wait(0x83D7B5u);  /* 83D7B5 cop */
    lda8_v(0x01);  /* 83D7B7 lda */
    jsl_frame(0x83D7B9u); abi_AnimStart(); cpu.s = (uint16_t)(cpu.s + 3);  /* 83D7B9 jsl */
L_83D7BD:
    actor_yield(0x83D7BDu);  /* 83D7BD brk */
    jsl_frame(0x83D7BFu); abi_ActorFall(); cpu.s = (uint16_t)(cpu.s + 3);  /* 83D7BF jsl */
    MEM8(dec, ea_dp(0x40));  /* 83D7C3 dec */
    if (!IF_Z) goto L_83D7BD;  /* 83D7C5 bne */
    abi_ActorEndPlaced();  /* 83D7C7 bra, 83D7F7 jml */
}

/*
 * @routine 83:D6D7 Kind54Start
 * @actor
 * WHAT IT DOES: Actor kind $54: leaps straight up from where it is placed
 * (speed by $07 bits 0-1, sound $25), trailing a copy every 8 frames, and
 * once back down at its start, sound $26, a pause of 64 frames, and up
 * again; with $07 bit 3 set it drops for 14 frames and ends. A copier's
 * cartridge RAM (the check at K54_CHECK) makes it hold Aladdin's y at its
 * own each time it turns to fall.
 * HOW: The byte at K54_CHECK + X up and compared: equal, K54_COPIER 1,
 * else 0. ACTOR_08 bit 1, ACTOR_09 bits $90 set. $07 bit 3: the drop
 * (below). Else K54_PROBES $D3A4, K54_HOME_Y its y, K54_TRAIL $18, ACTOR_08
 * bit 7 set, bit 3 cleared. Each leap: $07 bit 2 set, ActorSpawn5E, COP 6,
 * ACTOR_08 bit 3 set and bit 7 cleared, AnimStart 2, ActorSpeedYSet by
 * K54_RISES, gravity $20, SoundQueueAdd $25; each frame (BRK) ActorFall;
 * turning to fall: AnimStart 0 and (K54_COPIER) the player's y = its y;
 * K54_TRAIL down, at 0 $08 again and ActorSpawnCopy; until its y is at or
 * below K54_HOME_Y. Its y = that (A); $07 bit 2 cleared, ActorSpawn5E,
 * SoundQueueAdd $26, BRK, ACTOR_08 bit 7 set and bit 3 cleared, COP $40,
 * again. The drop: ACTOR_08 bits 0-1 set, ActorMotionReset 8, K54_FRAMES
 * $0E, COP 7, AnimStart 1, each frame (BRK) ActorFall K54_FRAMES times,
 * then ActorEndPlaced (JML at $83:D7F7).
 * Entry: M 8-bit, X 8-bit (twice its kind); D = the actor's record; DB =
 * $83; reached by the actor loop's jump for kind $54 in state $0C (the
 * table at $83:8CAB). Exit: never returns.
 */
void abi_Kind54Start(void)
{
    lda8(ea_longx(0x703000u));  /* 83D6D7 lda */
    ACC8(inc);  /* 83D6DB inc */
    sta8(ea_longx(0x703000u));  /* 83D6DC sta */
    cmp8_v(rd8(ea_longx(0x703000u)));  /* 83D6E0 cmp */
    if (!IF_Z) goto L_83D6EC;  /* 83D6E4 bne */
    lda8_v(0x01);  /* 83D6E6 lda */
    sta8(ea_dp(0x4F));  /* 83D6E8 sta */
    goto L_83D6EE;  /* 83D6EA bra */
L_83D6EC:
    stz8(ea_dp(0x4F));  /* 83D6EC stz */
L_83D6EE:
    lda8_v(0x02);  /* 83D6EE lda */
    tsb8(ea_dp(0x08));  /* 83D6F0 tsb */
    lda8_v(0x90);  /* 83D6F2 lda */
    tsb8(ea_dp(0x09));  /* 83D6F4 tsb */
    lda8(ea_dp(0x07));  /* 83D6F6 lda */
    and8_v(0x08);  /* 83D6F8 and */
    if (IF_Z) goto L_83D6FF;  /* 83D6FA beq */
    kind54_drop(); return;  /* 83D6FC jmp */
L_83D6FF:
    op_rep(0x20);  /* 83D6FF rep */
    lda16_v(0xD3A4);  /* 83D701 lda */
    sta16(ea_dp(0x16));  /* 83D704 sta */
    lda16(ea_dp(0x0F));  /* 83D706 lda */
    sta16(ea_dp(0x43));  /* 83D708 sta */
    op_sep(0x20);  /* 83D70A sep */
    lda8_v(0x18);  /* 83D70C lda */
    sta8(ea_dp(0x42));  /* 83D70E sta */
    lda8_v(0x80);  /* 83D710 lda */
    tsb8(ea_dp(0x08));  /* 83D712 tsb */
    lda8_v(0x08);  /* 83D714 lda */
    trb8(ea_dp(0x08));  /* 83D716 trb */
L_83D718:
    lda8_v(0x04);  /* 83D718 lda */
    tsb8(ea_dp(0x07));  /* 83D71A tsb */
    call_frame(0xD71E, abi_ActorSpawn5E);  /* 83D71C jsr */
    lda8_v(0x06);  /* 83D71F lda */
    actor_wait(0x83D721u);  /* 83D721 cop */
    lda8_v(0x08);  /* 83D723 lda */
    tsb8(ea_dp(0x08));  /* 83D725 tsb */
    lda8_v(0x80);  /* 83D727 lda */
    trb8(ea_dp(0x08));  /* 83D729 trb */
    lda8_v(0x02);  /* 83D72B lda */
    jsl_frame(0x83D72Du); abi_AnimStart(); cpu.s = (uint16_t)(cpu.s + 3);  /* 83D72D jsl */
    lda8(ea_dp(0x07));  /* 83D731 lda */
    and8_v(0x03);  /* 83D733 and */
    tax8();  /* 83D735 tax */
    ldy8(ea_absx(0xD3A8));  /* 83D736 ldy */
    jsl_frame(0x83D739u); abi_ActorSpeedYSet(); cpu.s = (uint16_t)(cpu.s + 3);  /* 83D739 jsl */
    lda8_v(0x20);  /* 83D73D lda */
    sta8(ea_dp(0x20));  /* 83D73F sta */
    lda8_v(0x25);  /* 83D741 lda */
    jsl_frame(0x83D743u); abi_SoundQueueAdd(); cpu.s = (uint16_t)(cpu.s + 3);  /* 83D743 jsl */
L_83D747:
    actor_yield(0x83D747u);  /* 83D747 brk */
    jsl_frame(0x83D749u); abi_ActorFall(); cpu.s = (uint16_t)(cpu.s + 3);  /* 83D749 jsl */
    lda8(ea_dp(0x1F));  /* 83D74D lda */
    if (IF_N) goto L_83D768;  /* 83D74F bmi */
    lda8(ea_dp(0x41));  /* 83D751 lda */
    if (!IF_N) goto L_83D768;  /* 83D753 bpl */
    lda8_v(0x00);  /* 83D755 lda */
    jsl_frame(0x83D757u); abi_AnimStart(); cpu.s = (uint16_t)(cpu.s + 3);  /* 83D757 jsl */
    lda8(ea_dp(0x4F));  /* 83D75B lda */
    if (IF_Z) goto L_83D768;  /* 83D75D beq */
    op_rep(0x20);  /* 83D75F rep */
    lda16(ea_dp(0x0F));  /* 83D761 lda */
    sta16(ea_abs(0x08EA));  /* 83D763 sta */
    op_sep(0x20);  /* 83D766 sep */
L_83D768:
    lda8(ea_dp(0x1F));  /* 83D768 lda */
    sta8(ea_dp(0x41));  /* 83D76A sta */
    MEM8(dec, ea_dp(0x42));  /* 83D76C dec */
    if (!IF_Z) goto L_83D777;  /* 83D76E bne */
    lda8_v(0x08);  /* 83D770 lda */
    sta8(ea_dp(0x42));  /* 83D772 sta */
    call_frame(0xD776, abi_ActorSpawnCopy);  /* 83D774 jsr */
L_83D777:
    op_rep(0x20);  /* 83D777 rep */
    lda16(ea_dp(0x43));  /* 83D779 lda */
    cmp16_v(rd16(ea_dp(0x0F)));  /* 83D77B cmp */
    op_sep(0x20);  /* 83D77D sep */
    if (IF_C) goto L_83D747;  /* 83D77F bcs */
    op_rep(0x20);  /* 83D781 rep */
    sta16(ea_dp(0x0F));  /* 83D783 sta */
    op_sep(0x20);  /* 83D785 sep */
    lda8_v(0x04);  /* 83D787 lda */
    trb8(ea_dp(0x07));  /* 83D789 trb */
    call_frame(0xD78D, abi_ActorSpawn5E);  /* 83D78B jsr */
    lda8_v(0x26);  /* 83D78E lda */
    jsl_frame(0x83D790u); abi_SoundQueueAdd(); cpu.s = (uint16_t)(cpu.s + 3);  /* 83D790 jsl */
    actor_yield(0x83D794u);  /* 83D794 brk */
    lda8_v(0x80);  /* 83D796 lda */
    tsb8(ea_dp(0x08));  /* 83D798 tsb */
    lda8_v(0x08);  /* 83D79A lda */
    trb8(ea_dp(0x08));  /* 83D79C trb */
    lda8_v(0x40);  /* 83D79E lda */
    actor_wait(0x83D7A0u);  /* 83D7A0 cop */
    goto L_83D718;  /* 83D7A2 jmp */
}

/*
 * @routine 83:D7FB Kind55Start
 * @actor
 * WHAT IT DOES: Actor kind $55: (unless $07 bits 0, 1 and 3 are all
 * clear, after 16 frames) leaps in an arc from where it is placed (across
 * by $07 bits 0-1, facing turned for bit 4; up $96 for variant 2, $3F
 * otherwise; sound $25), trailing a copy every 8 frames; once back down
 * at its start's height, sound $26, a pause of 64 frames, put back at its
 * start, and again. With $07 bit 3 set it drops and ends as kind $54's
 * does.
 * HOW: $07 AND $0B zero: COP $10. ACTOR_08 bit 1, ACTOR_09 bits $90 set.
 * $07 bit 3: Kind54Drop. Else K54_PROBES $D3AC, K54_HOME_X its x,
 * K55_HOME_Y its y, K54_TRAIL $18, ACTOR_08 bit 7 set, bit 3 cleared;
 * $07 bit 4: facing up. Each leap: $07 bit 2 set, ActorSpawn5E, COP 6,
 * ACTOR_08 bit 3 set and bit 7 cleared, AnimStart 2, ActorSpeedXSet by
 * K55_ACROSS, gravity $16, ActorSpeedYSet $96 (variant 2) or $3F,
 * SoundQueueAdd $25; each frame (BRK) ActorFly; turning to fall:
 * AnimStart 0; K54_TRAIL down, at 0 $08 again and ActorSpawnCopy; until
 * its y is at or below K55_HOME_Y. $07 bit 2 cleared, ActorSpawn5E,
 * SoundQueueAdd $26, BRK, ACTOR_08 bit 7 set and bit 3 cleared, COP $40,
 * its x and y back to K54_HOME_X and K55_HOME_Y, again.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $83; reached by
 * the actor loop's jump for kind $55 in state $0C (the table at
 * $83:8CAB). Exit: never returns.
 */
void abi_Kind55Start(void)
{
    lda8(ea_dp(0x07));  /* 83D7FB lda */
    and8_v(0x0B);  /* 83D7FD and */
    if (!IF_Z) goto L_83D805;  /* 83D7FF bne */
    lda8_v(0x10);  /* 83D801 lda */
    actor_wait(0x83D803u);  /* 83D803 cop */
L_83D805:
    lda8_v(0x02);  /* 83D805 lda */
    tsb8(ea_dp(0x08));  /* 83D807 tsb */
    lda8_v(0x90);  /* 83D809 lda */
    tsb8(ea_dp(0x09));  /* 83D80B tsb */
    lda8(ea_dp(0x07));  /* 83D80D lda */
    and8_v(0x08);  /* 83D80F and */
    if (!IF_Z) { kind54_drop(); return; }  /* 83D811 bne */
    op_rep(0x20);  /* 83D813 rep */
    lda16_v(0xD3AC);  /* 83D815 lda */
    sta16(ea_dp(0x16));  /* 83D818 sta */
    lda16(ea_dp(0x0C));  /* 83D81A lda */
    sta16(ea_dp(0x43));  /* 83D81C sta */
    lda16(ea_dp(0x0F));  /* 83D81E lda */
    sta16(ea_dp(0x45));  /* 83D820 sta */
    op_sep(0x20);  /* 83D822 sep */
    lda8_v(0x18);  /* 83D824 lda */
    sta8(ea_dp(0x42));  /* 83D826 sta */
    lda8_v(0x80);  /* 83D828 lda */
    tsb8(ea_dp(0x08));  /* 83D82A tsb */
    lda8_v(0x08);  /* 83D82C lda */
    trb8(ea_dp(0x08));  /* 83D82E trb */
    lda8(ea_dp(0x07));  /* 83D830 lda */
    and8_v(0x10);  /* 83D832 and */
    if (IF_Z) goto L_83D838;  /* 83D834 beq */
    MEM8(inc, ea_dp(0x14));  /* 83D836 inc */
L_83D838:
    lda8_v(0x04);  /* 83D838 lda */
    tsb8(ea_dp(0x07));  /* 83D83A tsb */
    call_frame(0xD83E, abi_ActorSpawn5E);  /* 83D83C jsr */
    lda8_v(0x06);  /* 83D83F lda */
    actor_wait(0x83D841u);  /* 83D841 cop */
    lda8_v(0x08);  /* 83D843 lda */
    tsb8(ea_dp(0x08));  /* 83D845 tsb */
    lda8_v(0x80);  /* 83D847 lda */
    trb8(ea_dp(0x08));  /* 83D849 trb */
    lda8_v(0x02);  /* 83D84B lda */
    jsl_frame(0x83D84Du); abi_AnimStart(); cpu.s = (uint16_t)(cpu.s + 3);  /* 83D84D jsl */
    lda8(ea_dp(0x07));  /* 83D851 lda */
    and8_v(0x03);  /* 83D853 and */
    tax8();  /* 83D855 tax */
    ldy8(ea_absx(0xD3B0));  /* 83D856 ldy */
    jsl_frame(0x83D859u); abi_ActorSpeedXSet(); cpu.s = (uint16_t)(cpu.s + 3);  /* 83D859 jsl */
    lda8_v(0x16);  /* 83D85D lda */
    sta8(ea_dp(0x20));  /* 83D85F sta */
    ldy8_v(0x96);  /* 83D861 ldy */
    lda8(ea_dp(0x07));  /* 83D863 lda */
    and8_v(0x03);  /* 83D865 and */
    cmp8_v(0x02);  /* 83D867 cmp */
    if (IF_Z) goto L_83D86D;  /* 83D869 beq */
    ldy8_v(0x3F);  /* 83D86B ldy */
L_83D86D:
    jsl_frame(0x83D86Du); abi_ActorSpeedYSet(); cpu.s = (uint16_t)(cpu.s + 3);  /* 83D86D jsl */
    lda8_v(0x25);  /* 83D871 lda */
    jsl_frame(0x83D873u); abi_SoundQueueAdd(); cpu.s = (uint16_t)(cpu.s + 3);  /* 83D873 jsl */
L_83D877:
    actor_yield(0x83D877u);  /* 83D877 brk */
    jsl_frame(0x83D879u); abi_ActorFly(); cpu.s = (uint16_t)(cpu.s + 3);  /* 83D879 jsl */
    lda8(ea_dp(0x1F));  /* 83D87D lda */
    if (IF_N) goto L_83D88B;  /* 83D87F bmi */
    lda8(ea_dp(0x41));  /* 83D881 lda */
    if (!IF_N) goto L_83D88B;  /* 83D883 bpl */
    lda8_v(0x00);  /* 83D885 lda */
    jsl_frame(0x83D887u); abi_AnimStart(); cpu.s = (uint16_t)(cpu.s + 3);  /* 83D887 jsl */
L_83D88B:
    lda8(ea_dp(0x1F));  /* 83D88B lda */
    sta8(ea_dp(0x41));  /* 83D88D sta */
    MEM8(dec, ea_dp(0x42));  /* 83D88F dec */
    if (!IF_Z) goto L_83D89A;  /* 83D891 bne */
    lda8_v(0x08);  /* 83D893 lda */
    sta8(ea_dp(0x42));  /* 83D895 sta */
    call_frame(0xD899, abi_ActorSpawnCopy);  /* 83D897 jsr */
L_83D89A:
    op_rep(0x20);  /* 83D89A rep */
    lda16(ea_dp(0x45));  /* 83D89C lda */
    cmp16_v(rd16(ea_dp(0x0F)));  /* 83D89E cmp */
    op_sep(0x20);  /* 83D8A0 sep */
    if (IF_C) goto L_83D877;  /* 83D8A2 bcs */
    lda8_v(0x04);  /* 83D8A4 lda */
    trb8(ea_dp(0x07));  /* 83D8A6 trb */
    call_frame(0xD8AA, abi_ActorSpawn5E);  /* 83D8A8 jsr */
    lda8_v(0x26);  /* 83D8AB lda */
    jsl_frame(0x83D8ADu); abi_SoundQueueAdd(); cpu.s = (uint16_t)(cpu.s + 3);  /* 83D8AD jsl */
    actor_yield(0x83D8B1u);  /* 83D8B1 brk */
    lda8_v(0x80);  /* 83D8B3 lda */
    tsb8(ea_dp(0x08));  /* 83D8B5 tsb */
    lda8_v(0x08);  /* 83D8B7 lda */
    trb8(ea_dp(0x08));  /* 83D8B9 trb */
    lda8_v(0x40);  /* 83D8BB lda */
    actor_wait(0x83D8BDu);  /* 83D8BD cop */
    op_rep(0x20);  /* 83D8BF rep */
    lda16(ea_dp(0x43));  /* 83D8C1 lda */
    sta16(ea_dp(0x0C));  /* 83D8C3 sta */
    lda16(ea_dp(0x45));  /* 83D8C5 lda */
    sta16(ea_dp(0x0F));  /* 83D8C7 sta */
    op_sep(0x20);  /* 83D8C9 sep */
    goto L_83D838;  /* 83D8CB jmp */
}

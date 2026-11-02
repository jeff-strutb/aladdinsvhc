/*
 * carrier_pair.c: an actor that sends in a pair: a carrier (kind $38) and
 * the thing it holds (kind $53, actor/kind53.c), each knowing the other.
 * Drafted by cdraft.py; named and described by hand.
 */
#include "machine.h"
#include "ops65.h"
#include "wram.h"
#include "actor.h"
#include "actor_anim.h"

void abi_ActorCreateIn(void);
void abi_ActorSpawnPlace(void);

#define NEW_RECORD 0x0020u  /* the record ActorCreateIn last took (word) */
#define RANGE_X 0x1A44u  /* the point actors are kept in range of (words) */
#define RANGE_Y 0x1A46u
#define PAIR_OTHER 0x4Eu  /* word: the record of the other of the pair */
#define PAIR_SENDER 0x4Cu  /* word: the record of the actor that sent them, 0 here */
#define PAIRS_LIVE 0x4Bu  /* actors it has sent that are live */
#define PAIR_WAIT 0x40u  /* frames until it sends the next */

/*
 * @routine 84:A056 CarrierPairSend
 * WHAT IT DOES: Sends in a carrier (kind $38) 40 pixels above the point
 * actors are kept in range of, and a thing it holds (kind $53) at the
 * carrier's spawn point $44, both new and facing as the sender, each with
 * the other's record in $4E; counts them live and waits $3C frames before
 * the next.
 * HOW: Its own position kept on the stack and set to the range point (y
 * less 40) for the placing; kind $38 by ActorCreateIn (Y = $4A), $4A up by
 * 2; the new record (from $0020) in its $4E; the carrier's $07 = its own
 * plus one, ActorSpawnPlace at $44, state $0C, facing and flip as its own,
 * $45, $46 and $4C (16-bit) cleared; $41 up, $40 = $3C. Then kind $53 the
 * same way, placed at the carrier's spawn point $44 less 4 across and 12
 * down, state $0C, its facing, $4C cleared; the two records crossed into
 * each other's $4E. Its position back, $4B up, and the free list's top up
 * by 4 (both records taken).
 * Entry: M 8-bit, X 8-bit; D = the sender's record; DB = $84; called by
 * JSR (84:9FE7). Exit: M and X 8-bit; A = the free list's new top (low
 * byte); C from its ADC.
 */
void abi_CarrierPairSend(void)
{
    op_rep(0x20);  /* 84A056 rep */
    lda16(ea_dp(ACTOR_X + 1));  /* 84A058 lda */
    pha16();  /* 84A05A pha */
    lda16(ea_dp(ACTOR_Y + 1));  /* 84A05B lda */
    pha16();  /* 84A05D pha */
    lda16(ea_abs(RANGE_X));  /* 84A05E lda */
    sta16(ea_dp(ACTOR_X + 1));  /* 84A061 sta */
    sec();  /* 84A063 sec */
    lda16(ea_abs(RANGE_Y));  /* 84A064 lda */
    sbc16_v(0x0028);  /* 84A067 sbc */
    sta16(ea_dp(ACTOR_Y + 1));  /* 84A06A sta */
    op_sep(0x20);  /* 84A06C sep */
    lda8_v(0x38);  /* 84A06E lda */
    sta8(ea_dp(ACTOR_06));  /* 84A070 sta */
    abi_ActorCreateIn();  /* 84A072 jsl */
    MEM8(inc, ea_dp(ACTOR_STREAM));  /* 84A076 inc */
    MEM8(inc, ea_dp(ACTOR_STREAM));  /* 84A078 inc */
    op_rep(0x10);  /* 84A07A rep */
    ldx16(ea_abs(NEW_RECORD));  /* 84A07C ldx */
    stx16(ea_dp(PAIR_OTHER));  /* 84A07F stx */
    lda8(ea_dp(ACTOR_07));  /* 84A081 lda */
    ACC8(inc);  /* 84A083 inc */
    sta8(ea_absx(ACTOR_07));  /* 84A084 sta */
    lda8_v(0x44);  /* 84A087 lda */
    sta8(ea_dp(ACTOR_SPAWN_AT));  /* 84A089 sta */
    abi_ActorSpawnPlace();  /* 84A08B jsl */
    lda8_v(0x0C);  /* 84A08F lda */
    sta8(ea_absx(ACTOR_STATE));  /* 84A091 sta */
    lda8(ea_dp(ACTOR_FACING));  /* 84A094 lda */
    sta8(ea_absx(ACTOR_FACING));  /* 84A096 sta */
    sta8(ea_absx(ACTOR_FLIP));  /* 84A099 sta */
    stz8(ea_absx(ACTOR_45));  /* 84A09C stz */
    stz8(ea_absx(ACTOR_46));  /* 84A09F stz */
    op_rep(0x20);  /* 84A0A2 rep */
    tdc();  /* 84A0A4 tdc */
    sta16(ea_absx(ACTOR_4C));  /* 84A0A5 sta */
    op_sep(0x20);  /* 84A0A8 sep */
    MEM8(inc, ea_dp(ACTOR_41));  /* 84A0AA inc */
    lda8_v(0x3C);  /* 84A0AC lda */
    sta8(ea_dp(PAIR_WAIT));  /* 84A0AE sta */
    op_sep(0x10);  /* 84A0B0 sep */
    ldy8(ea_dp(ACTOR_STREAM));  /* 84A0B2 ldy */
    lda8_v(0x53);  /* 84A0B4 lda */
    sta8(ea_dp(ACTOR_06));  /* 84A0B6 sta */
    abi_ActorCreateIn();  /* 84A0B8 jsl */
    op_rep(0x10);  /* 84A0BC rep */
    ldx16(ea_abs(NEW_RECORD));  /* 84A0BE ldx */
    lda8_v(0x44);  /* 84A0C1 lda */
    sta8(ea_dp(ACTOR_SPAWN_AT));  /* 84A0C3 sta */
    abi_ActorSpawnPlace();  /* 84A0C5 jsl */
    op_rep(0x21);  /* 84A0C9 rep */
    lda16(ea_absx(ACTOR_X + 1));  /* 84A0CB lda */
    adc16_v(0xFFFC);  /* 84A0CE adc */
    sta16(ea_absx(ACTOR_X + 1));  /* 84A0D1 sta */
    clc();  /* 84A0D4 clc */
    lda16(ea_absx(ACTOR_Y + 1));  /* 84A0D5 lda */
    adc16_v(0x000C);  /* 84A0D8 adc */
    sta16(ea_absx(ACTOR_Y + 1));  /* 84A0DB sta */
    op_sep(0x20);  /* 84A0DE sep */
    lda8_v(0x0C);  /* 84A0E0 lda */
    sta8(ea_absx(ACTOR_STATE));  /* 84A0E2 sta */
    lda8(ea_dp(ACTOR_FACING));  /* 84A0E5 lda */
    sta8(ea_absx(ACTOR_FACING));  /* 84A0E7 sta */
    sta8(ea_absx(ACTOR_FLIP));  /* 84A0EA sta */
    op_rep(0x20);  /* 84A0ED rep */
    tdc();  /* 84A0EF tdc */
    sta16(ea_absx(ACTOR_4C));  /* 84A0F0 sta */
    lda16(ea_dp(PAIR_OTHER));  /* 84A0F3 lda */
    sta16(ea_absx(PAIR_OTHER));  /* 84A0F5 sta */
    txy16();  /* 84A0F8 txy */
    tax16();  /* 84A0F9 tax */
    tya16();  /* 84A0FA tya */
    sta16(ea_absx(PAIR_OTHER));  /* 84A0FB sta */
    pla16();  /* 84A0FE pla */
    sta16(ea_dp(ACTOR_Y + 1));  /* 84A0FF sta */
    pla16();  /* 84A101 pla */
    sta16(ea_dp(ACTOR_X + 1));  /* 84A102 sta */
    op_sep(0x30);  /* 84A104 sep */
    MEM8(inc, ea_dp(PAIRS_LIVE));  /* 84A106 inc */
    clc();  /* 84A108 clc */
    lda8(ea_abs(ACTOR_FREE_TOP));  /* 84A109 lda */
    adc8_v(0x04);  /* 84A10C adc */
    sta8(ea_abs(ACTOR_FREE_TOP));  /* 84A10E sta */
    return;  /* 84A111 rts */
}

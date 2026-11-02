/*
 * backdrop_mover.c: helpers of the actor at $84:9044 that moves the
 * backdrop layer up and down against the play field, and holds its sounds
 * once the camera is past x $08E8.
 */
#include "machine.h"
#include "ops65.h"
void abi_BackdropOffsetStep(void);
void abi_CameraPastX08E8(void);
#include "actor.h"
#include "hw.h"
#include "wram.h"
#include "wram_player.h"
#include "wram_state.h"
#include "wram_video.h"

/*
 * @routine 84:9083 BackdropOffsetStep
 * WHAT IT DOES: Moves the backdrop layer against the play field by one
 * frame's step for the actor's facing, and counts the actor's frames.
 * HOW: The facing * 4 indexes the steps at $80:D7CA; its 32-bit value is
 * added to backdrop_offset_y ($1A2B); the actor's word at $40 steps down.
 * Entry: M 8-bit, X 8-bit; D = the actor; DB = $80; called by JSR. Exit:
 * M 8-bit; Z set when $40 reached 0; X = the facing * 4.
 */
void abi_BackdropOffsetStep(void)
{
    op_rep(0x20);  /* 849083 rep */
    lda16(ea_dp(0x14));  /* 849085 lda */
    ACC16(asl);  /* 849087 asl */
    ACC16(asl);  /* 849088 asl */
    tax8();  /* 849089 tax */
    clc();  /* 84908A clc */
    lda16(ea_abs(BACKDROP_OFFSET_Y));  /* 84908B lda */
    adc16_v(rd16(ea_absx(0xD7CA)));  /* 84908E adc */
    sta16(ea_abs(BACKDROP_OFFSET_Y));  /* 849091 sta */
    lda16(ea_abs(0x1A2D));  /* 849094 lda */
    adc16_v(rd16(ea_absx(0xD7CC)));  /* 849097 adc */
    sta16(ea_abs(0x1A2D));  /* 84909A sta */
    MEM16(dec, ea_dp(0x40));  /* 84909D dec */
    op_sep(0x20);  /* 84909F sep */
    return;  /* 8490A1 rts */
}

/*
 * @routine 84:90B7 CameraPastX08E8
 * WHAT IT DOES: Says whether the play field's scroll x has reached $08E8:
 * C set when it has.
 * HOW: A 16-bit compare of the scroll x with $08E8.
 * Entry: M 8-bit; DB a bank whose $0000-$1FFF is work RAM; called by JSR.
 * Exit: M 8-bit; C, N, Z from the compare; A = the scroll x.
 */
void abi_CameraPastX08E8(void)
{
    op_rep(0x20);  /* 8490B7 rep */
    lda16(ea_abs(0x1773));  /* 8490B9 lda */
    cmp16_v(0x08E8);  /* 8490BC cmp */
    op_sep(0x20);  /* 8490BF sep */
    return;  /* 8490C1 rts */
}

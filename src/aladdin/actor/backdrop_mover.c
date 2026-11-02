/*
 * backdrop_mover.c: helpers of the actor at $84:9044 that moves the
 * backdrop layer up and down against the play field, and holds its sounds
 * once the camera is past x $08E8.
 */
#include "machine.h"
#include "ops65.h"
void abi_MoverEndSound(void);
void abi_MoverSound(void);
void abi_SoundQueueAdd(void);
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

/*
 * @routine 84:90A2 MoverEndSound
 * WHAT IT DOES: Once the camera is past x $08E8, queues sound command $F1
 * one time; before that keeps the actor's flag clear so it can be queued.
 * HOW: CameraPastX08E8; not past: $43 cleared. Past with $43 clear:
 * SoundQueueAdd $F1, $43 up.
 * Entry: M 8-bit, X 8-bit; D = the actor; DB a bank whose $0000-$1FFF is
 * work RAM; called by JSL. Exit: M 8-bit; as SoundQueueAdd when it
 * queued.
 */
void abi_MoverEndSound(void)
{
    abi_CameraPastX08E8();  /* 8490A2 jsr */
    if (!IF_C) goto L_8490B4;  /* 8490A5 bcc */
    lda8(ea_dp(0x43));  /* 8490A7 lda */
    if (!IF_Z) goto L_8490B3;  /* 8490A9 bne */
    lda8_v(0xF1);  /* 8490AB lda */
    abi_SoundQueueAdd();  /* 8490AD jsl */
    MEM8(inc, ea_dp(0x43));  /* 8490B1 inc */
L_8490B3:
    return;  /* 8490B3 rtl */
L_8490B4:
    stz8(ea_dp(0x43));  /* 8490B4 stz */
    return;  /* 8490B6 rtl */
}

/*
 * @routine 84:90C2 MoverSound
 * WHAT IT DOES: Queues sound command A while the camera is not yet past x
 * $08E8.
 * HOW: A kept in X; CameraPastX08E8; not past: SoundQueueAdd with it.
 * Entry: M 8-bit, X 8-bit; A = the command; DB a bank whose $0000-$1FFF
 * is work RAM; called by JSR. Exit: X = the command; as SoundQueueAdd
 * when it queued.
 */
void abi_MoverSound(void)
{
    tax8();  /* 8490C2 tax */
    abi_CameraPastX08E8();  /* 8490C3 jsr */
    if (IF_C) goto L_8490CD;  /* 8490C6 bcs */
    txa8();  /* 8490C8 txa */
    abi_SoundQueueAdd();  /* 8490C9 jsl */
L_8490CD:
    return;  /* 8490CD rts */
}

/*
 * actor_pictures.c: sending an actor's new picture to its tile slot in
 * VRAM when its animation changes it.
 */
#include "machine.h"
#include "ops65.h"
void abi_ActorPictureSend(void);
void abi_ActorPictureQueue(void);
#include "actor.h"
#include "actor_anim.h"
#include "wram.h"
#include "wram_video.h"

/*
 * @routine 81:949E ActorPictureSend
 * WHAT IT DOES: When the actor shows its own picture (bit 6 of $09) and
 * the picture has changed, queues its graphics for VRAM.
 * HOW: Bit 6 clear: nothing. Else ActorPictureQueue.
 * Entry: M 8-bit; D = the actor; DB = $81; called by JSL. Exit: as
 * ActorPictureQueue; nothing to send: A low byte = 0, Z set.
 */
void abi_ActorPictureSend(void)
{
    goto L_81949E;
L_819493:
    return;  /* 819493 rtl */
L_81949E:
    lda8(ea_dp(0x09));  /* 81949E lda */
    and8_v(0x40);  /* 8194A0 and */
    if (IF_Z) goto L_819493;  /* 8194A2 beq */
    abi_ActorPictureQueue(); return;
}

/*
 * @routine 81:94A4 ActorPictureQueue
 * WHAT IT DOES: Queues the actor's picture, when it has changed, for the
 * NMI to write to its tile slot in VRAM.
 * HOW: The picture against the one last sent; a new one is kept and, from
 * the actor's picture list, its parts: each part a VRAM queue record (the
 * slot's VRAM address from the sprite attributes for the first, the last
 * part's address plus the part's step after; source, bank and size from
 * the part list), until the $FF that ends the list.
 * Entry: M 8-bit, X 8-bit; D = the actor; DB = $81; called by JSL. Exit:
 * unchanged picture: Z set, A low byte = the picture. Queued: M 8-bit,
 * X 8-bit; N, Z from the last compare (Z set).
 */
void abi_ActorPictureQueue(void)
{
    goto L_8194A4;
L_819491:
    op_sep(0x10);  /* 819491 sep */
L_819493:
    return;  /* 819493 rtl */
L_8194A4:
    lda8(ea_dp(0x27));  /* 8194A4 lda */
    cmp8_v(rd8(ea_dp(0x28)));  /* 8194A6 cmp */
    if (IF_Z) goto L_819493;  /* 8194A8 beq */
    sta8(ea_dp(0x28));  /* 8194AA sta */
    ACC8(asl);  /* 8194AC asl */
    sta8(ea_abs(SCRATCH));  /* 8194AD sta */
    stz8(ea_abs(0x000D));  /* 8194B0 stz */
    lda8(ea_abs(VRAM_QUEUE_COUNT));  /* 8194B3 lda */
    ACC8(asl);  /* 8194B6 asl */
    ACC8(asl);  /* 8194B7 asl */
    ACC8(asl);  /* 8194B8 asl */
    tax8();  /* 8194B9 tax */
    op_rep(0x30);  /* 8194BA rep */
    lda16_v(0x0000);  /* 8194BC lda */
    tay16();  /* 8194BF tay */
    lda16(ea_dpiy(0x29));  /* 8194C0 lda */
    clc();  /* 8194C2 clc */
    adc16_v(rd16(ea_abs(SCRATCH)));  /* 8194C3 adc */
    tay16();  /* 8194C6 tay */
    lda16(ea_absy(0x0000));  /* 8194C7 lda */
    tay16();  /* 8194CA tay */
    lda16(ea_dp(0x2B));  /* 8194CB lda */
    and16_v(0x01FF);  /* 8194CD and */
    ACC16(asl);  /* 8194D0 asl */
    ACC16(asl);  /* 8194D1 asl */
    ACC16(asl);  /* 8194D2 asl */
    ACC16(asl);  /* 8194D3 asl */
    clc();  /* 8194D4 clc */
    adc16_v(0x6000);  /* 8194D5 adc */
    sta16(ea_absx(VRAM_QUEUE));  /* 8194D8 sta */
L_8194DB:
    phx16();  /* 8194DB phx */
    lda16(ea_dp(0x06));  /* 8194DC lda */
    and16_v(0x00FF);  /* 8194DE and */
    ACC16(asl);  /* 8194E1 asl */
    tax16();  /* 8194E2 tax */
    plx16();  /* 8194E3 plx */
    clc();  /* 8194E4 clc */
    lda16(ea_absy(0x0000));  /* 8194E5 lda */
    sta16(ea_absx(0x085C));  /* 8194E8 sta */
    iny16();  /* 8194EB iny */
    iny16();  /* 8194EC iny */
    lda16(ea_absy(0x0000));  /* 8194ED lda */
    sta16(ea_absx(0x085E));  /* 8194F0 sta */
    iny16();  /* 8194F3 iny */
    iny16();  /* 8194F4 iny */
    op_sep(0x20);  /* 8194F5 sep */
    lda8(ea_absy(0x0000));  /* 8194F7 lda */
    sta8(ea_absx(0x0860));  /* 8194FA sta */
    iny16();  /* 8194FD iny */
    lda8_v(0x80);  /* 8194FE lda */
    sta8(ea_absx(0x0861));  /* 819500 sta */
    MEM8(inc, ea_abs(VRAM_QUEUE_COUNT));  /* 819503 inc */
    lda8(ea_absy(0x0000));  /* 819506 lda */
    cmp8_v(0xFF);  /* 819509 cmp */
    if (IF_Z) goto L_819491;  /* 81950B beq */
    op_rep(0x20);  /* 81950D rep */
    clc();  /* 81950F clc */
    txa16();  /* 819510 txa */
    adc16_v(0x0008);  /* 819511 adc */
    tax16();  /* 819514 tax */
    clc();  /* 819515 clc */
    lda16(ea_absy(0x0000));  /* 819516 lda */
    adc16_v(rd16(ea_absx(OAM_NEXT)));  /* 819519 adc */
    sta16(ea_absx(VRAM_QUEUE));  /* 81951C sta */
    iny16();  /* 81951F iny */
    iny16();  /* 819520 iny */
    goto L_8194DB;  /* 819521 bra */
}

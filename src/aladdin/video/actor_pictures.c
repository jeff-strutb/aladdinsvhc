/*
 * actor_pictures.c: sending an actor's new picture to its tile slot in
 * VRAM when its animation changes it.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
void abi_ActorPictureSend(void);
void abi_ActorPictureQueue(void);
void abi_ActorPictureSendFromKind(void);
void abi_ActorPictureQueueNew(void);
void abi_ActorPictureQueueParts(void);
#include "actor.h"
#include "actor_anim.h"
#include "wram.h"
#include "wram_video.h"

/* Per kind: where its graphics went in bank $7F (word) */
#define KIND_GFX 0x7FE500u

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
    uint8_t own = rd8(dpa(ACTOR_09)) & 0x40;

    a8_load(own);
    if (own)
        abi_ActorPictureQueue();
}

/*
 * @routine 81:9494 ActorPictureSendKind4
 * WHAT IT DOES: ActorPictureSend for an actor whose kind has set-up index
 * 4 only. Nothing in the game calls it (no JSR, JSL, JMP, pointer or word
 * in the ROM names $81:9494): unit_bank81_entries.box calls it at its
 * entry.
 * HOW: The kind's set-up index (kind_setup_of by $06) not 4: out
 * (ActorPictureNone's RTL); else on into ActorPictureSend.
 * Entry: M 8-bit, X 8-bit; D = the actor; DB = $80; called by JSL. Exit:
 * not 4: A low = the index, N, Z, C from CMP #4; else as ActorPictureSend.
 */
void abi_ActorPictureSendKind4(void)
{
    ldx8(ea_dp(0x06));
    lda8(ea_longx(0x82B000u));
    cmp8_v(0x04);
    if (!IF_Z)
        return;
    abi_ActorPictureSend();
}

/*
 * @routine 81:946F ActorFrameIsStep
 * WHAT IT DOES: With A equal to the actor's frame (ACTOR_FRAME, $2F), X =
 * the frame's time (ACTOR_FRAME_TIME, $26) less one. Nothing in the game
 * calls it.
 * HOW: CMP $0A:00C1 (long: work RAM's $00C1; its result is not used), CMP
 * $2F: not equal, out; else LDX $26, DEX. Bytes or words as M and X are
 * set.
 * Entry: any widths; D, A as above; called by JSL. Exit: not equal: N, Z,
 * C from CMP $2F; else X = $26 less one, N and Z from it, C from the CMP.
 */
void abi_ActorFrameIsStep(void)
{
    if (cpu.p & P_M) {
        cmp8_v(rd8(0x0A00C1u));           /* CMP $0A:00C1: work RAM's $00C1 */
        cmp8_v(rd8(ea_dp(ACTOR_FRAME)));
    } else {
        cmp16_v(rd16(0x0A00C1u));
        cmp16_v(rd16(ea_dp(ACTOR_FRAME)));
    }
    if (IF_Z) {
        if (cpu.p & P_X) {
            ldx8(ea_dp(ACTOR_FRAME_TIME));
            dex8();
        } else {
            ldx16(ea_dp(ACTOR_FRAME_TIME));
            dex16();
        }
    }
}

/*
 * @routine 81:947B ActorFramePictureOnce
 * WHAT IT DOES: Once for the actor (while its byte at $0843+X is 0), when
 * it shows its own picture (bit 6 of $09) and the picture numbered by its
 * frame (ACTOR_FRAME, $2F) is not the one last sent, queues that picture's
 * parts. Nothing in the game calls it.
 * HOW: $0843+X nonzero, bit 6 of $09 clear, or $2F = ACTOR_REDRAW: out
 * (ActorPictureNone's RTL). Else $0843+X counted up and, with A = $2F, on
 * at ActorPictureQueueNew.
 * Entry: M 8-bit, X 8-bit; D = the actor; X = its index into $0843; DB =
 * $81; called by JSL. Exit: out: A = the byte that ended it, N, Z from it
 * (CMP for $2F); queued: as ActorPictureQueueParts.
 */
void abi_ActorFramePictureOnce(void)
{
    lda8(ea_absx(0x0843));
    if (!IF_Z)
        return;
    lda8(ea_dp(ACTOR_09));
    and8_v(0x40);
    if (IF_Z)
        return;
    lda8(ea_dp(ACTOR_FRAME));
    cmp8_v(rd8(ea_dp(ACTOR_REDRAW)));
    if (IF_Z)
        return;
    MEM8(inc, ea_absx(0x0843));
    abi_ActorPictureQueueNew();
}

/*
 * @routine 81:95B0 ActorFramePictureParts
 * WHAT IT DOES: Queues the parts of the picture whose number * 2 is in
 * scratch $0C, as ActorPictureQueueParts. Nothing in the game calls it.
 * HOW: LDA $2F (its value is not used: the next step loads A), then on at
 * ActorPictureQueueParts.
 * Entry: M 8-bit, X 8-bit; as ActorPictureQueueParts; called by JSL. Exit:
 * as ActorPictureQueueParts.
 */
void abi_ActorFramePictureParts(void)
{
    lda8(ea_dp(ACTOR_FRAME));
    abi_ActorPictureQueueParts();
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
    uint8_t picture = rd8(dpa(ACTOR_PICTURE)), last = rd8(dpa(ACTOR_REDRAW));

    a8_load(picture);
    cmp8_of(picture, last);
    if (picture == last)
        return;
    abi_ActorPictureQueueNew();
}

/*
 * @routine 81:94AA ActorPictureQueueNew
 * WHAT IT DOES: The rest of ActorPictureQueue once the picture is new:
 * picture A kept as the one sent, and its parts queued.
 * HOW: A to ACTOR_REDRAW; A * 2 to scratch $0C (word); on into
 * ActorPictureQueueParts.
 * Entry: M 8-bit, X 8-bit; A low byte = the picture; D = the actor; DB =
 * $81; reached from ActorPictureQueue and by a branch from
 * ActorFramePictureOnce. Exit: as ActorPictureQueueParts.
 */
void abi_ActorPictureQueueNew(void)
{
    uint8_t picture = (uint8_t)cpu.a;

    wr8(dpa(ACTOR_REDRAW), picture);
    wr8(dba(SCRATCH), (uint8_t)(picture << 1));
    wr8(dba(SCRATCH + 1), 0);
    abi_ActorPictureQueueParts();
}

/*
 * @routine 81:94B3 ActorPictureQueueParts
 * WHAT IT DOES: Queues the parts of the picture whose number * 2 is in
 * scratch $0C, from the actor's picture list, as VRAM queue records.
 * HOW: As ActorPictureQueue's HOW, from the queue's end (vram_queue_count
 * * 8): the first part to the slot's VRAM address from the sprite
 * attributes, each after it to the last one's address plus its step, until
 * the $FF that ends the list.
 * Entry: M 8-bit, X 8-bit; scratch $0C = the picture * 2 (word); D = the
 * actor; DB = $81; reached from ActorPictureQueueNew and by a jump from
 * ActorFramePictureParts. Exit: M 8-bit, X 8-bit; N, Z from the last
 * compare (Z set).
 */
void abi_ActorPictureQueueParts(void)
{
    uint8_t more;
    uint16_t list, size, step;

    cpu.x = (uint8_t)(rd8(dba(VRAM_QUEUE_COUNT)) << 3);
    op_rep(0x30);
    /* the picture's parts from its list: the first to the slot's VRAM
       address, each after it to the last one's address plus its step */
    list = rd16(dba(rd16(dpa(0x29))));
    cpu.y = rd16(dba((uint16_t)(list + rd16(dba(SCRATCH)))));
    flag_set(P_C, 0);
    cpu.a = op_adc16((uint16_t)((dp16(ACTOR_LOOK) & 0x01FF) << 4), 0x6000, &cpu.p);
    wr16(dba(VRAM_QUEUE) + cpu.x, cpu.a);
    for (;;) {
        phx16();
        lda16(ea_dp(ACTOR_06));                  /* read, then X pulled back over it */
        and16_v(0x00FF);
        cpu.a = asl16_r(cpu.a);
        tax16();
        plx16();
        flag_set(P_C, 0);
        cpu.a = rd16(dba(0) + cpu.y);
        wr16(dba(VRAM_QUEUE + 2) + cpu.x, cpu.a);
        cpu.y = (uint16_t)(cpu.y + 2);
        size = rd16(dba(0) + cpu.y);
        wr16(dba(VRAM_QUEUE + 4) + cpu.x, size);
        cpu.y = (uint16_t)(cpu.y + 2);
        op_sep(P_M);
        wr8(dba(VRAM_QUEUE + 6) + cpu.x, rd8(dba(0) + cpu.y));
        cpu.y = (uint16_t)(cpu.y + 1);
        wr8(dba(VRAM_QUEUE + 7) + cpu.x, 0x80);
        inc8_at(dba(VRAM_QUEUE_COUNT));
        more = rd8(dba(0) + cpu.y);
        cpu.a = (uint16_t)((size & 0xFF00) | more);
        cmp8_of(more, 0xFF);
        if (more == 0xFF) {
            op_sep(P_X);
            return;
        }
        op_rep(P_M);
        flag_set(P_C, 0);
        cpu.x = op_adc16(cpu.x, 0x0008, &cpu.p);
        flag_set(P_C, 0);
        step = rd16(dba(0) + cpu.y);
        cpu.a = op_adc16(step, rd16(dba(OAM_NEXT) + cpu.x), &cpu.p);
        wr16(dba(VRAM_QUEUE) + cpu.x, cpu.a);
        cpu.y = (uint16_t)(cpu.y + 2);
    }
}

/*
 * @routine 81:9527 ActorPictureSendFromKind
 * WHAT IT DOES: As ActorPictureSend, for an actor whose graphics were
 * unpacked to bank $7F for its kind: when it shows its own picture (bit 6
 * of $09) and the picture has changed, queues the picture's graphics for
 * VRAM, their source the part's offset from where its kind's graphics
 * went (KIND_GFX).
 * HOW: Bit 6 clear, or the picture the one last sent: nothing (the RTL
 * at $81:9526, before its entry). Else the picture kept and, from the
 * actor's picture list, its first part as a VRAM queue record: the slot's
 * VRAM address from the sprite attributes, the source KIND_GFX by its
 * kind plus the part's offset, then the part's bank and size, VMAIN $80;
 * the queue counted up. The $FF that ends the list: M and X 8-bit and
 * out (by $81:9523). A second part goes on at FROM_KIND_NEXT_PART, which
 * no script reaches: its C is not written, and it runs as the original's
 * (jump_routine).
 * Entry: M 8-bit, X 8-bit; D = the actor; DB = $81; called by JSL. Exit:
 * nothing to send: A low byte = 0 or the picture, Z set. Queued: M
 * 8-bit, X 8-bit; N, Z from the last compare (Z set).
 */
void abi_ActorPictureSendFromKind(void)
{
    uint8_t own = rd8(dpa(ACTOR_09)) & 0x40;
    uint8_t picture, last;
    uint16_t q, list, part, vram, src, size, step;
    uint8_t more;

    a8_load(own);
    if (!own)
        return;
    picture = rd8(dpa(ACTOR_PICTURE));
    last = rd8(dpa(ACTOR_REDRAW));
    a8_load(picture);
    cmp8_of(picture, last);
    if (picture == last)
        return;
    wr8(dpa(ACTOR_REDRAW), picture);
    wr8(dba(SCRATCH), (uint8_t)(picture << 1));
    wr8(dba(SCRATCH + 1), 0);
    /* the queue's next record; the picture's first part from the list */
    q = (uint8_t)(rd8(dba(VRAM_QUEUE_COUNT)) << 3);
    op_rep(0x30);
    list = rd16(dba(rd16(dpa(0x29))));
    part = rd16(dba((uint16_t)(list + rd16(dba(SCRATCH)))));
    vram = (uint16_t)(((dp16(ACTOR_LOOK) & 0x01FF) << 4) + 0x6000);
    wr16(dba(VRAM_QUEUE + q), vram);
    cpu.x = q;
    cpu.y = part;
    for (;;) {                            /* each part, until the $FF */
        phx16();
        flag_set(P_C, 0);
        src = rd16(KIND_GFX + ((dp16(ACTOR_06) & 0x00FF) << 1));
        src = op_adc16(src, rd16(dba(cpu.y)), &cpu.p);
        plx16();
        wr16(dba(VRAM_QUEUE + 2) + cpu.x, src);
        cpu.y = (uint16_t)(cpu.y + 2);
        size = rd16(dba(cpu.y));
        wr16(dba(VRAM_QUEUE + 4) + cpu.x, size);
        cpu.y = (uint16_t)(cpu.y + 2);
        op_sep(0x20);
        wr8(dba(VRAM_QUEUE + 6) + cpu.x, rd8(dba(cpu.y)));
        cpu.y = (uint16_t)(cpu.y + 1);
        wr8(dba(VRAM_QUEUE + 7) + cpu.x, 0x80);
        wr8(dba(VRAM_QUEUE_COUNT), (uint8_t)(rd8(dba(VRAM_QUEUE_COUNT)) + 1));
        more = rd8(dba(cpu.y));
        cpu.a = (uint16_t)((size & 0xFF00) | more);
        cmp8_of(more, 0xFF);
        if (more == 0xFF) {
            op_sep(0x30);
            return;
        }
        /* $81:959A: the next record, at the last one's address plus the
           part's step */
        op_rep(0x20);
        flag_set(P_C, 0);
        cpu.x = op_adc16(cpu.x, 0x0008, &cpu.p);
        flag_set(P_C, 0);
        step = rd16(dba(cpu.y));
        cpu.a = op_adc16(step, rd16(dba(OAM_NEXT) + cpu.x), &cpu.p);
        wr16(dba(VRAM_QUEUE) + cpu.x, cpu.a);
        cpu.y = (uint16_t)(cpu.y + 2);
    }
}

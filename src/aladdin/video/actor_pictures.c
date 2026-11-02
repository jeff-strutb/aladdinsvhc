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
#include "actor.h"
#include "actor_anim.h"
#include "wram.h"
#include "wram_video.h"

/* Per kind: where its graphics went in bank $7F (word) */
#define KIND_GFX 0x7FE500u
/* Not yet filed (no script sends a picture of more than one part this
   way): ActorPictureSendFromKind's next part and on, at $81:959A. */
#define FROM_KIND_NEXT_PART 0x81959Au

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
    uint8_t picture = rd8(dpa(ACTOR_PICTURE)), last = rd8(dpa(ACTOR_REDRAW)), more;
    uint16_t list, size;

    a8_load(picture);
    cmp8_of(picture, last);
    if (picture == last)
        return;
    wr8(dpa(ACTOR_REDRAW), picture);
    wr8(dba(SCRATCH), (uint8_t)(picture << 1));
    wr8(dba(SCRATCH + 1), 0);
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
        cpu.a = op_adc16(rd16(dba(0) + cpu.y), rd16(dba(OAM_NEXT) + cpu.x), &cpu.p);
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
    uint16_t q, list, part, vram, src, size;

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
    phx16();
    flag_set(P_C, 0);
    src = op_adc16(rd16(KIND_GFX + ((dp16(ACTOR_06) & 0x00FF) << 1)), rd16(dba(part)), &cpu.p);
    plx16();
    wr16(dba(VRAM_QUEUE + 2 + q), src);
    size = rd16(dba((uint16_t)(part + 2)));
    wr16(dba(VRAM_QUEUE + 4 + q), size);
    op_sep(0x20);
    wr8(dba(VRAM_QUEUE + 6 + q), rd8(dba((uint16_t)(part + 4))));
    wr8(dba(VRAM_QUEUE + 7 + q), 0x80);
    wr8(dba(VRAM_QUEUE_COUNT), (uint8_t)(rd8(dba(VRAM_QUEUE_COUNT)) + 1));
    uint8_t more = rd8(dba((uint16_t)(part + 5)));
    cpu.a = (uint16_t)((size & 0xFF00) | more);
    cpu.y = (uint16_t)(part + 5);
    cmp8_of(more, 0xFF);
    if (more == 0xFF) {
        op_sep(0x30);
        return;
    }
    jump_routine(FROM_KIND_NEXT_PART);
}

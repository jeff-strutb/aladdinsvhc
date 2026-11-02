/*
 * camera_trigger.c: a level's task (created at $80:8943) that waits for
 * the camera to reach two places: at the first it frees the actors, at
 * the second (the player alive) it sets the player and the escort going
 * again and starts an actor of kind $6B.
 */
#include "machine.h"
#include "ops65.h"
void abi_ActorSpawnAt(void);
void abi_ActorsFreeAll(void);
void abi_CameraTriggerTask(void);
void abi_SoundQueueF6(void);
void abi_TaskExit(void);
void abi_TaskWait(void);
#include "actor.h"
#include "hw.h"
#include "wram.h"
#include "wram_player.h"
#include "wram_state.h"
#include "wram_video.h"

/*
 * @routine 80:A22D CameraTriggerTask
 * WHAT IT DOES: Waits for the camera to reach x $0Bxx, y $02xx and frees
 * every actor; then waits for x $0Fxx with the player alive, gives the
 * escort and the player new code, starts an actor of kind $6B at ($0F38,
 * $03B0) and queues sound command $F6 with parameter 2, and ends.
 * HOW: $00BD-$00BF cleared. Each frame (TaskWait 1) until the high bytes
 * of the scroll x and y are $0B and 2: ActorsFreeAll. Each frame until
 * the scroll x's high byte is $0F and the health nonzero: $03FA up; the
 * escort's $03 word $E554 and $07 5; $1A10 up; ActorsFreeAll; the
 * player's $07 9 and $03 word $CB3B; ActorSpawnAt of kind $6B (its $07
 * 1) at the spawn words; SoundQueueF6 2; TaskExit.
 * Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM; a
 * task's entry. Exit: never returns (TaskExit).
 */
void abi_CameraTriggerTask(void)
{
    stz8(ea_abs(0x00BD));  /* 80A22D stz */
    stz8(ea_abs(0x00BE));  /* 80A230 stz */
    stz8(ea_abs(0x00BF));  /* 80A233 stz */
L_80A236:
    lda8_v(0x01);  /* 80A236 lda */
    jsl_frame(0x80A238u); abi_TaskWait();  /* 80A238 jsl */
    lda8(ea_abs(0x1774));  /* 80A23C lda */
    cmp8_v(0x0B);  /* 80A23F cmp */
    if (!IF_Z) goto L_80A236;  /* 80A241 bne */
    lda8(ea_abs(0x1778));  /* 80A243 lda */
    cmp8_v(0x02);  /* 80A246 cmp */
    if (!IF_Z) goto L_80A236;  /* 80A248 bne */
    abi_ActorsFreeAll();  /* 80A24A jsl */
L_80A24E:
    lda8_v(0x01);  /* 80A24E lda */
    jsl_frame(0x80A250u); abi_TaskWait();  /* 80A250 jsl */
    lda8(ea_abs(0x1774));  /* 80A254 lda */
    cmp8_v(0x0F);  /* 80A257 cmp */
    if (!IF_Z) goto L_80A24E;  /* 80A259 bne */
    lda8(ea_abs(0x0367));  /* 80A25B lda */
    if (IF_Z) goto L_80A24E;  /* 80A25E beq */
    MEM8(inc, ea_abs(0x03FA));  /* 80A260 inc */
    op_rep(0x20);  /* 80A263 rep */
    lda16_v(0xE554);  /* 80A265 lda */
    sta16(ea_abs(0x092E));  /* 80A268 sta */
    op_sep(0x20);  /* 80A26B sep */
    lda8_v(0x05);  /* 80A26D lda */
    sta8(ea_abs(0x0932));  /* 80A26F sta */
    MEM8(inc, ea_abs(0x1A10));  /* 80A272 inc */
    abi_ActorsFreeAll();  /* 80A275 jsl */
    lda8_v(0x09);  /* 80A279 lda */
    sta8(ea_abs(0x08E2));  /* 80A27B sta */
    op_rep(0x20);  /* 80A27E rep */
    lda16_v(0xCB3B);  /* 80A280 lda */
    sta16(ea_abs(0x08DE));  /* 80A283 sta */
    lda16_v(0x0F38);  /* 80A286 lda */
    sta16(ea_abs(0x002C));  /* 80A289 sta */
    lda16_v(0x03B0);  /* 80A28C lda */
    sta16(ea_abs(0x002E));  /* 80A28F sta */
    op_sep(0x20);  /* 80A292 sep */
    lda8_v(0x6B);  /* 80A294 lda */
    xba();  /* 80A296 xba */
    lda8_v(0x01);  /* 80A297 lda */
    abi_ActorSpawnAt();  /* 80A299 jsr */
    op_sep(0x30);  /* 80A29C sep */
    lda8_v(0x02);  /* 80A29E lda */
    abi_SoundQueueF6();  /* 80A2A0 jsl */
    abi_TaskExit(); return;  /* 80A2A4 jml */
}

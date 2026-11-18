/*
 * frame_step.c: one frame of play: the actors' turns (the actor loop),
 * then the scroll, the map's tile changes, the play time, the palette
 * and sequence steps, and the sprites, with the frame's queues marked
 * ready for the vertical blank.
 */
#include "machine.h"
#include "native.h"
#include "ops65.h"
#include "cpu_ops.h"
#include "actor.h"

void abi_PlayerLoop(void);
void abi_ActorLoop(void);
void abi_OamBuildBegin(void);
void abi_ScrollUpdate(void);
void abi_TileChangesApply(void);
void abi_LayersScroll(void);
void abi_PlacementsScroll(void);
void abi_PlayTimeTick(void);
void abi_SequencesStep(void);
void abi_PaletteCyclesStep(void);
void abi_ActorsInView(void);
void abi_OamDrawActors(void);
void abi_BgFollowLayers(void);

#define FRAME_START_X 0x143Au       /* the player's x as the frame began: fraction, then pixels (3 bytes) */
#define FRAME_START_Y 0x143Du       /* and its y */
#define TILE_BLOCKS_LENGTH 0x1BE4u  /* the tile block queue's length in bytes (word) */
#define TILE_BLOCKS_PENDING 0x1BE6u /* nonzero: the block queue is ready */
#define OAM_0857 0x0857u            /* cleared, then counted up, each frame; purpose not yet established */
#define VRAM_QUEUE_PENDING 0x0858u  /* nonzero: the VRAM queue is ready */
#define VIEW_X 0x1773u              /* the view's left edge (pixels, word) */
#define VIEW_Y 0x1777u              /* the view's top edge (pixels, word) */
#define RANGE_X 0x1A44u             /* the point actors are kept in range of (word) */
#define RANGE_Y 0x1A46u
#define PAUSE_BLOCK 0x0404u         /* nonzero: no pause; with PAUSED, no sequence or palette steps */
#define PAUSED 0x0405u              /* counted up while paused */
#define VIEW_HALF 0x0080u           /* the range point: the view's corner + this, in x and y */

/*
 * @routine 81:8311 FrameStep
 * WHAT IT DOES: One frame of play: the player's turn and every other
 * actor's (the actor loop), the scroll and the map, the play time, the
 * sequence and palette cycle steps (not while paused or with PAUSE_BLOCK
 * set), and the sprites; the frame's tile block and VRAM queues are then
 * marked ready.
 * HOW: The player's x and y (3 bytes each) kept in FRAME_START_X/Y;
 * TILE_BLOCKS_LENGTH and OAM_0857 cleared. PlayerLoop (by JSL: the C waits
 * in actor_loop_call while the actors run), OamBuildBegin, ActorLoop (the
 * same), ScrollUpdate, TileChangesApply, LayersScroll, PlacementsScroll,
 * PlayTimeTick. RANGE_X/Y = VIEW_X/Y + $80. PAUSED or PAUSE_BLOCK clear:
 * SequencesStep, PaletteCyclesStep. ActorsInView, OamDrawActors;
 * OAM_0857, TILE_BLOCKS_PENDING and VRAM_QUEUE_PENDING counted up;
 * BgFollowLayers.
 * Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM; called
 * by JSR. Exit: M 8-bit; registers as BgFollowLayers leaves them.
 */
/* A JSL at `site` to fn: its return address on the stack while fn runs,
   as the original's (the stack the callees push to lies where theirs does). */
static void jsl_call(uint32_t site, void (*fn)(void))
{
    jsl_frame(site);
    fn();
    cpu.s = (uint16_t)(cpu.s + 3);
}

void abi_FrameStep(void)
{
    op_rep(0x20);
    lda16(ea_abs(PLAYER + ACTOR_X));
    sta16(ea_abs(FRAME_START_X));
    lda16(ea_abs(PLAYER + ACTOR_Y));
    sta16(ea_abs(FRAME_START_Y));
    ldx8(ea_abs(PLAYER + ACTOR_X + 2));
    stx8(ea_abs(FRAME_START_X + 2));
    ldx8(ea_abs(PLAYER + ACTOR_Y + 2));
    stx8(ea_abs(FRAME_START_Y + 2));
    op_sep(0x20);
    stz8(ea_abs(TILE_BLOCKS_LENGTH));
    stz8(ea_abs(TILE_BLOCKS_LENGTH + 1));
    stz8(ea_abs(OAM_0857));
    actor_loop_call(0x818336u, abi_PlayerLoop);
    jsl_call(0x81833Au, abi_OamBuildBegin);
    actor_loop_call(0x81833Eu, abi_ActorLoop);
    call_frame(0x8344, abi_ScrollUpdate);
    jsl_call(0x818345u, abi_TileChangesApply);
    call_frame(0x834B, abi_LayersScroll);
    jsl_call(0x81834Cu, abi_PlacementsScroll);
    call_frame(0x8352, abi_PlayTimeTick);
    op_rep(0x20);
    clc();
    lda16(ea_abs(VIEW_X));
    adc16_v(VIEW_HALF);
    sta16(ea_abs(RANGE_X));
    clc();
    lda16(ea_abs(VIEW_Y));
    adc16_v(VIEW_HALF);
    sta16(ea_abs(RANGE_Y));
    op_sep(0x20);
    lda8(ea_abs(PAUSED));
    ora8_v(rd8(ea_abs(PAUSE_BLOCK)));
    if (IF_Z) {
        call_frame(0x8375, abi_SequencesStep);
        call_frame(0x8378, abi_PaletteCyclesStep);
    }
    jsl_call(0x818379u, abi_ActorsInView);
    jsl_call(0x81837Du, abi_OamDrawActors);
    MEM8(inc, ea_abs(OAM_0857));
    MEM8(inc, ea_abs(TILE_BLOCKS_PENDING));
    MEM8(inc, ea_abs(VRAM_QUEUE_PENDING));
    call_frame(0x838C, abi_BgFollowLayers);
}

/*
 * @routine 81:830D FrameStepLong
 * WHAT IT DOES: One frame of play (FrameStep), for callers in other banks.
 * HOW: FrameStep by JSR, its return address on the stack while it runs
 * (FrameStep's C waits in the actor loop with it there), then RTL.
 * Entry: as FrameStep; called by JSL. Exit: as FrameStep.
 */
void abi_FrameStepLong(void)
{
    call_frame(0x830F, abi_FrameStep);
}

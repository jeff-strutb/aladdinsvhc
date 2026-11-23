/*
 * chain_piece.c: the pieces of the nine-piece chain (kinds $74 and $75) and its head (kind $73).
 */
#include "actor/actor_own.h"
#include "actor/kinds_e.h"
#include "audio/sound.h"
#include "game/beaten.h"
#include "player/player.h"
#include "system/task.h"

/*
 * @routine 84:8ADE ChainPieceEnd
 * WHAT IT DOES: A piece of the chain (or its head) goes back to what it
 * was doing before it was struck: its resume point from its own +$35-$36
 * (kept when it was struck), anim_flags bit 4 and flags bit 7 set; it goes
 * on at its next turn. The original leaves for the next record (ActorLoopNext, not
 * ActorLoopBack): the wait routine does not run in this frame, so the head and its
 * pieces take no path step in it (ACTOR_NEXT_RECORD).
 */
int ChainPieceEnd(Actor *a)
{
    a->resume = (uint16_t)(a->own.bytes[OWN_35] | a->own.bytes[OWN_36] << 8);
    a->anim_flags |= 0x10;
    a->flags |= 0x80;
    return ACTOR_NEXT_RECORD;
}

#define PIECE_NUMBER OWN_40     /* +$40: the piece's place in the chain (8: the one that ends the fight) */
#define STATE_GONE 0x8C         /* the head's state once it is going (and so the pieces') */
#define LAST_PIECE 8
#define BRIGHTNESS_SLOT 1       /* the shake's, then the fade's task: the scene waits while it runs */
#define HEAD_HEALTH OWN_11      /* the head's health (in its own bytes); negative: beaten */

enum {
    GONE_FLASH = CHAIN_GONE_FIRST,  /* each frame of the head's show of being struck */
    FW_IDLE1,                       /* the fight won: each frame until the first list has finished */
    FW_FADE,                        /* each frame down half a pixel through the shake */
    FW_WAIT_DIR,                    /* each frame until Aladdin is on a solid actor */
    FW_AFTER_BD,                    /* after its $BD frames */
    FW_SONG,
    FW_W1,
    FW_W2,
    FW_RISE,                        /* each frame of its rise */
    FC_IDLE1,                       /* the closing lists */
    FC_STEP_DOWN,
    FC_IDLE2,
    FC_AFTER_3F,
    FC_STEP_UP,
    FC_IDLE3,
    FC_FADE,                        /* each frame until the brightness task is gone */
    FW_STAY,                        /* for good */
};

/*
 * @routine 84:8D04 ActorsFollowSteps
 * WHAT IT DOES: Moves each actor of the list from the chain's tail to
 * where the one before it is, then on along its own direction (+$42, 9
 * bits) by its step (+$41 plus g.chain_step, at least 1).
 */
static void follow_steps(void)
{
    Actor *before = g.chain_tail, *r;

    for (r = before->next; r; r = r->next) {
        uint8_t step;

        r->x = before->x;
        r->y = before->y;
        step = (uint8_t)(r->own.bytes[OWN_41] + g.chain_step);
        if (!step || (step & 0x80))
            step = 1;
        ActorMoveAlong(r, (uint16_t)((r->own.bytes[OWN_42] | r->own.bytes[OWN_43] << 8) & 0x01FF), step);
        before = r;
    }
}

/*
 * @routine 84:8CE2 ActorPriorityByHeight
 * WHAT IT DOES: Sets the sprite priority (flags bits 4-5) of the chain's
 * head to 1 when its y is less than 128, else to 2.
 * @routine 84:8CC5 ActorsFollowIf8
 * WHAT IT DOES: While the piece's number is 8, moves its followers on
 * (follow_steps) and sets the head's sprite priority by its height.
 */
static void follow_if_8(const Actor *a)
{
    if (a->own.bytes[PIECE_NUMBER] != LAST_PIECE)
        return;
    follow_steps();
    g.chain_head->flags = (uint8_t)((g.chain_head->flags & 0xCF) | (pixels(g.chain_head->y) >= 0x80 ? 0x20 : 0x10));
}

/*
 * @routine 84:8A78 ChainPieceWork
 * WHAT IT DOES: What the chain's pieces (kinds $74 and $75) do each frame
 * they wait: animate, follow their path and the pieces after them, and,
 * unless the chain is untouchable, hurt Aladdin on touch on odd frames
 * (g.sprite_flicker bit 0) or turn his shots back on even ones; when the
 * head goes into state $8C the piece goes with it (its resume point kept,
 * its wait routine off); its +$08 bits 4 and 7 follow the head's.
 */
int ChainPieceWork(Actor *a)
{
    const Actor *head = g.chain_head;

    AnimStep(a);
    ActorPathStep(a);
    follow_if_8(a);
    if (!g.chain_untouchable) {
        if (g.sprite_flicker & 0x01)
            (void)ActorHurtsPlayerOnTouch(a);
        else
            (void)ActorTurnsShotBack(a);
    }
    if (head->state == STATE_GONE) {
        a->state = STATE_GONE;
        a->own.bytes[OWN_35] = (uint8_t)a->resume;
        a->own.bytes[OWN_36] = (uint8_t)(a->resume >> 8);
        a->flags &= 0x7F;
    }
    a->anim_flags = (uint8_t)((a->anim_flags & 0x6F) | (head->anim_flags & 0x90));
    return ACTOR_GOES_ON;
}

/* the fight won (84:8AF6): reached from ChainPieceGone when the head's health is negative */
static int fight_won(Actor *a)
{
    g.play_starts++;
    a->flags |= 0x80;
    if (a->own.bytes[PIECE_NUMBER] != LAST_PIECE) {
        a->resume = FW_STAY;
        return 1;
    }
    ActorListStart(a, CHAIN_SLIDE4, 0x04);
    a->resume = FW_IDLE1;
    return 1;
}

/*
 * @routine 84:8AC2 ChainPieceGone
 * WHAT IT DOES: A piece of the chain while the head shows being struck (its
 * second entry): it flashes with the head; then, with the head still
 * standing, goes back to what it was doing (ChainPieceEnd); with the head
 * beaten, the fight is won: all but the last piece (number 8) wait for
 * good; the last runs the scene after the fight: two lists of children,
 * the shake, sound and riser tasks, the fade waited on (moving down half a
 * pixel a frame), then, with Aladdin still alive, his code to PlayerSceneCued
 * and a wait; the song $36; it rises at a speed that slows until it turns,
 * sound $49 once the head is above y $B0; three closing lists and the
 * brightness task; once the fade is done TaskExec to the ending story
 * (the PAL console's path, 84:AFCC, is exempt).
 */
int ChainPieceGone(Actor *a)
{
    Actor *p = &g.actors[0];

    switch (a->resume) {
    case ACTOR_START_SECOND:
        a->resume = GONE_FLASH;
        return 1;
    case GONE_FLASH:
        ActorLookFlash(a);
        ActorParityBitsSet(a);
        follow_if_8(a);
        if (g.head_hit_frames)
            return 1;
        if ((int8_t)g.chain_head->own.bytes[HEAD_HEALTH] >= 0)
            return ChainPieceEnd(a);
        return fight_won(a);
    case FW_IDLE1:
        if (!ActorListAllDone(a))
            return 1;
        ActorListStart(a, CHAIN_STREAM14, 0x10);
        g.sprites_take_bg2_shift = 8;
        TaskCreate(1, 4, TASK_ENTRY_SHAKE);
        TaskCreate(2, 0, TASK_ENTRY_SOUND_WHILE_SLOT1);
        TaskCreate(3, 0, TASK_ENTRY_RISER_COURSES);
        TaskCreate(4, 0, TASK_ENTRY_SOUND_LATER);
        a->resume = FW_FADE;
        return 1;
    case FW_FADE:
        a->y = wrap24(a->y + 0x80);
        if (task_busy(BRIGHTNESS_SLOT))
            return 1;
        a->resume = g.health ? FW_WAIT_DIR : FW_STAY;
        return 1;
    case FW_WAIT_DIR:
        if (!g.player_on_actor)
            return 1;
        p->resume = PLAYER_AT_SCENE_CUED;
        p->flags &= 0x7F;
        p->anim_flags &= 0xEF;
        a->resume = FW_AFTER_BD;
        return 0xBD;
    case FW_AFTER_BD:
        a->resume = FW_SONG;
        return 1;
    case FW_SONG:
        SongPlay(0x36);
        a->resume = FW_W1;
        return 0xFF;
    case FW_W1:
        a->resume = FW_W2;
        return 0xFF;
    case FW_W2:
        g.chain_step = 0xD4;
        g.rise_sounded = 0;
        a->x = with_pixels(a->x, 0x02C8);
        a->y = with_pixels(a->y, 0x0150);
        g.chain_rise_speed = -0x5B9;      /* 5.7 pixels a frame up, slowing by 1/8 a frame */
        a->resume = FW_RISE;
        return 1;
    case FW_RISE:
        a->y = wrap24(a->y + (uint32_t)g.chain_rise_speed);
        g.chain_rise_speed = signed24(wrap24((uint32_t)g.chain_rise_speed + 0x20));
        if (!g.rise_sounded && pixels(g.chain_head->y) < 0x00B0) {
            g.rise_sounded++;
            SoundQueueAdd(0x49);
        }
        if (g.chain_step)
            g.chain_step++;
        if (g.chain_rise_speed < 0)
            return 1;
        g.head_anim = 0;
        a->probe_at = 1;
        ActorListStart(a, CHAIN_SLIDE2, 0x01);
        a->resume = FC_IDLE1;
        return 1;
    case FC_IDLE1:
        if (!ActorListAllDone(a))
            return 1;
        g.head_anim = 1;
        ActorListStart(a, CHAIN_SLIDE6, 0x01);
        a->resume = FC_STEP_DOWN;
        return 1;
    case FC_STEP_DOWN:
        if (!StepCountDown())
            return 1;
        a->resume = FC_IDLE2;
        return 1;
    case FC_IDLE2:
        if (!ActorListAllDone(a))
            return 1;
        a->resume = FC_AFTER_3F;
        return 0x3F;
    case FC_AFTER_3F:
        g.head_anim = 6;
        ActorListStart(a, CHAIN_LATE, 0x10);
        SoundQueueAdd(0x49);
        g.close_count++;
        a->resume = FC_STEP_UP;
        return 1;
    case FC_STEP_UP:
        if (!StepCountUp())
            return 1;
        a->resume = FC_IDLE3;
        return 1;
    case FC_IDLE3:
        if (!ActorListAllDone(a))
            return 1;
        g.head_anim = 6;
        TaskCreate(BRIGHTNESS_SLOT, 4, TASK_ENTRY_BRIGHTNESS_DOWN);
        a->resume = FC_FADE;
        return 1;
    case FC_FADE:
        if (task_busy(BRIGHTNESS_SLOT))
            return 1;
        LevelBeaten();
        TaskExecRunning(TASK_ENTRY_ENDING_STORY);
        return 1;
    default:                            /* FW_STAY */
        return 1;
    }
}

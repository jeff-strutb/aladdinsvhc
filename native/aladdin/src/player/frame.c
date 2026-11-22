/*
 * frame.c: Aladdin's work each frame his record waits, and what it keeps
 * up: the test box actors are tested against, the time he cannot be hurt,
 * his fall out of the level, and his place on screen (the record's
 * player/player_frame.c, player_box.c, player_bounds.c).
 */
#include "player/player_own.h"
#include "level/level.h"
#include "video/video.h"
#include "video/view.h"

#define NO_FALL_LEVEL_1 0x07    /* levels where falling out of the bottom does not hurt */
#define NO_FALL_LEVEL_2 0x12

/*
 * @routine 81:E911 PlayerBoxSet
 * WHAT IT DOES: The test box from his box `box`: its centre's offset from
 * him (across turned round while his flip is set), its half-sizes, and his
 * probe list.
 */
void PlayerBoxSet(Actor *a, uint8_t box)
{
    PlayerBoxEntry e = player_box_entry(box);

    g.player_box = box;
    g.test_box_offset_x = (int16_t)(a->flip ? (uint16_t)-e.offset_x : e.offset_x);
    g.test_box_offset_y = (int16_t)e.offset_y;
    g.test_box_half_w = e.half_w;
    g.test_box_half_h = e.half_h;
    a->probes = e.probes;
}

/*
 * @routine 81:E909 PlayerBoxUpdate
 * WHAT IT DOES: The test box from the box his code wants, when it is not
 * the one set already.
 */
void PlayerBoxUpdate(Actor *a)
{
    if (g.player_box_wanted != g.player_box)
        PlayerBoxSet(a, g.player_box_wanted);
}

/*
 * @routine 81:E8D5 TestBoxPlace
 * WHAT IT DOES: The test box's centre at his place plus its offsets.
 */
void TestBoxPlace(const Actor *a)
{
    g.test_box_x = (uint16_t)(x_pixels(a) + (uint16_t)g.test_box_offset_x);
    g.test_box_y = (uint16_t)(y_pixels(a) + (uint16_t)g.test_box_offset_y);
}

/*
 * @routine 81:E8EC PlayerFallenTest
 * WHAT IT DOES: True while he is above the level's bottom: false once his y
 * is more than 230 pixels below the scroll's lowest place (or below the
 * scroll itself while an empty block stops it up or down). Above the map's
 * top (his y negative) is above.
 * HOW: g.scroll_limit_y is the original's, (rows - 1) * 256, whatever the
 * view's height (view.h), so the fall is (rows - 1) * 256 + $E6 at any view.
 */
bool PlayerFallenTest(void)
{
    uint16_t y = y_pixels(&g.actors[0]), bottom = g.scroll_limit_y;

    if (y & 0x8000)
        return true;
    if ((uint8_t)g.camera_stopped_y)
        bottom = pixels(g.layers[0].scroll_y);
    return (uint16_t)(bottom + 0x00E6) >= y;
}

/*
 * @routine 81:E966 PlayerHurtTimeOut
 * WHAT IT DOES: The time he cannot be hurt is over: he can be hit again
 * (his +$08 bits 4 and 7 and g.player_flags bit 2 cleared).
 */
void PlayerHurtTimeOut(Actor *a)
{
    a->anim_flags &= 0x6F;
    g.player_flags &= 0xFB;
}

/*
 * @routine 81:E95C PlayerHurtCountdown
 * WHAT IT DOES: Counts down the time he cannot be hurt; when it runs out he
 * can be hit again.
 */
void PlayerHurtCountdown(Actor *a)
{
    if (!g.player_hurt_time || --g.player_hurt_time)
        return;
    PlayerHurtTimeOut(a);
}

/*
 * @routine 81:E990 PlayerKeepOnScreenX
 * WHAT IT DOES: Keeps the actor's x on screen: 16 to 240 pixels from the
 * play field's left edge (at or left of the least, it goes to the least).
 * The view (view.h): 16 to w - 16.
 */
void PlayerKeepOnScreenX(Actor *a)
{
    uint16_t scroll = pixels(g.layers[0].scroll_x), x = x_pixels(a);
    uint16_t low = (uint16_t)(scroll + screen_limit_low(0)), high = (uint16_t)(scroll + screen_limit_high(0) + 2 * view.dx);

    if (low >= x)
        set_x_pixels(a, low);
    else if (high < x)
        set_x_pixels(a, high);
}

/*
 * @routine 81:E9AD PlayerKeepOnScreen
 * WHAT IT DOES: Keeps the actor on screen: x as PlayerKeepOnScreenX, y 24
 * to 216 pixels from the play field's top. The view (view.h): 24 to h - 8.
 */
void PlayerKeepOnScreen(Actor *a)
{
    uint16_t scroll, y, low, high;

    PlayerKeepOnScreenX(a);
    scroll = pixels(g.layers[0].scroll_y);
    y = y_pixels(a);
    low = (uint16_t)(scroll + screen_limit_low(1));
    high = (uint16_t)(scroll + screen_limit_high(1) + 2 * view.dy);
    if (low >= y)
        set_y_pixels(a, low);
    else if (high < y)
        set_y_pixels(a, high);
}

/*
 * @routine 81:E864 PlayerFrameWork
 * WHAT IT DOES: His work each frame his record waits: his move's animation
 * started when the move changed, the time he cannot be hurt counted down,
 * his animation stepped, the view-edge rule run, his picture sent, the
 * camera's aim set by its last push, his fall out of the level's bottom
 * hurting him (with health left, but on levels 7 and $12), his +$08 bit 7
 * from g.player_flags, and the test box set. While a scene moves him
 * (g.player_scene), only his animation (not with bit 6, nor bit 7: the
 * byte $081E it waits for no code writes) and his picture are kept up and
 * the test box placed.
 */
void PlayerFrameWork(Actor *a)
{
    g.player_touched_66 = 0;
    if (g.player_scene) {
        if (!(g.player_scene & 0xC0))
            AnimStep(a);
        ActorPictureSend(a);
        TestBoxPlace(a);
        return;
    }
    if (g.player_move != g.player_move_was) {
        g.player_move_was = g.player_move;
        PlayerMoveStart(a, g.player_move);
    }
    PlayerHurtCountdown(a);
    if (!g.player_anim_held)
        AnimStep(a);
    ViewEdgeRuleRun(a);
    ActorPictureSend(a);
    if ((uint8_t)g.camera_push)
        g.camera_aim_x = (uint16_t)((g.camera_aim_x & 0xFF00) | camera_aim_of_push((uint8_t)g.camera_push));
    if (g.health && g.level != NO_FALL_LEVEL_1 && g.level != NO_FALL_LEVEL_2 && !PlayerFallenTest()) {
        a->flags &= 0x7F;
        PlayerEnter8CFrom1();
    }
    if (g.player_flags)
        a->anim_flags |= 0x80;
    else
        a->anim_flags &= 0x7F;
    g.player_on_actor = 0;
    g.player_actor_ground = 0;
    PlayerBoxUpdate(a);
    TestBoxPlace(a);
}

/*
 * @routine 83:91F8 ActorKind00Wait
 * WHAT IT DOES: The wait routine of kinds $00-$03 and $07 (his): his frame
 * work.
 */
int PlayerWait(Actor *a)
{
    PlayerFrameWork(a);
    return ACTOR_GOES_ON;
}

/* ---- state $8C: hit ---- */

static void enter_8c(void)
{
    Actor *p = &g.actors[0];

    p->state = 0x8C;
    g.player_hurt_count++;
    g.health--;
    p->anim_flags |= 0x80;
    g.player_flags |= 0x02;
}

/*
 * @routine 83:8008 PlayerEnter8C
 * WHAT IT DOES: Puts him into state $8C (hit): his hits counted, one health
 * fewer, his +$08 bit 7 and g.player_flags bit 1 set.
 * Also 83:8004 PlayerEnter8CLong.
 */
void PlayerEnter8C(void)
{
    enter_8c();
}

/*
 * @routine 83:8024 PlayerEnter8CFacing
 * WHAT IT DOES: Unless he is hit already (his +$08 bit 7), puts him into
 * state $8C turned away from the actor that hit him. Returns 1 when he was
 * hit. Also 83:8000 PlayerEnter8CFacingLong.
 */
int PlayerEnter8CFacing(const Actor *by)
{
    if (g.actors[0].anim_flags & 0x80)
        return 0;
    enter_8c();
    g.actors[0].facing = (uint8_t)((ActorSideOfPlayer(by) & 0x01) ^ 0x01);
    return 1;
}

/*
 * @routine 83:8056 PlayerEnter8CFrom1
 * WHAT IT DOES: He fell out: health 1 first (so it ends at 0), marked
 * fallen out, and state $8C. Also 83:8052 PlayerEnter8CFrom1Long.
 */
void PlayerEnter8CFrom1(void)
{
    g.health = 0x01;
    g.player_fallen_out++;
    enter_8c();
}

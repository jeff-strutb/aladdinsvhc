/*
 * kind81.c: actor kind $81, which runs its stage's closing scene (level
 * 18's) once Aladdin stands on firm ground at x $1E0 or more: the view
 * scrolls right to $200, the chain of nine is built, the screen shakes, the
 * song plays, the screen flashes white and fades back, and the fight with
 * the chain begins. Its wait routine does nothing (84:951C RTL). Its second
 * entry in the original's table is kind $82's start (84:951D).
 *
 * The chain's building (84:948E ChainBuildNine, 84:94EE ChildChainAdd) has
 * no other caller and is its own.
 */
#include "actor/kinds_c.h"
#include "audio/sound.h"
#include "map/map.h"
#include "player/player.h"

#define SCROLL_TO 0x0200        /* the view's x the scroll ends at */
#define K81_X 0x01E0            /* Aladdin's x from which the scene begins */
#define CHAIN_X 0x02C8          /* the chain is built here */
#define CHAIN_Y 0x00C0
#define SHAKE_SLOT 1
#define SHAKE_WAIT 0x3F         /* frames before the shake and before the song */

/* its own bytes: the chain's builder keeps the last child (a record index)
   at +$40 and how many at +$42; a child its number at +$40 and, when
   another follows, that one's index at +$4E */
typedef struct {
    uint8_t before[OWN_40];
    uint8_t last[2];            /* +$40-$41: the last child made (its index in g.actors; 0 none) */
    uint8_t count;              /* +$42: children made so far */
} Kind81Own;

_Static_assert(sizeof(Kind81Own) <= OWN_BYTES, "a view fits the own bytes");

static Kind81Own *own(Actor *a) { return (Kind81Own *)a->own.bytes; }

enum {
    WAIT_GROUND = ACTOR_RESUME_OWN, /* each frame until he stands at x $1E0 or more */
    SCROLL,                         /* each frame the view scrolls right */
    WAIT_CUE,                       /* each frame until the scene's cue */
    CHAIN_BUILT,                    /* after the frame the chain's built in */
    SHAKE_SET,                      /* after its 63 frames */
    SHAKE_WAIT_OVER,                /* each frame until the shake's task is gone */
    SONG,                           /* after 63 more */
    FLASH_UP,                       /* each frame of the fixed colour rising to white */
    UPLOAD_SECOND,                  /* after the first upload's frame */
    FLASH_HELD,                     /* after 8 frames */
    FLASH_DOWN,                     /* each frame of the colour falling to black */
};

/*
 * @routine 84:94EE ChildChainAdd
 * WHAT IT DOES: Creates a child of `kind` and adds it to the actor's chain:
 * the new child's next is the last one, the last one's +$4E (word) the new
 * one's index; the child is put at the actor's place and numbered (its
 * +$40 = the count so far); it becomes the last.
 */
static void chain_add(Actor *a, uint8_t kind)
{
    Kind81Own *o = own(a);
    Actor *child = ActorCreateChild(a, kind);
    uint16_t last = (uint16_t)(o->last[0] | o->last[1] << 8);
    uint16_t me;

    if (!child)
        return;
    me = (uint16_t)(child - g.actors);
    if (last) {
        child->next = &g.actors[last];
        g.actors[last].own.bytes[OWN_4E] = (uint8_t)me;
        g.actors[last].own.bytes[OWN_4F] = (uint8_t)(me >> 8);
    }
    child->x = with_pixels(child->x, pixels(a->x));
    child->y = with_pixels(child->y, pixels(a->y));
    child->own.bytes[OWN_40] = o->count++;
    o->last[0] = (uint8_t)me;
    o->last[1] = (uint8_t)(me >> 8);
}

/*
 * @routine 84:948E ChainBuildNine
 * WHAT IT DOES: Builds the actor's chain of nine children at x 712, y 192:
 * a head of kind $73, three pieces of kind $75, five of kind $74; keeps the
 * head in g.chain_head and the tail in g.chain_tail.
 */
static void chain_build_nine(Actor *a)
{
    static const uint8_t kinds[9] = { 0x73, 0x75, 0x75, 0x75, 0x74, 0x74, 0x74, 0x74, 0x74 };
    Kind81Own *o = own(a);
    int i;

    if (g.free_pool_top >= 0)
        (void)ActorFreeListSort((uint8_t)g.free_pool_top);
    a->x = with_pixels(a->x, CHAIN_X);
    a->y = with_pixels(a->y, CHAIN_Y);
    a->spawn_at = 0;
    o->last[0] = o->last[1] = 0;
    o->count = 0;
    for (i = 0; i < 9; i++) {
        chain_add(a, kinds[i]);
        if (i == 0)
            g.chain_head = &g.actors[o->last[0] | o->last[1] << 8];
    }
    g.chain_tail = &g.actors[o->last[0] | o->last[1] << 8];
}

/* The fixed colour, all three parts, one step up or down; returns the red. */
static uint8_t colour_step(int up)
{
    int d = up ? 1 : -1;

    g.fixed_red = (uint8_t)(g.fixed_red + d);
    g.fixed_green = (uint8_t)(g.fixed_green + d);
    g.fixed_blue = (uint8_t)(g.fixed_blue + d);
    return g.fixed_red;
}

/*
 * @routine 84:9391 Kind81Scene
 * WHAT IT DOES: Actor kind $81: waits until Aladdin stands on firm ground at
 * x $1E0 or more, then takes over: his code goes to the scene's (he walks
 * right), the view scrolls right to $200, and once the scene's cue comes
 * the chain of nine is built, the screen shakes, song $22 plays, the screen
 * flashes white and fades back, and the actor ends with the fight begun.
 */
static int wait_ground(Actor *a)
{
    uint16_t low;

    if (!PlayerOnFirmGround() || pixels(g.actors[0].x) < K81_X)
        return 1;
    g.play_starts++;
    SoundQueueF6With1();
    g.actors[0].resume = PLAYER_AT_SCENE_WALK_RIGHT;
    g.scroll_x_held++;
    /* the original counts a word up at $1A05, which straddles the scroll's
       limit across ($1A04, its high byte) and down ($1A06, its low byte) */
    low = (uint16_t)((g.scroll_limit_x >> 8) | (g.scroll_limit_y & 0xFF) << 8);
    low++;
    g.scroll_limit_x = (uint16_t)((g.scroll_limit_x & 0x00FF) | (low & 0xFF) << 8);
    g.scroll_limit_y = (uint16_t)((g.scroll_limit_y & 0xFF00) | (low >> 8));
    a->resume = SCROLL;
    return 1;
}

static int scroll(Actor *a)
{
    uint16_t view = (uint16_t)(pixels(g.layers[0].scroll_x) + 3);

    g.layers[0].scroll_x = with_pixels(g.layers[0].scroll_x, view);
    if (SCROLL_TO >= view)
        return 1;
    g.layers[0].scroll_x = with_pixels(g.layers[0].scroll_x, SCROLL_TO);
    g.layers[0].edges = 0;
    g.layers[1].edges = 0;
    a->resume = WAIT_CUE;
    return 1;
}

int Kind81Code(Actor *a)
{
    switch (a->resume) {
    case ACTOR_START:
        a->resume = WAIT_GROUND;
        return 1;
    case ACTOR_START_SECOND:
        return ActorRunAs(a, Kind82Show, ACTOR_START);
    case WAIT_GROUND:
        return wait_ground(a);
    case SCROLL:
        return scroll(a);
    case WAIT_CUE:
        if (!g.scene_go_on)
            return 1;
        g.play_starts = 0;
        chain_build_nine(a);
        g.bg_map_at[0] = 0x4000;            /* BG1's map the backdrop's, 64 wide: the original's $41 */
        g.bg_map_size[0] = 1;
        a->resume = CHAIN_BUILT;
        return 1;
    case CHAIN_BUILT:
        g.main_layers = 0x15;
        a->resume = SHAKE_SET;
        return SHAKE_WAIT;
    case SHAKE_SET:
        TaskCreate(SHAKE_SLOT, 0, TASK_ENTRY_SHAKE);
        SoundQueueAdd(0x4A);
        a->resume = SHAKE_WAIT_OVER;
        return 1;
    case SHAKE_WAIT_OVER:
        if (task_busy(SHAKE_SLOT))
            return 1;
        a->resume = SONG;
        return SHAKE_WAIT;
    case SONG:
        SongPlay(0x22);
        g.layer_setup_mode = 0x12;
        /* the flash: the fixed colour added, up from black to white */
        g.fixed_red = g.fixed_green = g.fixed_blue = 0;
        g.blend_prevent = g.blend_clip = g.blend_use_sub = g.blend_direct_colour = 0;
        g.window_use[4] = 0x0;
        g.window_use[5] = 0x8;
        g.blend_layers = 0x02;
        g.blend_half = g.blend_subtract = 0;
        a->resume = FLASH_UP;
        return 1;
    case FLASH_UP:
        if (colour_step(1) != 0x1F)
            return 1;
        SoundQueueAdd(0x46);
        g.bg_map_at[1] = 0x4800;            /* BG2's map the play field's: the original's $49 */
        g.bg_map_size[1] = 1;
        g.vram_upload_number = 0x0A;
        a->resume = UPLOAD_SECOND;
        return 1;
    case UPLOAD_SECOND:
        g.vram_upload_number = 0x0B;
        a->resume = FLASH_HELD;
        return 8;
    case FLASH_HELD:
        g.line_effects_on = 0x0E;
        g.main_layers = 0x15;
        a->resume = FLASH_DOWN;
        return 1;
    case FLASH_DOWN:
        if (colour_step(0))
            return 1;
        g.blend_prevent = 0x03;             /* colour math off again: the original's $30 */
        /* $7F:F047: the high byte of the screen map's place $23 (the map and the animation sets share those bytes) */
        g.screen_map.places[0x23] = (uint16_t)((g.screen_map.places[0x23] & 0x00FF) | 0x0C00);
        g.chain_go++;
        return ActorEndPlaced(a);
    default:
        return kind_lost(a, "Kind81Code");
    }
}

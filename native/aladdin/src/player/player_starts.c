/*
 * player_starts.c: Aladdin's code as his record starts (the record's
 * player/player_starts.c): by its kind the carpet, else by its +$07 the
 * level's own start (a walk in, then the level's tasks) or one of the
 * scenes that place him.
 */
#include "data/player_tables.h"
#include "player/player_own.h"
#include "system/task.h"

#define TEXT_BLINK_SLOT 2

/* by the kind (+$06, $81:CB59): 0 the start by +$07, 1-3 the carpet; kind 7
   reads past that table, into the next (81:DDC1 PlayerScenePlaceHigh) */
/* by +$07 ($81:CB61), the entry each start goes on at */
static const struct {
    uint16_t at;                /* a resume point of his code, or 0: the level's own start */
    const char *name;           /* not converted yet: its name */
} by_07[16] = {
    { 0, NULL },                                /*  0 81:CB81 the level's own start */
    { P_CARPET_START, NULL },                   /*  1 81:E3F5 */
    { 0, "PlayerScenePlaceHigh (81:DDC1): no level plays it, not converted" },    /*  2 */
    { 0, "PlayerScenePlaceHigh (81:DDC1): no level plays it, not converted" },    /*  3 */
    { 0, "PlayerScenePlaceLow (81:DDE0): no level plays it, not converted" },     /*  4 */
    { PLAYER_AT_START_DROPPED, NULL },      /*  5 */
    { PLAYER_AT_START_NEW_GAME, NULL },      /*  6 */
    { PLAYER_AT_STORY, NULL },             /*  7 */
    { 0, "PlayerSceneJumpIn (81:DF6F): no level plays it, not converted" },       /*  8 */
    { PLAYER_AT_SCENE_LAND, NULL },         /*  9 */
    { PLAYER_AT_SCENE_WALK_ON, NULL },       /* 10 */
    { PLAYER_AT_SCENE_WALK_RIGHT, NULL },    /* 11 */
    { 0, "PlayerSceneCued (81:E154): no level plays it, not converted" },         /* 12 */
    { 0, "PlayerSceneWalkOut (81:E0AE): no level plays it, not converted" },      /* 13 */
    { PLAYER_AT_ENDING_RIDE, NULL },        /* 14 */
    { PLAYER_AT_SCENE_STAND, NULL },        /* 15 */
};

/*
 * @routine 81:CBD1 PlayerStartTasks
 * WHAT IT DOES: The end of his start: below level $14 the text blink's task
 * starts; he stands still, and on into standing.
 */
int player_start_tasks(Actor *a)
{
    if (g.level < 0x14)
        TaskCreate(TEXT_BLINK_SLOT, 0, TASK_ENTRY_TEXT_BLINK);
    ActorSpeedXClear(a);
    PlayerMoveStart(a, 0x00);
    return PlayerGoTo(a, PLAYER_AT_STAND);
}

/* the level's own start ($07 0) */
static int level_start(Actor *a)
{
    uint8_t speed = 0x03;

    a->flags = 0xD0;
    a->own.bytes[OWN_0A] = 0x40;
    a->own_pictures = player_pictures();    /* his picture list (+$29) */
    a->redraw = 0xFF;
    g.player_flags &= 0x7F;
    if (g.level_restart)
        return player_start_tasks(a);
    if (g.level >= 0x14) {
        if (g.level == 0x1A)
            (void)ActorSpawnShared(a, 0x60);
        ActorSpeedXClear(a);
        PlayerMoveStart(a, 0x00);
        return PlayerGoTo(a, PLAYER_AT_STAND);
    }
    /* the walk in: 32 frames ($68 on level 6, faster) */
    PlayerMoveStart(a, 0x02);
    WAIT_FRAMES(a) = 0x20;
    if (g.level == 0x06) {
        WAIT_FRAMES(a) = 0x68;
        speed = 0x6C;
    }
    ActorSpeedXSet(a, speed);
    a->resume = P_WALK_IN_FRAME;
    return 1;
}

static int walk_in_frame(Actor *a)
{
    ActorMoveX(a);
    snap_to_ground(a);
    if (--WAIT_FRAMES(a))
        return 1;
    return player_start_tasks(a);
}

/*
 * @routine 81:CB3B PlayerStart
 * WHAT IT DOES: His record starts: the camera's aim and his first test
 * box; then by his kind the carpet, or by his +$07 the level's own start
 * (he walks in, 32 frames or $68 on level 6, unless the level restarts),
 * the carpet, or one of the scenes that place him.
 */
static int start(Actor *a)
{
    uint8_t n = a->spawn_byte;

    g.camera_aim_x = 0x0080;
    g.player_box_wanted = 0;
    PlayerBoxSet(a, 0x00);
    if (a->kind >= 1 && a->kind <= 3)
        return PlayerGoTo(a, P_CARPET_START);
    if (a->kind == 7)
        return player_unconverted(a, "PlayerScenePlaceHigh (81:DDC1): no level plays it, not converted");
    if (a->kind != 0 || n >= 16)
        return player_unconverted(a, "PlayerStart: a kind or +$07 past its tables");
    if (by_07[n].name)
        return player_unconverted(a, by_07[n].name);
    if (by_07[n].at)
        return PlayerGoTo(a, by_07[n].at);
    return level_start(a);
}

int player_starts_code(Actor *a)
{
    switch (a->resume) {
    case PLAYER_AT_START:   return start(a);
    case P_WALK_IN_FRAME:   return walk_in_frame(a);
    default:                return player_unconverted(a, "a resume point of player_starts.c");
    }
}

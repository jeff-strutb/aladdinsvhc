/*
 * player_starts.c: Aladdin's code as his record starts (the record's
 * player/player_starts.c): by its kind the carpet, else by its +$07 the
 * level's own start (a walk in, then the level's tasks) or one of the
 * scenes that place him.
 */
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
    { 0, "PlayerCarpetStart (81:E3F5)" },       /*  1 */
    { 0, "PlayerScenePlaceHigh (81:DDC1)" },    /*  2 */
    { 0, "PlayerScenePlaceHigh (81:DDC1)" },    /*  3 */
    { 0, "PlayerScenePlaceLow (81:DDE0)" },     /*  4 */
    { 0, "PlayerStartDropped (81:DDFF)" },      /*  5 */
    { 0, "PlayerStartNewGame (81:DE32)" },      /*  6 */
    { 0, "PlayerStory (81:DE79)" },             /*  7 */
    { 0, "PlayerSceneJumpIn (81:DF6F)" },       /*  8 */
    { 0, "PlayerSceneLand (81:DFBB)" },         /*  9 */
    { 0, "PlayerSceneWalkOn (81:E042)" },       /* 10 */
    { 0, "PlayerSceneWalkRight (81:E126)" },    /* 11 */
    { 0, "PlayerSceneCued (81:E154)" },         /* 12 */
    { 0, "PlayerSceneWalkOut (81:E0AE)" },      /* 13 */
    { 0, "PlayerEndingRide (81:E27A)" },        /* 14 */
    { 0, "PlayerSceneStand (81:E23E)" },        /* 15 */
};

/*
 * @routine 81:CBD1 PlayerStartTasks
 * WHAT IT DOES: The end of his start: below level $14 the text blink's task
 * starts; he stands still, and on into standing.
 */
static int start_tasks(Actor *a)
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
    a->own.bytes[OWN_29] = 0x07;        /* his picture list's word, $8907 */
    a->own.bytes[OWN_2A] = 0x89;
    a->redraw = 0xFF;
    g.player_flags &= 0x7F;
    if (g.level_restart)
        return start_tasks(a);
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
    return start_tasks(a);
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
        return player_unconverted(a, "PlayerCarpetStart (81:E3F5)");
    if (a->kind == 7)
        return player_unconverted(a, "PlayerScenePlaceHigh (81:DDC1)");
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

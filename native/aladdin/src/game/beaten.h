/*
 * beaten.h: the levels the player has beaten, kept in a file of their own in the engine's save folder, and the start of a
 * level chosen from that list (docs/aladdin/native/LEVELSELECT.md).
 */
#ifndef ALADDIN_BEATEN_H
#define ALADDIN_BEATEN_H

#include <stdint.h>

#define BEATEN_LEVELS 19            /* the levels in the game's progress, parts 0 to 6 (the ending's stages are not levels) */

/* the list: every level the player can choose, in the order the game plays them */
unsigned BeatenListLevel(unsigned i);       /* the level number of entry i (0 to BEATEN_LEVELS - 1) */
unsigned BeatenListPart(unsigned i);        /* its progress sequence, from 0 */
unsigned BeatenListStage(unsigned i);       /* its stage in it, from 0 */
unsigned BeatenListOf(unsigned level);      /* the entry of a level number, or BEATEN_LEVELS when it is not in the list */

/* the record */
int BeatenIs(unsigned level);               /* 1 when the level is beaten */
unsigned BeatenCount(void);                 /* how many are */
unsigned BeatenFind(unsigned i);            /* the list entries of the beaten levels, in order: entry i of them, as an index of the list */

/* Marks the level being played (g.level) as beaten, and writes the file when that is new. It reads the game's state and
   changes none of it. Called where a level's exit is passed. */
void LevelBeaten(void);

/* Sets the game's state as the title's GAME START with a new game's counts would have it, at the level of list entry `i`:
   the progress, the level, and the counts (3 lives, 3 health marks, 10 apples, nothing else carried), then the game not begun, so
   GameStartTask takes its new game path. The caller goes on as the title does for GAME START (sound, fade, TASK_ENTRY_GAME_START). */
void LevelSelectStart(unsigned i);

#endif

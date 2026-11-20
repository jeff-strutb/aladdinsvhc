/*
 * level.h: the level's own running parts (the record's level/): its value
 * sequences. Stage 5's level code adds the rest.
 */
#ifndef ALADDIN_LEVEL_H
#define ALADDIN_LEVEL_H

#include "system/state.h"

/* sequences.c */
void SequenceStart(Sequence *s);        /* 81:EC6F */
void SequencesStep(void);               /* 81:EC23 */

#endif

/*
 * code_table.h: the functions an actor record can run in, by name (actor/code_table.c, made by tools/gen_code_table.py).
 */
#ifndef ACTOR_CODE_TABLE_H
#define ACTOR_CODE_TABLE_H

#include <stdint.h>

#include "actor/actor.h"

typedef struct ActorCodeEntry {
    const char *name;
    ActorCode code;
} ActorCodeEntry;

extern const ActorCodeEntry actor_code_table[];     /* ends with a { 0, 0 } */

/* the id an actor's code is written as (the hash of its name), 0 for NULL; -1 when the function is in no table */
int64_t actor_code_id(ActorCode code);
/* the function an id names, or NULL when the id is 0 or unknown (*known is 0 then for an unknown id) */
ActorCode actor_code_of(uint32_t id, int *known);
/* every id is of one name: ends the run when two names hash alike (made once, at the first use) */
void actor_code_check(void);

#endif

/*
 * code_id.c: an actor's code as an id (code_table.h): the 32-bit FNV-1a hash of the function's name.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "actor/code_table.h"

static uint32_t hash_name(const char *s)
{
    uint32_t h = 2166136261u;

    for (; *s; s++)
        h = (h ^ (uint8_t)*s) * 16777619u;
    return h ? h : 1u;              /* 0 is NULL */
}

void actor_code_check(void)
{
    static int done;

    if (done)
        return;
    done = 1;
    for (const ActorCodeEntry *a = actor_code_table; a->name; a++)
        for (const ActorCodeEntry *b = a + 1; b->name; b++)
            if (hash_name(a->name) == hash_name(b->name) || a->code == b->code) {
                fprintf(stderr, "actor code table: %s and %s have one id or one function\n", a->name, b->name);
                exit(1);
            }
}

int64_t actor_code_id(ActorCode code)
{
    if (!code)
        return 0;
    for (const ActorCodeEntry *e = actor_code_table; e->name; e++)
        if (e->code == code)
            return hash_name(e->name);
    return -1;
}

ActorCode actor_code_of(uint32_t id, int *known)
{
    *known = 1;
    if (!id)
        return NULL;
    for (const ActorCodeEntry *e = actor_code_table; e->name; e++)
        if (hash_name(e->name) == id)
            return e->code;
    *known = 0;
    return NULL;
}

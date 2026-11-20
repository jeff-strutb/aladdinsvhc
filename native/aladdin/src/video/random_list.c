/*
 * random_list.c: a random entry of a list.
 */
#include "data/video_tables.h"
#include "system/system.h"
#include "video/video.h"

/*
 * @routine 81:AE8A RandomFromList
 * WHAT IT DOES: Picks a random entry of list `list` (the record's X / 2):
 * RandomNext masked by the list's mask picks it. Returns the entry, and in
 * the high byte the byte after it (the original leaves both in A).
 */
uint16_t RandomFromList(uint8_t list)
{
    uint8_t r = (uint8_t)(RandomNext() & random_list_byte(list, 0));

    return (uint16_t)(random_list_byte(list, 1u + r) | random_list_byte(list, 2u + r) << 8);
}

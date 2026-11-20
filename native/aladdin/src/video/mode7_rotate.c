/*
 * mode7_rotate.c: Mode 7's matrix set for a rotation by an angle.
 * (Mode7ZoomInTask is in tasks.c.)
 */
#include "data/video_tables.h"
#include "system/state.h"
#include "video/video.h"

/*
 * @routine 81:B25B RotateSine
 * WHAT IT DOES: Entry `at` (a byte offset) of the quarter-turn sine table,
 * divided by 16.
 */
static int16_t rotate_sine(unsigned at)
{
    return (int16_t)(rotate_sine_word(at) >> 4);
}

/*
 * @routine 81:B1CD Mode7Rotate
 * WHAT IT DOES: Sets Mode 7's matrix to a rotation by `angle` (256 to a
 * turn): a and d the cosine, b the sine and c its negative.
 * HOW: The angle's place in its quarter gives one entry of the quarter
 * table and its complement the other; the quarter picks which is the sine
 * and the signs.
 */
void Mode7Rotate(uint8_t angle)
{
    unsigned at = (unsigned)(angle & 0x3F) << 1;
    int16_t s = rotate_sine(at), c = rotate_sine((at ^ 0x7Fu) + 1);
    int16_t *m = g.m7_matrix;           /* a, b, c, d */

    switch (angle >> 6) {
    case 0:
        m[1] = s;
        m[2] = (int16_t)-s;
        m[0] = m[3] = c;
        break;
    case 1:
        m[0] = m[3] = (int16_t)-s;
        m[1] = c;
        m[2] = (int16_t)-c;
        break;
    case 2:
        m[2] = s;
        m[1] = (int16_t)-s;
        m[0] = m[3] = (int16_t)-c;
        break;
    default:
        m[0] = m[3] = s;
        m[2] = c;
        m[1] = (int16_t)-c;
        break;
    }
}

/*
 * @routine 81:B0FB Mode7RotateByTable
 * Dead: no code refers to it (the record); no long pointer to 81:B0FB in
 * the cartridge and no word $B0FB in bank $81 (video/NOTES).
 */

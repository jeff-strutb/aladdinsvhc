/*
 * text_number.c: a two-digit number written into the text layer's map.
 */
#include "system/state.h"
#include "video/video.h"

#define DIGIT_ATTRIBUTES 0x28

/*
 * @routine 83:8BDE TextNumberTwoDigits
 * WHAT IT DOES: Writes `number` as two digits ($30 + the tens and $30 + the
 * ones, as the original's divider gives them for 0-255) into the text map
 * at byte offset `at`, each with attribute $28, and has the map sent.
 * HOW: TextNumberTwoDigitsLong (83:8BDA), its call for other banks, is this.
 */
void TextNumberTwoDigits(uint8_t number, uint16_t at)
{
    unsigned i = at / 2u;

    if (i + 1 < 32 * 32) {
        g.text_map[i] = (uint16_t)(DIGIT_ATTRIBUTES << 8 | (uint8_t)(number / 10 + 0x30));
        g.text_map[i + 1] = (uint16_t)(DIGIT_ATTRIBUTES << 8 | (uint8_t)(number % 10 + 0x30));
    }
    g.text_map_upload++;
}

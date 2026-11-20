/*
 * player_tables.h: the cartridge's tables the player's code reads
 * (data/player_tables.c holds their places). Each reader gives an entry by
 * what it holds.
 */
#ifndef ALADDIN_PLAYER_TABLES_H
#define ALADDIN_PLAYER_TABLES_H

#include <stdint.h>

#include "data/data.h"

/* PadFacing (81:CE7C): the facing for pad 1's Right (bit 0) and Left (bit 1):
   0 right, 1 left, $FF neither or both */
uint8_t pad_facing(uint8_t right_left);

/* AnimOfMove (81:DCB1): the animation an actor of `kind` plays for `move` */
uint8_t anim_of_move(uint8_t kind, uint8_t move);

/* PlayerAirSteer (81:CF2E), by the way held (0-3, the low two bits of the
   ground's direction) and the dash (0 or 1) */
uint8_t air_accel_toward(uint8_t way, uint8_t dash);    /* the acceleration with the facing */
uint8_t air_accel_against(uint8_t way);                 /* against it */
uint16_t air_top_toward(uint8_t way, uint8_t dash);     /* the speed it stops at, with the facing (8.8) */
uint16_t air_least_against(uint8_t way, uint8_t dash);  /* the least it slows to, against it */
uint16_t air_settle(uint8_t dash);                      /* the speed it settles to, no way held */

/* PlayerJump (81:CFE0): the jump's motion preset by the low two bits of his
   speed's pixels */
uint8_t jump_preset(uint8_t speed_low_bits);

/* PlayerAppleThrow (81:CDE9), by pose (0-3, the box wanted's low two bits) */
uint16_t throw_x(uint8_t pose);         /* the hand's x from his, along his flip */
uint16_t throw_y(uint8_t pose);         /* its height above his y */
uint8_t throw_course(uint8_t pose);     /* the apple's +$07 */

/* PlayerBoxSet (81:E911): the player's boxes, 10 bytes each */
typedef struct PlayerBoxEntry {
    uint16_t offset_x;          /* the test box's centre from him, facing right */
    uint16_t offset_y;
    uint16_t half_w, half_h;
    DataRef probes;             /* his probe list, as his record's +$16 gets it (map/'s tests read it in bank $80) */
} PlayerBoxEntry;
PlayerBoxEntry player_box_entry(uint8_t box);

/* PlayerFrameWork (81:E864): the camera's aim across, by how the camera
   last moved (1 right, 2 left) */
uint8_t camera_aim_of_push(uint8_t push);

/* PlayerKeepOnScreenX, PlayerKeepOnScreen (81:E990, 81:E9AD): the least and
   the most pixel from the play field's left (side 0) and its top (side 1) */
uint16_t screen_limit_low(uint8_t side);
uint16_t screen_limit_high(uint8_t side);

#endif

/*
 * player_tables.c: the places of the tables the player's code reads
 * (player_tables.h). All are in bank $80, where the player's code runs
 * with its data bank.
 */
#include "data/player_tables.h"
#include "data/data_internal.h"

static uint8_t at8(uint16_t addr) { return cart8(cart_offset(0x80, addr)); }
static uint16_t at16(uint16_t addr) { return cart16(cart_offset(0x80, addr)); }

/* $80:C72C: four bytes by the Left and Right bits */
uint8_t pad_facing(uint8_t right_left) { return at8((uint16_t)(0xC72C + (right_left & 0x03))); }

/* $80:C9D3: by kind its row's offset in the same table, then by the row's
   offset plus the move (8-bit, as the original's X) the animation */
uint8_t anim_of_move(uint8_t kind, uint8_t move)
{
    uint8_t row = at8((uint16_t)(0xC9D3 + kind));

    return at8((uint16_t)(0xC9D3 + (uint8_t)(row + move)));
}

/* $80:C781 bytes by way * 2 + dash; $80:C787 bytes by way; $80:C78A and
   $80:C796 words by way * 2 + dash; $80:C7A2 words by dash */
uint8_t air_accel_toward(uint8_t way, uint8_t dash) { return at8((uint16_t)(0xC781 + 2u * way + dash)); }
uint8_t air_accel_against(uint8_t way) { return at8((uint16_t)(0xC787 + way)); }
uint16_t air_top_toward(uint8_t way, uint8_t dash) { return at16((uint16_t)(0xC78A + 4u * way + 2u * dash)); }
uint16_t air_least_against(uint8_t way, uint8_t dash) { return at16((uint16_t)(0xC796 + 4u * way + 2u * dash)); }
uint16_t air_settle(uint8_t dash) { return at16((uint16_t)(0xC7A2 + 2u * dash)); }

/* $80:C730: four bytes */
uint8_t jump_preset(uint8_t speed_low_bits) { return at8((uint16_t)(0xC730 + (speed_low_bits & 0x03))); }

/* $80:C718, $80:C720, $80:C728: words by pose (the course's low byte; its
   last two entries are the bytes of PadFacing's table, as the original
   reads them) */
uint16_t throw_x(uint8_t pose) { return at16((uint16_t)(0xC718 + 2u * (pose & 0x03))); }
uint16_t throw_y(uint8_t pose) { return at16((uint16_t)(0xC720 + 2u * (pose & 0x03))); }
uint8_t throw_course(uint8_t pose) { return at8((uint16_t)(0xC728 + 2u * (pose & 0x03))); }

/* $80:C7EB: 10 bytes a box (the box times 10, 8-bit as the original's X) */
PlayerBoxEntry player_box_entry(uint8_t box)
{
    uint16_t e = (uint16_t)(0xC7EB + (uint8_t)(box * 10u));
    PlayerBoxEntry b;

    b.offset_x = at16(e);
    b.offset_y = at16((uint16_t)(e + 2));
    b.half_w = at16((uint16_t)(e + 4));
    b.half_h = at16((uint16_t)(e + 6));
    b.probes.at = cart_offset(0x80, at16((uint16_t)(e + 8)));
    return b;
}

/* $80:C7E8: bytes by the camera's push */
uint8_t camera_aim_of_push(uint8_t push) { return at8((uint16_t)(0xC7E8 + push)); }

/* $80:C9B9 (across) and $80:C9C9 (down): the least word, then the most */
uint16_t screen_limit_low(uint8_t side) { return at16(side ? 0xC9C9 : 0xC9B9); }
uint16_t screen_limit_high(uint8_t side) { return at16(side ? 0xC9CB : 0xC9BB); }

/* $82:BE92: five words a box, at the box's byte offset (the original's Y) */
LandBox land_box(uint16_t box)
{
    uint32_t o = cart_offset(0x82, (uint16_t)(0xBE92 + box));
    LandBox b;

    b.x_offset = cart16(o);
    b.width = cart16(o + 2);
    b.y_offset = cart16(o + 4);
    b.height = cart16(o + 6);
    b.below = cart16(o + 8);
    return b;
}

/* $80:FE44: an x byte and a y byte by offset number (the number times 2,
   8-bit as the original's X) */
uint8_t attach_offset_x(uint8_t n) { return at8((uint16_t)(0xFE44 + (uint8_t)(n << 1))); }
uint8_t attach_offset_y(uint8_t n) { return at8((uint16_t)(0xFE45 + (uint8_t)(n << 1))); }

/* $80:C734, $80:C748, $80:C75C, $80:C770: bytes by an 8-bit index */
uint8_t held_preset(uint8_t frame) { return at8((uint16_t)(0xC734 + frame)); }
uint8_t held_spawn_at(uint8_t frame) { return at8((uint16_t)(0xC748 + frame)); }
uint8_t held_box(uint8_t frame) { return at8((uint16_t)(0xC75C + frame)); }
uint8_t bounce_row(uint8_t at) { return at8((uint16_t)(0xC770 + at)); }

/* $80:C7AA: 16 bytes; $80:C7B5 and $80:C7CB: two bytes a direction (across,
   then up and down; the direction times 2, 8-bit); $80:C7E1: a byte a
   direction */
uint8_t carpet_direction(uint8_t pad_low4) { return at8((uint16_t)(0xC7AA + (pad_low4 & 0x0F))); }
uint8_t carpet_rate_x(int alt, uint8_t direction) { return at8((uint16_t)((alt ? 0xC7CB : 0xC7B5) + (uint8_t)(direction << 1))); }
uint8_t carpet_rate_y(int alt, uint8_t direction) { return at8((uint16_t)((alt ? 0xC7CC : 0xC7B6) + (uint8_t)(direction << 1))); }
uint8_t carpet_tilt(uint8_t direction) { return at8((uint16_t)(0xC7E1 + direction)); }

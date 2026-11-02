/*
 * still_actors.c: actors that only stand, drift or fly off: kinds $7D
 * (keeps its spot from a table by its height), $7A (flies its course for
 * good), $4A (its own palette, an animation, nothing more), and the fly-
 * off end of the actor at $83:C763. Drafted by cdraft.py; named and
 * described by hand.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "wram.h"
#include "actor.h"
#include "actor_anim.h"

void abi_ActorEndPlaced(void);
void abi_ActorFly(void);
void abi_ActorKeepSpot(void);
void abi_ActorMotionPreset(void);
void abi_AnimStart(void);
void abi_PaletteLoad(void);

#define KIND7D_16 0x16u  /* word: $D820 */
#define KIND7D_SPOTS 0xD7D2u  /* by its y pixels' low six bits, two bytes kept in $40 and $41 (in the bank DB names) */
#define FLYOFF_LEFT 0x41u  /* frames left of its flight */

/*
 * @routine 84:90CE Kind7DKeepSpot
 * @actor
 * WHAT IT DOES: Actor kind $7D: takes two bytes from KIND7D_SPOTS by the
 * low six bits of its y (kept as its $07), then each frame keeps its place
 * in $4C and $4E (ActorKeepSpot), for good.
 * HOW: $09 = $80; $16 = $D820 (two 8-bit stores); $07 = y pixels and $3F;
 * KIND7D_SPOTS at twice it to $40 and $41; each frame (BRK first)
 * ActorKeepSpot.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $84; reached by
 * the actor loop's jump for kind $7D in state $0C (through the stub at
 * $83:929B). Exit: never returns.
 */
void abi_Kind7DKeepSpot(void)
{
    uint8_t spot;

    a8_load(0x80);
    wr8(dpa(ACTOR_09), 0x80);
    a8_load(0x20);
    wr8(dpa(KIND7D_16), 0x20);
    a8_load(0xD8);
    wr8(dpa(KIND7D_16 + 1), 0xD8);
    spot = rd8(dpa(ACTOR_Y + 1)) & 0x3F;
    wr8(dpa(ACTOR_07), spot);
    cpu.x = (uint8_t)(spot << 1);
    flag_set(P_C, 0);
    a8_load(rd8(dba(KIND7D_SPOTS) + cpu.x));
    wr8(dpa(ACTOR_40), (uint8_t)cpu.a);
    a8_load(rd8(dba(KIND7D_SPOTS + 1) + cpu.x));
    wr8(dpa(FLYOFF_LEFT), (uint8_t)cpu.a);
    for (;;) {
        actor_yield(0x8490ECu);
        abi_ActorKeepSpot();
    }
}


/*
 * @routine 83:C925 ActorFlyOffEnd
 * @actor
 * WHAT IT DOES: The end of the actor at $83:C763: it can no longer be hit,
 * flies off on motion $2B for $18 frames with animation 3, and ends.
 * HOW: Bit 7 of $09 cleared; A = $80 shifted right, C clear, so Y = $2A +
 * 1; ActorMotionPreset; AnimStart 3; $41 = $18; each frame ActorFly (BRK
 * first) until $41 is counted out; then ActorEndPlaced.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $83; reached by a
 * jump from that actor's code (83:C773). Exit: never returns; ends in
 * ActorEndPlaced.
 */
void abi_ActorFlyOffEnd(void)
{
    a8_load(0x80);
    trb_at(dpa(ACTOR_09), 0x80);          /* it can no longer be hit */
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x40);
    flag_set(P_C, 0);
    y8_load(0x2B);                        /* LDY #$2A; INY (the LSR left C clear) */
    abi_ActorMotionPreset();
    a8_load(0x03);
    abi_AnimStart();
    a8_load(0x18);
    wr8(dpa(FLYOFF_LEFT), 0x18);
    do {
        actor_yield(0x83C93Du);
        abi_ActorFly();
    } while (dec8_at(dpa(FLYOFF_LEFT)));
    abi_ActorEndPlaced();
}


/*
 * @routine 83:F757 Kind7AFly
 * @actor
 * WHAT IT DOES: Actor kind $7A: facing right, shown with animation 0, it
 * flies its course (motion $53) for good.
 * HOW: Facing and flip 0; $08 bit 1 and $09 bit 4 set; AnimStart 0;
 * ActorMotionPreset $53; each frame (BRK first) ActorFly.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $83; reached by
 * the actor loop's jump for kind $7A in state $0C. Exit: never returns.
 */
void abi_Kind7AFly(void)
{
    wr8(dpa(ACTOR_FACING), 0);
    wr8(dpa(ACTOR_FLIP), 0);
    wr8(dpa(ACTOR_08), (uint8_t)(rd8(dpa(ACTOR_08)) | 0x02));
    wr8(dpa(ACTOR_09), (uint8_t)(rd8(dpa(ACTOR_09)) | 0x10));
    a8_load(0x00);
    abi_AnimStart();
    y8_load(0x53);
    abi_ActorMotionPreset();
    for (;;) {
        actor_yield(0x83F773u);
        abi_ActorFly();
    }
}


/*
 * @routine 83:B496 Kind4AShow
 * @actor
 * WHAT IT DOES: Actor kind $4A: facing right, with its look $0600, loads
 * palette set $0A, shows animation 6 and stays.
 * HOW: $09 bits $90 set; facing and flip 0; its look $0600; PaletteLoad
 * $0A; AnimStart 6; BRK for good.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $83; reached by
 * the actor loop's jump for kind $4A in state $0C. Exit: never returns.
 */
void abi_Kind4AShow(void)
{
    wr8(dpa(ACTOR_09), (uint8_t)(rd8(dpa(ACTOR_09)) | 0x90));
    wr8(dpa(ACTOR_FACING), 0);
    wr8(dpa(ACTOR_FLIP), 0);
    dp16_set(ACTOR_LOOK, 0x0600);
    cpu.a = 0x0600;
    a8_load(0x0A);
    abi_PaletteLoad();
    a8_load(0x06);
    abi_AnimStart();
    for (;;)
        actor_yield(0x83B4B5u);
}

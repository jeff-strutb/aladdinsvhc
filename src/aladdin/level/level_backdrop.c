/*
 * level_backdrop.c: what a level does to its backdrop layer as it starts,
 * by level from the table at $81:8E0E: most levels nothing; level 6 has
 * the backdrop draw no edges; levels 7 and $17 also clear its tilemap.
 */
#include "machine.h"
#include "native.h"
#include "hw.h"
#include "wram_video.h"

#define BACKDROP_STARTS 0x818E0Eu   /* the handlers, by level (words, this bank) */
#define BACKDROP_EDGES (LAYER1 + LAYER_EDGES) /* the edges the backdrop layer draws */
#define BACKDROP_TILEMAP 0x3800u    /* its tilemap's word address in VRAM */
#define BACKDROP_TILEMAP_WORDS 0x1000u

/*
 * @routine 81:8E06 LevelBackdropStart
 * WHAT IT DOES: The level's own start for its backdrop layer: none for
 * most levels; for level 6 the backdrop draws no new edges as it scrolls;
 * for levels 7 and $17 its tilemap (VRAM $3800, $1000 words) is cleared
 * as well.
 * HOW: JMP through BACKDROP_STARTS by LEVEL: an RTS (levels 0-5 each have
 * their own, the others share one); level 6: BACKDROP_EDGES 0; levels 7
 * and $17: VMAIN $80, VMADD $3800, $1000 zero words to VMDATA (16-bit
 * registers), then BACKDROP_EDGES 0. The RTSes of levels 1-3 are not run
 * by a script.
 * Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM and
 * $2100-$21FF the PPU; called by JSR from the level's start (81:8222).
 * Exit: M 8-bit, X 8-bit; most levels: A = the level times 2, X too, N, Z
 * and C from the shift and the TAX; levels 7 and $17: A = $3800, X = 0, Z
 * set, N clear.
 */
void abi_LevelBackdropStart(void)
{
    uint16_t to, n;
    uint8_t level = rd8(dba((uint16_t)LEVEL));

    flag_set(P_C, level & 0x80);
    a8_load((uint8_t)(level << 1));
    x8_load((uint8_t)cpu.a);
    to = rd16(BACKDROP_STARTS + cpu.x);
    if (to >= 0x8E96 && to <= 0x8E9C)
        return;                     /* RTS */
    if (to == 0x8E9D) {
        wr8(dba((uint16_t)BACKDROP_EDGES), 0);
        return;
    }
    if (to == 0x8EA1) {
        a8_load(0x80);
        wr8(dba((uint16_t)VMAIN), 0x80);
        op_rep(0x30);
        a16_load(BACKDROP_TILEMAP);
        wr16(dba((uint16_t)VMADDL), BACKDROP_TILEMAP);
        for (n = BACKDROP_TILEMAP_WORDS; n; n--)
            wr16(dba((uint16_t)VMDATAL), 0);
        cpu.x = 0;
        nz16_of(0);
        op_sep(0x30);
        wr8(dba((uint16_t)BACKDROP_EDGES), 0);
        return;
    }
    jump_routine(0x810000u | to);   /* past the table's end: never */
}

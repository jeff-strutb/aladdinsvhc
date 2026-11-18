/*
 * collision.c: what map tile is at a point, and how high its surface is.
 *
 * An actor tests the map at probe points: offsets from its position, in a
 * list its record points to. A point's tile comes from the screen map (the
 * screen under the point) and the screen's tiles; the tile's behaviour byte
 * gives its kind (empty, solid, slope ...) and, for a slope, the surface's
 * height at the point's x. The point and the results are kept in scratch:
 * x $0C, y $0E, surface y $10, behaviour $12, kind $13, screen place $14,
 * tile place $16, screen tiles offset $18, mirrored $1A, tile word $1C.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "actor.h"
#include "wram.h"
#include "wram_video.h"

static uint32_t abs_db(uint16_t addr)
{
    return ((uint32_t)cpu.db << 16) | addr;
}

static uint32_t dp(uint8_t off)
{
    return (uint16_t)(cpu.d + off);
}

/* The probe list (its address, word) and the probe's offset in it. */
#define ACTOR_PROBES 0x16u
#define ACTOR_PROBE_AT 0x18u

#define POINT_X (SCRATCH + 0x00)
#define POINT_Y (SCRATCH + 0x02)
#define SURFACE_Y (SCRATCH + 0x04)
#define TILE_BEHAVIOUR_BYTE (SCRATCH + 0x06)
#define TILE_KIND (SCRATCH + 0x07)
#define SCREEN_PLACE (SCRATCH + 0x08)
#define TILE_PLACE (SCRATCH + 0x0A)
#define SCREEN_TILES (SCRATCH + 0x0C)
#define TILE_MIRRORED (SCRATCH + 0x0E)
#define TILE_WORD (SCRATCH + 0x10)
#define SLOPE_DROP (SCRATCH + 0x12)

#define SCREEN_MAP 0x7FF000u       /* per screen place, its tiles' offset in LEVEL_TILES (word) */
#define LEVEL_TILES 0x7F0000u      /* the screens' tiles, a word each */
/* Per behaviour's low 3 bits, the tile's kind (bank $81). */
#define TILE_KINDS 0xBF22u
#define KIND_SOLID 0x01
#define SLOPE_FLAT 0x70            /* a kind whose bits 4-6 are all set: flat, at its low-nibble height */
/* A view of the map 240 pixels high from the camera. */
#define VIEW_HEIGHT 0x00F0
#define CAMERA_Y (LAYER0 + LAYER_SCROLL_Y)
/* Levels whose map the player tests off the view's top and bottom too. */
#define LEVEL_TALL_A 0x000A
#define LEVEL_TALL_B 0x000F

/* The probe list's entry at Y: (D's list), Y (the data bank's). */
static uint16_t probe(uint16_t y)
{
    return rd16(((uint32_t)cpu.db << 16) + rd16(dp(ACTOR_PROBES)) + y);
}

/* The point's y: the probe's next word plus the actor's y. Leaves Y past
   it, X 16-bit. */
static void probe_y(void)
{
    uint16_t off;

    cpu.y = (uint8_t)(cpu.y + 2);
    cpu.p &= (uint8_t)~P_C;
    off = probe(cpu.y);
    cpu.a = op_adc16(off, rd16(dp(ACTOR_Y + 1)), &cpu.p);
    wr16(abs_db(POINT_Y), cpu.a);
    op_rep(P_X);
}

/*
 * @routine 81:B4AC ProbeAtActor
 * WHAT IT DOES: Makes the probe point at the actor's x and its probe's y.
 * HOW: Point x = the actor's x; point y = the probe's y offset plus the
 * actor's y (the probe at $18 in the list at $16).
 * Entry: X 8-bit; D = the actor; DB a bank whose $0000-$1FFF is work RAM
 * and that holds the probe list; called by JSR. Exit: M 16-bit, X 16-bit;
 * A = the point's y; Y = the probe's offset + 2; N, V, Z, C from its ADC;
 * X unchanged.
 */
void abi_ProbeAtActor(void)
{
    op_rep(P_M);
    op_sep(P_X);
    cpu.y = rd8(dp(ACTOR_PROBE_AT));
    cpu.a = rd16(dp(ACTOR_X + 1));
    wr16(abs_db(POINT_X), cpu.a);
    probe_y();
}

/*
 * @routine 81:B4B9 ProbeLeft
 * WHAT IT DOES: Makes the probe point to the actor's left: the probe's x
 * offset back from it, and its y offset down.
 * HOW: Point x = the actor's x less the probe's x offset; point y = its y
 * offset plus the actor's y.
 * Entry: X 8-bit; D = the actor; DB a bank whose $0000-$1FFF is work RAM
 * and that holds the probe list; called by JSR. Exit: as ProbeAtActor.
 */
void abi_ProbeLeft(void)
{
    uint16_t ax;

    op_rep(P_M);
    op_sep(P_X);
    cpu.y = rd8(dp(ACTOR_PROBE_AT));
    cpu.p |= P_C;
    ax = rd16(dp(ACTOR_X + 1));
    cpu.a = op_sbc16(ax, probe(cpu.y), &cpu.p);
    wr16(abs_db(POINT_X), cpu.a);
    probe_y();
}

/*
 * @routine 81:B4C9 ProbeRight
 * WHAT IT DOES: Makes the probe point to the actor's right: the probe's x
 * offset on from it, and its y offset down.
 * HOW: Point x = the probe's x offset plus the actor's x; point y = its y
 * offset plus the actor's y.
 * Entry: X 8-bit; D = the actor; DB a bank whose $0000-$1FFF is work RAM
 * and that holds the probe list; called by JSR. Exit: as ProbeAtActor.
 */
void abi_ProbeRight(void)
{
    uint16_t off;

    op_rep(P_M);
    op_sep(P_X);
    cpu.y = rd8(dp(ACTOR_PROBE_AT));
    cpu.p &= (uint8_t)~P_C;
    off = probe(cpu.y);
    cpu.a = op_adc16(off, rd16(dp(ACTOR_X + 1)), &cpu.p);
    wr16(abs_db(POINT_X), cpu.a);
    probe_y();
}

/* The player's point off the view's top or bottom (where that is tested):
   the original's ProbeOnView leaves through its caller's frame then. Makes
   ProbeOnView's own steps; returns nonzero when the point is off. */
static int probe_off_view(void)
{
    uint16_t cam, lv;

    cpu.a = cpu.d;                             /* TDC */
    cpu.p = nz16(cpu.p, cpu.a);
    op_cmp16(PLAYER);
    if (cpu.a != PLAYER)
        return 0;
    cam = rd16(abs_db(CAMERA_Y));
    cpu.a = cam;
    cpu.p = nz16(cpu.p, cam);
    if (cam != 0) {
        lv = rd16(abs_db(LEVEL));
        cpu.a = lv;
        op_cmp16(LEVEL_TALL_A);
        if (lv == LEVEL_TALL_A)
            return 0;
        op_cmp16(LEVEL_TALL_B);
        if (lv == LEVEL_TALL_B)
            return 0;
    }
    cpu.p |= P_C;
    cpu.a = rd16(abs_db(CAMERA_Y));
    op_cmp16(rd16(abs_db(POINT_Y)));
    if (cpu.p & P_C)
        goto off;
    cpu.a = op_adc16(cpu.a, VIEW_HEIGHT, &cpu.p);
    cpu.p |= P_C;
    op_cmp16(rd16(abs_db(POINT_Y)));
    if (!(cpu.p & P_C))
        goto off;
    return 0;
off:
    op_sep(P_M | P_X);
    wr8(abs_db(TILE_BEHAVIOUR_BYTE), 0);
    wr8(abs_db(TILE_KIND), 0);
    cpu.a &= 0xFF00;
    cpu.x = 0;
    cpu.p = nz8(cpu.p, 0);
    return 1;
}

/*
 * @routine 81:B7F2 ProbeOnView
 * WHAT IT DOES: For the player, makes a point above or below the view
 * read as an empty tile, except in levels $0A and $0F once the camera has
 * moved down (their maps are tested whole).
 * HOW: When D is the player's record and the point's y is at or above the
 * camera's, or more than 240 pixels below it, pulls its own return
 * address and returns from the routine that called it (the tile lookup)
 * with behaviour and kind 0.
 * Entry: M 16-bit, X 16-bit; D = the actor; DB a bank whose $0000-$1FFF
 * is work RAM; called by JSR from the tile lookup. Exit: on view: M and X
 * 16-bit, A and the flags from the last compare. Off: to the lookup's
 * caller; M and X 8-bit; A low byte = X = 0; Z set, N clear; C as the
 * compare left it.
 */
void abi_ProbeOnView(void)
{
    uint16_t back;

    if (!probe_off_view())
        return;
    (void)op_pull8();                          /* PLA, PLA: its own return */
    (void)op_pull8();
    back = op_pull8();                         /* RTS through the lookup's caller's */
    back |= (uint16_t)(op_pull8() << 8);
    cpu.pc = 0x810000u | (uint16_t)(back + 1);
    cpu.unwound = 1;
}

/*
 * @routine 81:B880 TileAtPlace
 * WHAT IT DOES: The tile at a place in a screen: its word, behaviour and
 * kind.
 * HOW: A (the tile's place) plus the screen's tiles offset indexes the
 * level's tiles; the tile number (word / 8) indexes the behaviour table;
 * the behaviour's bit 7 is kept as mirrored, its low 3 bits pick the kind.
 * Entry: M 16-bit, X 16-bit; A = the tile's place (byte offset in the
 * screen); DB = $80; called by JSR. Exit: M and X 8-bit; A low byte = X =
 * the kind; N, Z from it; C = bit 2 of the tile word; Y, V unchanged.
 */
void abi_TileAtPlace(void)
{
    uint16_t w;
    uint8_t b;

    cpu.p &= (uint8_t)~P_C;
    cpu.a = op_adc16(cpu.a, rd16(abs_db(SCREEN_TILES)), &cpu.p);
    cpu.x = cpu.a;
    w = rd16(LEVEL_TILES + cpu.x);
    wr16(abs_db(TILE_WORD), w);
    cpu.p = (uint8_t)((cpu.p & ~P_C) | ((w >> 2) & 1));
    cpu.a = (uint16_t)(w >> 3);
    cpu.x = cpu.a;
    op_sep(P_M);
    b = rd8(TILE_BEHAVIOUR + cpu.x);
    wr8(abs_db(TILE_BEHAVIOUR_BYTE), b);
    op_push8(b);                               /* PHA */
    wr8(abs_db(TILE_MIRRORED), (uint8_t)(b & 0x80));
    op_sep(P_X);
    b = op_pull8();                            /* PLA */
    cpu.x = b & 0x07;
    b = rd8(abs_db((uint16_t)(TILE_KINDS + cpu.x)));
    wr8(abs_db(TILE_KIND), b);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | b);
    cpu.x = b;
    cpu.p = nz8(cpu.p, b);
}

/*
 * @routine 81:B831 TileAt
 * WHAT IT DOES: The map tile at the probe point: its word, behaviour and
 * kind (for the player, an empty tile off the view).
 * HOW: ProbeOnView; the point's screen (x / 256 and y / 256, each plus
 * one, place it in the 32-wide screen map) gives the screen's tiles
 * offset, a negative one taken as 0; the point's 16-pixel tile place in the
 * screen; the tile's top y kept as the surface; then TileAtPlace.
 * Entry: M 16-bit, X 16-bit; D = the actor; DB = $80; called by JSR.
 * Exit: as TileAtPlace, or as ProbeOnView off the view.
 */
void abi_TileAt(void)
{
    uint16_t place, t;
    int off;

    op_push8(0xB8);                            /* JSR ProbeOnView's return ($B833) */
    op_push8(0x33);
    off = probe_off_view();
    cpu.s = (uint16_t)(cpu.s + 2);
    if (off)
        return;
    cpu.a = rd16(abs_db(POINT_X));
    op_xba();
    cpu.a = (uint16_t)(cpu.a + 1);
    op_asl16();
    op_and16(0x003E);
    wr16(abs_db(SCREEN_PLACE), cpu.a);
    cpu.a = rd16(abs_db(POINT_Y));
    op_and16(0xFFF0);
    wr16(abs_db(SURFACE_Y), cpu.a);
    op_xba();
    cpu.a = (uint16_t)(cpu.a + 1);
    cpu.a = (uint16_t)(cpu.a << 6);
    op_and16(0x03C0);
    place = (uint16_t)(cpu.a | rd16(abs_db(SCREEN_PLACE)));
    wr16(abs_db(SCREEN_PLACE), place);
    cpu.x = place;
    t = rd16(SCREEN_MAP + place);
    if (t & 0x8000)
        t = 0x0000;
    wr16(abs_db(SCREEN_TILES), t);
    cpu.a = (uint16_t)((rd16(abs_db(POINT_X)) >> 3) & 0x001E);
    wr16(abs_db(TILE_PLACE), cpu.a);
    cpu.a = (uint16_t)((rd16(abs_db(POINT_Y)) << 1) & 0x01E0);
    cpu.a |= rd16(abs_db(TILE_PLACE));
    wr16(abs_db(TILE_PLACE), cpu.a);
    abi_TileAtPlace();
}

/*
 * @routine 81:B82F TileAtProbe
 * WHAT IT DOES: The map tile at the probe point, from 8-bit code.
 * HOW: 16-bit registers, then TileAt.
 * Entry: D = the actor; DB = $80; called by JSR. Exit: as TileAt.
 */
void abi_TileAtProbe(void)
{
    op_rep(P_M | P_X);
    abi_TileAt();
}

/*
 * @routine 81:B8EB SurfaceHeight
 * WHAT IT DOES: The y of a tile's surface at the point's x, for a tile of
 * kind A.
 * HOW: A flat kind ($70 in bits 4-6) puts the surface its low nibble
 * below the tile's top. A slope's surface drops its low nibble less the
 * point's x in the tile (mirrored for a mirrored tile) shifted right by
 * bits 4-6. The result is ORed into the surface y (the tile's top).
 * Entry: M 8-bit, X 8-bit; A = X = the kind; DB a bank whose $0000-$1FFF is
 * work RAM; called by JSR. Exit: A low byte = 1; N, Z clear; X unchanged;
 * flat: Y unchanged, C set; slope: Y = 0, C from the SBC; mirrored byte
 * shifted left.
 */
void abi_SurfaceHeight(void)
{
    uint8_t kind = (uint8_t)cpu.x, a, n, drop;

    a = (uint8_t)(cpu.a & 0x70);
    cpu.p = cmp8_flags(cpu.p, a, SLOPE_FLAT);
    if (a == SLOPE_FLAT) {
        a = (uint8_t)((kind & 0x0F) | rd8(abs_db(SURFACE_Y)));
        wr8(abs_db(SURFACE_Y), a);
    } else {
        n = (uint8_t)(kind >> 4);
        cpu.y = n;
        drop = (uint8_t)(rd8(abs_db(POINT_X)) & 0x0F);
        a = rd8_rmw(abs_db(TILE_MIRRORED));
        wr8(abs_db(TILE_MIRRORED), (uint8_t)(a << 1));
        if (a & 0x80)
            drop ^= 0x0F;
        while (n != 0) {
            drop >>= 1;
            n--;
        }
        cpu.y = 0;
        wr8(abs_db(SLOPE_DROP), drop);
        cpu.p |= P_C;
        a = op_sbc8((uint8_t)(kind & 0x0F), rd8(abs_db(SLOPE_DROP)), &cpu.p);
        a |= rd8(abs_db(SURFACE_Y));
        wr8(abs_db(SURFACE_Y), a);
    }
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x01);
    cpu.p = nz8(cpu.p, 0x01);
}

/*
 * @routine 81:B8AD SurfaceHeightSolid
 * WHAT IT DOES: The surface's y for a tile of kind A, a solid tile (kind
 * 1) taken as flat at its top.
 * HOW: Kind 1 becomes $70 (flat, height 0); then SurfaceHeight.
 * Entry: as SurfaceHeight. Exit: as SurfaceHeight.
 */
void abi_SurfaceHeightSolid(void)
{
    uint8_t a = (uint8_t)cpu.a;

    cpu.p = cmp8_flags(cpu.p, a, KIND_SOLID);
    if (a == KIND_SOLID) {
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | SLOPE_FLAT);
        cpu.x = SLOPE_FLAT;
        cpu.p = nz8(cpu.p, SLOPE_FLAT);
    }
    abi_SurfaceHeight();
}

/*
 * @routine 81:B7D5 PointInGround
 * WHAT IT DOES: Says whether the probe point is in the ground: C set when
 * it is.
 * HOW: TileAt; an empty tile (kind 0) or kind with bit 7 is not ground; a
 * solid tile (kind 1) is; otherwise the point is in it when at or below
 * the surface (SurfaceHeight).
 * Entry: M 16-bit, X 16-bit; D = the actor; DB = $80; called by JSR.
 * Exit: M and X 8-bit; C as above; not ground or solid: A and N, Z as
 * TileAt leaves them (solid: Z set); else A = the point's y (16 bits), N,
 * Z from its compare with the surface.
 */
void abi_PointInGround(void)
{
    call_frame(0xB7D7, abi_TileAt);
    if ((cpu.p & P_Z) || (cpu.p & P_N)) {
        cpu.p &= (uint8_t)~P_C;
        return;
    }
    cpu.p = cmp8_flags(cpu.p, (uint8_t)cpu.a, KIND_SOLID);
    if ((uint8_t)cpu.a == KIND_SOLID) {
        cpu.p |= P_C;
        return;
    }
    call_frame(0xB7E2, abi_SurfaceHeight);
    op_rep(P_M | P_X);
    cpu.a = rd16(abs_db(POINT_Y));
    op_cmp16(rd16(abs_db(SURFACE_Y)));
    op_sep(P_M | P_X);
}

/*
 * @routine 81:B7CF PointInGroundLong
 * WHAT IT DOES: PointInGround for callers in other banks.
 * HOW: 16-bit registers, then PointInGround.
 * Entry: D = the actor; DB = $80; called by JSL. Exit: as PointInGround.
 */
void abi_PointInGroundLong(void)
{
    op_rep(P_M | P_X);
    call_frame(0xB7D3, abi_PointInGround);
}

/*
 * @routine 81:B7C3 ActorInGround
 * WHAT IT DOES: Says whether the actor's own position is in the ground.
 * HOW: The point = the actor's x and y; then PointInGroundLong.
 * Entry: D = the actor; DB = $80; called by JSL. Exit: as PointInGround.
 */
void abi_ActorInGround(void)
{
    op_rep(P_M | P_X);
    cpu.a = rd16(dp(ACTOR_X + 1));
    cpu.p = nz16(cpu.p, cpu.a);
    wr16(abs_db(POINT_X), cpu.a);
    cpu.a = rd16(dp(ACTOR_Y + 1));
    cpu.p = nz16(cpu.p, cpu.a);
    wr16(abs_db(POINT_Y), cpu.a);
    abi_PointInGroundLong();
}

/*
 * @routine 81:B7A3 GroundRight
 * WHAT IT DOES: Says whether the ground is at the probe point to the
 * actor's right.
 * HOW: ProbeRight, then PointInGround.
 * Entry: X 8-bit; D = the actor; DB = $80; called by JSR. Exit: as
 * PointInGround.
 */
void abi_GroundRight(void)
{
    call_frame(0xB7A5, abi_ProbeRight);
    abi_PointInGround();
}

/*
 * @routine 81:B7A8 GroundLeft
 * WHAT IT DOES: Says whether the ground is at the probe point to the
 * actor's left.
 * HOW: ProbeLeft, then PointInGround.
 * Entry: X 8-bit; D = the actor; DB = $80; called by JSR. Exit: as
 * PointInGround.
 */
void abi_GroundLeft(void)
{
    call_frame(0xB7AA, abi_ProbeLeft);
    abi_PointInGround();
}

/*
 * @routine 81:B79F GroundInFront
 * WHAT IT DOES: Says whether the ground is at the probe point in front of
 * the actor, the way it faces.
 * HOW: Facing right, GroundRight; facing left, GroundLeft.
 * Entry: M 8-bit; D = the actor; DB = $80; called by JSR. Exit: as
 * PointInGround.
 */
void abi_GroundInFront(void)
{
    uint8_t f = rd8(dp(ACTOR_FACING));

    cpu.a = (uint16_t)((cpu.a & 0xFF00) | f);
    cpu.p = nz8(cpu.p, f);
    if (f == 0)
        abi_GroundRight();
    else
        abi_GroundLeft();
}

/*
 * @routine 81:B7AD GroundBehind
 * WHAT IT DOES: Says whether the ground is at the probe point behind the
 * actor.
 * HOW: Its flip clear (facing left), GroundRight; set (facing right),
 * GroundLeft.
 * Entry: M 8-bit; D = the actor; DB = $80; called by JSR. Exit: as
 * PointInGround.
 */
void abi_GroundBehind(void)
{
    uint8_t f = rd8(dp(ACTOR_FLIP));

    cpu.a = (uint16_t)((cpu.a & 0xFF00) | f);
    cpu.p = nz8(cpu.p, f);
    if (f == 0)
        call_frame(0xB7B3, abi_ProbeRight);
    else
        call_frame(0xB7B8, abi_ProbeLeft);
    abi_PointInGround();
}

/*
 * @routine 81:B7BB GroundRightLong
 * WHAT IT DOES: GroundRight for callers in other banks.
 * HOW: GroundRight.
 * Entry: as GroundRight; called by JSL. Exit: as PointInGround.
 */
void abi_GroundRightLong(void)
{
    call_frame(0xB7BD, abi_GroundRight);
}

/*
 * @routine 81:B7BF GroundLeftLong
 * WHAT IT DOES: GroundLeft for callers in other banks.
 * HOW: GroundLeft.
 * Entry: as GroundLeft; called by JSL. Exit: as PointInGround.
 */
void abi_GroundLeftLong(void)
{
    call_frame(0xB7C1, abi_GroundLeft);
}

/*
 * @routine 81:B82B TileAtProbeLong
 * WHAT IT DOES: TileAtProbe for callers in other banks.
 * HOW: TileAtProbe.
 * Entry: as TileAtProbe; called by JSL. Exit: as TileAt.
 */
void abi_TileAtProbeLong(void)
{
    call_frame(0xB82D, abi_TileAtProbe);
}

/* Per behaviour's low 3 bits: nonzero, a wall the actor is pushed out of
   (WALL_KINDS); nonzero, a floor it settles on (FLOOR_KINDS) (bank $81). */
#define WALL_KINDS 0xBF0Au
#define FLOOR_KINDS 0xBF12u
#define BEHAVIOUR_THROUGH 0x40     /* behaviour bit: the actor passes through it */

/*
 * @routine 81:B795 GroundInFrontLong
 * WHAT IT DOES: GroundInFront for callers in other banks.
 * HOW: GroundInFront.
 * Entry: as GroundInFront; called by JSL. Exit: as PointInGround.
 */
void abi_GroundInFrontLong(void)
{
    call_frame(0xB797, abi_GroundInFront);
}

/*
 * @routine 81:B5F6 GroundBehindBit4
 * WHAT IT DOES: Tests the ground behind the actor, and moves the tile
 * behaviour's bit 4 into bit 5 when it is set.
 * HOW: GroundBehind; then the behaviour byte, when bit 4 is set, shifts
 * left.
 * Entry: M 8-bit; D = the actor; DB = $80; called by JSL. Exit: as
 * GroundBehind, but A low byte = the behaviour's bit 4 and N, Z from it;
 * C its bit 7 when shifted.
 */
void abi_GroundBehindBit4(void)
{
    uint8_t b;

    call_frame(0xB5F8, abi_GroundBehind);
    b = (uint8_t)(rd8(abs_db(TILE_BEHAVIOUR_BYTE)) & 0x10);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | b);
    cpu.p = nz8(cpu.p, b);
    if (b) {
        uint8_t v = rd8_rmw(abs_db(TILE_BEHAVIOUR_BYTE));

        cpu.p = (uint8_t)((cpu.p & ~P_C) | (v >> 7));
        v = (uint8_t)(v << 1);
        wr8(abs_db(TILE_BEHAVIOUR_BYTE), v);
        cpu.p = nz8(cpu.p, v);
    }
}

/*
 * @routine 81:B5B3 GroundBehindBit3
 * WHAT IT DOES: Tests the ground behind the actor at probes $10 and then
 * $14, until a tile with behaviour bit 3.
 * HOW: GroundBehind at probe $10; without bit 3, again at probe $14.
 * Entry: M 8-bit; D = the actor; DB = $80; called by JSL. Exit: as
 * GroundBehind, but A low byte = the last tile's behaviour bit 3, N, Z
 * from it.
 */
void abi_GroundBehindBit3(void)
{
    static const uint8_t probes[2] = { 0x10, 0x14 };
    static const uint16_t backs[2] = { 0xB5B9, 0xB5C7 };   /* the two JSRs' returns */
    uint8_t b = 0;
    int i;

    for (i = 0; i < 2; i++) {
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | probes[i]);
        cpu.p = nz8(cpu.p, probes[i]);
        wr8(dp(ACTOR_PROBE_AT), probes[i]);
        call_frame(backs[i], abi_GroundBehind);
        b = (uint8_t)(rd8(abs_db(TILE_BEHAVIOUR_BYTE)) & 0x08);
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | b);
        cpu.p = nz8(cpu.p, b);
        if (b)
            break;
    }
}

/* ActorOutOfWall's tests (and ActorOutOfWallByFlip's): GroundInFront at
   probe A (`back` its JSR's return), then the tile a wall kind the actor
   cannot pass through. None: C clear, 0. */
static int wall_in_front(uint16_t back)
{
    uint8_t b;

    wr8(dp(ACTOR_PROBE_AT), (uint8_t)cpu.a);
    call_frame(back, abi_GroundInFront);
    if (!(cpu.p & P_C))
        goto none;
    b = (uint8_t)(rd8(abs_db(TILE_BEHAVIOUR_BYTE)) & 0x07);
    cpu.x = b;
    b = rd8(abs_db((uint16_t)(WALL_KINDS + b)));
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | b);
    cpu.p = nz8(cpu.p, b);
    if (b == 0)
        goto none;
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | BEHAVIOUR_THROUGH);
    b = rd8(abs_db(TILE_BEHAVIOUR_BYTE));
    cpu.p = (uint8_t)((cpu.p & ~(P_N | P_V | P_Z)) | (b & (P_N | P_V)) | ((b & BEHAVIOUR_THROUGH) ? 0 : P_Z));
    if (b & BEHAVIOUR_THROUGH)
        goto none;
    return 1;
none:
    cpu.p &= (uint8_t)~P_C;
    return 0;
}

/* ActorOutOfWall's move ($81:B632 on, M 16-bit), the point's x in its
   tile in `a`: back by a + 1, or (`right`) on by 16 - a; C set after. */
static void wall_push(uint16_t a, int right)
{
    if (!right) {
        a++;
    } else {
        cpu.p |= P_C;
        a = op_sbc16(a, 0x0010, &cpu.p);
    }
    a = (uint16_t)(~a + 1);
    cpu.p &= (uint8_t)~P_C;
    cpu.a = op_adc16(a, rd16(dp(ACTOR_X + 1)), &cpu.p);
    wr16(dp(ACTOR_X + 1), cpu.a);
    op_sep(P_M | P_C);
}

/*
 * @routine 81:B60C ActorOutOfWall
 * WHAT IT DOES: When the probe in front of the actor is in a wall, moves
 * the actor out of it: C set when it was moved.
 * HOW: GroundInFront at probe A; a tile whose kind is a wall and that the
 * actor cannot pass through moves the actor's x by the point's distance
 * past the tile's near edge: back by (x in tile) + 1 in its left half,
 * on by 16 - (x in tile) in its right half.
 * Entry: M 8-bit; D = the actor; A low byte = the probe's offset; DB =
 * $81; called by JSR. Exit: moved: M 8-bit, C set; A = the new x (16
 * bits); N, Z from its ADC. Not: C clear; A, N, Z as the tests left them.
 */
void abi_ActorOutOfWall(void)
{
    uint16_t a;

    if (!wall_in_front(0xB610))
        return;
    op_rep(P_M);
    a = (uint16_t)(rd16(abs_db(POINT_X)) & 0x000F);
    wall_push(a, a & 0x0008);
}

/*
 * @routine 81:B647 ActorOutOfWallByFlip
 * WHAT IT DOES: As ActorOutOfWall, but the side the actor moves to is
 * the way bit 0 of its flip says, not the half of the tile its point is
 * in: C set when it was moved. Nothing but ActorOutOfWallsByFlip calls it.
 * HOW: ActorOutOfWall's tests at probe A; a wall moves the actor's x on by
 * 16 - (x in tile) when bit 0 of its flip is set, else back by (x in
 * tile) + 1 (ActorOutOfWall's own steps from there).
 * Entry: M 8-bit; D = the actor; A low byte = the probe's offset; DB =
 * $80; called by JSR. Exit: as ActorOutOfWall.
 */
void abi_ActorOutOfWallByFlip(void)
{
    uint16_t a, f;

    if (!wall_in_front(0xB64B))
        return;
    op_rep(P_M);
    f = rd16(dp(ACTOR_FLIP));
    cpu.p = (uint8_t)((cpu.p & ~P_C) | (f & 1));
    a = (uint16_t)(rd16(abs_db(POINT_X)) & 0x000F);
    wall_push(a, f & 1);
}

/*
 * @routine 81:B566 ActorOutOfWallsByFlip
 * WHAT IT DOES: Moves the actor out of the walls in front of it at probe
 * 4, again while it moves, then the same at probe $0C, each time the way
 * bit 0 of its flip says. No code refers to it.
 * HOW: ActorOutOfWallByFlip at probe 4 until it moves the actor no more,
 * then at probe $0C until it moves it no more.
 * Entry: M 8-bit; D = the actor; DB = $80; called by JSL. Exit: M 8-bit;
 * C clear; A, N, Z as the last test left them.
 */
void abi_ActorOutOfWallsByFlip(void)
{
    do {
        lda8_v(0x04);
        call_frame(0xB56A, abi_ActorOutOfWallByFlip);
    } while (cpu.p & P_C);
    do {
        lda8_v(0x0C);
        call_frame(0xB571, abi_ActorOutOfWallByFlip);
    } while (cpu.p & P_C);
}

/*
 * @routine 81:B608 ActorOutOfWallLong
 * WHAT IT DOES: ActorOutOfWall for callers in other banks.
 * HOW: ActorOutOfWall.
 * Entry: as ActorOutOfWall; called by JSL. Exit: as ActorOutOfWall.
 */
void abi_ActorOutOfWallLong(void)
{
    call_frame(0xB60A, abi_ActorOutOfWall);
}

/*
 * @routine 81:B5AD ActorOutOfWallProbe4
 * WHAT IT DOES: Moves the actor out of a wall at probe 4 in front of it.
 * HOW: ActorOutOfWall at probe 4.
 * Entry: M 8-bit; D = the actor; DB = $80; called by JSL. Exit: as
 * ActorOutOfWall.
 */
void abi_ActorOutOfWallProbe4(void)
{
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x04);
    cpu.p = nz8(cpu.p, 0x04);
    call_frame(0xB5B1, abi_ActorOutOfWall);
}

/*
 * @routine 81:B59F ActorOutOfWalls
 * WHAT IT DOES: Moves the actor out of a wall in front of it, testing
 * probes $0C, 8 and 4 until one moves it.
 * HOW: ActorOutOfWall at probes $0C and 8 while it returns C clear, then
 * ActorOutOfWallProbe4.
 * Entry: M 8-bit; D = the actor; DB = $80; called by JSL. Exit: as the
 * last ActorOutOfWall.
 */
void abi_ActorOutOfWalls(void)
{
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x0C);
    cpu.p = nz8(cpu.p, 0x0C);
    call_frame(0xB5A3, abi_ActorOutOfWall);
    if (cpu.p & P_C)
        return;
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x08);
    cpu.p = nz8(cpu.p, 0x08);
    call_frame(0xB5AA, abi_ActorOutOfWall);
    if (cpu.p & P_C)
        return;
    abi_ActorOutOfWallProbe4();
}

/* A floor tile under the point (TileAtProbe): returns 1 floor, 0 not a
   floor, -1 out (a kind with bit 7, or one the actor passes through: the
   caller gives up). `strict` treats passing through as out, else as not
   a floor; `back` is the JSR's return. */
static int floor_at(int strict, uint16_t back)
{
    uint8_t b, k;

    call_frame(back, abi_TileAtProbe);
    if (cpu.p & P_Z)
        return 0;
    if (cpu.p & P_N)
        return strict ? -1 : 0;
    b = rd8(abs_db(TILE_BEHAVIOUR_BYTE));
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | b);
    cpu.p = nz8(cpu.p, b);
    cpu.p = (uint8_t)((cpu.p & ~P_Z) | ((b & BEHAVIOUR_THROUGH) ? 0 : P_Z));
    if (b & BEHAVIOUR_THROUGH)
        return strict ? -1 : 0;
    cpu.x = b & 0x07;
    k = rd8(abs_db((uint16_t)(FLOOR_KINDS + cpu.x)));
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | k);
    cpu.p = nz8(cpu.p, k);
    return k != 0;
}

/*
 * @routine 81:B673 ActorSettleOnFloor
 * WHAT IT DOES: When a floor tile is under the actor's left or right
 * probe at offset 8, and the ground is under the actor itself, stops its
 * fall and stands it on that ground's tile top.
 * HOW: ProbeLeft at probe 8, then TileAtProbe: a floor kind (and not
 * passed through) goes on; else the same at ProbeRight. A point above the
 * map's top (negative y) gives up with A = 0. Then the tile under the
 * actor's own x at that y: when it is ground, the vertical speed is
 * cleared and y moves down to the next tile boundary (16 less the
 * point's y in the tile).
 * Entry: M 8-bit, X 8-bit; D = the actor; DB = $80; called by JSL.
 * Exit: M 8-bit; X as the last test leaves it (8-bit, or 16-bit when the
 * point was above the top). Above the top or a pass-through or kind with
 * bit 7: A low byte = 0, Z set. Settled: A = the new y (16 bits); N, V, Z,
 * C from its ADC. Else: A, flags from the last test.
 */
void abi_ActorSettleOnFloor(void)
{
    int f;
    uint16_t a;

    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x08);
    cpu.p = nz8(cpu.p, 0x08);
    wr8(dp(ACTOR_PROBE_AT), 0x08);
    call_frame(0xB679, abi_ProbeLeft);
    op_sep(P_M);
    cpu.x = rd16(abs_db(POINT_Y));
    cpu.p = nz16(cpu.p, cpu.x);
    if (cpu.x & 0x8000)
        goto out;
    f = floor_at(0, 0xB683);
    if (f != 1) {
        call_frame(0xB699, abi_ProbeRight);
        op_sep(P_M);
        cpu.x = rd16(abs_db(POINT_Y));
        cpu.p = nz16(cpu.p, cpu.x);
        if (cpu.x & 0x8000)
            goto out;
        f = floor_at(1, 0xB6A3);
        if (f < 0)
            goto out;
        if (f == 0)
            return;
    }
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | rd8(dp(ACTOR_X + 1)));
    wr8(abs_db(POINT_X), (uint8_t)cpu.a);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | rd8(dp(ACTOR_X + 2)));
    cpu.p = nz8(cpu.p, (uint8_t)cpu.a);
    wr8(abs_db(POINT_X + 1), (uint8_t)cpu.a);
    call_frame(0xB6C3, abi_TileAtProbe);
    if (cpu.p & P_Z)
        return;
    if (cpu.p & P_N)
        goto out;
    wr8(dp(ACTOR_SPEED_Y + 2), 0);
    op_rep(P_M);
    wr16(dp(ACTOR_SPEED_Y), 0);
    a = (uint16_t)(rd16(abs_db(POINT_Y)) & 0x000F);
    a = (uint16_t)(~a + 1);
    cpu.p &= (uint8_t)~P_C;
    a = op_adc16(a, 0x0010, &cpu.p);
    cpu.p &= (uint8_t)~P_C;
    cpu.a = op_adc16(a, rd16(dp(ACTOR_Y + 1)), &cpu.p);
    wr16(dp(ACTOR_Y + 1), cpu.a);
    op_sep(P_M);
    return;
out:
    cpu.a &= 0xFF00;
    cpu.p = nz8(cpu.p, 0);
}

/* Per behaviour's low 3 bits: nonzero, ground the player does not stand
   firm on (bank $81). */
#define SOFT_KINDS 0xBF1Au
#define Y_SAVED (SCRATCH + 0x1E)   /* scratch $2A: the actor's y kept */

/*
 * @routine 81:B8B6 SurfaceHeightOver
 * WHAT IT DOES: The surface's y for a tile of kind A; for a solid tile
 * (kind 1), the surface of the tile above it, if any.
 * HOW: Kind 1: the surface moves up a tile and the tile place a row; a
 * place past the screen's top takes the screen above's tiles; then
 * TileAtPlace: an empty tile above returns Z set; else SurfaceHeight with
 * its kind.
 * Entry: M 8-bit, X 8-bit; A = X = the kind; DB = $80; called by JSR.
 * Exit: as SurfaceHeight; or, the tile above empty, A low byte = X = 0,
 * Z set, M and X 8-bit.
 */
void abi_SurfaceHeightOver(void)
{
    uint8_t a = (uint8_t)cpu.a;
    uint16_t v, t;

    cpu.p = cmp8_flags(cpu.p, a, KIND_SOLID);
    if (a != KIND_SOLID) {
        abi_SurfaceHeight();
        return;
    }
    op_rep(P_M | P_X);
    cpu.p |= P_C;
    v = op_sbc16(rd16(abs_db(SURFACE_Y)), 0x0010, &cpu.p);
    wr16(abs_db(SURFACE_Y), v);
    cpu.p |= P_C;
    v = op_sbc16(rd16(abs_db(TILE_PLACE)), 0x0020, &cpu.p);
    v &= 0x01FE;
    cpu.a = v;
    op_cmp16(0x01E0);
    if (cpu.p & P_C) {
        op_push8((uint8_t)(v >> 8));           /* PHA */
        op_push8((uint8_t)v);
        cpu.p |= P_C;
        t = op_sbc16(rd16(abs_db(SCREEN_PLACE)), 0x0040, &cpu.p);
        cpu.x = t;
        t = rd16(SCREEN_MAP + t);
        wr16(abs_db(SCREEN_TILES), t);
        cpu.a = op_pull8();                    /* PLA */
        cpu.a |= (uint16_t)(op_pull8() << 8);
        cpu.p = nz16(cpu.p, cpu.a);
    }
    call_frame(0xB8E8, abi_TileAtPlace);
    if (cpu.p & P_Z)
        return;
    abi_SurfaceHeight();
}

/* TileAt at the probe point made by `probe`, the surface by `surface`
   when there is a tile; returns 1 when the surface step says the tile is
   ground (Z clear after it). `at_probe` takes the lookup with 16-bit
   registers first (TileAtProbe). `back` holds the three JSRs' returns
   (the probe, the lookup, the surface), as the original pushes them. */
static int ground_at(const uint16_t back[3], void (*probe)(void), int at_probe, void (*surface)(void))
{
    call_frame(back[0], probe);
    call_frame(back[1], at_probe ? abi_TileAtProbe : abi_TileAt);
    if ((cpu.p & P_Z) || (cpu.p & P_N))
        return -1;
    call_frame(back[2], surface);
    return !(cpu.p & P_Z);
}

/* The actor's y moved by the surface less the point's y. */
static uint16_t onto_surface(void)
{
    uint16_t v;

    cpu.p |= P_C;
    v = rd16(abs_db(SURFACE_Y));
    v = op_sbc16(v, rd16(abs_db(POINT_Y)), &cpu.p);
    cpu.p &= (uint8_t)~P_C;
    return op_adc16(v, rd16(dp(ACTOR_Y + 1)), &cpu.p);
}

/* ActorSnapToGround's ground test (its code from $81:B45C), from the
   actor's y as the entry left it; ActorSnapToGroundHere and
   ActorSnapToGround5 branch to it. Entered M 16-bit: ground found stands
   the actor on it (C set); else the kept y comes back. */
static void snap_test(void)
{
    static const uint16_t at[3] = { 0xB462, 0xB465, 0xB46C };
    static const uint16_t left[3] = { 0xB471, 0xB474, 0xB47B };
    static const uint16_t right[3] = { 0xB480, 0xB483, 0xB48A };

    op_sep(P_M);
    wr8(dp(ACTOR_PROBE_AT), 0);
    if (ground_at(at, abi_ProbeAtActor, 0, abi_SurfaceHeightOver) == 1
        || ground_at(left, abi_ProbeLeft, 0, abi_SurfaceHeightSolid) == 1
        || ground_at(right, abi_ProbeRight, 0, abi_SurfaceHeightSolid) == 1) {
        op_rep(P_M);
        cpu.a = onto_surface();
        wr16(dp(ACTOR_Y + 1), cpu.a);
        op_sep(P_M | P_X | P_C);
        return;
    }
    op_rep(P_M);
    cpu.a = rd16(abs_db(Y_SAVED));
    cpu.p = nz16(cpu.p, cpu.a);
    wr16(dp(ACTOR_Y + 1), cpu.a);
    op_sep(P_M | P_X | P_Z);
}

/* The two entries that move the actor down first: its y kept, then
   `down` pixels added (C clear before the ADC). */
static void snap_from_below(uint16_t down)
{
    uint16_t v;

    op_rep(P_M);
    cpu.p &= (uint8_t)~P_C;
    v = rd16(dp(ACTOR_Y + 1));
    wr16(abs_db(Y_SAVED), v);
    cpu.a = op_adc16(v, down, &cpu.p);
    wr16(dp(ACTOR_Y + 1), cpu.a);
}

/*
 * @routine 81:B44F ActorSnapToGround
 * WHAT IT DOES: When there is ground just under the actor (within 3
 * pixels), stands it on that ground: C set. Else leaves it where it was.
 * HOW: Keeps the actor's y and moves it 3 down; tests probe 0 at the
 * actor's x (the tile above for a solid tile), then to its left and right
 * (a solid tile flat at its top); the first ground found moves the y by
 * the surface less the point's y. With none, the kept y comes back.
 * Entry: M 8-bit, X 8-bit; D = the actor; DB = $80; called by JSL.
 * Exit: M and X 8-bit; on ground: C set, A = the new y (16 bits), N, V, Z
 * from its ADC; none: Z set, A = the kept y (16 bits), N as it, C as the
 * last test left it.
 */
void abi_ActorSnapToGround(void)
{
    snap_from_below(0x0003);
    snap_test();
}

/*
 * @routine 81:B440 ActorSnapToGround5
 * WHAT IT DOES: When there is ground just under the actor (within 5
 * pixels), stands it on that ground: C set. Else leaves it where it was.
 * No code refers to it.
 * HOW: Keeps the actor's y and moves it 5 down, then ActorSnapToGround's
 * ground test.
 * Entry: M 8-bit, X 8-bit; D = the actor; DB a bank whose $0000-$1FFF is
 * work RAM and that holds the probe list and the tile kinds; called by
 * JSL. Exit: as ActorSnapToGround.
 */
void abi_ActorSnapToGround5(void)
{
    snap_from_below(0x0005);
    snap_test();
}

/*
 * @routine 81:B437 ActorSnapToGroundHere
 * WHAT IT DOES: When there is ground at the actor's y, stands it on that
 * ground: C set. Else leaves it where it was. No code refers to it.
 * HOW: Keeps the actor's y, then ActorSnapToGround's ground test from
 * there.
 * Entry: M 8-bit, X 8-bit; D = the actor; DB a bank whose $0000-$1FFF is
 * work RAM and that holds the probe list and the tile kinds; called by
 * JSL. Exit: as ActorSnapToGround.
 */
void abi_ActorSnapToGroundHere(void)
{
    op_rep(P_M);
    cpu.a = rd16(dp(ACTOR_Y + 1));
    cpu.p = nz16(cpu.p, cpu.a);
    wr16(abs_db(Y_SAVED), cpu.a);
    snap_test();
}

/* ActorOutOfGround from its tile on ($81:B520, also reached by BRA from
   ActorOutOfGroundAtActor), the surface found: when the point is at or
   below it the raised y is kept, the actor tested 15 pixels above, and
   the kept y taken. Else Z set and the actor left where it is. */
static void out_of_ground_tile(void)
{
    uint16_t v;

    op_rep(P_M | P_X);
    cpu.a = rd16(abs_db(POINT_Y));
    op_cmp16(rd16(abs_db(SURFACE_Y)));
    if (!(cpu.p & P_C)) {
        op_sep(P_M | P_X | P_Z);
        return;
    }
    v = onto_surface();
    wr16(abs_db(Y_SAVED), v);
    cpu.p &= (uint8_t)~P_C;
    cpu.a = op_adc16(v, 0xFFF1, &cpu.p);
    wr16(dp(ACTOR_Y + 1), cpu.a);
    op_sep(P_M | P_X);
    call_frame(0xB541, abi_ProbeAtActor);
    call_frame(0xB544, abi_TileAtProbe);
    if (!(cpu.p & P_Z) && !(cpu.p & P_N)) {
        op_rep(P_M | P_X);
        cpu.p |= P_C;
        cpu.a = rd16(abs_db(SURFACE_Y));
        cpu.a = op_sbc16(cpu.a, rd16(abs_db(POINT_Y)), &cpu.p);
        if (!(cpu.p & P_C)) {
            cpu.a = op_adc16(cpu.a, rd16(dp(ACTOR_Y + 1)), &cpu.p);
            wr16(abs_db(Y_SAVED), cpu.a);
        }
    }
    op_rep(P_M | P_X);
    cpu.a = rd16(abs_db(Y_SAVED));
    cpu.p = nz16(cpu.p, cpu.a);
    wr16(dp(ACTOR_Y + 1), cpu.a);
    op_sep(P_M | P_X);
}

/*
 * @routine 81:B4F3 ActorOutOfGround
 * WHAT IT DOES: When the actor is in the ground at its probes, raises it
 * onto the surface, unless that would leave it in the ground 15 pixels
 * higher too, where it goes as far as that allows.
 * HOW: Probe 0 at the actor's x (the tile above for a solid tile), then to
 * its left and right (a solid tile flat at its top), stopping at the
 * first tile; with none, Z set. When the point is at or below the surface
 * the raised y is kept in scratch $2A, and the actor tested at 15 pixels
 * above it: ground there with its surface above the point lowers the kept
 * y by the difference. The actor takes the kept y.
 * Entry: X 8-bit; D = the actor; DB = $80; called by JSL. Exit: M and X
 * 8-bit; no tile, or the point above the surface: Z set; raised: A = the
 * new y (16 bits), N, Z from it, C as the last test left it.
 */
void abi_ActorOutOfGround(void)
{
    int r;
    static const uint16_t at[3] = { 0xB4F7, 0xB4FA, 0xB501 };
    static const uint16_t left[3] = { 0xB506, 0xB509, 0xB510 };

    if (cpu.p & P_M)
        wr8(dp(ACTOR_PROBE_AT), 0);
    else
        wr16(dp(ACTOR_PROBE_AT), 0);
    r = ground_at(at, abi_ProbeAtActor, 1, abi_SurfaceHeightOver);
    if (r < 0) {
        r = ground_at(left, abi_ProbeLeft, 1, abi_SurfaceHeightSolid);
        if (r < 0) {
            call_frame(0xB515, abi_ProbeRight);
            call_frame(0xB518, abi_TileAtProbe);
            if (cpu.p & P_Z)
                return;
            if (cpu.p & P_N) {
                op_sep(P_M | P_X | P_Z);
                return;
            }
            call_frame(0xB51F, abi_SurfaceHeightSolid);
        }
    }
    out_of_ground_tile();
}

/*
 * @routine 81:B4E4 ActorOutOfGroundAtActor
 * WHAT IT DOES: ActorOutOfGround at the actor's own x only, with the probe
 * as its record has it: when the actor is in the ground there, raises it
 * onto the surface (as far as 15 pixels higher allows). No code refers to
 * it.
 * HOW: ProbeAtActor, TileAtProbe: no tile, or a kind with bit 7, gives up
 * with Z set; else SurfaceHeightOver, then ActorOutOfGround's steps from
 * its tile.
 * Entry: X 8-bit; D = the actor; $18 = the probe's offset; DB = $80;
 * called by JSL. Exit: as ActorOutOfGround.
 */
void abi_ActorOutOfGroundAtActor(void)
{
    static const uint16_t at[3] = { 0xB4E6, 0xB4E9, 0xB4F0 };

    if (ground_at(at, abi_ProbeAtActor, 1, abi_SurfaceHeightOver) < 0) {
        op_sep(P_M | P_X | P_Z);
        return;
    }
    out_of_ground_tile();
}

/*
 * @routine 81:B756 PlayerOnFirmGround
 * WHAT IT DOES: Says whether the player stands on firm ground at probe 0:
 * C set when the ground to its left or right is solid and not soft.
 * HOW: With the player's record as the direct page (its own kept on the
 * stack): GroundLeft at probe 0, its behaviour not $20 or $40 and its
 * kind not soft; else the same at GroundRight (behaviour not $40).
 * Entry: M 8-bit, X 8-bit; DB = $80; called by JSL. Exit: D as at entry,
 * N and Z from it (PLD); C as above; M and X 8-bit; A, X, Y as the last
 * test left them.
 */
void abi_PlayerOnFirmGround(void)
{
    uint8_t b, k;
    int firm = 0;

    cpu.x = 0;
    cpu.p = nz8(cpu.p, 0);
    op_push8((uint8_t)(cpu.d >> 8));           /* PHD */
    op_push8((uint8_t)cpu.d);
    op_rep(P_M);
    cpu.a = PLAYER;
    cpu.p = nz16(cpu.p, cpu.a);
    cpu.d = cpu.a;
    cpu.p = nz16(cpu.p, cpu.d);
    op_sep(P_M);
    wr8(dp(ACTOR_PROBE_AT), (uint8_t)cpu.x);
    call_frame(0xB769, abi_GroundLeft);
    if (cpu.p & P_C) {
        b = rd8(abs_db(TILE_BEHAVIOUR_BYTE));
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | b);
        cpu.p = (uint8_t)((cpu.p & ~P_Z) | ((b & 0x60) ? 0 : P_Z));
        if (!(b & 0x60)) {
            cpu.x = b & 0x07;
            k = rd8(abs_db((uint16_t)(SOFT_KINDS + cpu.x)));
            cpu.a = (uint16_t)((cpu.a & 0xFF00) | k);
            cpu.p = nz8(cpu.p, k);
            if (k == 0)
                firm = 1;
        }
    }
    if (!firm) {
        call_frame(0xB77D, abi_GroundRight);
        if (cpu.p & P_C) {
            b = rd8(abs_db(TILE_BEHAVIOUR_BYTE));
            cpu.a = (uint16_t)((cpu.a & 0xFF00) | b);
            cpu.p = (uint8_t)((cpu.p & ~P_Z) | ((b & BEHAVIOUR_THROUGH) ? 0 : P_Z));
            if (!(b & BEHAVIOUR_THROUGH)) {
                cpu.x = b & 0x07;
                k = rd8(abs_db((uint16_t)(SOFT_KINDS + cpu.x)));
                cpu.a = (uint16_t)((cpu.a & 0xFF00) | k);
                cpu.p = nz8(cpu.p, k);
                if (k == 0)
                    firm = 1;
            }
        }
    }
    cpu.d = op_pull8();                        /* PLD */
    cpu.d |= (uint16_t)(op_pull8() << 8);
    cpu.p = nz16(cpu.p, cpu.d);
    cpu.p = (uint8_t)((cpu.p & ~P_C) | (firm ? P_C : 0));
}

/* PlayerOnFirmGround's test at probe offset X ($81:B75C), the code the
   probe 8 and probe 4 entries share. */
/* Ground under one side of the player that does not give way: a tile his
   probe meets whose behaviour lets him stand (not passed through) and
   whose kind is not soft. Left side first, then right. */
static int firm_side(int left, uint8_t through)
{
    uint8_t behaviour, kind, soft;

    if (left)
        call_frame(0xB769, abi_GroundLeft);
    else
        call_frame(0xB77D, abi_GroundRight);
    if (!(cpu.p & P_C))
        return 0;
    behaviour = rd8(dba(TILE_BEHAVIOUR_BYTE));
    a8_load(behaviour);
    flag_set(P_Z, !(behaviour & through));
    if (behaviour & through)
        return 0;
    kind = behaviour & 0x07;
    x8_load(kind);
    soft = rd8(dba(SOFT_KINDS) + kind);
    a8_load(soft);
    return !soft;
}

static void firm_ground_at_x(void)
{
    int firm;

    phd();
    cpu.a = cpu.d = PLAYER;
    nz16_of(PLAYER);
    wr8(dpa(ACTOR_PROBE_AT), (uint8_t)cpu.x);
    firm = firm_side(1, 0x60) || firm_side(0, BEHAVIOUR_THROUGH);
    pld();
    flag_set(P_C, firm);
}

/*
 * @routine 81:B74E PlayerOnFirmGroundProbe8
 * WHAT IT DOES: Says whether the player stands on firm ground at probe 8:
 * C set when the ground to its left or right is solid and not soft.
 * HOW: PlayerOnFirmGround's test with the probe offset 8.
 * Entry: M 8-bit, X 8-bit; DB = $80; called by JSL. Exit: as
 * PlayerOnFirmGround.
 */
void abi_PlayerOnFirmGroundProbe8(void)
{
    x8_load(0x08);
    firm_ground_at_x();
}

/*
 * @routine 81:B75A PlayerOnFirmGroundProbe4
 * WHAT IT DOES: Says whether the player stands on firm ground at probe 4:
 * C set when the ground to its left or right is solid and not soft.
 * HOW: PlayerOnFirmGround's test with the probe offset 4.
 * Entry: M 8-bit, X 8-bit; DB = $80; called by JSL. Exit: as
 * PlayerOnFirmGround.
 */
void abi_PlayerOnFirmGroundProbe4(void)
{
    x8_load(0x04);
    firm_ground_at_x();
}

/* ActorSettleOnGround: the tile at the probe point is ground to stand on:
   not empty, not negative, not passed through (`back` the TileAtProbe
   JSR's return). The flags as the tests leave them. */
static int ground_at_probe(uint16_t back)
{
    call_frame(back, abi_TileAtProbe);
    if (cpu.p & (P_Z | P_N))
        return 0;
    lda8_v(BEHAVIOUR_THROUGH);
    bit8_v(rd8(abs_db(TILE_BEHAVIOUR_BYTE)));
    return (cpu.p & P_Z) != 0;
}

/*
 * @routine 81:B6E7 ActorSettleOnGround
 * WHAT IT DOES: When ground is at the actor's probe 8 to the left, or else
 * to the right, stops its fall and stands it on that tile's top. No code
 * refers to it.
 * HOW: ProbeLeft at probe 8, then TileAtProbe: a kind that is not empty,
 * not negative and not passed through goes on to the settling; else the
 * same at ProbeRight, which gives up otherwise. Settling: the vertical
 * speed cleared and y moved down to the next tile boundary (16 less the
 * point's y in the tile), as ActorSettleOnFloor.
 * Entry: M 8-bit, X 8-bit; D = the actor; DB a bank whose $0000-$1FFF is
 * work RAM and that holds the probe list and the tile kinds ($80); called
 * by JSL. Exit: M and X 8-bit. Settled: A = the new y (16 bits); N,
 * V, Z, C from its ADC. Else: A low byte and the flags as
 * the last test left them.
 */
void abi_ActorSettleOnGround(void)
{
    lda8_v(0x08);
    wr8(dp(ACTOR_PROBE_AT), 0x08);
    call_frame(0xB6ED, abi_ProbeLeft);
    if (!ground_at_probe(0xB6F0)) {
        call_frame(0xB6FE, abi_ProbeRight);
        if (!ground_at_probe(0xB701))
            return;
    }
    wr8(dp(ACTOR_SPEED_Y + 2), 0);
    op_rep(P_M);
    wr16(dp(ACTOR_SPEED_Y), 0);
    lda16(abs_db(POINT_Y));
    and16_v(0x000F);
    eor16_v(0xFFFF);
    ACC16(inc);
    clc();
    adc16_v(0x0010);
    clc();
    adc16_v(rd16(dp(ACTOR_Y + 1)));
    wr16(dp(ACTOR_Y + 1), cpu.a);
    op_sep(P_M);
}

#define GROUND_COUNT 0x002Bu       /* ActorGroundCount: the points in the ground */
#define WALL_X_BEFORE 0x002Au      /* ActorOutOfWallsMovePlayer: the actor's x before (word) */
#define PLAYER_X 0x08E7u           /* the player's x (pixels, word) */

/*
 * @routine 81:B72D ActorGroundCount
 * WHAT IT DOES: Counts how many of the actor's two probe-8 points, right
 * and left, are in the ground (0 to 2). Nothing but ActorGroundCountLong
 * calls it.
 * HOW: The count at $2B from 0; ProbeRight at probe 8, then PointInGround:
 * in the ground, the count up; the same with ProbeLeft.
 * Entry: M 8-bit, X 8-bit; D = the actor; DB a bank whose $0000-$1FFF is
 * work RAM and that holds the probe list and the tile kinds ($80); called
 * by JSR. Exit: M and X 8-bit; A low byte = the count; N, Z from it; C as
 * the last PointInGround left it.
 */
void abi_ActorGroundCount(void)
{
    lda8_v(0x08);
    wr8(dp(ACTOR_PROBE_AT), 0x08);
    stz8(ea_abs(GROUND_COUNT));
    call_frame(0xB736, abi_ProbeRight);
    call_frame(0xB739, abi_PointInGround);
    if (IF_C)
        MEM8(inc, ea_abs(GROUND_COUNT));
    call_frame(0xB741, abi_ProbeLeft);
    call_frame(0xB744, abi_PointInGround);
    if (IF_C)
        MEM8(inc, ea_abs(GROUND_COUNT));
    lda8(ea_abs(GROUND_COUNT));
}

/*
 * @routine 81:B729 ActorGroundCountLong
 * WHAT IT DOES: ActorGroundCount for callers in other banks. No code
 * refers to it.
 * HOW: ActorGroundCount.
 * Entry: as ActorGroundCount; called by JSL. Exit: as ActorGroundCount.
 */
void abi_ActorGroundCountLong(void)
{
    call_frame(0xB72B, abi_ActorGroundCount);
}

/*
 * @routine 81:B575 ActorOutOfWallsMovePlayer
 * WHAT IT DOES: Moves the actor out of a wall in front of it at probe 4,
 * or else at probe 8, and the player across by as much (an actor that
 * carries him). C set when it was moved. No code refers to it.
 * HOW: The actor's x pixels kept at $2A; ActorOutOfWall at probe 4, and
 * when not moved at probe 8; moved: the new x less the kept one added to
 * the player's x.
 * Entry: M 8-bit; D = the actor; DB a bank whose $0000-$1FFF is work RAM
 * and that holds the probe list and the tile kinds ($80); called by JSL.
 * Exit: M 8-bit; moved: C set, A = the player's new x (16 bits), N, Z
 * from its ADC. Not: C clear; A, N, Z as ActorOutOfWall left them.
 */
void abi_ActorOutOfWallsMovePlayer(void)
{
    lda8(ea_dp(ACTOR_X + 1));
    sta8(ea_abs(WALL_X_BEFORE));
    lda8(ea_dp(ACTOR_X + 2));
    sta8(ea_abs(WALL_X_BEFORE + 1));
    lda8_v(0x04);
    call_frame(0xB583, abi_ActorOutOfWall);
    if (!IF_C) {
        lda8_v(0x08);
        call_frame(0xB58A, abi_ActorOutOfWall);
        if (!IF_C) {
            op_sep(0x20);
            return;
        }
    }
    op_rep(0x20);
    lda16(ea_dp(ACTOR_X + 1));
    sbc16_v(rd16(ea_abs(WALL_X_BEFORE)));
    clc();
    adc16_v(rd16(ea_abs(PLAYER_X)));
    sta16(ea_abs(PLAYER_X));
    sec();
    op_sep(0x20);
}

/*
 * @routine 81:B5D0 GroundBehindKind7
 * WHAT IT DOES: Says whether the tile behind the actor at probe $20, or
 * else at probe $24, has behaviour low bits 7: A = 1 when one has, else
 * 0. No code refers to it, and no level's map has such a tile.
 * HOW: GroundBehind at probe $20, its tile's behaviour byte's low 3 bits
 * against 7; else the same at probe $24.
 * Entry: M 8-bit; D = the actor; DB a bank whose $0000-$1FFF is work RAM
 * and that holds the probe list and the tile kinds ($80); called by JSL.
 * Exit: M and X 8-bit; A low byte = the answer; N, Z from it; C from the
 * last compare.
 */
void abi_GroundBehindKind7(void)
{
    static const uint8_t probes[2] = { 0x20, 0x24 };
    static const uint16_t backs[2] = { 0xB5D6, 0xB5E6 };   /* the two JSRs' returns */
    int i;

    for (i = 0; i < 2; i++) {
        lda8_v(probes[i]);
        wr8(dp(ACTOR_PROBE_AT), probes[i]);
        call_frame(backs[i], abi_GroundBehind);
        lda8(abs_db(TILE_BEHAVIOUR_BYTE));
        and8_v(0x07);
        cmp8_v(0x07);
        if (IF_Z) {
            lda8_v(0x01);
            return;
        }
    }
    lda8_v(0x00);
}

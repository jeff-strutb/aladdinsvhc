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
#include "cpu_ops.h"
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
    cpu.y = (uint8_t)(cpu.y + 2);
    cpu.p &= (uint8_t)~P_C;
    cpu.a = op_adc16(probe(cpu.y), rd16(dp(ACTOR_Y + 1)), &cpu.p);
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
    op_rep(P_M);
    op_sep(P_X);
    cpu.y = rd8(dp(ACTOR_PROBE_AT));
    cpu.p |= P_C;
    cpu.a = op_sbc16(rd16(dp(ACTOR_X + 1)), probe(cpu.y), &cpu.p);
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
    op_rep(P_M);
    op_sep(P_X);
    cpu.y = rd8(dp(ACTOR_PROBE_AT));
    cpu.p &= (uint8_t)~P_C;
    cpu.a = op_adc16(probe(cpu.y), rd16(dp(ACTOR_X + 1)), &cpu.p);
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
 * screen); DB = $81; called by JSR. Exit: M and X 8-bit; A low byte = X =
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
 * Entry: M 16-bit, X 16-bit; D = the actor; DB = $81; called by JSR.
 * Exit: as TileAtPlace, or as ProbeOnView off the view.
 */
void abi_TileAt(void)
{
    uint16_t place, t;

    if (probe_off_view())
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
 * Entry: D = the actor; DB = $81; called by JSR. Exit: as TileAt.
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
 * Entry: M 16-bit, X 16-bit; D = the actor; DB = $81; called by JSR.
 * Exit: M and X 8-bit; C as above; not ground or solid: A and N, Z as
 * TileAt leaves them (solid: Z set); else A = the point's y (16 bits), N,
 * Z from its compare with the surface.
 */
void abi_PointInGround(void)
{
    abi_TileAt();
    if ((cpu.p & P_Z) || (cpu.p & P_N)) {
        cpu.p &= (uint8_t)~P_C;
        return;
    }
    cpu.p = cmp8_flags(cpu.p, (uint8_t)cpu.a, KIND_SOLID);
    if ((uint8_t)cpu.a == KIND_SOLID) {
        cpu.p |= P_C;
        return;
    }
    abi_SurfaceHeight();
    op_rep(P_M | P_X);
    cpu.a = rd16(abs_db(POINT_Y));
    op_cmp16(rd16(abs_db(SURFACE_Y)));
    op_sep(P_M | P_X);
}

/*
 * @routine 81:B7CF PointInGroundLong
 * WHAT IT DOES: PointInGround for callers in other banks.
 * HOW: 16-bit registers, then PointInGround.
 * Entry: D = the actor; DB = $81; called by JSL. Exit: as PointInGround.
 */
void abi_PointInGroundLong(void)
{
    op_rep(P_M | P_X);
    abi_PointInGround();
}

/*
 * @routine 81:B7C3 ActorInGround
 * WHAT IT DOES: Says whether the actor's own position is in the ground.
 * HOW: The point = the actor's x and y; then PointInGroundLong.
 * Entry: D = the actor; DB = $81; called by JSL. Exit: as PointInGround.
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
 * Entry: X 8-bit; D = the actor; DB = $81; called by JSR. Exit: as
 * PointInGround.
 */
void abi_GroundRight(void)
{
    abi_ProbeRight();
    abi_PointInGround();
}

/*
 * @routine 81:B7A8 GroundLeft
 * WHAT IT DOES: Says whether the ground is at the probe point to the
 * actor's left.
 * HOW: ProbeLeft, then PointInGround.
 * Entry: X 8-bit; D = the actor; DB = $81; called by JSR. Exit: as
 * PointInGround.
 */
void abi_GroundLeft(void)
{
    abi_ProbeLeft();
    abi_PointInGround();
}

/*
 * @routine 81:B79F GroundInFront
 * WHAT IT DOES: Says whether the ground is at the probe point in front of
 * the actor, the way it faces.
 * HOW: Facing right, GroundRight; facing left, GroundLeft.
 * Entry: M 8-bit; D = the actor; DB = $81; called by JSR. Exit: as
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
 * Entry: M 8-bit; D = the actor; DB = $81; called by JSR. Exit: as
 * PointInGround.
 */
void abi_GroundBehind(void)
{
    uint8_t f = rd8(dp(ACTOR_FLIP));

    cpu.a = (uint16_t)((cpu.a & 0xFF00) | f);
    cpu.p = nz8(cpu.p, f);
    if (f == 0)
        abi_GroundRight();
    else
        abi_GroundLeft();
}

/*
 * @routine 81:B7BB GroundRightLong
 * WHAT IT DOES: GroundRight for callers in other banks.
 * HOW: GroundRight.
 * Entry: as GroundRight; called by JSL. Exit: as PointInGround.
 */
void abi_GroundRightLong(void)
{
    abi_GroundRight();
}

/*
 * @routine 81:B7BF GroundLeftLong
 * WHAT IT DOES: GroundLeft for callers in other banks.
 * HOW: GroundLeft.
 * Entry: as GroundLeft; called by JSL. Exit: as PointInGround.
 */
void abi_GroundLeftLong(void)
{
    abi_GroundLeft();
}

/*
 * @routine 81:B82B TileAtProbeLong
 * WHAT IT DOES: TileAtProbe for callers in other banks.
 * HOW: TileAtProbe.
 * Entry: as TileAtProbe; called by JSL. Exit: as TileAt.
 */
void abi_TileAtProbeLong(void)
{
    abi_TileAtProbe();
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
    abi_GroundInFront();
}

/*
 * @routine 81:B5F6 GroundBehindBit4
 * WHAT IT DOES: Tests the ground behind the actor, and moves the tile
 * behaviour's bit 4 into bit 5 when it is set.
 * HOW: GroundBehind; then the behaviour byte, when bit 4 is set, shifts
 * left.
 * Entry: M 8-bit; D = the actor; DB = $81; called by JSL. Exit: as
 * GroundBehind, but A low byte = the behaviour's bit 4 and N, Z from it;
 * C its bit 7 when shifted.
 */
void abi_GroundBehindBit4(void)
{
    uint8_t b;

    abi_GroundBehind();
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
 * Entry: M 8-bit; D = the actor; DB = $81; called by JSL. Exit: as
 * GroundBehind, but A low byte = the last tile's behaviour bit 3, N, Z
 * from it.
 */
void abi_GroundBehindBit3(void)
{
    static const uint8_t probes[2] = { 0x10, 0x14 };
    uint8_t b = 0;
    int i;

    for (i = 0; i < 2; i++) {
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | probes[i]);
        cpu.p = nz8(cpu.p, probes[i]);
        wr8(dp(ACTOR_PROBE_AT), probes[i]);
        abi_GroundBehind();
        b = (uint8_t)(rd8(abs_db(TILE_BEHAVIOUR_BYTE)) & 0x08);
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | b);
        cpu.p = nz8(cpu.p, b);
        if (b)
            break;
    }
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
    uint8_t b;
    uint16_t a;

    wr8(dp(ACTOR_PROBE_AT), (uint8_t)cpu.a);
    abi_GroundInFront();
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
    op_rep(P_M);
    a = (uint16_t)(rd16(abs_db(POINT_X)) & 0x000F);
    if (!(a & 0x0008)) {
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
    return;
none:
    cpu.p &= (uint8_t)~P_C;
}

/*
 * @routine 81:B608 ActorOutOfWallLong
 * WHAT IT DOES: ActorOutOfWall for callers in other banks.
 * HOW: ActorOutOfWall.
 * Entry: as ActorOutOfWall; called by JSL. Exit: as ActorOutOfWall.
 */
void abi_ActorOutOfWallLong(void)
{
    abi_ActorOutOfWall();
}

/*
 * @routine 81:B5AD ActorOutOfWallProbe4
 * WHAT IT DOES: Moves the actor out of a wall at probe 4 in front of it.
 * HOW: ActorOutOfWall at probe 4.
 * Entry: M 8-bit; D = the actor; DB = $81; called by JSL. Exit: as
 * ActorOutOfWall.
 */
void abi_ActorOutOfWallProbe4(void)
{
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x04);
    cpu.p = nz8(cpu.p, 0x04);
    abi_ActorOutOfWall();
}

/*
 * @routine 81:B59F ActorOutOfWalls
 * WHAT IT DOES: Moves the actor out of a wall in front of it, testing
 * probes $0C, 8 and 4 until one moves it.
 * HOW: ActorOutOfWall at probes $0C and 8 while it returns C clear, then
 * ActorOutOfWallProbe4.
 * Entry: M 8-bit; D = the actor; DB = $81; called by JSL. Exit: as the
 * last ActorOutOfWall.
 */
void abi_ActorOutOfWalls(void)
{
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x0C);
    cpu.p = nz8(cpu.p, 0x0C);
    abi_ActorOutOfWall();
    if (cpu.p & P_C)
        return;
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x08);
    cpu.p = nz8(cpu.p, 0x08);
    abi_ActorOutOfWall();
    if (cpu.p & P_C)
        return;
    abi_ActorOutOfWallProbe4();
}

/* A floor tile under the point (TileAtProbe): returns 1 floor, 0 not a
   floor, -1 out (a kind with bit 7, or one the actor passes through: the
   caller gives up). `strict` treats passing through as out, else as not
   a floor. */
static int floor_at(int strict)
{
    uint8_t b, k;

    abi_TileAtProbe();
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
 * Entry: M 8-bit, X 8-bit; D = the actor; DB = $81; called by JSL.
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
    abi_ProbeLeft();
    op_sep(P_M);
    cpu.x = rd16(abs_db(POINT_Y));
    cpu.p = nz16(cpu.p, cpu.x);
    if (cpu.x & 0x8000)
        goto out;
    f = floor_at(0);
    if (f != 1) {
        abi_ProbeRight();
        op_sep(P_M);
        cpu.x = rd16(abs_db(POINT_Y));
        cpu.p = nz16(cpu.p, cpu.x);
        if (cpu.x & 0x8000)
            goto out;
        f = floor_at(1);
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
    abi_TileAtProbe();
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

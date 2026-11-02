/*
 * placement.c: creating a level's placed actors as the camera comes near
 * them.
 *
 * The level's placements are indexed in 32-pixel bands across and down
 * (actor/placement_index.c). Each time the camera has moved 32 pixels
 * from the last band it placed from, the actors of the band now coming
 * into reach are created, those within 384 pixels of the camera and not
 * already placed or gone. A created actor of a kind gets its record from
 * the free list and its kind's set-up (graphics: a tile slot, or the
 * attributes of a slot already loaded).
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "cpu_ops.h"
#include "native.h"
void abi_PlacementsIndex(void);
void abi_PlacementsStart(void);
void abi_ShiftRight5(void);
void abi_PlacementsScroll(void);
void abi_PlacementsAcross(void);
void abi_PlacementsDown(void);
void abi_ActorCreate(void);
void abi_ActorCreateIn(void);
void abi_ActorKindSetup(void);
void abi_ActorKindTilesShared(void);
void abi_ActorKindTilesQueue(void);
void abi_ActorKindTiles(void);
void abi_TileSlotsClaim2(void);
void abi_TileSlotsClaim3(void);
void abi_ActorCreateChild(void);
void abi_ActorCreateChild8(void);
void abi_TileSlotFill(void);
void abi_TileSlotClaim(void);
void abi_ActorSpawnInit(void);
void abi_ActorPlace(void);
#include "actor.h"
#include "actor_anim.h"
#include "wram.h"
#include "wram_video.h"

/* A word or byte of the data bank. */
static uint16_t db16(uint16_t addr) { return rd16(dba(addr)); }

/* One axis: the camera at `camera` moved 32 or more from where the
   placements were last brought in (`last`); `last` moves 32 that way, the
   two bands (`near`, `far`) a band, and the band coming into view is
   brought in by `bring`. */
static void scroll_axis(uint16_t camera, uint16_t last, uint16_t near, uint16_t far, void (*bring)(void))
{
    uint16_t d, x = 0;
    uint8_t step, band;

    op_rep(0x30);
    cpu.x = 0;
    dp16_set(0x2A, 0);
    flag_set(P_C, 1);
    d = op_sbc16(db16(camera), db16(last), &cpu.p);
    if (d & 0x8000) {
        inc16_at(dpa(0x2A));
        x = 2;
        cpu.x = x;
        d = (uint16_t)((d ^ 0xFFFF) + 1);
    }
    cpu.a = d;
    cmp16_of(d, 0x0020);
    if (d < 0x0020)
        return;
    cpu.a = op_sbc16(db16(last), db16((uint16_t)(0xA794 + x)), &cpu.p);
    wr16(dba(last), cpu.a);
    op_sep(0x30);
    y8_load(step = rd8(dba((uint16_t)(0xA798 + x))));
    flag_set(P_C, 0);
    wr8(dba(near), op_adc8(step, rd8(dba(near)), &cpu.p));
    flag_set(P_C, 0);
    band = op_adc8(step, rd8(dba(far)), &cpu.p);
    wr8(dba(far), band);
    x8_load(band);
    a8_load(rd8(dpa(0x2A)));
    if (!(uint8_t)cpu.a)
        x8_load(rd8(dba(near)));
    bring();
}

/*
 * @routine 82:A79C PlacementsScroll
 * WHAT IT DOES: Creates the placed actors the camera's move brings into
 * reach: a band of placements across, or down, each time the camera is 32
 * pixels or more from where the last band was placed.
 * HOW: For x, then y: the camera less the last placing point; 32 or more
 * either way moves the point by 32 and the band numbers by one, and places
 * the band ahead (PlacementsAcross, PlacementsDown), the one on the side it
 * moved to.
 * Entry: D = 0; DB any; called by JSL. Exit: M and X 8-bit; DB as at
 * entry.
 */
void abi_PlacementsScroll(void)
{
    uint8_t db = cpu.db;

    op_sep(P_M);
    cpu.db = 0x82;
    a8_load(0x82);
    scroll_axis(0x1773, 0x1BEF, 0x1BEB, 0x1BED, abi_PlacementsDown);
    scroll_axis(0x1777, 0x1BF1, 0x1BE9, 0x1BE7, abi_PlacementsAcross);
    op_sep(0x30);
    cpu.db = db;
    nz8_of(db);
}

/* A placement's offset in the list ($6D on, 6 bytes each) for entry
   `n`. */
static uint16_t entry_at(uint16_t n)
{
    flag_set(P_C, 0);
    return op_adc16((uint16_t)(n * 6), dp16(0x6D), &cpu.p);
}

/* The placement at `at` (its kind at $8A7A) created and placed, unless
   its flags ($7E:C600 by number) say it is gone or its position (at
   `pos`) is more than $40 outside the view from `camera`. */
static void bring_one(uint16_t at, uint16_t pos, uint16_t camera, int kind_byte)
{
    uint16_t d;

    op_sep(0x30);
    x8_load(rd8(dpa(0x12)));
    a8_load(rd8(0x7EC600u + cpu.x) & 0x81);
    if ((uint8_t)cpu.a)
        return;
    op_rep(0x31);
    cpu.y = at;
    d = op_adc16(db16((uint16_t)(pos + at)), 0x0040, &cpu.p);
    flag_set(P_C, 1);
    d = op_sbc16(d, db16(camera), &cpu.p);
    cpu.a = d;
    cmp16_of(d, 0x0180);
    if (d >= 0x0180)
        return;
    if (kind_byte) {
        op_sep(P_M);
        a8_load(rd8(dba((uint16_t)(0x8A7A + at))));
    } else {
        a16_load(db16((uint16_t)(0x8A7A + at)));
    }
    op_sep(P_X);
    abi_ActorCreate();
    if (cpu.p & P_C)
        return;
    cpu.y = at;
    abi_ActorPlace();
}

/*
 * @routine 82:A83E PlacementsAcross
 * WHAT IT DOES: Creates the actors of band X across that are near the
 * camera and not yet placed.
 * HOW: The band's run in the placement order; each placement not placed
 * or gone, whose x is within 384 pixels round the camera's band, is created
 * (ActorCreate with its kind) and placed (ActorPlace).
 * Entry: M 8-bit, X 8-bit; D = 0; DB = $82; X = the band; called by JSR.
 * Exit: M and X 8-bit.
 */
void abi_PlacementsAcross(void)
{
    uint8_t n;

    a8_load(n = rd8(0x7EC300u + cpu.x));
    if (!n)
        return;
    wr8(dpa(0x0C), n);
    wr8(dpa(0x0D), rd8(0x7EC400u + cpu.x));
    do {
        /* each of the band's placements, by the band's list of numbers */
        op_rep(P_M);
        x8_load(rd8(dpa(0x0D)));
        dp16_set(0x12, rd16(0x7EC500u + cpu.x) & 0x00FF);
        cpu.a = entry_at(dp16(0x12));
        dp16_set(0x0E, cpu.a);
        inc16_at(dpa(0x12));
        op_sep(P_M);
        bring_one(dp16(0x0E), 0x8A7C, 0x1BEF, 0);
        op_sep(0x30);
        inc8_at(dpa(0x0D));
    } while (dec8_at(dpa(0x0C)));
}

/*
 * @routine 82:A89D PlacementsDown
 * WHAT IT DOES: Creates the actors of band X down that are near the camera
 * and not yet placed.
 * HOW: The band's placements, from its first; each not placed or gone,
 * whose y is within 384 pixels round the camera's band, is created
 * (ActorCreate with its kind) and placed (ActorPlace).
 * Entry: M 8-bit, X 8-bit; D = 0; DB = $82; X = the band; called by JSR.
 * Exit: M and X 8-bit.
 */
void abi_PlacementsDown(void)
{
    uint8_t n;
    uint16_t left;

    a8_load(n = rd8(0x7EC100u + cpu.x));
    if (!n)
        return;
    /* the band's placements, one after another from its first */
    wr8(dpa(0x0C), n);
    wr8(dpa(0x0D), 0);
    op_rep(P_M);
    dp16_set(0x12, rd16(0x7EC200u + cpu.x) & 0x00FF);
    dp16_set(0x10, entry_at(dp16(0x12)));
    inc16_at(dpa(0x12));
    do {
        bring_one(dp16(0x10), 0x8A7E, 0x1BF1, 1);
        op_rep(0x21);
        cpu.a = op_adc16(dp16(0x10), 0x0006, &cpu.p);
        dp16_set(0x10, cpu.a);
        inc16_at(dpa(0x12));
        left = dec16_at(dpa(0x0C));
    } while (left && !(left & 0x8000));
    op_sep(0x30);
}

/*
 * @routine 82:AA2B ActorCreate
 * WHAT IT DOES: Creates an actor of kind A, if a record is free and its
 * kind's set-up succeeds: C clear.
 * HOW: The free list's top; none free: C set, A = 1. Else ActorCreateIn.
 * Entry: M 8-bit, X 8-bit; A = the kind; DB any; called by JSL. Exit: as
 * ActorCreateIn; none free: M and X 8-bit, C set, A low byte = 1.
 */
void abi_ActorCreate(void)
{
    y8_load(rd8(dba((uint16_t)ACTOR_FREE_TOP)));
    if (!((uint8_t)cpu.y & 0x80)) {
        abi_ActorCreateIn();
        return;
    }
    op_sep(0x31);
    a8_load(0x01);
}

/*
 * @routine 82:AA30 ActorCreateIn
 * WHAT IT DOES: Creates an actor of kind A in the free list's top record.
 * HOW: Keeps the kind; the record's $06 = the kind; the kind's set-up
 * (ActorKindSetup); on success the record leaves the free list and the
 * kind's count of live actors steps up.
 * Entry: A = the kind; Y = the free list's top offset; DB = $82; falls
 * in from ActorCreate. Exit: created: C clear, X 16-bit = the record.
 * Failed: M and X 8-bit, C set, A low byte = 1.
 */
void abi_ActorCreateIn(void)
{
    uint16_t x;
    uint8_t n;

    /* the kind (a byte or word, as M is) and the free list's top record */
    if (cpu.p & P_M) {
        wr8(dba(0x001C), (uint8_t)cpu.a);
        wr8(dba(0x001D), 0);
    } else {
        wr16(dba(0x001C), cpu.a);
        wr16(dba(0x001D), 0);
    }
    op_rep(P_X);
    x = db16((uint16_t)(ACTOR_FREE_LIST + cpu.y));
    cpu.x = x;
    nz16_of(x);
    if (cpu.p & P_M)
        wr8(dba((uint16_t)(ACTOR_06 + x)), (uint8_t)cpu.a);
    else
        wr16(dba((uint16_t)(ACTOR_06 + x)), cpu.a);
    wr16(dba(0x0020), x);
    abi_ActorKindSetup();
    if (cpu.p & P_C) {
        op_sep(0x31);
        a8_load(0x01);
        return;
    }
    /* taken from the free list; one more of the kind */
    dec8_at(dba((uint16_t)ACTOR_FREE_TOP));
    dec8_at(dba((uint16_t)ACTOR_FREE_TOP));
    x = db16(0x001C);
    a8_load(n = (uint8_t)(rd8(KIND_COUNT + x) + 1));
    wr8(KIND_COUNT + x, n);
    cpu.x = x = db16(0x0020);
    nz16_of(x);
}

/*
 * @routine 82:AA61 ActorKindSetup
 * WHAT IT DOES: Runs the set-up of actor kind A for its new record.
 * HOW: Keeps the kind * 2; with the data bank at $82, the kind's set-up
 * index picks its routine from the set-up table.
 * Entry: A = the kind; called by JSR. Exit: M 8-bit, X 16-bit; C and the
 * rest as the set-up leaves them (C set: it failed).
 */
void abi_ActorKindSetup(void)
{
    uint8_t kind, db = cpu.db;

    op_sep(0x30);
    kind = (uint8_t)cpu.a;
    y8_load(kind);
    wr8(dba(0x0022), (uint8_t)(kind << 1));
    wr8(dba(0x0023), kind >> 7);
    nz8_of(kind >> 7);
    flag_set(P_C, 0);
    /* the kind's set-up routine, by its class at $82:B000 */
    cpu.db = 0x82;
    a8_load(0x82);
    x8_load(rd8(dba((uint16_t)(0xB000 + kind))));
    call_routine(0x820000u | rd16(0x820000u | (uint16_t)(0xAA81 + cpu.x)), 0);
    cpu.db = db;
    nz8_of(db);
    op_sep(P_M);
    op_rep(P_X);
}

/* A tile slot (or run of 2 or 3) by the kind's extra slots; C set when
   none was free. */
static void claim_by_size(void)
{
    cpu.x = db16(0x001C);
    a8_load(rd8(CELL_EXTRA_SLOTS + cpu.x));
    if (!(uint8_t)cpu.a) {
        abi_TileSlotClaim();
        return;
    }
    a8_load((uint8_t)(cpu.a - 1));
    if (!(uint8_t)cpu.a)
        abi_TileSlotsClaim2();
    else
        abi_TileSlotsClaim3();
}

/*
 * @routine 82:AA89 ActorKindTilesShared
 * WHAT IT DOES: Gives a new actor its kind's graphics: the slot already
 * loaded for the kind, or a new tile slot (one, two or three slots).
 * HOW: Graphics loaded (the cell's flag): the record takes the cell's
 * sprite attributes and slot, whose holders step up. Else claims one slot
 * (TileSlotClaim), two (TileSlotsClaim2) or three (TileSlotsClaim3) as the
 * cell needs, then queues their graphics (ActorKindTilesQueue).
 * Entry: M 8-bit; scratch $1C = the kind, $20 = the record, $22 = the
 * kind * 2; called by JSR. Exit: C clear when given, C set when no slot was
 * free.
 */
void abi_ActorKindTilesShared(void)
{
    uint16_t rec, x;

    op_rep(P_X);
    cpu.y = rec = db16(0x0020);
    cpu.x = x = db16(0x0022);
    a8_load(rd8(0x7FE701u + x));
    if (!((uint8_t)cpu.a & 0x80)) {
        claim_by_size();
        if (cpu.p & P_C)
            return;
        abi_ActorKindTilesQueue();
        flag_set(P_C, 0);
        return;
    }
    /* the kind's tiles already loaded: share its slot */
    a8_load(rd8(CELL_ATTRS + x));
    wr8(dba((uint16_t)(0x002B + rec)), (uint8_t)cpu.a);
    a8_load(rd8(0x7FE701u + x));
    wr8(dba((uint16_t)(0x002C + rec)), (uint8_t)cpu.a);
    cpu.x = db16(0x001C);
    a8_load(rd8(0x7FEB00u + cpu.x));
    cpu.a &= 0x00FF;
    wr8(dba((uint16_t)(0x0039 + rec)), (uint8_t)cpu.a);
    cpu.x = cpu.a;
    inc8_at(dba((uint16_t)(ACTOR_TILE_SLOT_USERS + cpu.x)));
    flag_set(P_C, 0);
}

/*
 * @routine 82:AAE7 ActorKindTilesQueue
 * WHAT IT DOES: Queues a kind's graphics for the NMI to upload to the
 * actor's tile slot, and fills the slot's record (TileSlotFill).
 * HOW: A VRAM queue record: the slot's VRAM address from the cell's
 * attributes, the graphics' source and bank $7F, the slot's size; a zero
 * size is not queued. Kind $33 (X = $66) only fills.
 * Entry: M 8-bit, X 16-bit; X = the kind * 2; called by JSR. Exit: as
 * TileSlotFill; C clear.
 */
void abi_ActorKindTilesQueue(void)
{
    uint16_t x = cpu.x, y, length;

    op_rep(P_M);
    cmp16_of(x, 0x0066);
    if (x == 0x0066) {
        op_sep(0x30);
        abi_TileSlotFill();
        return;
    }
    /* a VRAM queue entry: the slot's VRAM address, the graphics' place in
       bank $7F and their length */
    cpu.y = y = (uint16_t)((db16((uint16_t)VRAM_QUEUE_COUNT) & 0x00FF) << 3);
    flag_set(P_C, 0);
    wr16(dba((uint16_t)(VRAM_QUEUE + y)), op_adc16((uint16_t)((rd16(CELL_ATTRS + x) & 0x01FF) << 4), 0x6000, &cpu.p));
    wr16(dba((uint16_t)(0x085C + y)), rd16(0x7FE500u + x));
    wr16(dba((uint16_t)(0x085E + y)), 0x007F);
    cpu.x = x = rd16(0x82AC18u + x);
    a16_load(length = rd16(0x82AC58u + x) & 0x3FFF);
    wr16(dba((uint16_t)(0x085F + y)), length);
    op_sep(P_M);
    if (!length) {
        flag_set(P_C, 0);
        return;
    }
    a8_load(0x80);
    wr8(dba((uint16_t)(0x0861 + y)), 0x80);
    op_sep(P_X);
    inc8_at(dba((uint16_t)VRAM_QUEUE_COUNT));
    inc8_at(dba(0x0403));
    abi_TileSlotFill();
}

/*
 * @routine 82:AB3F ActorKindTiles
 * WHAT IT DOES: Gives a new actor a new tile slot (one, two or three) for
 * its kind's graphics.
 * HOW: As the cell needs: TileSlotClaim, TileSlotsClaim2 or
 * TileSlotsClaim3; then TileSlotFill.
 * Entry: scratch $1C = the kind, $20 = the record; called by JSR. Exit: C
 * clear when given; C set when no slot was free.
 */
void abi_ActorKindTiles(void)
{
    op_rep(P_X);
    cpu.y = db16(0x0020);
    claim_by_size();
    if (cpu.p & P_C)
        return;
    abi_TileSlotFill();
    flag_set(P_C, 0);
}

/* The run of slots from X taken by the kind: the first's count, its cell;
   the record's slot (X + 1), the kind's, and its attributes from the
   slot's ($82:AC06). */
static void claim_run(uint16_t x)
{
    uint16_t rec = cpu.y, attrs;
    uint8_t slot;

    inc8_at(dba((uint16_t)(0x15E9 + x)));
    wr8(dba((uint16_t)(TILE_SLOT_CELL + x)), rd8(dba(0x001C)));
    slot = (uint8_t)(x + 1);
    a8_load(slot);
    cpu.a = slot;
    wr8(dba((uint16_t)(0x0039 + rec)), slot);
    wr8(0x7FEB00u + db16(0x001C), slot);
    op_rep(P_M);
    cpu.a = (uint16_t)(slot << 1);
    attrs = db16((uint16_t)(0xAC06 + cpu.a));
    cpu.a = attrs;
    wr16(dba((uint16_t)(0x002B + rec)), attrs);
    cpu.x = db16(0x0022);
    wr16(CELL_ATTRS + cpu.x, attrs);
    op_sep(P_M);
    flag_set(P_C, 0);
}

/*
 * @routine 82:AC10 TileSlotsClaim2
 * WHAT IT DOES: Claims two neighbouring tile slots for a kind's graphics,
 * if two are free: C clear.
 * HOW: The first pair of free neighbours among slots 1-4; its second
 * slot's holders step up, then TileSlotClaim's steps with the first.
 * Entry: M 8-bit, X 16-bit; scratch $1C = the kind, $20 = the record;
 * called by JSR. Exit: C set when no pair was free; else as TileSlotClaim.
 */
void abi_TileSlotsClaim2(void)
{
    uint16_t x;

    /* two free slots side by side, from the first three */
    for (x = 0; x < 3; x++) {
        cpu.x = x;
        a8_load(rd8(dba((uint16_t)(0x15E9 + x))) | rd8(dba((uint16_t)(0x15EA + x))));
        if (!(uint8_t)cpu.a) {
            inc8_at(dba((uint16_t)(0x15EA + x)));
            claim_run(x);
            return;
        }
    }
    flag_set(P_C, 1);
}

/*
 * @routine 82:AC34 TileSlotsClaim3
 * WHAT IT DOES: Claims three neighbouring tile slots for a kind's
 * graphics, if three are free: C clear.
 * HOW: Slots 1-3 or 2-4, the first free run; its second and third slots'
 * holders step up, then TileSlotClaim's steps with the first.
 * Entry: M 8-bit, X 16-bit; scratch $1C = the kind, $20 = the record;
 * called by JSR. Exit: C set when no run was free; else as TileSlotClaim.
 */
void abi_TileSlotsClaim3(void)
{
    uint16_t x;

    /* three free slots in a row, from the first two */
    for (x = 0; x < 2; x++) {
        cpu.x = x;
        a8_load(rd8(dba((uint16_t)(0x15E9 + x))) | rd8(dba((uint16_t)(0x15EA + x))) | rd8(dba((uint16_t)(0x15EB + x))));
        if (!(uint8_t)cpu.a) {
            inc8_at(dba((uint16_t)(0x15EA + x)));
            inc8_at(dba((uint16_t)(0x15EB + x)));
            claim_run(x);
            return;
        }
    }
    flag_set(P_C, 1);
}

/*
 * @routine 81:A9A6 ActorCreateChild
 * WHAT IT DOES: Creates an actor of kind A as a child of the actor,
 * placed at its spawn point, if the kind can be created.
 * HOW: ActorCreate; on success ActorSpawnInit for the new record.
 * Entry: M 8-bit, X 8-bit; D = the spawner; A = the kind; DB = $81;
 * called by JSL. Exit: C clear when created (X 16-bit = the record); else
 * as ActorCreate.
 */
void abi_ActorCreateChild(void)
{
    abi_ActorCreate();
    if (cpu.p & P_C)
        return;
    abi_ActorSpawnInit();
    flag_set(P_C, 0);
}

/*
 * @routine 81:A9B1 ActorCreateChild8
 * WHAT IT DOES: ActorCreateChild, returning with 8-bit registers and A =
 * 0 when created.
 * HOW: ActorCreate; on success ActorSpawnInit, then M and X 8-bit and A
 * = 0.
 * Entry: as ActorCreateChild. Exit: created: M and X 8-bit, A low byte =
 * 0, Z set; else as ActorCreate.
 */
void abi_ActorCreateChild8(void)
{
    abi_ActorCreate();
    if (cpu.p & P_C)
        return;
    abi_ActorSpawnInit();
    op_sep(0x30);
    a8_load(0x00);
}

/* `v` shifted right 5 (ShiftRight5), into `at`. */
static void band_of(uint16_t v, uint16_t at)
{
    cpu.a = v;
    abi_ShiftRight5();
    wr16(dba(at), cpu.a);
}

/*
 * @routine 82:A72C PlacementsStart
 * WHAT IT DOES: Indexes the level's placements as it starts, sets the
 * placing points from the camera, and creates the actors of the 12 bands
 * down from 64 pixels left of the camera.
 * HOW: With D = 0 and DB = $82: PlacementsIndex; the last placement's
 * flags byte ($7E:C6FF) set to $FF; from the scroll y, placed_y and the
 * bands (y - 64) / 32 and (y + 288) / 32 at placed_band_x; from the scroll
 * x, placed_x and the bands (x + 288) / 32 and (x - 64) / 32 at
 * placed_band_y; PlacementsDown for 12 bands from (x - 64) / 32.
 * Entry: DB, D any; called by JSL. Exit: M 8-bit, X 8-bit; D and DB as at
 * entry; Y = 0.
 */
void abi_PlacementsStart(void)
{
    uint16_t d = cpu.d, at;
    uint8_t db = cpu.db, x, n;

    op_sep(0x30);
    cpu.d = 0;
    cpu.db = 0x82;
    a8_load(0x82);
    abi_PlacementsIndex();
    a8_load(0xFF);
    wr8(0x7EC6FFu, 0xFF);
    /* the bands in view from the camera, from $40 before to $120 past */
    op_rep(0x21);
    at = db16(0x1777);
    wr16(dba(0x1BF1), at);
    band_of(op_adc16(at, 0x0120, &cpu.p), 0x1BE9);
    flag_set(P_C, 1);
    band_of(op_sbc16(at, 0x0040, &cpu.p), 0x1BE7);
    at = db16(0x1773);
    wr16(dba(0x1BEF), at);
    flag_set(P_C, 0);
    band_of(op_adc16(at, 0x0120, &cpu.p), 0x1BEB);
    flag_set(P_C, 1);
    band_of(op_sbc16(at, 0x0040, &cpu.p), 0x1BED);
    op_sep(0x30);
    /* 12 bands down from the first */
    x = (uint8_t)cpu.a;
    for (n = 0x0C; n; n--) {
        cpu.x = x;
        cpu.y = n;
        abi_PlacementsDown();
        x++;
    }
    cpu.x = x;
    cpu.y = 0;
    cpu.db = db;
    cpu.d = d;
    nz16_of(d);
}

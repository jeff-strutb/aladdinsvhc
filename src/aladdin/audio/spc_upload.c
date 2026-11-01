/*
 * spc_upload.c: sending code and data to the SPC700 through its IPL
 * transfer protocol.
 *
 * The lists are indexed by a multiple of 3 in the pointer table at
 * $85:8000 (config/aladdin/spclists.csv): the driver (0), two sample sets,
 * then songs. A list is blocks of a length word, an APU address word and
 * the bytes; a block of length 0 ends it, its address where the SPC700
 * goes on.
 */
#include "machine.h"
#include "cpu_ops.h"
#include "hw.h"
#include "wram.h"

#define SPC_LISTS 0x858000u        /* 3 bytes a list: address (bit 15 dropped), bank (bit 7 set) */
#define IPL_READY 0xBBAAu          /* ports 0-1 while the SPC700 waits in its IPL */
#define IPL_START 0xCC             /* the first counter value */
#define SOUND_SETS_LOADED 0x00000Au /* nonzero once the driver and sample sets are in (cleared at power-on) */
#define DRIVER_TO_IPL 0xF9         /* the command that sends a running driver back to the IPL */

/* Direct-page bytes the upload uses: the command, and the list's long
   pointer (its bank steps as the data crosses $8000). */
#define DP_COMMAND 0x34
#define DP_LIST 0x0C

static uint32_t dp(uint8_t off)
{
    return (uint16_t)(cpu.d + off);
}

/* LDA [$0C],Y, 8-bit and 16-bit: the pointer is read each time. */
static uint8_t list8(void)
{
    return rd8((rd24(dp(DP_LIST)) + cpu.y) & 0xFFFFFF);
}

static uint16_t list16(void)
{
    return rd16((rd24(dp(DP_LIST)) + cpu.y) & 0xFFFFFF);
}

static void set_a8(uint8_t v)
{
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
}

/* CMP APUIO0 until it reads back A low: C and Z set when it does. */
static void await_port0(void)
{
    uint8_t a = (uint8_t)cpu.a;

    do
        cpu.p = cmp8_flags(cpu.p, a, rd8(APUIO0));
    while (!(cpu.p & P_Z));
}

/*
 * @routine 80:B4D0 SpcUpload
 * WHAT IT DOES: Sends list X (the driver, a sample set or a song) to the
 * SPC700. Unless it is the driver, a running driver is first asked back
 * into the IPL with command A, once it has taken the last sound command.
 * HOW: The IPL protocol: ports 0-1 read $BBAA when it waits; each block's
 * address goes to ports 2-3 and a nonzero port 1 says data follows, port 0
 * the counter; each byte goes to port 1 with the counter (0 up) to port 0,
 * the next sent once port 0 echoes the last; the next block's counter is
 * the last plus 3 (4 with the carry), never 0. The last block's address
 * goes with port 1 = 0. Then it waits for the new driver to report 1 on
 * port 2, the sound sequence number's start. X and Y are 16-bit while it
 * works; Y crossing $8000 steps the list's bank.
 * Entry: M 8-bit, X 8-bit; A = the command; X = the list's index; D a
 * page whose $0C-$0E and $34 it may use; DB a bank whose $0000-$1FFF is
 * work RAM and whose $2140-$2143 are the APU ports; called by JSL. Exit:
 * A low = 1, A high as the transfer leaves it; X = the last block's
 * length's low byte (0), Y the list position's low byte; P as at entry.
 */
void abi_SpcUpload(void)
{
    uint8_t p0, a, pushed;
    uint16_t x;

    wr8(dp(DP_COMMAND), (uint8_t)cpu.a);
    p0 = cpu.p;
    op_rep(P_M | P_X);
    cpu.a = 0x8000;
    wr16(dp(DP_LIST), 0x8000);
    cpu.a = (uint16_t)(rd16(SPC_LISTS + cpu.x) & 0x7FFF);
    cpu.y = cpu.a;
    op_sep(P_M);
    a = (uint8_t)(rd8(SPC_LISTS + 2 + cpu.x) | 0x80);
    wr8(dp(DP_LIST + 2), a);
    set_a8((uint8_t)cpu.x);
    if ((uint8_t)cpu.x != 0) {
        a = rd8(SOUND_SEQ);
        set_a8(a);
        do
            cpu.p = cmp8_flags(cpu.p, a, rd8(APUIO2));
        while (!(cpu.p & P_Z));
        goto send;
    }
    for (;;) {
        op_rep(P_M);
        cpu.a = IPL_READY;
        op_cmp16(rd16(APUIO0));
        op_sep(P_M);
        if (cpu.p & P_Z)
            break;
send:
        set_a8(rd8(dp(DP_COMMAND)));
        wr8(APUIO0, (uint8_t)cpu.a);
    }
    set_a8(IPL_START);
    for (;;) {
        /* A block's header: B539 */
        pushed = (uint8_t)cpu.a;
        op_rep(P_M);
        cpu.a = list16();
        cpu.y = (uint16_t)(cpu.y + 2);
        x = cpu.a;
        cpu.a = list16();
        cpu.y = (uint16_t)(cpu.y + 2);
        wr16(APUIO2, cpu.a);
        op_sep(P_M);
        cpu.p = (uint8_t)((nz16(cpu.p, (uint16_t)(x - 1)) & ~P_C) | (x >= 1 ? P_C : 0));
        a = (uint8_t)(cpu.p & P_C);
        cpu.p &= (uint8_t)~P_C;
        cpu.p = nz8(cpu.p, a);
        set_a8(a);
        wr8(APUIO1, a);
        op_adc8(a, 0x7F, &cpu.p);
        set_a8(pushed);
        cpu.p = nz8(cpu.p, pushed);
        wr8(APUIO0, pushed);
        await_port0();
        if (!(cpu.p & P_V))
            break;
        /* The block's first byte: B50C */
        set_a8(list8());
        cpu.y++;
        op_xba();
        set_a8(0);
        cpu.p = nz8(cpu.p, 0);
        for (;;) {
            op_rep(P_M);
            wr16(APUIO0, cpu.a);
            op_sep(P_M);
            x--;
            if (x == 0)
                break;
            /* The next byte: B514 */
            op_xba();
            set_a8(list8());
            cpu.y++;
            if (cpu.y & 0x8000) {
                wr8(dp(DP_LIST + 2), (uint8_t)(rd8_rmw(dp(DP_LIST + 2)) + 1));
                cpu.y = 0;
            }
            op_xba();
            await_port0();
            set_a8((uint8_t)(cpu.a + 1));
            cpu.p = nz8(cpu.p, (uint8_t)cpu.a);
        }
        cpu.x = x;
        cpu.p = nz16(cpu.p, x);
        await_port0();
        do {
            a = op_adc8((uint8_t)cpu.a, 3, &cpu.p);
            set_a8(a);
        } while (a == 0);
    }
    cpu.x = x;
    op_sep(P_M | P_X);
    set_a8(1);
    cpu.p = nz8(cpu.p, 1);
    do
        cpu.p = cmp8_flags(cpu.p, 1, rd8(APUIO2));
    while (!(cpu.p & P_Z));
    wr8(SOUND_SEQ, 1);
    op_plp(p0);
}

/*
 * @routine 80:B4AA SoundSetsLoad
 * WHAT IT DOES: Puts the sound driver and both sample sets into the
 * SPC700 the first time it is called after power-on, and restarts the
 * sound command sequence.
 * HOW: Once marked, SpcUpload of lists 0, 3 and 6 with command $F9; the
 * sequence number then starts at 1 either way.
 * Entry: M 8-bit, X 8-bit; D a page whose $0C-$0E and $34 SpcUpload may
 * use; DB a bank whose $0000-$1FFF is work RAM and whose $2140-$2143 are
 * the APU ports; called by JSL. Exit: A low = 1, N and Z clear. Loaded:
 * X, Y and A high as the last SpcUpload leaves them. The other flags
 * unchanged.
 */
void abi_SoundSetsLoad(void)
{
    static const uint8_t lists[3] = { 0, 3, 6 };
    uint8_t a = rd8(SOUND_SETS_LOADED);
    int i;

    cpu.a = (uint16_t)((cpu.a & 0xFF00) | a);
    cpu.p = nz8(cpu.p, a);
    if (a == 0) {
        a = (uint8_t)(rd8_rmw(SOUND_SETS_LOADED) + 1);
        wr8(SOUND_SETS_LOADED, a);
        for (i = 0; i < 3; i++) {
            cpu.x = lists[i];
            cpu.a = (uint16_t)((cpu.a & 0xFF00) | DRIVER_TO_IPL);
            abi_SpcUpload();
        }
    }
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 1);
    cpu.p = nz8(cpu.p, 1);
    wr8(SOUND_SEQ, 1);
}

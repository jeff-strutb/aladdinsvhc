/*
 * snesbox: run the original ROM headless and record what it executes.
 *
 *   snesbox --rom ROM --script FILE --out DIR [--core DYLIB] [--trace]
 *          [--check LIB | --lockstep LIB] [--only ADDR,ADDR...] [--rows FILE]
 *
 * Script, one command per line ('#' starts a comment):
 *   wait N               N frames, no buttons
 *   hold B[+B...] N      hold buttons for N frames
 *   press B[+B...] [N]   hold for N frames (default 2), then 2 frames released
 *   shot NAME            write the current frame to DIR/NAME.ppm
 *   dump NAME            write WRAM to DIR/NAME.wram
 *   save NAME            write a state to DIR/NAME.state
 *   load PATH            read a state
 *   poke ADDR BYTE[,BYTE...]
 *                        write bytes from S-CPU address ADDR (hex, 24-bit) into
 *                        work RAM ($7E-$7F, or the low 8K mirror in banks
 *                        $00-$3F), between frames;
 *                        for scripts that start from save data the game itself
 *                        could hold (see each script's header)
 *   set ADDR BYTE[,BYTE...]
 *                        as poke, but written when the next call starts, so
 *                        the game itself never runs on those bytes
 *   call ADDR [long] [keep] [noreturn] [a=HEX] [x=] [y=] [p=] [db=] [d=] [frames=N]
 *        [press=B[+B...]@F ...] [frame=PPDD] [php] [phb] [rti=PP] [vblank]
 *                        run the routine at ADDR once as a unit test: entered
 *                        by a JSR (by a JSL with long) from a stub in work RAM
 *                        with those registers, the game's state as the script
 *                        left it; then the machine is put back as it was
 *                        before the call (with keep, the game goes on from
 *                        where it was, the call's changes kept); with
 *                        noreturn, for a path that stops the game, the
 *                        frames run and the machine stays as it left it;
 *                        each press (up to 8) holds those buttons in
 *                        frames F and F+1 of the call (counted from 0), for
 *                        a loop that waits for the player; frame (with
 *                        long) enters a routine's inside the way its own
 *                        start leaves the stack: a JSL to a trampoline
 *                        that pushes P = PP and DB = DD above the return
 *                        address (PEA) and jumps (JML) to ADDR, for code
 *                        that ends by pulling them (PLB, PLP, RTL); frame
 *                        without long is the same with a JSR and a JMP,
 *                        for code that ends PLB, PLP, RTS; php
 *                        (without long) is the same for code that ends
 *                        PLP, RTS: a JSR to a trampoline that pushes P
 *                        (PHP, so P = p=) and jumps (JMP) to ADDR; phb
 *                        (with long) the same for code that ends PLB,
 *                        RTL: a JSL to a trampoline that pushes the data
 *                        bank (PHB, so DB = db=) and jumps (JML); rti
 *                        enters ADDR as an interrupt does, for an interrupt
 *                        handler: a stub in bank 00 pushes the frame an
 *                        interrupt pushes (PB and PC of its own loop, then
 *                        P = PP) and jumps (JML) to ADDR, whose RTI lands
 *                        on the loop; vblank
 *                        lets the call start in the vertical blank, for a
 *                        screen that runs with the NMI off and whose loop
 *                        runs original code only then
 *   repeat N ... end     run the enclosed lines N times (nests)
 * Buttons: B Y SELECT START UP DOWN LEFT RIGHT A X L R, player 1; prefix
 * with 2. for player 2 (2.START). Joined with +.
 *
 * With --trace, DIR receives rommap.bin (per ROM byte: who touched it and
 * how), wram_exec.bin, wramcode.bin and edges.csv (control transfers).
 *
 * With --check, every call of a routine that LIB (the hybrid build of the C
 * decomp) implements is checked against its C; see check.c. The report goes
 * to DIR/check.txt. With --lockstep, the C replaces those routines in a
 * second copy of the game run in step with the original; see check.c. The
 * report goes to DIR/lockstep.txt.
 */
#include <dlfcn.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libretro.h"
#include "check.h"
#include "spccall.h"

static void die(const char *fmt, ...) {
    va_list ap; va_start(ap, fmt); vfprintf(stderr, fmt, ap); va_end(ap);
    fputc('\n', stderr); exit(1);
}

static void *core;
void snesbox_trace_begin(void);
void snesbox_trace_break(void);
int snesbox_trace_write(const char *dir);
uint8_t *snesbox_memory(const char *name, size_t *size);
#define SYM(name) static __typeof__(&name) name##_p;
SYM(retro_init) SYM(retro_set_environment) SYM(retro_set_video_refresh) SYM(retro_set_audio_sample)
SYM(retro_set_audio_sample_batch) SYM(retro_set_input_poll) SYM(retro_set_input_state) SYM(retro_load_game)
SYM(retro_run) SYM(retro_serialize_size) SYM(retro_serialize) SYM(retro_unserialize)
SYM(snesbox_trace_begin) SYM(snesbox_trace_write) SYM(snesbox_memory) SYM(snesbox_trace_break)
/* The trace takes no edge across a state loaded here (a load, or a unit
   call's machine put back): the next instruction was not reached from the
   last. A unit call's register jump needs none: the edges it makes lead to
   and from the stub in work RAM, which is no ROM routine. */
static void trace_break(void) { if (snesbox_trace_break_p) snesbox_trace_break_p(); }
/* The core's running cooperative thread (each processor has one): the
   cycle log keeps the S-CPU's probes apart by it (check.c). */
void *(*snesbox_co_active)(void);
/* After a state is loaded: the trace and the SPC700 checks start again
   from it. */
static void state_loaded(void)
{
    trace_break();
    spc_state_loaded();
}

static const char *outdir;
static char sysdir[4096];
static unsigned pixfmt = RETRO_PIXEL_FORMAT_0RGB1555;
static uint32_t frame[512 * 512];
static unsigned fw, fh;
static unsigned buttons;
static long frames;
/* SNESBOX_SPCLOG=FILE: each SPC700 read and write of $F0-$FF, as
   nativeshvc's NSHVC_SPCLOG gives the native console's: 8 bytes, its time
   in SPC700 half cycles (the core's SMP clocks, 2 a cycle; little endian,
   32 bits), 1 write or 0 read, the address (16 bits), the value. */
struct SpcLogCb {
    void (*insn)(uint16_t pc, uint16_t prev_end, uint8_t opcode);
    void (*mem)(int write, uint16_t addr, uint8_t data);
    void (*step)(uint32_t clocks);
};
static FILE *spclog_f;
static uint64_t spclog_t;
static void spclog_mem(int write, uint16_t addr, uint8_t data)
{
    if ((addr & 0xFFF0) != 0x00F0) return;
    uint32_t t = (uint32_t)spclog_t;
    uint8_t rec[8] = { (uint8_t)t, (uint8_t)(t >> 8), (uint8_t)(t >> 16), (uint8_t)(t >> 24),
                       (uint8_t)write, (uint8_t)addr, (uint8_t)(addr >> 8), data };
    fwrite(rec, 1, 8, spclog_f);
}
static void spclog_step(uint32_t clocks) { spclog_t += clocks & 0x7FFFFFFFu; }   /* SNESBOX_SMP_IDLE off */

static bool env(unsigned cmd, void *data) {
    switch (cmd) {
    case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY:
    case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY:
        *(const char **)data = sysdir; return true;
    case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT:
        pixfmt = *(const unsigned *)data; return true;
    case RETRO_ENVIRONMENT_GET_CAN_DUPE:
        *(bool *)data = true; return true;
    case RETRO_ENVIRONMENT_GET_VARIABLE: {
        /* The core's speed shortcuts are all off: scanline PPU, fast DSP and
           delayed coprocessor sync trade accuracy for speed. No overclock. */
        struct retro_variable *v = data;
        static const char *fixed[][2] = {
            {"bsnes_ppu_fast", "OFF"}, {"bsnes_dsp_fast", "OFF"}, {"bsnes_coprocessor_delayed_sync", "OFF"},
            {"bsnes_cpu_overclock", "100"}, {"bsnes_cpu_sfx_overclock", "100"}, {"bsnes_cpu_sa1_overclock", "100"},
        };
        for (unsigned i = 0; i < sizeof fixed / sizeof fixed[0]; i++)
            if (!strcmp(v->key, fixed[i][0])) { v->value = fixed[i][1]; return true; }
        return false;
    }
    default:
        return false;
    }
}

/* --rows FILE: each frame's first and last line with any pixel not black
   (-1 -1 for a frame all black), as "row FRAME FIRST LAST": where the
   picture's forced blank and brightness leave it shown, for comparing the
   native port's raster timing with the original's */
static FILE *rows_f;
static void video(const void *data, unsigned w, unsigned h, size_t pitch) {
    if (!data) return;
    if (w > 512) w = 512;
    if (h > 512) h = 512;
    fw = w; fh = h;
    for (unsigned y = 0; y < h; y++) {
        const uint8_t *row = (const uint8_t *)data + y * pitch;
        for (unsigned x = 0; x < w; x++) {
            uint32_t c;
            if (pixfmt == RETRO_PIXEL_FORMAT_XRGB8888) c = ((const uint32_t *)row)[x];
            else {
                uint16_t p = ((const uint16_t *)row)[x];
                if (pixfmt == RETRO_PIXEL_FORMAT_RGB565)
                    c = ((p >> 11 & 31) << 19) | ((p >> 5 & 63) << 10) | ((p & 31) << 3);
                else
                    c = ((p >> 10 & 31) << 19) | ((p >> 5 & 31) << 11) | ((p & 31) << 3);
            }
            frame[y * 512 + x] = c;
        }
    }
    if (rows_f) {
        int first = -1, last = -1;
        for (unsigned y = 0; y < h; y++)
            for (unsigned x = 0; x < w; x++)
                if (frame[y * 512 + x] & 0xFFFFFF) {
                    if (first < 0) first = (int)y;
                    last = (int)y;
                    break;
                }
        fprintf(rows_f, "row %ld %d %d\n", frames, first, last);
    }
}
/* --sound FILE: the DSP's output from power-on as the chip makes it (the
   core's snesbox_dsp_sample, before its resampler), 16-bit stereo, left
   first, 32,040 frames a second; the native port's --sound writes the same */
static FILE *sound_f;
/* --apulog FILE: what the DSP is given, for the native mixer's replay
   check: the APU RAM at power-on (65,536 bytes), then each write the
   SPC700 makes, 8 bytes (little-endian): the DSP samples made before it
   (32 bits), the DSP's cycle within the sample it is making (0-31), the
   address (16 bits), the byte */
static FILE *apulog_f;
static uint32_t dsp_samples;
static void dsp_sample(int16_t l, int16_t r)
{
    int16_t p[2] = { l, r };
    if (sound_f) fwrite(p, 2, 2, sound_f);
    dsp_samples++;
}
/* --voices FILE: with each DSP sample, each voice's state (harness patch
   0013: 8 x 5 32-bit values, envelope level and mode, interpolation
   position, BRR block address and offset) */
static FILE *voices_f;
static void dsp_voices(const int32_t *st) { fwrite(st, 4, 40, voices_f); }
static void apu_write(uint16_t address, uint8_t data, int phase)
{
    uint8_t rec[8] = { (uint8_t)dsp_samples, (uint8_t)(dsp_samples >> 8), (uint8_t)(dsp_samples >> 16),
                       (uint8_t)(dsp_samples >> 24), (uint8_t)phase, (uint8_t)address, (uint8_t)(address >> 8), data };
    fwrite(rec, 1, 8, apulog_f);
}
static void audio(int16_t l, int16_t r) { (void)l; (void)r; }
static size_t audio_batch(const int16_t *d, size_t n) { (void)d; return n; }
static void poll(void) {}
static int16_t input(unsigned port, unsigned device, unsigned index, unsigned id) {
    (void)index;
    if (port > 1 || device != RETRO_DEVICE_JOYPAD) return 0;
    unsigned b = buttons >> (port * 16) & 0xffff;
    if (id == RETRO_DEVICE_ID_JOYPAD_MASK) return (int16_t)b;
    return (b >> id) & 1;
}

static int lockstep;

/* --cyclog FILE [--cyclog-frames A-B]: each S-CPU cycle of the original,
   one line: "o"/"n"/"r" ADDR VALUE CLOCKS for an opcode fetch, an operand
   fetch or a data read, "w" ADDR VALUE CLOCKS for a write, "i" CLOCKS for
   an internal cycle, "x" where an interrupt starts; "F n" at frame n's
   start (frames counted from 1). DMA and refresh time are not CPU cycles
   and are not logged. For giving the decomp's timing its cycle edges. */
static FILE *cyclog_f;
static long cyclog_lo = 1, cyclog_hi = 0x7FFFFFFF;
static int *cyclog_who, cyclog_on;
static uint32_t cyclog_addr; static uint8_t cyclog_data;
static void cyclog_read(uint32_t addr, uint8_t data) { cyclog_addr = addr; cyclog_data = data; }
static void cyclog_write(uint32_t addr, uint8_t data, int kind) { (void)kind; cyclog_addr = addr; cyclog_data = data; }
static void cyclog_timing(int kind, int clocks, uint32_t addr)
{
    if (!cyclog_on) return;
    if (kind == 1) {
        int who = cyclog_who ? *cyclog_who : 0;
        fprintf(cyclog_f, "%c %06X %02X %d\n", who == 1 ? 'o' : who == 2 ? 'n' : 'r', addr, cyclog_data, clocks);
    } else if (kind == 2)
        fprintf(cyclog_f, "w %06X %02X %d\n", addr, cyclog_data, clocks);
    else if (kind == 3)
        fprintf(cyclog_f, "i %d\n", clocks);
    else if (kind == 4)
        fputs("x\n", cyclog_f);
}

static void run(long n)
{
    while (n-- > 0) {
        if (cyclog_f) {
            cyclog_on = frames + 1 >= cyclog_lo && frames + 1 <= cyclog_hi;
            if (cyclog_on) fprintf(cyclog_f, "F %ld\n", frames + 1);
        }
        if (lockstep) lockstep_before_frame(frames);
        retro_run_p();
        if (lockstep) lockstep_after_frame(frames);
        frames++;
    }
}

static unsigned parse_buttons(const char *s) {
    static const char *names[] = {"B", "Y", "SELECT", "START", "UP", "DOWN", "LEFT", "RIGHT", "A", "X", "L", "R"};
    unsigned m = 0;
    char buf[256]; snprintf(buf, sizeof buf, "%s", s);
    char *save = NULL;
    for (char *t = strtok_r(buf, "+", &save); t; t = strtok_r(NULL, "+", &save)) {
        unsigned i, shift = 0;
        if (!strncmp(t, "2.", 2)) { t += 2; shift = 16; }
        for (i = 0; i < 12 && strcmp(t, names[i]); i++) {}
        if (i == 12) die("unknown button: %s", t);
        m |= 1u << (i + shift);
    }
    return m;
}

static void write_file(const char *name, const char *ext, const void *p, size_t n) {
    if (lockstep && lockstep_role() == 2) return;  /* the original writes the files */
    char path[4096]; snprintf(path, sizeof path, "%s/%s%s", outdir, name, ext);
    FILE *f = fopen(path, "wb"); if (!f) die("cannot write %s", path);
    fwrite(p, 1, n, f); fclose(f);
}

static void shot(const char *name) {
    if (lockstep && lockstep_role() == 2) return;
    char path[4096]; snprintf(path, sizeof path, "%s/%s.ppm", outdir, name);
    FILE *f = fopen(path, "wb"); if (!f) die("cannot write %s", path);
    fprintf(f, "P6\n%u %u\n255\n", fw, fh);
    for (unsigned y = 0; y < fh; y++)
        for (unsigned x = 0; x < fw; x++) {
            uint32_t c = frame[y * 512 + x];
            fputc(c >> 16 & 255, f); fputc(c >> 8 & 255, f); fputc(c & 255, f);
        }
    fclose(f);
}

static void *load_file(const char *path, size_t *n) {
    FILE *f = fopen(path, "rb"); if (!f) die("cannot read %s", path);
    fseek(f, 0, SEEK_END); *n = (size_t)ftell(f); fseek(f, 0, SEEK_SET);
    void *p = malloc(*n); if (fread(p, 1, *n, f) != *n) die("short read %s", path);
    fclose(f); return p;
}

static char lines[20000][256];
static int nlines;
static const char *script_path;

/* poke ADDR BYTE[,BYTE...]: both lockstep processes run the script, so a
   poke lands the same way on each side. */
static void poke(const char *line, int lineno) {
    char addr[64] = "", bytes[1024] = "";
    if (sscanf(line, "%*s %63s %1023s", addr, bytes) != 2) die("%s:%d: poke ADDR BYTE[,BYTE...]", script_path, lineno);
    unsigned long a = strtoul(addr[0] == '$' ? addr + 1 : addr, NULL, 16);
    unsigned bank = a >> 16 & 0xFF, off = a & 0xFFFF;
    const char *view = NULL; size_t base = 0;
    if (bank == 0x7E || bank == 0x7F) { view = "wram"; base = (size_t)(bank - 0x7E) << 16 | off; }
    else if (bank <= 0x3F && off < 0x2000) { view = "wram"; base = off; }
    else die("%s:%d: poke: $%06lX is not work RAM", script_path, lineno, a);
    size_t n; uint8_t *p = snesbox_memory_p(view, &n);
    if (!p) die("%s:%d: poke: no %s view", script_path, lineno, view);
    for (char *s = bytes, *e; *s; s = *e ? e + 1 : e) {
        unsigned long v = strtoul(s, &e, 16);
        if (e == s || v > 0xFF || (*e && *e != ',')) die("%s:%d: poke: bad byte list", script_path, lineno);
        if (base >= n) die("%s:%d: poke: past the end of %s", script_path, lineno, view);
        p[base++] = (uint8_t)v;
    }
}

/* spcset APU BYTE[,BYTE...]: stage SPC700 RAM bytes for the next spccall.
   spccall APU [a=..] [x=..] [y=..] [psw=..] [at=APU]: call the SPC700
   routine at APU the next time the SPC700 reaches the instruction at `at`
   (default $0897, the drivers' timer wait), with the staged bytes in RAM
   and the registers given (the rest as they are); it returns to where the
   SPC700 was (spccall.c). */
static int spc_unit_ok = -1;
static void spc_need(int lineno)
{
    if (spc_unit_ok < 0) spc_unit_ok = spc_unit_init(core);
    if (!spc_unit_ok) die("%s:%d: the core has no SPC700 hooks (patch 0006)", script_path, lineno);
}

static void spcset(const char *line, int lineno) {
    char addr[64] = "", bytes[1024] = "";
    if (sscanf(line, "%*s %63s %1023s", addr, bytes) != 2) die("%s:%d: spcset APU BYTE[,BYTE...]", script_path, lineno);
    spc_need(lineno);
    unsigned long a = strtoul(addr[0] == '$' ? addr + 1 : addr, NULL, 16);
    for (char *s = bytes, *e; *s; s = *e ? e + 1 : e) {
        unsigned long v = strtoul(s, &e, 16);
        if (e == s || v > 0xFF || (*e && *e != ',') || a > 0xFFFF) die("%s:%d: spcset: bad byte list", script_path, lineno);
        spc_unit_set((uint16_t)a++, (uint8_t)v);
    }
}

static void spccall(const char *line, int lineno) {
    char tok[16][64]; int k = 0;
    const char *s = line;
    while (k < 16 && sscanf(s, "%63s", tok[k]) == 1) { s = strstr(s, tok[k]) + strlen(tok[k]); k++; }
    if (k < 2) die("%s:%d: spccall APU [a= x= y= psw= at=] [noreturn]", script_path, lineno);
    spc_need(lineno);
    if (spc_unit_busy()) die("%s:%d: spccall: the previous one has not run yet", script_path, lineno);
    unsigned long target = strtoul(tok[1][0] == '$' ? tok[1] + 1 : tok[1], NULL, 16), at = 0x0897;
    int has[4] = {0, 0, 0, 0}; uint8_t reg[4] = {0, 0, 0, 0};
    for (int i = 2; i < k; i++) {
        /* noreturn: a routine that never returns (the driver's start); for
           t3.py, which judges its C by the frames after it. */
        if (!strcmp(tok[i], "noreturn")) continue;
        char *eq = strchr(tok[i], '=');
        if (!eq) die("%s:%d: spccall: bad option %s", script_path, lineno, tok[i]);
        unsigned long v = strtoul(eq + 1, NULL, 16);
        if (!strncmp(tok[i], "a=", 2)) { has[0] = 1; reg[0] = (uint8_t)v; }
        else if (!strncmp(tok[i], "x=", 2)) { has[1] = 1; reg[1] = (uint8_t)v; }
        else if (!strncmp(tok[i], "y=", 2)) { has[2] = 1; reg[2] = (uint8_t)v; }
        else if (!strncmp(tok[i], "psw=", 4)) { has[3] = 1; reg[3] = (uint8_t)v; }
        else if (!strncmp(tok[i], "at=", 3)) at = v;
        else die("%s:%d: spccall: bad option %s", script_path, lineno, tok[i]);
    }
    spc_unit_call((uint16_t)target, (uint16_t)at, has, reg);
}

/* call: run one routine as a unit test, from the state the script has
   reached. At the next instruction boundary the CPU's registers are taken
   and it runs a stub in low work RAM ($1FE0; $7F:FFE0 for bank $7F, out of
   the code the game copies to $7F:0800): JSR (then NOP) or JSL to the
   routine, then at $1FE4 a BRA to itself. The routine is entered by
   a real JSR/JSL, so --check and --lockstep take it as a call like any
   other. When it returns, the whole machine is put back as it was before
   the call (a state taken just before it), so the game goes on as if the
   call had not been made: nothing it relies on (the PPU multiplier's
   factor, work RAM) is left changed, and the frames the call ran are not
   kept; the coverage and the checks of the call are.
   It starts only outside the vertical blank ($4212 bit 7). */
struct SnesboxRegs { uint16_t a, x, y, s, d; uint8_t db, p, e; uint32_t pc; };
static void (*snesbox_core_regs_p)(struct SnesboxRegs *);
static void (*snesbox_core_set_regs_p)(const struct SnesboxRegs *);
static uint8_t (*snesbox_core_bus_read_p)(uint32_t);
static int (**cpu_exec_hook)(uint32_t);
static int (*exec_before)(uint32_t);
static struct SnesboxRegs call_from, call_regs;
static uint32_t call_stub, call_loop;
static int call_state;   /* 1 to start at the next boundary, 2 running, 0 done */
static int checklib_on;  /* --check: the checker is told when a call starts */
static int call_keep;    /* keep: the machine is not put back after the call */
static long call_frame = -1;  /* frame=: the P and DB pushed above the return */
static long call_rti = -1;    /* rti=: entered as an interrupt, this P pushed */
static int call_frame_long;   /* frame= with long (JSL), else with a JSR */
static int call_php;           /* php: (JSR) the entry P pushed above the return */
static int call_phb;           /* phb: (JSL) the entry DB pushed above the return */
static int call_vblank;        /* vblank: may start in the vertical blank (NMI off) */
static int call_noreturn;     /* noreturn: told to lockstep when the call starts */
static uint32_t call_target;

/* set: bytes staged for the next call, written when it starts (after the
   state it is put back to was taken), so the game never runs on them. */
static char staged[64][1024];
static int staged_line[64], nstaged;

static int call_exec(uint32_t pc)
{
    /* Not in the vertical blank, where the NMI would land between the JSR and the
       routine's first instruction (a script line runs at a frame's end).
       Under --lockstep, not where the original and the hybrid differ. */
    /* SNESBOX_CALL_WHY=ADDR: while a unit call waits to start, the
       conditions at each instruction in ADDR's 16-byte line. */
    if (call_state == 1 && getenv("SNESBOX_CALL_WHY") && (pc & 0xFFFFF0) == strtoul(getenv("SNESBOX_CALL_WHY"), NULL, 16))
        fprintf(stderr, "why side %d pc %06X ok %d 3030 %02X 4212 %02X clock %u\n", lockstep ? lockstep_role() : 0, pc,
                lockstep_call_ok(pc), 0, snesbox_core_bus_read_p(0x004212), lockstep_clock());
    /* With vblank, also in the vertical blank: for a screen that runs with
       the NMI off (no NMI can land between the JSR and the routine) and
       whose loop runs original code only then (the rest of it is C). */
    if (call_state == 1 && lockstep_call_ok(pc)
        && (call_vblank || !(snesbox_core_bus_read_p(0x004212) & 0x80))) {
        snesbox_core_regs_p(&call_from);
        if (getenv("SNESBOX_CALL_DEBUG"))
            fprintf(stderr, "call starts (side %d) frame %ld at %06X S=%04X clock %u\n", lockstep ? lockstep_role() : 0, frames, call_from.pc, call_from.s, lockstep_clock());
        if (getenv("SNESBOX_CALL_DEBUG") && lockstep) lockstep_debug_state();
        if (checklib_on) check_unit_call(call_from.pc, call_from.s);
        /* Entered by a JML from the trampoline: the checker is told where
           the call starts (S after the return and the frame) and ends. */
        if (call_noreturn && lockstep) lockstep_unit_noreturn(call_target);
        if (call_frame >= 0 && call_frame_long) {
            if (checklib_on) check_unit_entry(call_target, call_loop, call_from.s, (uint16_t)(call_from.s - 5));
            if (lockstep) lockstep_unit_entry(call_target, call_loop, call_from.s, (uint16_t)(call_from.s - 5));
        }
        if (call_frame >= 0 && !call_frame_long) {
            if (checklib_on) check_unit_entry(call_target, call_stub + 3, call_from.s, (uint16_t)(call_from.s - 4));
            if (lockstep) lockstep_unit_entry(call_target, call_stub + 3, call_from.s, (uint16_t)(call_from.s - 4));
        }
        if (call_rti >= 0) {
            if (checklib_on) check_unit_entry(call_target, call_loop, call_from.s, (uint16_t)(call_from.s - 4));
            if (lockstep) lockstep_unit_entry(call_target, call_loop, call_from.s, (uint16_t)(call_from.s - 4));
        }
        if (call_phb) {
            if (checklib_on) check_unit_entry(call_target, call_loop, call_from.s, (uint16_t)(call_from.s - 4));
            if (lockstep) lockstep_unit_entry(call_target, call_loop, call_from.s, (uint16_t)(call_from.s - 4));
        }
        if (call_php) {
            if (checklib_on) check_unit_entry(call_target, call_stub + 3, call_from.s, (uint16_t)(call_from.s - 3));
            if (lockstep) lockstep_unit_entry(call_target, call_stub + 3, call_from.s, (uint16_t)(call_from.s - 3));
        }
        for (int i = 0; i < nstaged; i++) poke(staged[i], staged_line[i]);
        struct SnesboxRegs r = call_regs;
        r.s = call_from.s; r.e = 0; r.pc = call_stub;
        snesbox_core_set_regs_p(&r);
        call_state = 2;
        return 1;
    }
    if (call_state == 2 && pc == call_loop) {
        call_state = 0;
        if (getenv("SNESBOX_CALL_DEBUG"))
            fprintf(stderr, "call ends (side %d) frame %ld clock %u\n", lockstep ? lockstep_role() : 0, frames, lockstep_clock());
        if (call_keep) { snesbox_core_set_regs_p(&call_from); return 1; }   /* the game goes on from where it was */
        return 0;   /* parked in the stub's loop until the machine is put back: see call() */
    }
    return exec_before ? exec_before(pc) : 0;
}

static void call(const char *line, int lineno) {
    char tok[16][64]; int k = sscanf(line, "%*s %63s %63s %63s %63s %63s %63s %63s %63s %63s %63s %63s %63s %63s %63s %63s %63s",
                                     tok[0], tok[1], tok[2], tok[3], tok[4], tok[5], tok[6], tok[7], tok[8], tok[9], tok[10], tok[11],
                                     tok[12], tok[13], tok[14], tok[15]);
    if (k < 1) die("%s:%d: call ADDR [long] [keep] [noreturn] [a=] [x=] [y=] [p=] [db=] [d=] [frames=] [press=B@F]", script_path, lineno);
    char ab[64]; int j = 0;
    for (const char *c = tok[0]; *c; c++) if (*c != '$' && *c != ':') ab[j++] = *c;
    ab[j] = 0;
    uint32_t addr = (uint32_t)strtoul(ab, NULL, 16), bank = addr >> 16;
    int lng = 0, noreturn = 0, npress = 0; long frames_max = 600, press_at[8];
    unsigned press_mask[8];
    call_keep = 0;
    struct SnesboxRegs cur; snesbox_core_regs_p(&cur);
    call_regs = cur;
    for (int i = 1; i < k; i++) {
        unsigned long v = strtoul(strchr(tok[i], '=') ? strchr(tok[i], '=') + 1 : "0", NULL, 16);
        if (!strcmp(tok[i], "long")) lng = 1;
        else if (!strcmp(tok[i], "noreturn")) noreturn = 1;
        else if (!strcmp(tok[i], "keep")) call_keep = 1;
        else if (!strncmp(tok[i], "a=", 2)) call_regs.a = (uint16_t)v;
        else if (!strncmp(tok[i], "x=", 2)) call_regs.x = (uint16_t)v;
        else if (!strncmp(tok[i], "y=", 2)) call_regs.y = (uint16_t)v;
        else if (!strncmp(tok[i], "p=", 2)) call_regs.p = (uint8_t)v;
        else if (!strncmp(tok[i], "db=", 3)) call_regs.db = (uint8_t)v;
        else if (!strncmp(tok[i], "d=", 2)) call_regs.d = (uint16_t)v;
        else if (!strncmp(tok[i], "frames=", 7)) frames_max = strtol(tok[i] + 7, NULL, 10);
        else if (!strncmp(tok[i], "frame=", 6)) call_frame = (long)(v & 0xFFFF);
        else if (!strncmp(tok[i], "rti=", 4)) call_rti = (long)(v & 0xFF);
        else if (!strcmp(tok[i], "php")) call_php = 1;
        else if (!strcmp(tok[i], "phb")) call_phb = 1;
        else if (!strcmp(tok[i], "vblank")) call_vblank = 1;
        else if (!strncmp(tok[i], "press=", 6) && strchr(tok[i], '@') && npress < 8) {
            char pb[64]; snprintf(pb, sizeof pb, "%s", tok[i] + 6);
            *strchr(pb, '@') = 0;
            press_mask[npress] = parse_buttons(pb);
            press_at[npress++] = strtol(strchr(tok[i], '@') + 1, NULL, 10);
        }
        else die("%s:%d: call: bad option %s", script_path, lineno, tok[i]);
    }
    if (call_regs.p & 0x10) { call_regs.x &= 0x00FF; call_regs.y &= 0x00FF; }  /* 8-bit index */
    size_t n; uint8_t *w = snesbox_memory_p("wram", &n);
    size_t at = 0; uint32_t sb = 0;
    if (lng || call_rti >= 0) { at = 0x1FE0; sb = 0x00; }
    else if (bank == 0x7F) { at = 0x1FFE0; sb = 0x7F; }   /* $7F:FFE0: outside the code copied to $7F:0800 */
    else if (bank == 0x7E || bank < 0x40 || (bank >= 0x80 && bank < 0xC0)) { at = 0x1FE0; sb = bank; }
    else die("%s:%d: call: a JSR can't reach bank %02X from work RAM; use long", script_path, lineno, bank);
    uint8_t stub[16]; int m = 0;
    if (call_php && lng) die("%s:%d: call: php is for a JSR call (frame= for long)", script_path, lineno);
    if (call_phb && !lng) die("%s:%d: call: phb is for a JSL call (long)", script_path, lineno);
    call_target = addr;
    call_noreturn = noreturn;
    call_frame_long = lng;
    if (call_rti >= 0) {
        /* The frame an interrupt pushes, the PB and PC of the loop below,
           then P: PEA #(bank << 8 | loop >> 8), PEA #(loop << 8 | P), JML
           ADDR. The RTI lands on the loop. */
        uint16_t l = (uint16_t)(at + 10);
        stub[m++] = 0xF4; stub[m++] = (uint8_t)(l >> 8); stub[m++] = (uint8_t)sb;
        stub[m++] = 0xF4; stub[m++] = (uint8_t)call_rti; stub[m++] = (uint8_t)l;
        stub[m++] = 0x5C; stub[m++] = (uint8_t)addr; stub[m++] = (uint8_t)(addr >> 8); stub[m++] = (uint8_t)bank;
    } else if ((call_frame >= 0 && lng) || call_phb) {
        /* JSL to the trampoline at +6; there PEA #frame (or PHB), JML ADDR. */
        uint16_t t = (uint16_t)(at + 6);
        stub[m++] = 0x22; stub[m++] = (uint8_t)t; stub[m++] = (uint8_t)(t >> 8); stub[m++] = (uint8_t)sb;
    } else if (call_frame >= 0) {
        /* JSR to the trampoline at +6; there PEA #frame, JMP ADDR (the stub
           runs in the routine's bank). The RTS lands on the NOP at +3. */
        uint16_t t = (uint16_t)(at + 6);
        stub[m++] = 0x20; stub[m++] = (uint8_t)t; stub[m++] = (uint8_t)(t >> 8); stub[m++] = 0xEA;
    } else if (call_php) {
        /* JSR to the trampoline at +6; there PHP, JMP ADDR (the stub runs
           in the routine's bank). The RTS lands on the NOP at +3. */
        uint16_t t = (uint16_t)(at + 6);
        stub[m++] = 0x20; stub[m++] = (uint8_t)t; stub[m++] = (uint8_t)(t >> 8); stub[m++] = 0xEA;
    } else if (lng) { stub[m++] = 0x22; stub[m++] = (uint8_t)addr; stub[m++] = (uint8_t)(addr >> 8); stub[m++] = (uint8_t)bank; }
    else { stub[m++] = 0x20; stub[m++] = (uint8_t)addr; stub[m++] = (uint8_t)(addr >> 8); stub[m++] = 0xEA; }
    stub[m++] = 0x80; stub[m++] = 0xFE;
    call_stub = sb << 16 | (uint32_t)(at & 0xFFFF);
    call_loop = call_stub + (uint32_t)(m - 2);
    if (call_frame >= 0) {
        stub[m++] = 0xF4; stub[m++] = (uint8_t)call_frame; stub[m++] = (uint8_t)(call_frame >> 8);
        if (lng) { stub[m++] = 0x5C; stub[m++] = (uint8_t)addr; stub[m++] = (uint8_t)(addr >> 8); stub[m++] = (uint8_t)bank; }
        else { stub[m++] = 0x4C; stub[m++] = (uint8_t)addr; stub[m++] = (uint8_t)(addr >> 8); }
    }
    if (call_phb) {
        stub[m++] = 0x8B;
        stub[m++] = 0x5C; stub[m++] = (uint8_t)addr; stub[m++] = (uint8_t)(addr >> 8); stub[m++] = (uint8_t)bank;
    }
    if (call_php) {
        stub[m++] = 0x08;
        stub[m++] = 0x4C; stub[m++] = (uint8_t)addr; stub[m++] = (uint8_t)(addr >> 8);
    }
    memcpy(w + at, stub, (size_t)m);
    /* The machine as it was, put back when the call returns, so that the
       game goes on as if the call had not been made. */
    /* Not under --lockstep, which never puts the machine back (below): to
       serialize, bsnes first runs the machine to a point where every
       thread can be saved, and the hybrid's CPU can be inside a C call
       that spans the frame's end, so it would run that call on, off the
       record (4 frames in unit_fx's second call). */
    size_t sn = 0; void *snap = NULL;
    if (!lockstep) {
        sn = retro_serialize_size_p(); snap = malloc(sn);
        if (!snap || !retro_serialize_p(snap, sn)) die("%s:%d: call: serialize failed", script_path, lineno);
    }
    int open_calls = checklib_on ? check_depth() : 0;
    call_state = 1;
    if (getenv("SNESBOX_CALL_DEBUG"))
        fprintf(stderr, "call command (side %d) frame %ld clock %u\n", lockstep ? lockstep_role() : 0, frames, lockstep_clock());
    long f = 0;
    while (call_state && f < frames_max) {
        buttons = 0;
        for (int q = 0; q < npress; q++)
            if (f >= press_at[q] && f < press_at[q] + 2) buttons |= press_mask[q];
        run(1);
        f++;
    }
    buttons = 0;
    if (lockstep && !call_state) {
        for (long end = lockstep_unit_end(frames); frames < end; f++) run(1);
        /* One frame more on both sides: the original's last frame can end
           inside a later call, whose rest the hybrid would wait for in
           vain (and then compare nothing); this way the frame the unit
           call ended in is always compared. */
        run(1);
    }
    if (getenv("SNESBOX_CALL_DEBUG"))
        fprintf(stderr, "call command done (side %d) frame %ld clock %u\n", lockstep ? lockstep_role() : 0, frames, lockstep_clock());
    nstaged = 0;
    /* Under --lockstep the machine is not put back: the two sides' records
       of calls would go out of step. A lockstep script has a call as its
       last (t3.py runs a script of unit calls one call at a time), so the
       machine after it is never used for another. */
    if (!call_state && !call_keep && !lockstep) {
        if (!retro_unserialize_p(snap, sn)) die("%s:%d: call: unserialize failed", script_path, lineno);
        state_loaded();
        if (checklib_on) check_rollback(open_calls);
    }
    free(snap);
    if (call_state && !noreturn) die("%s:%d: call %06X did not return in %ld frames", script_path, lineno, addr, frames_max);
    call_state = 0;
    call_frame = -1;
    call_rti = -1;
    call_php = 0;
    call_phb = 0;
    call_vblank = 0;
}

static int block_end(int i) {  /* index of the matching end for a repeat at i */
    int depth = 0;
    for (int j = i + 1; j < nlines; j++) {
        char a[64] = "";
        sscanf(lines[j], "%63s", a);
        if (!strcmp(a, "repeat")) depth++;
        else if (!strcmp(a, "end")) { if (depth-- == 0) return j; }
    }
    die("%s:%d: repeat without end", script_path, i + 1);
    return -1;
}

static void exec_range(int from, int to) {
    for (int i = from; i < to; i++) {
        char a[64] = "", b[256] = "", c[64] = "";
        int k = sscanf(lines[i], "%63s %255s %63s", a, b, c);
        if (k <= 0) continue;
        if (!strcmp(a, "repeat") && k >= 2) {
            int e = block_end(i);
            for (long n = atol(b); n > 0; n--) exec_range(i + 1, e);
            i = e;
        }
        else if (!strcmp(a, "end")) die("%s:%d: end without repeat", script_path, i + 1);
        else if (!strcmp(a, "wait") && k >= 2) { buttons = 0; run(atol(b)); }
        else if (!strcmp(a, "hold") && k >= 3) { buttons = parse_buttons(b); run(atol(c)); buttons = 0; }
        else if (!strcmp(a, "press") && k >= 2) { buttons = parse_buttons(b); run(k >= 3 ? atol(c) : 2); buttons = 0; run(2); }
        else if (!strcmp(a, "shot") && k >= 2) shot(b);
        else if (!strcmp(a, "dump") && k >= 2) {
            size_t n; uint8_t *p = snesbox_memory_p("wram", &n); write_file(b, ".wram", p, n);
        }
        else if (!strcmp(a, "save") && k >= 2) {
            size_t n = retro_serialize_size_p(); void *p = malloc(n);
            if (!retro_serialize_p(p, n)) die("serialize failed");
            write_file(b, ".state", p, n); free(p);
        }
        else if (!strcmp(a, "poke") && k >= 3) poke(lines[i], i + 1);
        else if (!strcmp(a, "call") && k >= 2) call(lines[i], i + 1);
        else if (!strcmp(a, "spcset") && k >= 3) spcset(lines[i], i + 1);
        else if (!strcmp(a, "spccall") && k >= 2) spccall(lines[i], i + 1);
        else if (!strcmp(a, "set") && k >= 3) {
            if (nstaged == 64) die("%s:%d: set: more than 64 for one call", script_path, i + 1);
            snprintf(staged[nstaged], sizeof staged[0], "%s", lines[i]);
            staged_line[nstaged++] = i + 1;
        }
        else if (!strcmp(a, "load") && k >= 2) {
            size_t n; void *p = load_file(b, &n);
            if (lockstep) lockstep_before_load();
            if (!retro_unserialize_p(p, n)) die("unserialize failed: %s", b);
            state_loaded();
            /* Calls still open (one that never returns, as a stop screen)
               belong to the machine the load replaced: they never end. */
            if (checklib_on) check_rollback(0);
            if (lockstep) { lockstep_state_loaded(); spc_lockstep_state_loaded(); }
            /* The core takes the hook away while it runs original code
               for the checker (an interrupt handler, a routine with no C)
               and puts it back after; a load can leave such a run where it
               stopped, with the hook still away. */
            *cpu_exec_hook = call_exec;
            free(p);
        }
        else die("%s:%d: bad command", script_path, i + 1);
    }
}

static void script(const char *path) {
    FILE *f = fopen(path, "r"); if (!f) die("cannot read script %s", path);
    script_path = path;
    while (nlines < 20000 && fgets(lines[nlines], sizeof lines[0], f)) {
        char *h = strchr(lines[nlines], '#'); if (h) *h = 0;
        nlines++;
    }
    fclose(f);
    exec_range(0, nlines);
}

int main(int argc, char **argv) {
    const char *rom = NULL, *scr = NULL, *corepath = "tools/toolchains/bsnes/bsnes_libretro.dylib";
    int trace = 0;
    const char *checklib = NULL, *only = NULL, *locklib = NULL;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--rom") && i + 1 < argc) rom = argv[++i];
        else if (!strcmp(argv[i], "--script") && i + 1 < argc) scr = argv[++i];
        else if (!strcmp(argv[i], "--out") && i + 1 < argc) outdir = argv[++i];
        else if (!strcmp(argv[i], "--core") && i + 1 < argc) corepath = argv[++i];
        else if (!strcmp(argv[i], "--trace")) trace = 1;
        else if (!strcmp(argv[i], "--check") && i + 1 < argc) checklib = argv[++i];
        else if (!strcmp(argv[i], "--only") && i + 1 < argc) only = argv[++i];
        else if (!strcmp(argv[i], "--lockstep") && i + 1 < argc) locklib = argv[++i];
        else if (!strcmp(argv[i], "--voices") && i + 1 < argc) {
            if (!(voices_f = fopen(argv[++i], "wb"))) die("cannot write %s", argv[i]);
        }
        else if (!strcmp(argv[i], "--apulog") && i + 1 < argc) {
            if (!(apulog_f = fopen(argv[++i], "wb"))) die("cannot write %s", argv[i]);
        }
        else if (!strcmp(argv[i], "--sound") && i + 1 < argc) {
            if (!(sound_f = fopen(argv[++i], "wb"))) die("cannot write %s", argv[i]);
        }
        else if (!strcmp(argv[i], "--rows") && i + 1 < argc) {
            if (!(rows_f = fopen(argv[++i], "w"))) die("cannot write %s", argv[i]);
        }
        else if (!strcmp(argv[i], "--cyclog") && i + 1 < argc) {
            if (!(cyclog_f = fopen(argv[++i], "w"))) die("cannot write %s", argv[i]);
        }
        else if (!strcmp(argv[i], "--cyclog-frames") && i + 1 < argc) {
            if (sscanf(argv[++i], "%ld-%ld", &cyclog_lo, &cyclog_hi) != 2) die("--cyclog-frames A-B");
        }
        else die("usage: snesbox --rom ROM --script FILE --out DIR [--core DYLIB] [--trace]");
    }
    if (!rom || !scr || !outdir) die("usage: snesbox --rom ROM --script FILE --out DIR [--core DYLIB] [--trace]");
    snprintf(sysdir, sizeof sysdir, "%s", outdir);

    core = dlopen(corepath, RTLD_NOW | RTLD_LOCAL);
    if (!core) die("dlopen %s: %s", corepath, dlerror());
#define LOAD(name) if (!(*(void **)&name##_p = dlsym(core, #name))) die("missing symbol %s", #name);
    LOAD(retro_init) LOAD(retro_set_environment) LOAD(retro_set_video_refresh) LOAD(retro_set_audio_sample)
    LOAD(retro_set_audio_sample_batch) LOAD(retro_set_input_poll) LOAD(retro_set_input_state) LOAD(retro_load_game)
    LOAD(retro_run) LOAD(retro_serialize_size) LOAD(retro_serialize) LOAD(retro_unserialize)
    LOAD(snesbox_trace_begin) LOAD(snesbox_trace_write) LOAD(snesbox_memory) LOAD(snesbox_trace_break)
    *(void **)&snesbox_co_active = dlsym(core, "co_active");
    LOAD(snesbox_core_regs) LOAD(snesbox_core_set_regs) LOAD(snesbox_core_bus_read)
    if (!(cpu_exec_hook = dlsym(core, "snesbox_cpu_exec"))) die("missing symbol snesbox_cpu_exec");
    if (sound_f || apulog_f) {
        void (**tap)(int16_t, int16_t) = dlsym(core, "snesbox_dsp_sample");
        if (!tap) die("missing symbol snesbox_dsp_sample (the core predates patch 0011)");
        *tap = dsp_sample;
    }
    if (voices_f) {
        void (**tap)(const int32_t *) = dlsym(core, "snesbox_dsp_voices");
        if (!tap) die("missing symbol snesbox_dsp_voices (the core predates patch 0013)");
        *tap = dsp_voices;
    }
    if (getenv("SNESBOX_SPCLOG")) {
        void (*set_cb)(const struct SpcLogCb *, int (*)(uint16_t, uint16_t)) = dlsym(core, "snesbox_smp_set_callbacks");
        static const struct SpcLogCb cb = {NULL, spclog_mem, spclog_step};
        if (!set_cb) die("missing symbol snesbox_smp_set_callbacks");
        if (checklib || locklib) die("SNESBOX_SPCLOG is for a plain run");
        if (!(spclog_f = fopen(getenv("SNESBOX_SPCLOG"), "wb"))) die("cannot write %s", getenv("SNESBOX_SPCLOG"));
        set_cb(&cb, NULL);
    }
    if (apulog_f) {
        void (**tap)(uint16_t, uint8_t, int) = dlsym(core, "snesbox_apu_write");
        if (!tap) die("missing symbol snesbox_apu_write (the core predates patch 0012)");
        *tap = apu_write;
    }

    retro_set_environment_p(env);
    retro_init_p();
    retro_set_video_refresh_p(video);
    retro_set_audio_sample_p(audio);
    retro_set_audio_sample_batch_p(audio_batch);
    retro_set_input_poll_p(poll);
    retro_set_input_state_p(input);

    size_t n; void *data = load_file(rom, &n);
    struct retro_game_info gi = {rom, data, n, NULL};
    if (!retro_load_game_p(&gi)) die("load failed: %s", rom);
    if (trace) snesbox_trace_begin_p();
    if (cyclog_f) {
        if (checklib || locklib) die("--cyclog is for a plain run");
        struct SnesboxCallbacks { void (*insn)(uint32_t); void (*write)(uint32_t, uint8_t, int);
            void (*read)(uint32_t, uint8_t); void (*interrupt)(uint32_t, uint16_t); void (*timing)(int, int, uint32_t); };
        static struct SnesboxCallbacks cb = {NULL, cyclog_write, cyclog_read, NULL, cyclog_timing};
        void (*set_cb)(const struct SnesboxCallbacks *) = (void (*)(const struct SnesboxCallbacks *))dlsym(core, "snesbox_set_callbacks");
        if (!set_cb) die("the core has no callbacks");
        cyclog_who = (int *)dlsym(core, "snesbox_cpu_who");
        if (!cyclog_who) die("the core does not export snesbox_cpu_who");
        set_cb(&cb);
    }
    if (apulog_f) {
        size_t n; uint8_t *a = snesbox_memory_p("apuram", &n);
        if (!a || n != 65536) die("no APU RAM view");
        fwrite(a, 1, n, apulog_f);
    }
    if (checklib) {
        char rp[4096]; snprintf(rp, sizeof rp, "%s/check.txt", outdir);
        if (check_begin(core, checklib, only, rp)) die("check setup failed");
        checklib_on = 1;
    }
    if (locklib) {
        char rp[4096]; snprintf(rp, sizeof rp, "%s/lockstep.txt", outdir);
        if (lockstep_begin(core, locklib, only, rp) < 0) die("lockstep setup failed");
        lockstep = 1;
    }
    /* The unit-call hook, in front of the checker's for good: the core
       saves and restores the hook around the original code it runs for
       the checker, and a hook swapped in for one call while such a run was
       open (a frame ending in an interrupt handler the hybrid took inside
       its C) was undone when it ended, so the call never started. */
    exec_before = *cpu_exec_hook;
    *cpu_exec_hook = call_exec;
    script(scr);
    if (checklib) check_end();
    if (lockstep) lockstep_end(frames);
    if (trace && snesbox_trace_write_p(outdir)) die("trace write failed");
    if (cyclog_f) fclose(cyclog_f);
    if (rows_f) fclose(rows_f);
    if (sound_f) fclose(sound_f);
    if (apulog_f) fclose(apulog_f);
    if (spclog_f) fclose(spclog_f);
    if (voices_f) fclose(voices_f);
    printf("frames %ld\n", frames);
    return 0;
}

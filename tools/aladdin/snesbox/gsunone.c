/* gsunone.c: the checker's coprocessor interface (gsucall.h) for a
   cartridge with no Super FX. check.c drives a coprocessor through these
   calls; with none, there are no coprocessor routines, nothing to check
   and nothing to compare. */
#include "gsucall.h"

int gsu_load(void *core, void *lib, const char *only) { (void)core; (void)lib; (void)only; return 0; }
int gsu_count(void) { return 0; }
void gsu_state_loaded(int running) { (void)running; }
void gsu_check_begin(FILE *report) { (void)report; }
void gsu_check_foreign(uint32_t addr) { (void)addr; }
void gsu_check_rows(FILE *report) { (void)report; }
int gsu_lockstep_prepare(void) { return 0; }
void gsu_lockstep_role(int role, FILE *report, volatile int *stuck) { (void)role; (void)report; (void)stuck; }
void gsu_before_frame(long f) { (void)f; }
int gsu_after_frame(long f, char *buf, size_t n) { (void)f; (void)buf; (void)n; return 0; }
void gsu_lockstep_done(void) {}
void gsu_lockstep_past_end(void) {}
int gsu_lockstep_is_past_end(void) { return 0; }
int gsu_lockstep_ring_room(void) { return 1; }
void gsu_lockstep_state_loaded(void) {}
int gsu_probe(unsigned id) { (void)id; return 0; }
void gsu_timing_line(FILE *report) { (void)report; }
int gsu_lockstep_summary(char *buf, size_t n) { if (n) buf[0] = 0; return 0; }

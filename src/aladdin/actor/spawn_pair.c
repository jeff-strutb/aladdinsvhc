/*
 * spawn_pair.c: two children of hit box $A3 started at the spawner's
 * spawn point and moved to the point it keeps at $48.
 */
#include "machine.h"
#include "ops65.h"
void abi_ActorSpawn(void);
void abi_ActorSpawnPairA3(void);
#include "actor.h"
#include "hw.h"
#include "wram.h"
#include "wram_player.h"
#include "wram_state.h"
#include "wram_video.h"

/*
 * @routine 84:8173 ActorSpawnPairA3
 * WHAT IT DOES: Starts two children with hit box $A3, their $07 0 and 1,
 * each placed at the spawner's point in its $48 (x) and $4A (y); while it
 * runs the spawner's $43 is set.
 * HOW: $43 up; for each child the spawn point 0 ($21), ActorSpawn, its
 * $07, and the whole pixels of its x and y from the two words; a spawn
 * with no record free ends it; then $43 cleared.
 * Entry: M 8-bit, X 8-bit; D = the spawner; DB = $81; called by JSR.
 * Exit: M 8-bit, X 8-bit (spawned) or as ActorSpawn left them; $43 = 0.
 */
void abi_ActorSpawnPairA3(void)
{
    MEM8(inc, ea_dp(0x43));  /* 848173 inc */
    stz8(ea_dp(0x21));  /* 848175 stz */
    lda8_v(0xA3);  /* 848177 lda */
    abi_ActorSpawn();  /* 848179 jsl */
    if (IF_N) goto L_8481AF;  /* 84817D bmi */
    lda8_v(0x00);  /* 84817F lda */
    sta8(ea_absx(0x0007));  /* 848181 sta */
    op_rep(0x20);  /* 848184 rep */
    lda16(ea_dp(0x48));  /* 848186 lda */
    sta16(ea_absx(SCRATCH));  /* 848188 sta */
    lda16(ea_dp(0x4A));  /* 84818B lda */
    sta16(ea_absx(0x000F));  /* 84818D sta */
    op_sep(0x30);  /* 848190 sep */
    stz8(ea_dp(0x21));  /* 848192 stz */
    lda8_v(0xA3);  /* 848194 lda */
    abi_ActorSpawn();  /* 848196 jsl */
    if (IF_N) goto L_8481AF;  /* 84819A bmi */
    lda8_v(0x01);  /* 84819C lda */
    sta8(ea_absx(0x0007));  /* 84819E sta */
    op_rep(0x20);  /* 8481A1 rep */
    lda16(ea_dp(0x48));  /* 8481A3 lda */
    sta16(ea_absx(SCRATCH));  /* 8481A5 sta */
    lda16(ea_dp(0x4A));  /* 8481A8 lda */
    sta16(ea_absx(0x000F));  /* 8481AA sta */
    op_sep(0x30);  /* 8481AD sep */
L_8481AF:
    stz8(ea_dp(0x43));  /* 8481AF stz */
    return;  /* 8481B1 rts */
}

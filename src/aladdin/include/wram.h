/*
 * wram.h: work RAM, by name. Addresses are in bank $00's view of the low
 * 8 KB ($0000-$1FFF, the same bytes as $7E:0000-$1FFF), as the original's
 * absolute and direct-page accesses reach them.
 */
#ifndef ALADDIN_WRAM_H
#define ALADDIN_WRAM_H

/* Sound commands to the SPC700 */
#define SOUND_SEQ 0x000002u        /* sequence number of the last command sent; the driver echoes it on APU port 2 */
#define SOUND_HOLD 0x000371u       /* nonzero: no command is sent this frame */
#define SOUND_SENDING 0x000372u    /* nonzero while a command is being sent */
#define SOUND_QUEUE_READ 0x0003BBu /* queue index of the next command to send (0-31) */
#define SOUND_QUEUE_WRITE 0x0003BDu /* queue index where the next command is added (0-31) */
#define SOUND_QUEUE 0x0003BFu      /* 32-entry ring of command bytes */

#endif

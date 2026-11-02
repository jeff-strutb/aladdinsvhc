/*
 * nmi_uploads.c: uploads the NMI makes on request flags the retail game
 * never sets: character sets from $7F:D000 or $7F:D400, a stepped row
 * upload from $7F:B000, two fixed blocks, and a palette upload from a
 * table.
 */
#include "machine.h"
#include "ops65.h"
void abi_PaletteUploadFrom(void);
void abi_UploadBlockA(void);
void abi_UploadBlockB(void);
void abi_UploadCharsets(void);
void abi_UploadStepped(void);
#include "actor.h"
#include "hw.h"
#include "wram.h"
#include "wram_player.h"
#include "wram_state.h"
#include "wram_video.h"






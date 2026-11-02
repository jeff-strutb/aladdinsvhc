/*
 * ending.c: the ending: its zoom scene's task, the stages task that shows
 * the levels played back, and the text pages with their extras.
 */
#include "machine.h"
#include "ops65.h"
void abi_ActorCreate(void);
void abi_ActorRecordsInit(void);
void abi_DemoStartLong(void);
void abi_EndingStagesTask(void);
void abi_EndingTextTask(void);
void abi_KeptOptionsSeal(void);
void abi_Mode7Rotate(void);
void abi_NewGameStateLong(void);
void abi_OamBufferClear(void);
void abi_PaletteLoad(void);
void abi_PlayStartCountsLong(void);
void abi_SceneSpinZoomTask(void);
void abi_SongPlay(void);
void abi_StageCountdownSet(void);
void abi_TaskCreate(void);
void abi_TaskWait(void);
void abi_TextBandsCommit(void);
void abi_TextBandsConceal(void);
void abi_TextBandsReveal(void);
void abi_TextLayerClear(void);
void abi_TextPageDraw(void);
void abi_TextPageExtra(void);
#include "actor.h"
#include "hw.h"
#include "wram.h"
#include "wram_player.h"
#include "wram_state.h"
#include "wram_video.h"





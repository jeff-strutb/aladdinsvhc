/*
 * title_task.c: the title: its start (the opening story first) and its
 * task: the Mode 7 title, the demo when left alone, the menu and the
 * level select.
 */
#include "machine.h"
#include "ops65.h"
void abi_ActorCreate(void);
void abi_ActorsFreeAll(void);
void abi_DemoStart(void);
void abi_DisplayDefaults(void);
void abi_GameRamClear(void);
void abi_LayersFillLong(void);
void abi_LevelCountersClear(void);
void abi_LevelMapBuildLong(void);
void abi_LevelNumberText(void);
void abi_LevelSelect(void);
void abi_Mode7ScreenSetup(void);
void abi_NewGameState(void);
void abi_NmiDisable(void);
void abi_NmiEnable(void);
void abi_OamBufferClear(void);
void abi_PadConfirmPressed(void);
void abi_PaletteCycleStartLong(void);
void abi_PaletteLoad(void);
void abi_PaletteSave(void);
void abi_PlayStartCounts(void);
void abi_ScrollStateClear(void);
void abi_SongPlay(void);
void abi_SoundQueueAdd(void);
void abi_SoundQueueF6(void);
void abi_SoundQueueF6With1(void);
void abi_TaskCreate(void);
void abi_TaskExec(void);
void abi_TaskSlotsFree1To6(void);
void abi_TaskWait(void);
void abi_TextLayerClear(void);
void abi_TitleStartTask(void);
void abi_TitleTask(void);
void abi_VramCopy(void);
#include "actor.h"
#include "hw.h"
#include "wram.h"
#include "wram_player.h"
#include "wram_state.h"
#include "wram_video.h"



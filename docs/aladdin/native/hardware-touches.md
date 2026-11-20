# Every hardware touch in the game's C, and what replaces it

> The conversion's starting inventory, taken 2026-11-19 over the decomp's C (`src/aladdin` at `cffb00c`), with
> `tools/nshvc-find-hardware` of nativeshvc `8d1745d` (API 6.5). The plan is [PLAN.md](PLAN.md). Every site of a real hardware
> touch is in [`hardware-touches-sites.txt`](hardware-touches-sites.txt) (636 entries for 632 lines: a line naming registers of two kinds is listed under each), with its replacement.

**Numbers first.** The finder reports 22,345 lines in 85,916 lines of C (340 files: 326 `.c`, 14 headers). 574 of its
lines touch real hardware; 38 more lines name registers its patterns miss, and 20 reach cartridge RAM the board does not
have: 632 lines in all. The rest of its count is register style (15,680 lines: `abi_` 6,799, `cpu.` 6,007, `op_` 2,874) and data at SNES addresses (5,459 of the
5,723 "bus access" lines). **No engine gap was found:** every hardware touch has a call in `docs/api.md` or needs none.

## 1. The finder's totals

`python3 tools/nshvc-find-hardware src/aladdin`, run through `cpu-run -j 1 -m 1G`. A line is counted once for each kind it
matches.

| Kind | Lines | actor | audio | game | include | input | level | map | math | player | system | video |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| register-convention routine (`abi_`) | 6,799 | 3,942 | 295 | 1,084 | 1 | 1 | 182 | 107 | 6 | 601 | 89 | 491 |
| register file (`cpu.`) | 6,007 | 2,824 | 93 | 448 | 181 | 2 | 184 | 348 | 34 | 305 | 198 | 1,390 |
| bus access (`rd8`, `wr8`...) | 5,723 | 2,761 | 50 | 407 | 55 | 2 | 383 | 200 | 28 | 459 | 155 | 1,223 |
| instruction helper (`op_`) | 2,874 | 1,537 | 39 | 212 | 48 | 0 | 81 | 118 | 4 | 148 | 55 | 632 |
| DMA or HDMA | 367 | 4 | 0 | 14 | 60 | 0 | 66 | 0 | 0 | 0 | 35 | 188 |
| picture register | 200 | 10 | 0 | 9 | 41 | 0 | 23 | 0 | 0 | 0 | 35 | 82 |
| sound port | 151 | 0 | 117 | 0 | 34 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| console connector | 95 | 66 | 0 | 10 | 3 | 0 | 3 | 0 | 0 | 4 | 1 | 8 |
| video memory port | 80 | 1 | 0 | 0 | 9 | 0 | 4 | 0 | 0 | 0 | 2 | 64 |
| interrupt or wait | 37 | 0 | 0 | 0 | 8 | 0 | 0 | 0 | 0 | 0 | 26 | 3 |
| joypad | 16 | 0 | 0 | 0 | 11 | 0 | 0 | 0 | 0 | 0 | 5 | 0 |

The hardware kinds (the last seven rows, and the bus-access lines whose address is a register) are 981 distinct lines:
124 are `include/hw.h`'s own definitions, 146 are comments, 137 are code whose match is a work-RAM name that the finder's
patterns catch (`HDMA_TABLE_AT`, `HDMA_TABLES`, `TS_SHADOW`, `DMA_FOUR_REGS`...), and **574 touch hardware**.

## 2. Hardware

| Touch | What it is | Sites | Replaced by |
|---|---|---:|---|
| `spc_rd`, `spc_wr`, `spc_wait_until` | the SPC700 driver's memory, DSP and ports (`audio/sound_driver.c` 101, `include/spc_ops.h` 19, `include/spc.h` 9, `audio/dsp_write.c` 2) | 131 | the driver as music code: `nshvc_voice_*`, `nshvc_echo`, `nshvc_echo_writes`, `nshvc_sound_volume`, `nshvc_sound_noise_rate`, `nshvc_music_receive`; its RAM becomes the driver's own C state |
| `jump_routine` 78, `call_routine` 14 | calls by address (handler tables, tail jumps), and `jump_routine_pushed`'s declaration | 93 | a direct C call, or a `switch` on a named id |
| `HDMAEN`, `DMAP1`-`7`, `BBAD1`-`7`, `A1T1`-`7`, `A1B1`-`7`, `DASB0`-`7`, `A2A1`-`7`, `NTRL1`-`7` | HDMA, channels 1 to 7 (`level/level_screen_setups.c` 110 of the names, `system/interrupts.c` 29, `video/colour_band_task.c` 7, `game/ending.c` 6) | 106 | `nshvc_lines` in `draw`, from the game's per-line array, the field by the register (section 3) |
| `DMAP0`, `BBAD0`, `A1T0`, `A1B0`, `DAS0`, `MDMAEN` | DMA, always channel 0 (every `MDMAEN` write is 1) | 82 | an object write where the data changes (section 4) |
| `VMADDL`, `VMAIN`, `VMDATAL`, `VMDATAH`, `VMADDH` | video memory port | 60 | `nshvc_tiles_write`, `nshvc_map_write_snes` |
| `WRMPYA`, `WRMPYB`, `RDMPYL`, `RDMPYH`, `WRDIVL`, `WRDIVB`, `RDDIVL` | multiplier and divider (`math/multiply.c` 12, `game/options_screen.c` 4, `video/text_number.c` 4, `actor/actor_moves.c` 4) | 24 | ordinary C |
| `M7A`, `M7B`, `M7C`, `M7D`, `M7X`, `M7SEL` | Mode 7 (`video/mode7_scale.c` 13, `video/mode7_screen.c` 2, `video/screen_setup.c` 1) | 16 | `nshvc_mode7_set`; `M7SEL` is `$80` or 0, so `outside` 1 or 0. `Mode7ElementScale` (`mode7_scale.c:36-42`) uses `M7A`/`M7B` as the multiplier: ordinary C |
| `NMITIMEN`, `RDNMI`, `TIMEUP`, `HTIMEL`/`H`, `VTIMEL`/`H` | interrupts | 15 | gone: the NMI's work moves to `update` and `draw`; the IRQ's change at a line is `nshvc_line` (section 5) |
| `APUIO0`-`3` | sound ports, S-CPU side (`audio/spc_upload.c` 18 names, `audio/sound_command.c` 8) | 13 | `nshvc_music_send` for the commands; the upload handshake goes (`nshvc_sample_load` and a message, section 6) |
| `WH0`-`WH3` | window edges | 9 | `nshvc_windows.w[]`; by line `NSHVC_LINE_WIN1_LEFT` to `NSHVC_LINE_WIN2_RIGHT` |
| `INIDISP` | brightness and forced blank | 8 | `nshvc_screen_set` (`.brightness`, `.blank`); by line `NSHVC_LINE_BRIGHTNESS`, `NSHVC_LINE_BLANK` |
| `BG1HOFS`-`BG3VOFS` | layer scroll (`video/nmi_upload.c`, `ScrollRegistersWrite`) | 8 | `nshvc_layer.scroll_x`/`_y`; by line `NSHVC_LINE_SCROLL_X`/`_Y` + layer |
| `RDVRAML`, `RDVRAMH` | reading video memory back (`video/mode7.c:120-153`, `video/nmi_uploads.c:246`) | 5 | none: the game reads its own copy of what it wrote (section 4) |
| `TM`, `TS` | layers on the main and sub screens | 4 | `nshvc_layer.enabled_main`/`_sub`, `nshvc_sprites_show`; by line `NSHVC_LINE_MAIN`/`_SUB` + layer, `NSHVC_LINE_SPRITES_MAIN`/`_SUB` |
| `CGWSEL`, `CGADSUB` | colour maths | 4 | `nshvc_blend_set`; by line `NSHVC_LINE_SUBTRACT`, `_HALF`, `_USE_FIXED`, `_BLEND_LAYERS`, `_CLIP`, `_PREVENT` |
| `COLDATA` | fixed colour | 4 | `nshvc_blend.fixed_bgr15`; by line `NSHVC_LINE_FIXED` |
| `HVBJOY`, `JOY1L` (and `JOY2L` by index) | joypads, auto-read wait, H-blank wait (`system/joypad.c` 3, `system/interrupts.c` 1) | 4 | the `nshvc_input` given to `update`, read by the game's own controls; the waits go |
| `BG1SC`, `BG2SC`, `BG3SC`, `BG12NBA`, `BG34NBA` | layer maps and tile bases, all in the NMI | 5 | `nshvc_layer.map`, `.tiles`: engine objects per screen kind |
| `OAMADDL`, `OAMADDH` | sprite table address, before the sprite DMA (both written 0: no priority rotation) | 2 | `nshvc_sprites_set(list, count, 0)` |
| `TMW`, `TSW` | window masks | 2 | `nshvc_windows.mask_main`/`_sub`; by line `NSHVC_LINE_MASK_MAIN`/`_SUB` + layer |
| `MPYM`, `MPYH` | the product of `M7A` x `M7B` | 2 | ordinary C multiplication |
| `SETINI` | overscan, interlace: only ever written 0 (`video/screen_setup.c`) | 2 | nothing: `nshvc_screen`'s defaults |
| `CGADD` | palette address, before the palette DMA | 1 | `nshvc_palette_write` |
| `BGMODE` | picture mode | 1 | `nshvc_mode_set`; by line `NSHVC_LINE_MODE` |
| `W12SEL`, `W34SEL`, `WOBJSEL`, `WBGLOG`, `WOBJLOG` | window use and logic, all in the NMI | 5 | `nshvc_windows.layer_use`; by line `NSHVC_LINE_WIN_USE` + layer |
| `MOSAIC` | mosaic | 1 | `nshvc_layer.mosaic`; by line `NSHVC_LINE_MOSAIC` + layer |
| `OBSEL` | sprite sizes and base: `$03` only (`video/screen_setup.c:78`), 8 x 8 and 16 x 16 | 1 | `nshvc_sprite.width`/`.height`/`.tiles` |

The table's sites add up to 608; with the 8 below that the finder has no pattern for, 616 entries in the sites file.

The NMI handler (`system/interrupts.c`, `NmiHandler` 80:8111) writes most picture registers once each, from work-RAM
shadows the game keeps (`TM_SHADOW` $0399, `BGMODE_SHADOW` $039D, `INIDISP_SHADOW` $03B8, `HDMAEN_SHADOW` $03B6 and the rest
in `include/wram.h` and the head of `system/interrupts.c`). So the register lines are few and the real inputs to `draw` are
those shadows: each becomes a named field of the picture state that `draw` reads.

### Touches the finder does not look for

Its patterns miss `W34SEL`, `WOBJSEL`, `WBGLOG`, `WOBJLOG`, `BG12NBA`, `BG34NBA`, `DASB`, `A2A`, `NTRL` and some multiplier
lines; those 30 lines are counted in the table above and marked in the sites file. These it has no pattern for at all:

| Touch | Sites | Where | Replaced by |
|---|---:|---|---|
| `SLHV`, `OPVCT` (the beam's line) | 4 | `system/interrupts.c:322,324` (the IRQ's line), `video/unpack.c:407-408` (unpack until line 240, then wait a frame) | gone: the IRQ's line is a fixed line (section 5); the unpacking is done in one update |
| `STAT78` (PAL or NTSC) | 2 | `system/interrupts.c:323`, `game/password_screen.c:137` (`RegionCheck`) | the NTSC answer as a constant: the board is NTSC and the PAL branch never runs (the routine's own comment) |
| `MEMSEL`, `WRIO` | 2 | `system/power_on.c:103,113` | gone |
| Cartridge RAM at `$70:xxxx` | 20 | copier checks: `actor/kind20.c:433,437`, `actor/kind27.c:255`, `actor/kind_waits_rest.c:935`, `game/level_end.c:512`, `actor/guard_start.c:139,142`, `game/player_scene_moves.c:222`; the input recorder `system/input_modes.c:128-161` (10) | none: the board has no cartridge RAM, so each read is open bus ($70, the last byte on the bus, as the C's comments say). The native C keeps the retail board's result as a constant; the copier branches (`kind20.c:439` loops for ever) are the census's to list as dead on retail |

## 3. The per-line effects (HDMA), by target

Every HDMA channel's target, from the code and, for the window effects, from the register-set table the routine indexes
(9 pairs of DMAP, BBAD at `$BCA9` of the data bank, indexed by Y = 0 to $10 in `video/window_effects.c`'s `start` calls;
read from bank $80 of the ROM, the only bank where `$BCA1` holds the channel bits 01 to 80 and `$BCA9` DMAP/BBAD pairs; the
routines' comments say DB = $82, which stage 4 settles from a recording).

| Where | Channel, DMAP, BBAD | Register | `nshvc_line_field` |
|---|---|---|---|
| `level/level_screen_setups.c:61`, `:338` | 4 or 2, $02, $0F | BG2HOFS (write twice) | `NSHVC_LINE_SCROLL_X + 1` |
| `level/level_screen_setups.c:332`, `:594`, `:869`, `:943` | 1, 3 or 7, $00, $2C | TM | `NSHVC_LINE_MAIN + layer`, `NSHVC_LINE_SPRITES_MAIN` |
| `level/level_screen_setups.c:582`, `:908` | 1 or 2, $00, $05 | BGMODE | `NSHVC_LINE_MODE`, `NSHVC_LINE_BG3_PRIORITY` |
| `level/level_screen_setups.c:588` | 2, $02, $12 | BG3VOFS (write twice) | `NSHVC_LINE_SCROLL_Y + 2` |
| `level/level_screen_setups.c:856` | 6, $03, $21 | CGADD, CGDATA: one palette entry a line | `NSHVC_LINE_PALETTE + entry` |
| `video/colour_band_task.c:58` | 7, $00, $32 | COLDATA | `NSHVC_LINE_FIXED` (the channel bits folded into the colour by the game) |
| `game/ending.c:358` | 5, $40 (indirect), $24 | W34SEL | `NSHVC_LINE_WIN_USE + 2`, `+ 3` |
| `video/window_effects.c` set 0, $10 | free channel 4-7, $40, $2C / $2D | TM / TS | `NSHVC_LINE_MAIN`/`_SUB + layer` |
| set 2 | $43, $21 | CGADD, CGDATA | `NSHVC_LINE_PALETTE + entry` |
| set 4 | $42, $0E | BG1VOFS | `NSHVC_LINE_SCROLL_Y + 0` |
| set 6, $0E | $41 / $01, $26 | WH0, WH1 | `NSHVC_LINE_WIN1_LEFT`, `_RIGHT` |
| set $0C | $41, $28 | WH2, WH3 | `NSHVC_LINE_WIN2_LEFT`, `_RIGHT` |
| set 8 | $40, $05 | BGMODE | `NSHVC_LINE_MODE` |
| set $0A | $42, $1B | M7A (write twice) | `NSHVC_LINE_M7_A` |

Indirect channels ($40 set) read their values through pointers in the table; the game's C resolves them into the per-line
array before `nshvc_lines`. The tables live at `$7E:F400` up (`HDMA_TABLES`) and `$7E:F800` (`WINDOW_EDGES`); the NMI
restarts channels 1 to 7 each frame (`A2A` = `A1T`, `system/interrupts.c:325-337`), which the native game does not need:
`draw` sends the whole picture each time.

## 4. DMA, by destination

| Destination | DMAP, BBAD | Where | Replaced by |
|---|---|---|---|
| VRAM, words | $01, $18 (`$1801`) | `video/nmi_upload.c:38` (text buffer), `video/vram_queue.c` (queue, lists, uploads by number), `video/scroll_layer.c:1203,1234` (layer columns and rows), `video/unpack.c:139`, `video/nmi_uploads.c:199` | `nshvc_map_write_snes` for map words, `nshvc_tiles_write(..., NSHVC_TILES_PLANAR)` for tiles, by what the VRAM address holds on that screen |
| VRAM, low bytes | $00, $18 | `video/nmi_uploads.c:49` (`UploadCharsets`, VMAIN 0: the Mode 7 characters) | `nshvc_tiles_write(..., NSHVC_TILES_MODE7)` |
| VRAM address and data in turn | $04, $16 | `video/vram_words.c:41` (`VramWordsUpload`: address, word pairs) | `nshvc_map_write_snes`, one word each |
| VRAM to work RAM | $81, $39 | `video/nmi_uploads.c:234` (`UploadBlockB`) | none: the game copies from its own array |
| CGRAM | $00, $22 | `video/nmi_uploads.c:272` (`PaletteUploadFrom`) | `nshvc_palette_write` |
| OAM | (`system/interrupts.c:224-241`) | the sprite table from `OAM_BUFFER` $7E:FA80 | `nshvc_sprites_set` from the game's own sprite list (`video/oam.c` builds it) |

`video/mode7.c`'s `Mode7MapToBuffer` and `VramWordsToBuffer` read VRAM back through the port to convert graphics for Mode 7.
The API has no read of a tile set or a map, and needs none: the native game keeps the arrays it writes (as the zelda3
example keeps Mode 7's tiles and map, `docs/conversion-guide.md` section 8).

## 5. Interrupts

- **NMI** (`NmiHandler`, 80:8111): frame counters; forced blank; scroll registers; work-RAM moves; HDMA tables' addresses;
  sprite DMA; the VRAM queue, lists, tile blocks, words, text buffer, uploads by number, character sets, stepped uploads,
  blocks A and B, the palette; the window, layer, colour and mosaic registers from their shadows; the IRQ's line; BGMODE;
  the Mode 7 matrix; INIDISP; each waiting task counted down; the sound command sent. In the native game the countdown,
  counters and sound send are `update`'s; the uploads are object writes made when the game queues them; the registers
  become the picture `draw` sets.
- **IRQ** (`IrqHandler`, 80:8332): turned on by `LevelScreenSetup` (`level/level_screen_setups.c:623`, `NMITIMEN_SHADOW`
  $31) for every level. The NMI leaves the screen blanked; at the IRQ's line, in that line's H-blank, it turns on HDMA,
  INIDISP from its shadow, TM and TS. The line is `V_1C7D` when nonzero (`level_screen_setups.c:162` sets $0C), else 8,
  unless the V counter latched in the NMI reads 5 to $7F, when it is that plus 3 plus the carry (`interrupts.c:319-330`).
  Native: `nshvc_line` from the line after it for `NSHVC_LINE_BLANK`, `NSHVC_LINE_BRIGHTNESS`, `NSHVC_LINE_MAIN`/`_SUB`,
  and the HDMA effects' arrays start there.
- **BRK and COP** are the actors' yields (`docs/aladdin/ACTORS.md`), not hardware: PLAN.md section 4.

## 6. Sound

- **S-CPU side** (`audio/sound_command.c`): one command a frame through port 0 (parameter on port 3), sent only after the
  driver echoes the last sequence number on port 2. Native: each command is a `nshvc_music_send` message, in order; the
  echo wait goes. The game reads nothing else from the SPC700 (port 2's echo and the upload handshake are the only reads).
- **Uploads** (`audio/spc_upload.c`): the driver, two sample sets and the songs go by the IPL protocol from the lists at
  `$85:8000` (`config/aladdin/spclists.csv`, 42 entries); commands $F9 and $FA send the running driver back to take a new
  upload. Native: the game thread loads each set's samples with `nshvc_sample_load` and passes the song data, then sends a
  message that the set is ready; the music code never waits.
- **Driver** (`audio/sound_driver.c`, 28 routines; `audio/dsp_write.c`): timer 0 at $40 (`sound_driver.c:188`), 8 ms, a
  tick every 256 DSP samples; the music and the effects on alternate ticks. It writes key on and off, pitch, volume,
  ADSR and GAIN, the source, the main and echo volumes, echo feedback, the FIR (`EchoFirSet`), `ESA` and `EDL`
  (`sound_driver.c:1623-1626`) and `FLG`. It reads no DSP state (no ENVX, OUTX or ENDX read in `audio/*.c`). Native:
  `nshvc_music.hz` 32,040 with a driver tick every 256 samples, calling `nshvc_voice_key_on`/`_key_off`/`_pitch`/`_volume`/
  `_adsr`/`_gain`/`_options`, `nshvc_echo`, `nshvc_echo_writes`, `nshvc_sound_volume`, `nshvc_sound_noise_rate`.

## 7. Data at SNES addresses

The 5,723 "bus access" lines, divided by what the address is. Method: the first argument of each `rd8`/`rd16`/`rd24`/`wr8`/
`wr16` call, its names resolved from the file's and the headers' `#define`s; a line with several calls goes to the first
kind in this table's order of hardware, cartridge RAM, ROM, work RAM.

| What the address is | Lines | Replaced by |
|---|---:|---|
| work RAM through the data bank (`dba`, `ea_abs`, `abs_db`, `in_db` of a name or number below $2000) | 2,091 | fields of the game's typed state |
| work RAM through the direct page (`dpa`, `ea_dp`, `dp`): an actor's record (D = the record) or D = 0 | 1,919 | fields of the actor record, or of the state |
| ROM data | 477 | typed tables read from `cart.dat` through `nshvc_asset_*` (PLAN.md section 3) |
| ... through the data bank, offset $8000 up | 337 | |
| ... long addresses | 133 | |
| ... a bank held in a variable (`map/level_map.c:85-97`, `video/palette.c:73`, `actor/motion.c:544,613`) | 7 | |
| work RAM, long addresses ($7E/$7F; 9 of them `video/mode7_screen.c` with DB = $7F) | 274 | fields and arrays of the game's state |
| hardware registers | 264 | section 2 |
| work RAM, bank-0 names (`TEXT_TILEMAP_UPLOAD`, `SOUND_QUEUE + x`...) | 261 | fields of the game's typed state |
| computed addresses (pointers in variables, the task stacks) | 221 | pointers and indices into typed records and tables, one by one |
| an actor record through a pointer plus a field (`rec + ACTOR_STATE`) | 141 | `Actor *` and its field |
| helpers in `include/` whose address is a parameter | 55 | never brought over |
| cartridge RAM area `$70:xxxx` | 20 | section 2 |

Limits of this division: "through the data bank, offset $8000 up" is taken as ROM because the data bank is a ROM bank
there; the four files that set DB to $7E or $7F (`video/unpack.c`, `video/parallax.c`, `map/level_map.c`,
`video/mode7_screen.c`) have no such line, but a routine called with DB left at $7E or $7F by its caller would be
miscounted, and each is read when it is converted. Data reached through instruction helpers (`lda16(ea_absx(TABLE))`) is
in the 2,874 "instruction helper" lines, not here. About 540 work-RAM addresses already have a `#define` name in the C
(480 below $2000, 58 in $7E/$7F); 92 of them are named only by their address (`V_xxxx`).

## 8. Register style the finder does not see

Machine primitives of `include/machine.h` and `native.h`, all of which go in the same pass (PLAN.md section 4):
`actor_yield` 619 sites, `actor_wait` 207, `actor_leave` 32, `task_yield` 3 (in `system/task.c`), `task_hand_off` 4,
`actor_hand_off` 2, `actor_loop_call` 2, `cpu_loop_forever` 2; the pushed return addresses `jsl_frame` 1,460 and
`call_frame` 1,042; `goto` over translated assembly 1,086; flag tests `IF_` 1,495; the SPC700's `spc.` 672. A finder result
of 0 is required and not sufficient.

## 9. Engine gaps

None. Each thing Aladdin does was checked against `docs/api.md`:

| Aladdin does | API |
|---|---|
| one palette entry changed a line (HDMA mode 3 on CGADD, CGDATA) | `NSHVC_LINE_PALETTE + entry` |
| the picture mode changed a line (BGMODE by HDMA) | `NSHVC_LINE_MODE` |
| Mode 7's A changed a line (M7A by HDMA) | `NSHVC_LINE_M7_A` |
| the fixed colour a line (COLDATA by HDMA) | `NSHVC_LINE_FIXED` |
| the window use a line (W34SEL by indirect HDMA) | `NSHVC_LINE_WIN_USE + layer` |
| the screen turned on at a line (the IRQ) | `nshvc_line` with `NSHVC_LINE_BLANK`, `_BRIGHTNESS`, `_MAIN`, `_SUB` |
| VRAM read back | not needed: the game's own arrays |
| sample sets and songs loaded during play | `nshvc_sample_load` on the game thread, a message to the music code |
| echo with its FIR, delay, feedback, write disable | `nshvc_echo`, `nshvc_echo_writes` |
| two joypads | `nshvc_input`, the game's own controls |

Things the native game decides for itself, from the code, with no engine call: the IRQ's line, the copier checks' and the
region check's retail results, and the beam-timed unpacking.

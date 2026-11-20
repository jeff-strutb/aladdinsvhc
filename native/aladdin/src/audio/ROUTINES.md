# Sound routines: the record against the native code

Every `@routine` of the record's sound (`src/aladdin/audio`, 44 routines, and the 59 entry points its C has), with where
its behaviour lives here, or why it has no counterpart.

How it was checked: `native/checks/sound/sound_check` replays the original's runs (snesbox `--apulog`: the APU RAM at
power-on, then every SPC700 write with its DSP sample time) on the native driver and compares every voice's sample,
envelope, pitch and volumes, its echo, the whole mix and the key-ons and key-offs before every tick. On 41 runs (the 39
songs, the 80 effect entries one at a time, and a mixed run: a song with all 80 entries over it, pause, mono, the fades
and the other driver commands) and the power-on probe: 330,680 ticks, 0 with a difference (`docs/aladdin/native/SOUND.md`).

## The driver (85:80BD-85:8AB8, SPC700): `driver.c`

| Routine | Native |
|---|---|
| 85:80BD SoundDriverStart | `driver_start` (its RAM clearing and set-up), `driver_tick` (its loop's tick: the stepping and updating halves); its port poll is `music_tick` taking messages |
| 85:8222 DspSetup | `driver_start`: PMON and NON 0 (`nshvc_voice_options`), the directory (samples are engine samples), echo set 0 |
| 85:823B PortCommandPoll, PortCommandPollEnd | No counterpart: the command arrives as a message (`music_take`); its count and echo on ports 2-3 were the S-CPU's handshake |
| 85:825B SoundCommand | `sound_start` (below $F0), `driver_command` ($F0 up; SoundCommandTable's jump) |
| 85:82E1 VoiceStateClear, 85:82F8 VoiceDpClear | `memset` of the effect channel in `sound_start` |
| 85:832A SoundStopAll | `stop_all` |
| SoundCmdF1 | `effects_stop` |
| 85:8346 SequencerReset, 85:835C PageClear50 | `sequencer_reset` |
| SoundCmdFA, SoundCmdFB | `driver_command($F9)`, `driver_song` for each block, then `driver_command($FB)` (`music_take`'s MUSIC_UPLOAD and the command after it) |
| SongStartKeep | `song_start` |
| 85:83AE SoundCmdFC, SoundCmdFD | `tempo_offset_set`, `driver_command($FD)` |
| SoundCmdF3, SoundCmdF4 | `driver_command($F3)`, `music_go_on` |
| 85:83F6 VoicesReleaseFree | `voices_release_free` |
| SoundCmdF5, SoundCmdF6, FadeStart | `fade_start` |
| 85:8442 FadeClear, 85:8448 FadeSet | `fade_clear`; FadeSet's two stores are in `fade_start` |
| SoundCmdF7, SoundCmdF8 | `driver_command($F7)`, `($F8)` |
| 85:8453 SoundUpload | `driver_command($F9)` (everything stops) with the blocks as `driver_song`. Its handshake has no counterpart, nor its reset of the timer's pending count: ticks are not left pending here |
| 85:84AE VoiceTick | `voice_tick`, with its note in `note` |
| 85:861D TrackByte, TrackByteEnd | `track_byte` |
| SeqCmd00-03, 05-0D, 16-1D | `command_run`, by command |
| 85:876E VoiceFlagsSet, VoiceFlagsToggle, VoiceFlagsStore | `flags_set`; the toggles and stores in `command_run` |
| SeqCmd0E-11, SeqLoop, SeqLoopSkip | `seq_loop` (commands $0E-$15) |
| SeqCmd17 | `command_run` case $17 |
| 85:8799 VoiceStop | `voice_stop` |
| 85:87A4 VoiceSfxMark | `voice_sfx_mark` |
| SeqCmd1A's stores into the page-1 arrays | `page_store` (the original's arrays' layout kept for a large setting number; no track uses one) |
| SeqCmd1B, EchoSetup, EchoSetupEnd | `command_run` case $1B, `echo_setup` |
| 85:8850 EchoOff | `echo_off` |
| 85:8869 EchoFirSet | `fir_set` |
| VoiceSlide | `voice_slide` |
| 85:8914 VoiceVolumeSet | `volume_set` |
| 85:8985 CurveAt16, 85:898D CurveAt8, 85:898B CurveAtY, CurveShift | `curve_at16`, `curve_at8`, `curve_at_y`, `curve_shift` |
| 85:89D3 VoicePitchUpdate | `pitch_update` |
| 85:89E6 VoicePitchSet, VoicePitchFrom | `pitch_set` (VoicePitchFrom is `pitch_out` of the pitch as it is) |
| VoicePitchVibrato | `pitch_vibrato` |
| VoicePitchOut | `pitch_out` |
| VoiceInstrument, VoiceInstrumentFrom, VoiceInstrumentEnd | `instrument_set` (VoiceInstrument is it with the value less 1) |
| 85:8211 VoiceRelease | `voice_release` |
| 85:8218 VoiceAdsr1Set | `adsr_off` |
| 85:8AAD DspWriteVoice, 85:8AB8 DspWrite | No counterpart: each register write is the engine call for it where the value is made (`out_*`): SRCN `nshvc_voice_sample`, ADSR1/ADSR2/GAIN `nshvc_voice_adsr` or `nshvc_voice_gain` by ADSR1's bit 7, PITCH, VOL, KON, KOF, EON `nshvc_voice_options`, MVOL `nshvc_sound_volume`, EVOL/EFB/EDL/FIR `nshvc_echo`, FLG `nshvc_echo_writes`, `nshvc_sound_mute`, `nshvc_sound_noise_rate`. ESA has none: the engine's echo buffer is its own, and the original's ($E800-$FFFF) overlaps none of the sound data, which ends at $E7F9 |

## The S-CPU's side: `sound.c`

| Routine | Native |
|---|---|
| 81:FA00 SoundQueueAdd | `SoundQueueAdd`, the ring in `g` |
| 81:F9F0 SoundQueueF0, 81:F9F7 SoundQueueF2, 81:F9FE SoundQueueF1 | `SoundQueueF0`, `SoundQueueF2`, `SoundQueueF1`. Their store of the last special command ($03F8) is dropped: nothing reads it |
| 81:F9E3 SoundQueueF5WithA, 81:F9E1 SoundQueueF5With4 | `SoundQueueF5WithA`, `SoundQueueF5With4` |
| 81:F9D4 SoundQueueF6, 81:F9D2 SoundQueueF6With1 | `SoundQueueF6`, `SoundQueueF6With1` |
| 80:B463 SoundCommandSend | `SoundCommandSend`. Its wait for the driver's echo of the sequence number, the hold flag and the sending flag have no counterpart: a command reaches the driver's queue at once, and nothing else holds it |
| 80:B4AA SoundSetsLoad | `SoundSetsLoad`: the data of lists 0, 1 and 2 (the original's X 0, 3, 6) built once, the directory's 78 entries loaded as 34 engine samples, MUSIC_START, then the sets' two uploads ($F9 each) |
| 80:B4D0 SpcUpload | `send_upload`: one message of the list's blocks. The IPL handshake has no counterpart |
| 81:FA23 SongPlay | `SongPlay`: the song's list as an upload, then $FB. Its check of the sending flag is dead here (nothing sends while it runs) |

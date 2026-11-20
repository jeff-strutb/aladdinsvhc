# Aladdin on nativeshvc: the sound (stage 3)

The driver is the game's music code in `native/aladdin/src/audio` (`driver.c`, the original's SPC700 driver as typed C;
`music.c`, the engine's `nshvc_music.tick`; `sound.c`, the game's side; `sound.h`, what the rest of the game calls;
`ROUTINES.md`, every routine of the record and where it went). It needs nativeshvc API 6.6 (`nshvc_voice_sample`, nativeshvc `fce4dc0`).

## What the driver does

- **Per tick.** One tick every 256 DSP samples (timer 0 at $40: 64 counts of 128 SPC700 cycles, 125.16 a second);
  `nshvc_music.hz` is 32,040 and `music_tick` counts the samples, as zelda3 and Stunt Race FX do. Ticks take turns. A
  stepping tick runs the music's 8 channels at the song's tempo (with its offset) unless paused, steps the fade, runs
  each effect channel at its own tempo, then keys off. An updating tick gives the echo its settings (or goes on settling
  after a delay change), writes each keyed or restored voice's envelope (or its music instrument back), then keys on.
  Commands from the game run between ticks, as the original polled its ports there.
- **Channels.** 16 channels on 8 voices: music 0-7, effects 8-15, effect n over voice n. A voice an effect holds is not
  touched by its music channel, which keeps counting, and gets its instrument back when the effect ends.
- **DSP registers and the engine.** SRCN `nshvc_voice_sample` (new in 6.6: the SNES takes a changed source at the
  voice's next loop; the driver changes it about 8 ms before key-on); ADSR1, ADSR2, GAIN `nshvc_voice_adsr` or
  `nshvc_voice_gain`, by ADSR1's bit 7 as the DSP chooses; PITCHL/H `nshvc_voice_pitch`; VOLL/R `nshvc_voice_volume`;
  KON `nshvc_voice_key_on`, KOF `nshvc_voice_key_off`; EON `nshvc_voice_options`; MVOL `nshvc_sound_volume`; EVOL,
  EFB, EDL and FIR0-7 `nshvc_echo`; FLG `nshvc_echo_writes`, `nshvc_sound_mute`, `nshvc_sound_noise_rate`. PMON and
  NON are written 0 once; DIR names the directory, whose 78 entries are 34 engine samples (`nshvc_sample_load`, BRR,
  each looping inside itself). ESA has no call: the engine has no sound memory and keeps its own echo buffer (Jeff
  asked what is right for the engine: an echo address only matters where the echo shares memory with data, which the
  engine's model does not have; the delay, its size, is the console capability, and the original's buffer,
  $E800-$FFFF, overlaps none of the sound data, which ends at $E7F9).
- **The data.** The lists at $85:8000 through `data/` (`sound_list_blocks`, `sound_list_block`): list 0 (the driver's
  tables at $02C0-$0CDB and its echo sets at $0D10), list 1 (the 81 effect entries at $1DB0, with the music's volume
  at $1DB1), list 2 (the sample directory at $3300, 78 instruments of 6 bytes at $3438, the BRR samples at
  $360C-$E7F9), lists 16-54 (the 39 songs, each one block at $0D20). The driver reads them as one 64 KB space in which
  positions are the data's own (its tracks jump by position).
- **Commands.** Below $F0 a sound (effect entry n, taken off the 81 until below; entries 0 and 25 are empty, and no
  entry is a song); $F0 stop all, $F1 stop effects, $F2 stop music, $F3 pause, $F4 go on, $F5/$F6 fade out/in (the
  next queued byte the speed's high byte), $F7 stereo, $F8 mono, $F9 an upload (everything stops), $FA an upload and
  the song, $FB the loaded song from its start, $FC/$FD tempo offset and detune (port 1, which the game leaves 0).
  The game sends $F0-$F8 through its queue (SoundQueueF0/F1/F2, F5WithA (the story card), F6 and F6With1 (the fades),
  the options' stereo/mono, pause $F3/$F4), effects by SoundQueueAdd (literal values and values from data tables), songs
  by SongPlay (22 files), $F9/$FA only inside uploads, never $FB-$FF. The driver never runs command 0 (a 0 on the port
  is no command), so effect entry 0 cannot sound.

## How it was checked

The original: `native/checks/sound/original.sh` runs the ROM in snesbox on the project's bsnes core from power-on,
parks the S-CPU at frame 400 (after the game's own load and first song) and plays each song by the game's own
SongPlay for 60 s, each effect entry 1-80 alone, and a mixed run (song $19 with all 80 entries over it 20 frames
apart, then $F3 $F4 $F8 $F7, a fade out at $0400, in at $0800, a fade of speed 0, $F1 $FB $FC $FD, entry $31, $F2 $FB
$F0). Each run gives `apu.bin` (every SPC700 write with its DSP sample time) and `sound.raw`.

**Voice events, tick for tick** (`sound_check`): the native driver given each command when the original's took it
and stepped when the original's ticked; before every tick, every voice's sample, envelope, pitch and volumes and echo,
the whole mix, and the key-ons (with their samples) and key-offs since the last tick, against the original's DSP
registers. "Same" means every one of those equal at every tick. Not compared: when inside a tick each write lands.

**Sound** (`sound_play`, then `sound_compare`): the native driver on the engine to the original's schedule (each tick
at the sample the original's began), headless with `--sound`, against the original's sound, both counted from
power-on, from the driver's start (sample 147,480). And, to separate the engine's mixer from the driver,
`dsp_replay`: the original's own DSP writes, each before the sample after the one it was made in, on the engine.
"Delayed" is the native delayed by the 0-63 samples that leave the least difference; the difference is its energy over
the original's.

| Run | Ticks | Ticks different | Key-ons | Key-offs | Samples | Samples different | First difference | Loudness | Delayed, difference | Original's writes on the engine: loudness | its difference |
|---|---|---|---|---|---|---|---|---|---|---|---|
| effects | 25421 | 0 | 690 | 1450 | 6555388 | 2810302 | 197596 | 0.901-1.136 | 11.1 % | 0.970-1.029 | 4.3 % |
| mixed | 8639 | 0 | 1082 | 1182 | 2260557 | 2050508 | 197596 | 0.944-1.011 | 41.0 % | 0.986-1.008 | 8.8 % |
| song_10 | 7578 | 0 | 124 | 150 | 1987603 | 141464 | 197596 | 0.993-1.009 | 23.4 % | 0.998-1.000 | 2.7 % |
| song_11 | 7577 | 0 | 674 | 696 | 1987734 | 1925444 | 197596 | 0.994-1.038 | 39.0 % | 0.997-1.002 | 7.2 % |
| song_12 | 7577 | 0 | 99 | 128 | 1987602 | 300935 | 197596 | 0.994-1.009 | 41.9 % | 0.999-1.002 | 5.3 % |
| song_13 | 7577 | 0 | 1324 | 1346 | 1989841 | 1932660 | 197596 | 0.949-1.034 | 44.8 % | 0.983-1.020 | 4.6 % |
| song_14 | 7578 | 0 | 74 | 101 | 1987604 | 166953 | 197596 | 0.984-1.009 | 76.3 % | 0.923-1.006 | 54.6 % |
| song_15 | 7577 | 0 | 1305 | 1322 | 1989886 | 1939152 | 197596 | 0.986-1.011 | 55.5 % | 0.993-1.003 | 6.9 % |
| song_16 | 7578 | 0 | 753 | 772 | 1988665 | 1938305 | 197596 | 0.983-1.023 | 53.3 % | 0.997-1.003 | 6.9 % |
| song_17 | 7576 | 0 | 1485 | 1503 | 1988665 | 1937855 | 197596 | 0.983-1.009 | 56.4 % | 0.996-1.004 | 10.7 % |
| song_18 | 7576 | 0 | 982 | 1004 | 1990797 | 1939605 | 197596 | 0.987-1.009 | 47.7 % | 0.998-1.002 | 5.0 % |
| song_19 | 7577 | 0 | 746 | 763 | 1988931 | 1938220 | 197596 | 0.990-1.011 | 62.4 % | 0.997-1.002 | 16.5 % |
| song_1A | 7577 | 0 | 978 | 999 | 1989336 | 1938943 | 197596 | 0.988-1.020 | 44.9 % | 0.996-1.002 | 6.7 % |
| song_1B | 7577 | 0 | 623 | 643 | 1989846 | 1939464 | 197596 | 0.962-1.093 | 85.4 % | 0.984-1.003 | 8.8 % |
| song_1C | 7578 | 0 | 313 | 335 | 1987604 | 1925568 | 197596 | 0.818-1.306 | 90.6 % | 0.870-1.119 | 17.8 % |
| song_1D | 7578 | 0 | 462 | 484 | 1987603 | 1925466 | 197596 | 0.884-1.166 | 57.5 % | 0.872-1.140 | 36.7 % |
| song_1E | 7576 | 0 | 956 | 978 | 1987603 | 1925312 | 197596 | 0.953-1.032 | 76.8 % | 0.973-1.018 | 25.4 % |
| song_1F | 7576 | 0 | 124 | 146 | 1987603 | 1932962 | 197596 | 0.983-1.017 | 124.8 % | 0.990-1.006 | 73.6 % |
| song_20 | 7578 | 0 | 68 | 95 | 1987603 | 90435 | 197596 | 0.970-1.009 | 66.0 % | 0.999-1.000 | 10.3 % |
| song_21 | 7578 | 0 | 1700 | 1718 | 1988665 | 1938118 | 197596 | 0.988-1.045 | 59.2 % | 0.998-1.002 | 15.6 % |
| song_22 | 7577 | 0 | 790 | 810 | 1988288 | 1938026 | 197596 | 0.984-1.009 | 66.4 % | 0.996-1.003 | 15.1 % |
| song_23 | 7578 | 0 | 163 | 187 | 1987603 | 322511 | 197596 | 0.994-1.009 | 117.5 % | 0.994-1.005 | 118.5 % |
| song_24 | 7578 | 0 | 510 | 531 | 1988665 | 1937363 | 197596 | 0.787-1.492 | 99.2 % | 0.936-1.066 | 15.6 % |
| song_25 | 7578 | 0 | 785 | 804 | 1987603 | 1936969 | 197596 | 0.997-1.009 | 33.0 % | 0.998-1.001 | 7.4 % |
| song_26 | 7576 | 0 | 827 | 849 | 1990798 | 1903582 | 197596 | 0.948-1.238 | 78.5 % | 0.983-1.013 | 3.8 % |
| song_27 | 7576 | 0 | 160 | 188 | 1987604 | 302943 | 197596 | 0.993-1.009 | 40.9 % | 0.998-1.002 | 17.7 % |
| song_28 | 7578 | 0 | 81 | 108 | 1987604 | 77579 | 197596 | 1.009-1.043 | 58.9 % | 1.005-1.015 | 12.0 % |
| song_29 | 7577 | 0 | 1103 | 1120 | 1988321 | 1937724 | 197596 | 0.992-1.009 | 52.7 % | 0.999-1.002 | 9.1 % |
| song_2A | 7575 | 0 | 58 | 80 | 1987066 | 97628 | 197596 | 1.000-1.009 | 51.4 % | 0.998-1.000 | 27.7 % |
| song_2B | 7577 | 0 | 667 | 684 | 1987800 | 1937230 | 197596 | 0.981-1.028 | 160.7 % | 0.995-1.006 | 15.5 % |
| song_2C | 7578 | 0 | 65 | 88 | 1987603 | 149944 | 197596 | 0.999-1.009 | 49.6 % | 0.999-1.002 | 8.9 % |
| song_2D | 7576 | 0 | 740 | 757 | 1987603 | 1935051 | 197596 | 0.987-1.017 | 72.9 % | 0.994-1.004 | 10.0 % |
| song_2E | 7578 | 0 | 429 | 450 | 1987604 | 1937353 | 197596 | 0.995-1.011 | 96.6 % | 0.995-1.007 | 73.6 % |
| song_2F | 7578 | 0 | 600 | 621 | 1987604 | 1937422 | 197596 | 0.981-1.017 | 42.5 % | 0.983-1.024 | 13.1 % |
| song_30 | 7578 | 0 | 989 | 1011 | 1987603 | 1933252 | 197596 | 0.963-1.040 | 72.7 % | 0.985-1.016 | 24.5 % |
| song_31 | 7578 | 0 | 99 | 124 | 1987603 | 272746 | 197596 | 0.984-1.009 | 126.8 % | 0.999-1.004 | 21.2 % |
| song_32 | 7577 | 0 | 889 | 908 | 1987791 | 1937381 | 197596 | 0.955-1.021 | 101.3 % | 0.986-1.012 | 32.5 % |
| song_33 | 7578 | 0 | 62 | 85 | 1987603 | 121492 | 197596 | 0.998-1.009 | 26.0 % | 0.999-1.001 | 15.4 % |
| song_34 | 7578 | 0 | 161 | 185 | 1988132 | 485327 | 197596 | 0.950-1.016 | 61.2 % | 0.995-1.001 | 7.0 % |
| song_35 | 7576 | 0 | 497 | 519 | 1987603 | 1932926 | 197596 | 0.985-1.010 | 17.0 % | 0.998-1.003 | 5.2 % |
| song_36 | 7576 | 0 | 165 | 189 | 1987604 | 408734 | 197596 | 0.988-1.009 | 64.3 % | 0.997-1.002 | 10.6 % |

The power-on probe (900 frames of the game's own sound: its load, 2 songs, a fade-in) gave 1,110 ticks, 0 different,
109 key-ons, 137 key-offs. In all, 330,680 ticks, 0 different.

## What this shows, and what is not done

- The driver matches the original in every voice event at every tick on every song, every effect entry and the mixed
  run.
- The sound does not match sample for sample, and cannot as things are. In every run the first difference is at
  sample 197,596, in the game's own first song, where the native's first key-on comes at its tick's first sample and
  the original's 20 to 29 samples later (song $10: 10 at 20, 25 at 21, 19 at 28...), its SPC700 working through the tick.
  The native driver's calls all come at a tick's start, and the engine has no notion of when within a tick.
- The engine's mixer is not sample-exact against bsnes either: the original's own writes on the engine differ by
  2.7 % to 118.5 % of the original's energy (song $23), the best delay 0 or 1 sample. Whether that is the replay
  applying each write at a whole sample (bsnes takes a register at its own cycle within the sample) or the mixer
  itself is not established. Loudness agrees within 0.87-1.14 a second.
- Not done: the native game playing its own sound (no game logic calls the sound yet: SoundSetsLoad, SongPlay and the
  queue are called by the system and game areas when they land); `tools/nshvc-check` on the attract play (the game
  must run first). Not checked: power-on with the S-CPU's timing (each run here starts the native driver at the
  original's start), and SRCN changed between instrument and key-on audibly (`nshvc_voice_sample` is the call for it;
  its own test is nativeshvc's `tests/scenes/voice_sample.c`).

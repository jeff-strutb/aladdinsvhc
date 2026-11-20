# What the game needs from nativeshvc

Requests from the Aladdin conversion to the engine's owner, through Jeff. Each is general, for every game, and names
no game in the engine.

## 1. A voice's sample changed without a key-on (Jeff, 2026-11-19): API 6.6, `nshvc_voice_sample`

**What the game needed.** The sound driver sets a voice's instrument, and so its DSP source (SRCN), about one tick
(8 ms) before the note's key-on: `src/aladdin/audio/sound_driver.c:1324-1329` (VoiceInstrumentFrom's write of DSP
register 4, reached from sequencer command $08 and from the restore of a music voice after an effect,
`sound_driver.c:240-243`). On the SNES the old note, still fading, goes on into the new source's loop when it next
reaches its end, because the DSP reads the current source's directory entry there. API 6.5 changed a voice's sample
only at key-on (`nshvc_voice_key_on`), so the old note would have kept its own loop.

**What the engine gained.** `void nshvc_voice_sample(uint32_t v, nshvc_sample s)`: the voice's sample from then on.
Key-on starts it, and a playing voice goes on into its loop point, its filter history kept, when it next ends the
block with the end flag. `nshvc_voice_key_on` sets the voice's sample too. In nativeshvc: `include/nshvc/nshvc.h`,
`src/engine/sound.c` (a voice's `source` in place of the sample queued for key-on), `docs/api.md` (section 6.3 and the
stability rules), `README.md`, `docs/native-engine-plan.md`, `tests/scenes/voice_sample.c`.

**API version.** 6.6.

**Jeff's approval**, 2026-11-19, in this session, answering whether to accept the difference or add an engine call:
"if you cloned nativesvhc, you can add this engine call in nativesvhc as a new spec and i'll commit. I own nativesvhc
so that's fine (if it's proper console architecture)", then "we already cloned nativesvhc in
projects\strutb\nativesvhc. do it there, properly". The change was made in that checkout and left uncommitted, for
him to commit.

**How it was checked on the engine's own tests.** `tests/scenes/voice_sample.c`, headless on Linux: a voice keyed on
sample A and given sample B before A's first end is the same, level, output and end flag, in each of 64,079 samples,
as a voice playing one sample made of A's blocks then B's looped ones; a control voice left on A differs in 58,016,
so the test sees the change; the run's left (the switched voice) and right (the joined sample) are the same in all
64,080 samples at full precision (`voice_sample --check`). Not run: the other examples' sound before and after.

**Where the game uses it.** `native/aladdin/src/audio/driver.c` (`out_source`, from `instrument_set` and
`driver_start`). The game's build needs API 6.6.

## Considered, no request

- **The echo buffer's address (ESA).** The driver sets it (`sound_driver.c:1623`), and the engine has no call for
  it. Jeff asked what is right for the engine: the engine has no sound memory, so an echo address means something
  only where the echo shares memory with data, which the engine's model does not have; the delay (EDL), the buffer's
  size, is the console capability, and `nshvc_echo` takes it. The original's buffer ($E800-$FFFF) overlaps none of the
  sound data, which ends at $E7F9, so dropping it changes no sound.
- **When inside a driver tick a voice change lands.** The original's writes fall over its tick's SPC700 time (its
  key-ons 20-29 samples after the tick begins); the native driver's calls come at a tick's start. That is the
  original's processor speed, not a console capability: no request (`docs/aladdin/native/SOUND.md`).
- **The engine's mixer against bsnes.** The original's own DSP writes, given to the engine before the sample after
  the one each was made in, differ from bsnes's output by 2.7 % to 118.5 % of the energy per run
  (`docs/aladdin/native/SOUND.md`). Whether that is the whole-sample timing of the replay or the mixer is not
  established; it is for the engine's owner to look at, and no request is made until it is.

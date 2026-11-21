# Departures: accidents of the original the native game leaves out

For Jeff's review. A line is here only when leaving the accident out does not hurt gameplay, with the evidence.
Rule (Jeff, 2026-11-20): an accident is not reproduced unless leaving it out hurts gameplay.

## 1. A ledge that faces the wrong way clears the fraction of his horizontal speed

- **Where:** `player/player_states.c`, `ledge_grab` (the record's 81:D470 PlayerLedgeGrab, its jump to 81:D0E0 PlayerAirLand).
- **What the original does:** when he falls beside a ledge tile whose mirror does not suit his facing, he does not catch it and goes on in the air
  (PlayerAirLand). The code before that jump leaves the CPU's accumulator 16 bits wide, so the first instruction of the map's ActorOutOfGround,
  `STZ` on his +$18 (the probe number), stores two bytes and also clears +$19, the fraction byte of his horizontal speed. A speed of $0180
  (1.5 pixels a frame) becomes $0100. Nothing in the game's design says so; it is a side effect of the register width.
- **What the native game does:** nothing: his speed stays $0180 (his speed is not touched on that path).
- **Why it does not hurt gameplay:** (a) the original's own scripted plays do not reach the path: the native run of the attract demo
  (12,000 updates, a counter on the path) and of the ending's stage play-backs (`PROBE_ENTRY=44 PROBE_ITEMS=60`, 30,000 updates, stages 4 to $0B)
  made zero visits; (b) in free play the only effect is half a pixel a frame less speed across for the frames until the air's speed rule
  (+$20 a frame, to the top speed $0180) brings it back: 4 frames in the case seen (rand_pw4_2 frame 2005).
- **The check's expected difference:** `rand_pw4_2` (level $0B, kind $00 alone, frames 1,107-2,235): from frame 2005 `speed_x` differs ($0180 native,
  $0100 original; equal again after 7 frames) and `x` from 2006 (native ahead by $80 a frame, a constant offset after, until the hit at 2024
  puts his record from the dump); no other field of his differs besides the hand-over frames (a hit at 2024) and the exec frame (2234). Observed
  2026-11-20: 37 fields over frames 2005-2024, 2234, 2235: speed_x 7, x 21, y 2, speed_y 2, hand-over and exec fields.

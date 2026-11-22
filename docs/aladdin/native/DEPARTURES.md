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

## 2. The platform course maker (kind $47) stalls the game for 85 frames at level $05's start

- **Where:** `actor/course_platform.c`, `PlatformCourseStart` (the record's 83:C96E): the course maker's first turn.
- **What the original does:** the maker works its whole course out in one turn without yielding (a waypoint every 38 steps from x $2A to x $1000),
  which takes the original 85 frames of the CPU: from frame 1,142 to 1,227 of `rand_stage_a1_s1` (build/aladdin/wip-kinds-d/work/rec.keepsstage_a1_s1)
  the maker's record stays in state $08 (running), the next record (kind $70) stays new ($0C), and Aladdin's and the guard's records and all of work RAM
  but 26 bytes are unchanged between frames 1,150 and 1,200: the game does not advance while it computes.
- **What the native game does:** computes the same course in the one frame, then goes on; the platforms start 85 frames of play earlier than the original's.
- **Why it does not hurt gameplay:** nothing moves during the original's stall (the player and the guard are the same in every dump of it), so the game
  after it is the same, 85 frames sooner; the course itself is the same waypoints.
- **The check's expected difference:** that play's window from 1,142 on: kinds $47, $70, $08 differ for the whole window by the 85 frames (their
  frame-to-frame offset); not comparable without shifting the dump. Level $05's other kinds are unaffected.

## 3. A carrier-pair sender keeps its own kind (the original turns it into kind $53)

- **Where:** `actor/carrier_pair.c` `CarrierPairSend` (the record's 84:A056), called by kind $97's sender (`actor/kind97.c`).
- **What the original does:** before each of its two creations it stores the new kind in its own +$06 (`LDA #$38 / STA $06`, then `#$53`), where only the
  creation routine's A needed the kind, and never puts $97 back. After the first pair its record is a kind $53 (a thing the carrier holds) that runs
  kind $97's code: in `rand_stage_a5_s1` record 9 is kind $97 at frame 2054 and $53 from 2055 (`build/aladdin/wip-kinds-d/work/rec.keepsstage_a5_s1`,
  +$06 $97 -> $53 and `kind_count[$53]`, `[$38]` up with the pair); `rand_stage_a5_s2` the same at 3219.
- **What the native game does:** the sender stays kind $97.
- **Why it does not hurt gameplay:** the sender's flags are 0 (no wait routine, no sprite, it cannot be hit), so nothing runs or draws by its kind byte; its
  code goes on by its resume point, not its kind. The only things that read +$06 of a record are its hit box, its animation set, its wait routine and
  the kind counts when it ends; the first three are unused for a sender, and nothing in the game reads `kind_count` to limit a kind ($97's or $53's):
  the native tree has no reader of it.
- **The check's expected difference:** kind $97 and kind $53 on `rand_stage_a5_s1` (from 2055) and `a5_s2` (from 3219): the sender's `kind` (+$06) $97
  native, $53 original, then everything the check derives from the kind (its comparison as kind $53). The records of the pair's carrier and thing
  ($38, $53) are exact.

## 4. Starting a song costs the starting task one video frame in the original (SongPlay uploads to the sound processor)

- **Where:** `audio/` `SongPlay` (81:FA23): the record's code uploads the song's list to the SPC700 through `SpcUpload` (sound commands held, `SOUND_HOLD`); the native game starts the song at once.
- **What the original does:** the upload takes video time inside the caller's turn. Seen twice on the original: (a) p3-0 (level 8) when the player's life-loss state starts song $2A, one video frame in which the play-time tick does not step (four video frames of frozen player, three logic frames); (b) p1-1, level 5's closing scene: the scene task (slot 3 in the original) sets cue $BF and calls SongPlay($2F) in update 9072 and is still running (state $08, wait 0) when the frame is dumped; its next TaskWait($10) is then counted from the following frame, so cue $BD (set by the effect task created when that wait ends) comes 17 updates after $BF, 16 in the native game (original $BF 9072, $BD 9089; native 9072, 9088; the camera move and the figures $2E/$70/$93 follow one update earlier too). The HDMA channels are free then ($03B6 = $80, bits 4-6 free) and the effect task's first run is in the update it is created: not the cause.
- **What the native game does:** the song starts at once; a task that starts a song goes on in the same frame.
- **Why it does not hurt gameplay:** the cost is a hardware wait (the upload) that the native sound has no equivalent of; it moves a scripted scene or a life's loss by one frame. The player's control during such a frame is not lost (the original's own logic frame is simply late).
- **The check's expected difference:** one update (one logic frame) later in the original, per song started by a task or by an actor's state that then waits: the first difference of p1-1 at update 9089 (placements) and the actors that follow; p3-0's one lag frame at the player's death.

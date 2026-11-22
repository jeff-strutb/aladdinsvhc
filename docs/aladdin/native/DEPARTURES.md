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

**Every place a song is started while play is running** (`SongPlay` callers outside the still screens and the level load, 2026-11-21; "slot 0" = inside the main task's turn, i.e. the actor loop's frame: the extra video frame is then a LAG frame of the whole game, every actor and the player held one video frame together, nothing relative changes; "task" = a separate task's turn: the caller's own waits start one frame later than the actors'):
- the player's loss of a life, `player/player_states.c` loss_first, song $2A, and the carpet's `player/carpet.c`, $2A: slot 0. SEEN on the original: p3-0 update 4199 (the one lag frame, R1 in NATIVE-FAULTS.txt). The player is out of control (the fall of $B0 frames); the whole sequence is one video frame longer, nothing relative moves.
- the level's exit, `player/exits.c`, song $28: slot 0 (his turn); not measured; he is walking into the exit spot, out of control after it.
- kind $81 (`actor/kind81.c` 195, $22), kind $8F/$94 (`kind_8f_94.c`, $27), the chain's eighth piece after the fight (`chain_piece.c`, $36), kind $6B (`kind6b.c`, its song): slot 0 (an actor's turn): one lag frame for everything, as the life's loss; not measured. The chain's $36 comes after the boss is beaten (the scene after the fight).
- the level scenes started during play, `level/level03_scene.c` ($SONG_AFTER, $SONG_END), `level0d_scene.c`, `level10_scene.c`, `level05_end_scene.c` (first and second song): tasks; the scene's own waits go one frame late against the actors. SEEN: level 5's closing scene, p1-1: cue $BD 17 updates after $BF in the original, 16 natively; the scene is scripted (Aladdin does not control him there).
What the shift changes for the player: nothing he can act on in any of these places (each is a death, an exit or a scripted scene). Where it is a lag frame the whole game shifts by one frame together and everything after it is the same, one frame later; where it is a task, the scene's later steps (cues, the camera, figures placed) come one frame earlier natively, relative to the actors, by the one frame of the original's upload, and then everything after keeps that distance (p1-1: the camera and the figures follow BD, the player's 8 px difference at 9235 is the camera's one-frame head start). Only p3-0 (death) and p1-1 (level 5) were measured; the other lines follow from the code (SongPlay's SpcUpload with SOUND_HOLD) and are not measured.

## 5. Unpacking a screen's graphics takes the original's task about a hundred video frames; the native game unpacks at once

- **Where:** `video/unpack.c` `UnpackSet` and `UnpackGroup` (81:BAFB, 81:BB49), called by the title task (`game/title_task.c`, `UnpackSet(1)`, `(4)`, `($2F)` and the sets after) and by every screen and level load.
- **What the original does:** the unpack runs in the calling task's own turn, group by group, and takes the processor's time: with START pressed in the opening story, the title task is the only task running for about a hundred video frames (task table dumped at frames 10, 30, 60, 90, 120, 150, 180, 200 and 222 after START: only slot 0, state 8 "running" at 90 and 120, waiting again from 150), and a 60-frame trace from frame 62 shows `UnpackGroup` entered 3,172 times; the screen is black meanwhile. Measured by mean brightness per picture (every 4 frames): black from 26 to 222 frames after START in the original (196 frames), from 32 to 128 updates after START natively (96); the fade out before, the 64-frame white flash and the ribbon and logo after it are the same (title intro matched at one constant offset, edges within 1 px).
- **What the native game does:** unpacks all of a set in the call; the next step of the task goes on in the same frame.
- **Why it does not hurt gameplay:** the cost is the original's processing time, which the native processor does not need; it only moves the title's white flash earlier by about 94 frames after START, before the menu is up (what the pad does on that black screen was not measured). The same unpack time exists at every level load, where the game is not in play; nothing in play unpacks.
- **The check's expected difference:** after any screen or level load, the native game's first frames come earlier than the original's by the original's unpack time (the title: about 94 frames after START in the opening story; a level load: the "load takes it about 59 frames longer" the plays' notes already name, `native/checks/plays/to_box.py`); recordings by update are lined up with the original's `lstart`/`lhold`, which wait for the level's first logic frame, so no later difference follows.

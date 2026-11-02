; story_scenes.asm: steps of the story that runs at $80:AF89, six scenes
; by slot 0's parameter (each set up by an entry of the table at
; $80:B092), faded in and out, with 64 frames between scenes. A scene may
; leave a sound command for its end.
; C: src/aladdin/game/story_scenes.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

; Slot 0's own data, read as the scene ends ($80:B02F): when nonzero, the
; parameter of sound command $F6 queued then, and the frames waited after
; it.
!SCENE_END_SOUND = $007C
!SCENE_END_WAIT = $007D
!STORY_SCENE_TASK = $81F344     ; the task the story's scenes start in slot 4
!STORY_FADE_IN_TASK = $819E8E   ; the screen's fade in, a task
!STORY_FADE_OUT_TASK = $819EB9  ; its fade out

; @routine 80:B0AE StoryScene2Start
; WHAT IT DOES: Sets up the story's second scene: sound command $F6 with
; parameter 2 to be queued as it ends, then a wait of 64 frames, and the
; scene's task at $81:F344 started.
; HOW: The two slot 0 bytes; the task created in slot 4 with parameter 3.
; Entry: M 8-bit, X 8-bit; called by JSR (through the table at
; $80:B092); DB a bank whose $0000-$1FFF is work RAM. Exit: as the C states.
%at($80B0AE)
StoryScene2Start:
    lda.b #$02
    sta.w !SCENE_END_SOUND
    lda.b #$40
    sta.w !SCENE_END_WAIT
    jsl.l TaskCreate
    db 4*24, 3                  ; slot 4, parameter 3
    dl !STORY_SCENE_TASK
    rts

; @routine 80:B17B StoryFadeInStart
; WHAT IT DOES: Starts the screen fading in, over the next frames, as a
; scene of the story begins.
; HOW: Creates the fade-in task ($81:9E8E) in slot 1, with parameter 4.
; Entry: any widths; called by JSR from the story's task; DB a bank whose
; $0000-$1FFF is work RAM. Exit: as the C states.
%at($80B17B)
StoryFadeInStart:
    jsl.l TaskCreate
    db 1*24, 4                  ; slot 1, parameter 4
    dl !STORY_FADE_IN_TASK
    rts

; @routine 80:B185 StoryFadeOutStart
; WHAT IT DOES: Starts the screen fading out, over the next frames, as a
; scene of the story ends.
; HOW: Creates the fade-out task ($81:9EB9) in slot 1, with parameter 4.
; Entry: any widths; called by JSR from the story's task; DB a bank whose
; $0000-$1FFF is work RAM. Exit: as the C states.
StoryFadeOutStart:
    jsl.l TaskCreate
    db 1*24, 4                  ; slot 1, parameter 4
    dl !STORY_FADE_OUT_TASK
    rts

; @routine 80:B18F StoryWait64Frames
; WHAT IT DOES: Makes the story's task wait 64 frames between scenes,
; letting the other tasks run.
; HOW: TaskWait with A = 64.
; Entry: M 8-bit; called by JSR from the story's task. Exit: as the C states.
StoryWait64Frames:
    lda.b #$40
    jsl.l TaskWait
    rts


; The scenes' actors: per entry (6 bytes), the kind (0: the player), the
; byte for its $07, then x and y (words).
!scene_actors = $DB5D           ; data bank $80
!SPAWN_X = $002C                ; where ActorSpawnAt places the actor: x, then y (words)
!SPAWN_Y = $002E

; @routine 80:B196 StoryActorPlace
; WHAT IT DOES: Places entry X of the story scenes' actors ($80:DB5D): a
; new actor of its kind, or, for kind 0, the player.
; HOW: The entry's x and y into the spawn words. A kind: in A's high
; byte, the $07 byte in its low, ActorSpawnAtLong. Kind 0: the player's
; $06 cleared, its $07 the byte, the low bytes of x and y its whole
; pixels' low bytes with their fractions cleared, and its state $0C (new).
; Entry: M 8-bit, X 8-bit; X = the entry's offset; DB = $80; called by
; JSR. Exit: M 8-bit, X 8-bit.
%at($80B196)
StoryActorPlace:
    rep.b #$20
    lda.w !scene_actors+2,x
    sta.w !SPAWN_X
    lda.w !scene_actors+4,x
    sta.w !SPAWN_Y
    sep.b #$20
    lda.w !scene_actors,x
    beq .player
    xba
    lda.w !scene_actors+1,x
    jsl.l ActorSpawnAtLong
.done:
    sep.b #$30
    rts
.player:
    stz.w !player+!actor_06
    lda.w !scene_actors+1,x
    sta.w !player+!actor_07
    ldx.w !SPAWN_X
    stx.w !player+!actor_x+1
    stz.w !player+!actor_x
    ldx.w !SPAWN_Y
    stx.w !player+!actor_y+1
    stz.w !player+!actor_y
    lda.b #$0C
    sta.w !player+!actor_state
    bra .done

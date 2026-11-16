; kind81_scene.asm: actor kind $81, which runs its stage's closing scene
; once Aladdin stands on firm ground at x $1E0 or more.
; C: src/aladdin/actor/kind81_scene.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"
incsrc "../include/actor.inc"

arch 65816

!player_x = !player+!actor_x+1  ; the player's x, whole pixels (word)
!player_code = !player+!actor_03 ; where the player's code goes on (word)
!SCENE_ON = $03FA               ; counted up while the scene runs; 0 again once the chain is built
!V_1A10 = $1A10                 ; counted up (word) as the scene starts
!V_1A05 = $1A05                 ; counted up (word) as the scene starts
!VIEW_X = $1773                 ; the view's left edge (pixels, word)
!V_1787 = $1787                 ; cleared once the view is in place
!V_18DD = $18DD                 ; cleared once the view is in place
!CUE_1B1F = $1B1F               ; the scene goes on once this is nonzero
!V_03A0 = $03A0                 ; $41 as the chain is built
!V_0399 = $0399                 ; $15 after the chain, and again after the flash
!V_0087 = $0087                 ; the scene waits for this to be 0
!V_083F = $083F                 ; $12 as the flash begins
!V_03A1 = $03A1                 ; $49 at the flash's height
!V_7FF047 = $7FF047             ; $0C as the scene ends
!SCENE_DONE = $1B18             ; counted up as the scene ends
!PLAYER_SCENE_CODE = $E126      ; the player's code for the scene
!SCENE_SHAKE_TASK = $81A1D5     ; ShakeTask, started in slot 1, parameter 0

; @routine 84:9391 Kind81Scene
; @actor
; WHAT IT DOES: Actor kind $81: waits until Aladdin stands on firm ground
; at x $1E0 or more, then takes over: his code goes to the scene's, the
; view scrolls right to $200, and once CUE_1B1F comes the chain of nine
; is built, the screen shakes, song $22 plays, the screen flashes white
; and fades back, and the actor ends.
; HOW: Each frame (BRK first) until PlayerOnFirmGround (C set) with his x
; $1E0 or more. SCENE_ON up, SoundQueueF6With1, his code $E126, V_1A10 and
; V_1A05 up. Each frame VIEW_X up by 3 until past $200, then $200;
; V_1787 and V_18DD cleared. Each frame until CUE_1B1F: SCENE_ON 0,
; ChainBuildNine, V_03A0 = $41, a yield, V_0399 = $15, COP $3F, TaskCreate
; (slot 1, ShakeTask), sound $4A; each frame until V_0087 is 0; COP $3F;
; SongPlay $22; V_083F = $12; the fixed colour black, colour math off,
; WOBJSEL $80, CGADSUB 2; each frame the colour up by one to $1F; sound
; $46, V_03A1 = $49, VRAM upload $0A, a yield, upload $0B; COP 8; HDMAEN
; $0E, V_0399 = $15; each frame the colour down by one to 0; CGWSEL $30,
; V_7FF047 = $0C, SCENE_DONE up; ActorEndPlaced.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
; $0000-$1FFF is work RAM; reached by the actor loop's jump for kind $81
; in state $0C (the table at $83:8DAB, through the stub at $83:92C2).
; Exit: never returns; ends in ActorEndPlaced.
%at($849391)
Kind81Scene:
    brk.b #$00
    jsl.l PlayerOnFirmGround
    bcc Kind81Scene
    rep.b #$20
    lda.w !player_x
    cmp.w #$01E0
    sep.b #$20
    bcc Kind81Scene
    inc.w !SCENE_ON
    jsl.l SoundQueueF6With1
    rep.b #$20
    lda.w #!PLAYER_SCENE_CODE
    sta.w !player_code
    inc.w !V_1A10
    inc.w !V_1A05
.scroll:
    brk.b #$00
    clc
    lda.w !VIEW_X
    adc.w #$0003
    sta.w !VIEW_X
    lda.w #$0200
    cmp.w !VIEW_X
    bcs .scroll
    sta.w !VIEW_X
    sep.b #$20
    stz.w !V_1787
    stz.w !V_18DD
.cue:
    brk.b #$00
    lda.w !CUE_1B1F
    beq .cue
    stz.w !SCENE_ON
    jsr.w ChainBuildNine
    lda.b #$41
    sta.w !V_03A0
    brk.b #$00
    lda.b #$15
    sta.w !V_0399
    lda.b #$3F
    cop.b #$00
    jsl.l TaskCreate
    db 1*24, 0                  ; slot 1, parameter 0
    dl !SCENE_SHAKE_TASK
    lda.b #$4A
    jsl.l SoundQueueAdd
.quiet:
    brk.b #$00
    lda.w !V_0087
    bne .quiet
    lda.b #$3F
    cop.b #$00
    lda.b #$22
    jsl.l SongPlay
    lda.b #$12
    sta.w !V_083F
    stz.w !coldata_red
    stz.w !coldata_green
    stz.w !coldata_blue
    stz.w !cgwsel_shadow
    lda.b #$80
    sta.w !wobjsel_shadow
    lda.b #$02
    sta.w !cgadsub_shadow
.brighten:
    lda.b #$01
    cop.b #$00
    inc.w !coldata_red
    inc.w !coldata_green
    inc.w !coldata_blue
    lda.w !coldata_red
    cmp.b #$1F
    bne .brighten
    lda.b #$46
    jsl.l SoundQueueAdd
    lda.b #$49
    sta.w !V_03A1
    lda.b #$0A
    sta.w !vram_upload_number
    brk.b #$00
    lda.b #$0B
    sta.w !vram_upload_number
    lda.b #$08
    cop.b #$00
    lda.b #$0E
    sta.w !hdmaen_shadow
    lda.b #$15
    sta.w !V_0399
.darken:
    lda.b #$01
    cop.b #$00
    dec.w !coldata_red
    dec.w !coldata_green
    dec.w !coldata_blue
    lda.w !coldata_red
    bne .darken
    lda.b #$30
    sta.w !cgwsel_shadow
    lda.b #$0C
    sta.l !V_7FF047
    inc.w !SCENE_DONE
    jml.l ActorEndPlaced

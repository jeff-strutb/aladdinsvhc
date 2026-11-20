; chain_piece.asm: what the pieces of the nine-piece chain (kinds $74 and
; $75, behind its head of kind $73) do each frame they wait.
; C: src/aladdin/actor/chain_piece.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"
incsrc "../include/actor.inc"
incsrc "../include/wram_state.inc"
incsrc "../include/wram_player.inc"

arch 65816

!CHAIN_HEAD = $1B14             ; the head's record (word; child_chain.asm)
!CHAIN_UNTOUCHABLE = $1B2C      ; nonzero: no touch tests (the head's K73_UNTOUCHABLE)
!V_032D = $032D                 ; the high byte of the word at $032C
!STATE_GONE = $8C               ; the head's state once it is going (and so the pieces')
!PIECE_RESUME = $35             ; word: where the piece resumes, kept from actor_03
!HEAD_HIT_FRAMES = $1B29        ; frames left of the head's show of being struck (kind73.asm)
!HEAD_HEALTH = $11              ; the head's health (by X = CHAIN_HEAD); negative: beaten
!HEAD_ANIM = $1B20              ; the head's animation (kind73.asm's K73_ANIM)
!PIECE_NUMBER = $40             ; the piece's place in the chain (8: the one that ends the fight)
!PIECE_FRAMES = $44             ; frames left of a pause
!FIGHT_WON = $03FA              ; counted up by each piece as the fight ends
!V_1A16 = $1A16                 ; 8 as the scene after the fight starts
!SCENE_WAIT = $0087             ; nonzero while the scene's fade runs
!V_005D = $005D                 ; nonzero on a PAL console (RegionCheck): the other scene after
!PLAYER_CODE_AFTER = $E154      ; the player's code once the scene has him
!RISE_V = $1B19                 ; $D4, counted up to 0 as the piece rises
!RISE_SOUNDED = $1B31           ; set once the head has risen past y $B0 (sound $49)
!RISE_SPEED = $1B2D             ; 24-bit, fraction first: the piece's rise, down by $20 a frame
!RISE_1B30 = $1B30              ; $20
!CLOSE_COUNT = $1B32            ; counted up as the closing list starts
!HEAD_Y_PIXELS = $0F            ; (by X = CHAIN_HEAD) the head's y pixels word

; @routine 84:8A78 ChainPieceWork
; WHAT IT DOES: What the chain's pieces (kinds $74 and $75) do each frame
; they wait: animate, follow their path and the pieces after them, and,
; unless the chain is untouchable, hurt Aladdin on touch on odd counts of
; V_032D or turn his shots back on even ones; when the head goes into
; state $8C the piece goes with it; its $08 bits 4 and 7 follow the
; head's.
; HOW: AnimStep, ActorPathStep, ActorsFollowIf8; CHAIN_UNTOUCHABLE zero:
; V_032D bit 0 set ActorHurtsPlayerOnTouchLong, clear
; ActorTurnsShotBackLong. With X 16-bit = CHAIN_HEAD: the head's state
; $8C: the piece's state $8C, PIECE_RESUME = its $03 word, $09 bit 7
; cleared (ASL, LSR). SCRATCH = the head's $08 AND $90; the piece's $08 =
; its $08 AND $6F OR SCRATCH.
; Entry: M 8-bit, X 8-bit; D = the piece's record; DB a bank whose
; $0000-$1FFF is work RAM; called by JSL from the kinds' stubs ($83:926B,
; $83:9278) through the actor loop's wait table. Exit: M 8-bit, X 8-bit;
; A = the new $08, N and Z from it; X = the low byte of CHAIN_HEAD's
; record; Y as the touch tests left it, or the low byte of $03 when the
; head went; C clear when the head went, else from comparing $8C with the
; head's state.
%at($848A78)
ChainPieceWork:
    jsl.l AnimStep
    jsr.w ActorPathStep
    jsr.w ActorsFollowIf8
    lda.w !CHAIN_UNTOUCHABLE
    bne .head
    lda.w !V_032D
    and.b #$01
    beq .shots
    jsl.l ActorHurtsPlayerOnTouchLong
    bra .head
.shots:
    jsl.l ActorTurnsShotBackLong
.head:
    rep.b #$10
    ldx.w !CHAIN_HEAD
    lda.b #!STATE_GONE
    cmp.w !actor_state,x
    bne .bits
    sta.b !actor_state
    ldy.b !actor_03
    sty.b !PIECE_RESUME
    asl.b !actor_09
    lsr.b !actor_09
.bits:
    lda.w !actor_08,x
    and.b #$90
    sta.w !scratch
    lda.b !actor_08
    and.b #$6F
    ora.w !scratch
    sta.b !actor_08
    sep.b #$10
    rtl

; @routine 84:8AC2 ChainPieceGone
; @actor
; WHAT IT DOES: A piece of the chain while the head shows being struck:
; it flashes with the head; then, with the head still standing, goes back
; to what it was doing (ChainPieceEnd); with the head beaten, the fight is
; won: FIGHT_WON up, and all but the last piece wait for good; the last
; (PIECE_NUMBER 8) runs the scene after the fight: two lists of children,
; the shake, sound and riser tasks, the fade waited on (moving down half a
; pixel a frame), then, with Aladdin still alive, his code set to
; PLAYER_CODE_AFTER and a wait; the song $36; it rises at a speed that
; slows ($1B2D) until it turns, sound $49 once the head is above y $B0;
; three closing lists and the brightness task; once the fade is done
; TaskExec to the scene that follows ($80:AF42, or $84:AFCC on a PAL
; console).
; HOW: Each frame (BRK) ActorLookFlash, ActorParityBitsSet,
; ActorsFollowIf8 until HEAD_HIT_FRAMES 0. The head's HEAD_HEALTH (X
; 16-bit = CHAIN_HEAD) positive: on into ChainPieceEnd. Negative: FIGHT_WON
; up; actor_09 OR $80; PIECE_NUMBER not 8: ActorYieldForever. 8:
; ActorListStart $89CD (4), BRK until ActorListFindIdle; ActorListStart
; $89BB ($10); V_1A16 8; TaskCreate slots 1-4 (ShakeTask 4,
; SoundWhileSlot1Task, RiserCoursesTask, $81:A2FF); each frame (BRK) its y
; + $80 (24-bit) until SCENE_WAIT 0; health 0: BRK for good; BRK until
; v_1428; the player's code word PLAYER_CODE_AFTER, his $09 bit 7 and $08
; bit 4 cleared; COP $BD, BRK, SongPlay $36, COP $FF twice; RISE_V $D4,
; RISE_SOUNDED 0, x $02C8, y $0150, RISE_SPEED $FFFA47, RISE_1B30 $20;
; each frame (BRK) y + RISE_SPEED, RISE_SPEED + $20, the head above y $B0
; the first time: sound $49; RISE_V nonzero: up; until RISE_SPEED's top
; byte is not negative. HEAD_ANIM 0, actor_18 1, ActorListStart $89D1 (1),
; BRK until idle; HEAD_ANIM 1, ActorListStart $89C9 (1), BRK until
; StepCountDown gives 0, BRK until idle; $3F frames; HEAD_ANIM 6,
; ActorListStart $8A2E ($10), sound $49, CLOSE_COUNT up, BRK until
; StepCountUp gives 0, BRK until idle; HEAD_ANIM 6; TaskCreate slot 1
; BrightnessDownTask 4; BRK until SCENE_WAIT 0; TaskExec.
; Entry: M 8-bit, X 8-bit; D = the piece's record; DB a bank whose
; $0000-$1FFF is work RAM and that holds the lists (bank $84); reached by
; the actor loop's jump for the piece in state $8C (ChainPieceWork puts it
; there with the head). Exit: never returns.
%at($848AC2)
ChainPieceGone:
    brk.b #$00
    jsr.w ActorLookFlash
    jsr.w ActorParityBitsSet
    jsr.w ActorsFollowIf8
    lda.w !HEAD_HIT_FRAMES
    bne ChainPieceGone
    rep.b #$10
    ldx.w !CHAIN_HEAD
    lda.w !HEAD_HEALTH,x
    sep.b #$10
    bmi ChainPieceEnd_won

; @routine 84:8ADE ChainPieceEnd
; @actor
; WHAT IT DOES: A piece of the chain (or its head) goes back to what it
; was doing before it was struck: its resume address from PIECE_RESUME,
; its $08 bit 4 and $09 bit 7 set, and the actor loop goes on.
; HOW: actor_03/actor_04 = PIECE_RESUME; actor_08 OR $10; actor_09 OR
; $80; JML ActorLoopNext.
; Entry: M 8-bit, X 8-bit; D = the actor's record; reached from
; ChainPieceGone and Kind73Hit, and by the actor loop's jump. Exit: never
; returns (into the actor loop).
%at($848ADE)
ChainPieceEnd:
    lda.b !PIECE_RESUME
    sta.b !actor_03
    lda.b !PIECE_RESUME+1
    sta.b !actor_04
    lda.b !actor_08
    ora.b #$10
    sta.b !actor_08
    lda.b !actor_09
    ora.b #$80
    sta.b !actor_09
    jml.l ActorLoopNext
.won:
    inc.w !FIGHT_WON
    lda.b !actor_09
    ora.b #$80
    sta.b !actor_09
    lda.b !PIECE_NUMBER
    cmp.b #$08
    beq .last
    jmp.w ActorYieldForever
.last:
    ldx.b #$CD
    ldy.b #$89
    lda.b #$04
    jsr.w ActorListStart
.first_list:
    brk.b #$00
    jsr.w ActorListFindIdle
    bcc .first_list
    ldx.b #$BB
    ldy.b #$89
    lda.b #$10
    jsr.w ActorListStart
    lda.b #$08
    sta.w !V_1A16
    jsl.l TaskCreate
    db 1*24, 4
    dl ShakeTask
    jsl.l TaskCreate
    db 2*24, 0
    dl SoundWhileSlot1Task
    jsl.l TaskCreate
    db 3*24, 0
    dl RiserCoursesTask
    jsl.l TaskCreate
    db 4*24, 0
    dl $81A2FF
.fade:
    brk.b #$00
    clc
    lda.b !actor_y
    adc.b #$80
    sta.b !actor_y
    lda.b !actor_y+1
    adc.b #$00
    sta.b !actor_y+1
    lda.b !actor_y+2
    adc.b #$00
    sta.b !actor_y+2
    lda.w !SCENE_WAIT
    bne .fade
    lda.w !health
    bne .alive
.dead:
    brk.b #$00
    bra .dead
.alive:
    brk.b #$00
    lda.w !v_1428
    beq .alive
    lda.b #!PLAYER_CODE_AFTER&255
    sta.w !player+!actor_03
    lda.b #!PLAYER_CODE_AFTER>>8
    sta.w !player+!actor_04
    asl.w !player+!actor_09
    lsr.w !player+!actor_09
    lda.w !player+!actor_08
    and.b #$EF
    sta.w !player+!actor_08
    lda.b #$BD
    cop.b #$00
    brk.b #$00
    lda.b #$36
    jsl.l SongPlay
    lda.b #$FF
    cop.b #$00
    lda.b #$FF
    cop.b #$00
    lda.b #$D4
    sta.w !RISE_V
    stz.w !RISE_SOUNDED
    rep.b #$30
    lda.w #$02C8
    sta.b !actor_x+1
    lda.w #$0150
    sta.b !actor_y+1
    sep.b #$30
    lda.b #$47
    sta.w !RISE_SPEED
    lda.b #$FA
    sta.w !RISE_SPEED+1
    lda.b #$FF
    sta.w !RISE_SPEED+2
    lda.b #$20
    sta.w !RISE_1B30
.rise:
    brk.b #$00
    clc
    lda.b !actor_y
    adc.w !RISE_SPEED
    sta.b !actor_y
    lda.b !actor_y+1
    adc.w !RISE_SPEED+1
    sta.b !actor_y+1
    lda.b !actor_y+2
    adc.w !RISE_SPEED+2
    sta.b !actor_y+2
    clc
    lda.w !RISE_SPEED
    adc.b #$20
    sta.w !RISE_SPEED
    lda.w !RISE_SPEED+1
    adc.b #$00
    sta.w !RISE_SPEED+1
    lda.w !RISE_SPEED+2
    adc.b #$00
    sta.w !RISE_SPEED+2
    lda.w !RISE_SOUNDED
    bne .rise_v
    rep.b #$30
    ldx.w !CHAIN_HEAD
    lda.w !HEAD_Y_PIXELS,x
    cmp.w #$00B0
    sep.b #$30
    bcs .rise_v
    inc.w !RISE_SOUNDED
    lda.b #$49
    jsl.l SoundQueueAdd
.rise_v:
    lda.w !RISE_V
    beq .rise_on
    inc.w !RISE_V
.rise_on:
    lda.w !RISE_SPEED+2
    bmi .rise
    stz.w !HEAD_ANIM
    lda.b #$01
    sta.b !actor_18
    ldx.b #$D1
    ldy.b #$89
    lda.b #$01
    jsr.w ActorListStart
.close_1:
    brk.b #$00
    jsr.w ActorListFindIdle
    bcc .close_1
    lda.b #$01
    sta.w !HEAD_ANIM
    ldx.b #$C9
    ldy.b #$89
    lda.b #$01
    jsr.w ActorListStart
.close_2:
    brk.b #$00
    jsr.w StepCountDown
    bne .close_2
.close_2_idle:
    brk.b #$00
    jsr.w ActorListFindIdle
    bcc .close_2_idle
    lda.b #$3F
    sta.b !PIECE_FRAMES
.pause:
    brk.b #$00
    dec.b !PIECE_FRAMES
    bne .pause
    lda.b #$06
    sta.w !HEAD_ANIM
    ldx.b #$2E
    ldy.b #$8A
    lda.b #$10
    jsr.w ActorListStart
    lda.b #$49
    jsl.l SoundQueueAdd
    inc.w !CLOSE_COUNT
.close_3:
    brk.b #$00
    jsr.w StepCountUp
    bne .close_3
.close_3_idle:
    brk.b #$00
    jsr.w ActorListFindIdle
    bcc .close_3_idle
    lda.b #$06
    sta.w !HEAD_ANIM
    jsl.l TaskCreate
    db 1*24, 4
    dl BrightnessDownTask
.fade_out:
    brk.b #$00
    lda.w !SCENE_WAIT
    bne .fade_out
    lda.w !V_005D
    beq .ntsc
    jsl.l TaskExec
    dl $84AFCC
.ntsc:
    jsl.l TaskExec
    dl $80AF42

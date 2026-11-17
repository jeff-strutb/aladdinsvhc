; kind4f.asm: actor kind $4F, a thing that opens once when struck: unless
; the flag its $07 names in V_035A is already set (then it shows open), it
; waits to be struck, then sounds, opens and lets out its children.
; C: src/aladdin/actor/kind4f.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

!K4F_FRAMES = $40               ; frames left of a pause
!K4F_FLAG = $41                 ; the V_035A bit its $07 names
!K4F_OPEN = $42                 ; set when it is struck (by its wait), or at once when already open
!V_035A = $035A                 ; flags of things already opened
!K4F_FLAGS = $D366              ; by $07 AND 3: the bit (four bytes of kind $52's code at $83:D366, read as data)

; @routine 83:D01A Kind4FStart
; @actor
; WHAT IT DOES: Actor kind $4F: shown closed (pose 9) until struck, then
; sound $12, 5 frames, open (pose $0A), a ring of children, 5 frames, its
; variant's child, and it stays open; when the flag its $07 names is
; already set in V_035A it is shown open at once.
; HOW: ACTOR_08 bits 0-1, ACTOR_09 bits $90 and ACTOR_LOOK's high byte
; bit 1 set (TSB); K4F_OPEN 0; K4F_FLAG = K4F_FLAGS by $07 AND 3; ANDed
; with V_035A nonzero: K4F_OPEN up, AnimStart $0A, stays. Else AnimStart
; 9; BRK until K4F_OPEN; SoundQueueAdd $12, 5 frames (BRK); AnimStart
; $0A, ActorSpawnRing; 5 frames; ActorVariantChildStep; BRK for good.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $83 (the flag
; bytes' bank) with work RAM at $0000-$1FFF; reached by the actor loop's
; jump for kind $4F in state $0C (the table at $83:8CAB). Exit: never
; returns.
%at($83D01A)
Kind4FStart:
    lda.b #$03
    tsb.b !actor_08
    lda.b #$90
    tsb.b !actor_09
    lda.b #$02
    tsb.b !actor_look+1
    stz.b !K4F_OPEN
    lda.b !actor_07
    and.b #$03
    tax
    lda.w !K4F_FLAGS,x
    sta.b !K4F_FLAG
    and.w !V_035A
    beq .closed
    inc.b !K4F_OPEN
    lda.b #$0A
    jsl.l AnimStart
    jmp.w .stay
.closed:
    lda.b #$09
    jsl.l AnimStart
.until_struck:
    brk.b #$00
    lda.b !K4F_OPEN
    beq .until_struck
    lda.b #$05
    sta.b !K4F_FRAMES
    lda.b #$12
    jsl.l SoundQueueAdd
.sound_pause:
    brk.b #$00
    dec.b !K4F_FRAMES
    bne .sound_pause
    lda.b #$0A
    jsl.l AnimStart
    jsr.w ActorSpawnRing
    lda.b #$05
    sta.b !K4F_FRAMES
.open_pause:
    brk.b #$00
    dec.b !K4F_FRAMES
    bne .open_pause
    jsr.w ActorVariantChildStep
.stay:
    brk.b #$00
    bra .stay

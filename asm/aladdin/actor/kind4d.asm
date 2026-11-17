; kind4d.asm: actor kind $4D, a thing that gives way when it is hit,
; leaps and falls, then drops its children once. Drafted by cdraft.py;
; named and described by hand.
; C: src/aladdin/actor/kind4d.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

!K4D_CHECK = $701000             ; the copier check's bytes: work RAM on a copier, none on the cartridge
!K4D_FILL = $AA                  ; what a copier's low bytes are filled with
!K4D_FILLED = $0000              ; the bytes filled, $0000-$001F
!K4D_FRAMES = $40                ; frames left
!K4D_HITS = $42                  ; set when it is hit (1: the first time)

; @routine 83:CE7B Kind4DStart
; @actor
; WHAT IT DOES: Actor kind $4D, a thing that gives way when it is hit:
; once K4D_HITS is set it sounds $12, waits a frame (8 on the first hit),
; leaps up and falls for 53 frames (on the first hit facing away from the
; player), then in pose 2 is sent up again with sound $3C, drops its
; children once (ActorFlagOnce) and stays. First it counts the byte at
; $70:1000 + X up and, when the count stays written (work RAM there,
; which the cartridge does not have), fills bytes $0000-$001F with $AA:
; a copier check.
; HOW: The byte at K4D_CHECK + X up and compared; equal: X = $1C down to
; 0, $AA to $0003 + X, then to $0000 and $0001. ACTOR_08 bit 1 and
; ACTOR_09 bits $90 set (TSB); AnimStart 0; K4D_HITS 0. Each frame (BRK
; first) until K4D_HITS is nonzero. SoundQueueAdd $12; K4D_FRAMES = 8
; when K4D_HITS is 1, else 1; that many BRKs. AnimStart 1; ActorSpeedYSet
; with entry $7E; ACTOR_ACCEL_Y $38; K4D_FRAMES $35; K4D_HITS 1: facing
; and flip = the player's $15 EOR 1. Each frame (BRK first) ActorFall
; until K4D_FRAMES runs out. AnimStart 2; ActorSpeedYSet with entry $2A;
; SoundQueueAdd $3C; ACTOR_08 bits 0-1 set (TSB); ActorFlagOnce; BRK for
; good. No script runs the fill: written from its bytes.
; Entry: M 8-bit, X 8-bit; X as the actor loop's jump leaves it; D = the
; actor's record; DB = $80; reached by the actor loop's jump for kind $4D
; in state $0C (the table at $83:8CAB). Exit: never returns.
%at($83CE7B)
Kind4DStart:
    lda.l !K4D_CHECK,x
    inc a
    sta.l !K4D_CHECK,x
    cmp.l !K4D_CHECK,x
    bne .start
    ldx.b #$1C
    lda.b #!K4D_FILL
.fill:
    sta.w !K4D_FILLED+3,x
    dex
    bpl .fill
    sta.w !K4D_FILLED
    sta.w !K4D_FILLED+1
.start:
    lda.b #$02
    tsb.b !actor_08
    lda.b #$90
    tsb.b !actor_09
    lda.b #$00
    jsl.l AnimStart
    stz.b !K4D_HITS
.wait:
    brk.b #$00
    lda.b !K4D_HITS
    beq .wait
    lda.b #$12
    jsl.l SoundQueueAdd
    ldx.b #$01
    lda.b !K4D_HITS
    dec a
    bne .frames
    ldx.b #$08
.frames:
    stx.b !K4D_FRAMES
.pause:
    brk.b #$00
    dec.b !K4D_FRAMES
    bne .pause
    lda.b #$01
    jsl.l AnimStart
    ldy.b #$7E
    jsl.l ActorSpeedYSet
    lda.b #$38
    sta.b !actor_accel_y
    lda.b #$35
    sta.b !K4D_FRAMES
    lda.b !K4D_HITS
    dec a
    bne .fall
    lda.w !player+!actor_flip
    eor.b #$01
    sta.b !actor_facing
    sta.b !actor_flip
.fall:
    brk.b #$00
    jsl.l ActorFall
    dec.b !K4D_FRAMES
    bne .fall
    lda.b #$02
    jsl.l AnimStart
    ldy.b #$2A
    jsl.l ActorSpeedYSet
    lda.b #$3C
    jsl.l SoundQueueAdd
    lda.b #$03
    tsb.b !actor_08
    jsr.w ActorFlagOnce
.stay:
    brk.b #$00
    bra .stay

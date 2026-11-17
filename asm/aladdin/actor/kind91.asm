; kind91.asm: actor kind $91, a marker that steers the level's escort
; actor: by its $07 it sets the escort's mode once one has passed the
; other, or at once, or sets or clears one of the escort's flags, and ends.
; C: src/aladdin/actor/kind91.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

; @routine A5:F534 Kind91Start
; @actor
; WHAT IT DOES: Actor kind $91, a marker for the level's escort actor.
; By $07: 0, once the escort is idle and this marker is at or past the
; escort's x, the escort's mode 4 (and so each frame on); 1, once the
; escort is idle and at or past the marker's x, mode 5 (each frame on);
; 2, mode 6 if the escort is idle; 3, if the escort is busy: idle again,
; mode 1 and the escort moved to the marker's x and y; 4, ESCORT_46 set;
; 5, ESCORT_46 cleared. Variants 2 to 5 end at once.
; HOW: ACTOR_09 bit 7 set (its kind's wait routine runs, Kind91FreeOff);
; JMP through KIND91_VARIANTS by $07 * 2. Variants 0 and 1: each frame
; (BRK) ESCORT_45 tested; when 0, X 16-bit, the marker's x and the
; escort's compared (0: marker's - escort's, 1: escort's - marker's) and
; with no borrow ESCORT_MODE 4 or 5; M back to 8-bit (X stays 16-bit).
; Variants 2 to 5 end in ActorEndPlaced.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
; $0000-$1FFF is work RAM; reached by the JML of kind $91's stub
; ($83:936F) from the actor loop in the start state. Exit: never returns:
; 0 and 1 run on, 2 to 5 end in ActorEndPlaced.
%at($A5F534)
Kind91Start:
    lda.b #$80
    tsb.b !actor_09
    lda.b !actor_07
    asl a
    tax
    jmp.w (.variants,x)
.variants:
    dw .after_escort, .escort_after, .idle_mode6, .busy_mode1, .flag_set, .flag_clear
.after_escort:
    brk.b #$00
    lda.w !actor_escort+69
    bne .after_escort
    rep.b #$10
    ldx.b !actor_x+1
    cpx.w !actor_escort+!actor_x+1
    bcc .after_escort_not
    lda.b #$04
    sta.w !actor_escort+77
.after_escort_not:
    sep.b #$20
    bra .after_escort
.escort_after:
    brk.b #$00
    lda.w !actor_escort+69
    bne .escort_after
    rep.b #$10
    ldx.w !actor_escort+!actor_x+1
    cpx.b !actor_x+1
    bcc .escort_after_not
    lda.b #$05
    sta.w !actor_escort+77
.escort_after_not:
    sep.b #$20
    bra .escort_after
.idle_mode6:
    lda.w !actor_escort+69
    bne .idle_mode6_end
    lda.b #$06
    sta.w !actor_escort+77
.idle_mode6_end:
    jml.l ActorEndPlaced
.busy_mode1:
    lda.w !actor_escort+69
    beq .busy_mode1_end
    stz.w !actor_escort+69
    lda.b #$01
    sta.w !actor_escort+77
    rep.b #$20
    lda.b !actor_x+1
    sta.w !actor_escort+!actor_x+1
    lda.b !actor_y+1
    sta.w !actor_escort+!actor_y+1
    sep.b #$20
.busy_mode1_end:
    jml.l ActorEndPlaced
.flag_set:
    lda.b #$01
    sta.w !actor_escort+70
    jml.l ActorEndPlaced
.flag_clear:
    stz.w !actor_escort+70
    jml.l ActorEndPlaced

; kind49.asm: actor kind $49, which carries the player along as it moves.
; C: src/aladdin/actor/kind49.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

; @routine 84:804D Kind49Carry
; @actor
; WHAT IT DOES: Actor kind $49: each frame it carries the player along by
; however far it moved (its own moves are its kind's wait work).
; HOW: actor_09 bits $90 set (bit 7: its kind's routine runs each frame
; it waits); then each frame a yield and ActorCarryPlayerLong.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $83; reached by
; the actor loop's jump for kind $49 in state $0C (the table at $83:8CAB,
; through the stub at $83:923C). Exit: never returns.
%at($84804D)
Kind49Carry:
    lda.b !actor_09
    ora.b #$90
    sta.b !actor_09
.ride:
    brk.b #$00
    jsl.l ActorCarryPlayerLong
    bra .ride

; @routine 84:805B Kind49Wait
; WHAT IT DOES: What actors of kind $49 do each frame they wait: test
; whether they carry the player, with a box by their $07 ($34, $38 or
; $3C), and end when out of range.
; HOW: JMP through the table at $84:8062 by $07 * 2 to a part that loads
; Y with the box ($84:8068 for $07 0, which no script meets: LDY #$34);
; K49_CARRY_BOX = Y; X = 0, ActorTestCarriesLong; X = $18 and on into
; ActorFreeOutOfRangeLong (JML).
; Entry: M 8-bit, X 8-bit; D = the actor; DB = $80; called by JSL from
; the kind's stub ($83:9240) through the actor loop's wait table at
; $83:8F4F. Exit: as ActorFreeOutOfRangeLong.
%at($84805B)
Kind49Wait:
    lda.b !actor_07
    asl a
    tax
    jmp.w (.parts,x)
.parts:
    dw .box34, .box38, .box3c
.box34:
    ldy.b #$34
    bra .test
.box38:
    ldy.b #$38
    bra .test
.box3c:
    ldy.b #$3C
.test:
    sty.b !actor_4c
    ldx.b #$00
    jsl.l ActorTestCarriesLong
    ldx.b #$18
    jml.l ActorFreeOutOfRangeLong


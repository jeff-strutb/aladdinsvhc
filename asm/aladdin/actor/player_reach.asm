; player_reach.asm: whether the player is within an actor's reach, by an entry of the reach
; table. Drafted by cdraft.py; named and described by hand.
; C: src/aladdin/actor/player_reach.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"
incsrc "../include/wram.inc"

arch 65816

; In bank $80: 8 bytes an entry: the x reach, the y reach (bytes), ...
!REACH_TABLE = $D9B0

; @routine 83:FCF5 PlayerInReach
; WHAT IT DOES: Tells whether the player is within entry X's reach of the
; actor: C clear when in reach (a reach of 0 is not tested).
; HOW: The x reach r, nonzero: the actor's x pixels plus r less the
; player's compared unsigned with 2r; C set (out) returns. Then the y
; reach r, nonzero: its y pixels plus r less the player's compared with r.
; 16-bit A; $002C, $002E hold r and 2r.
; Entry: M 8-bit, X 8-bit; X = the entry's offset; D = the actor's record;
; DB = $80; called by JSR (by 83:FCA5). The game passes only entry 0,
; whose reaches are 0 (83:AED2 loads X = 8, then X = 0): the comparisons
; are reached only by unit_player_in_reach.box. Exit: M 16-bit; C clear:
; in reach; C set: out. A as the last comparison left it.
%at($83FCF5)
PlayerInReach:
    rep.b #$20
    clc
    lda.w !REACH_TABLE,x
    and.w #$00FF
    beq .lFD16
    sta.w !scratch+32
    asl a
    sta.w !scratch+34
    clc
    lda.b !actor_x+1
    adc.w !scratch+32
    sec
    sbc.w !player+!actor_x+1
    cmp.w !scratch+34
    bcs .lFD2B
.lFD16:
    lda.w !REACH_TABLE+1,x
    and.w #$00FF
    beq .lFD2B
    sta.w !scratch+32
    clc
    adc.b !actor_y+1
    sec
    sbc.w !player+!actor_y+1
    cmp.w !scratch+32
.lFD2B:
    rts

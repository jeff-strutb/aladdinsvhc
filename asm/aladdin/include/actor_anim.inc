; actor_anim.inc: actors' animations (actor/anim.asm): the fields of an
; actor's record they use, and the tables a level loads (the same names as
; the C's actor_anim.h, in lower case for addresses).

includeonce

; Record fields (offsets in the actor's record, its direct page; the
; others in actor.inc). !actor_06 picks its animation set too, and bit 3 of
; !actor_08 says it is animated.
!ACTOR_PICTURE_GFX = $22        ; the frame's picture: its graphics (word, from the set's base)
!ACTOR_ANIM_TABLE = $24         ; its set's animation table, in the animation data (word)
!ACTOR_FRAME_TIME = $26         ; frames the frame has left
!ACTOR_PICTURE = $27            ; the frame's picture; at a start, the animation's number
!ACTOR_REDRAW = $28             ; $FF: its picture changed
!ACTOR_ANIM = $2D               ; the animation's offset in its set's table (its number times 2)
!ACTOR_FRAME = $2F              ; the frame in the animation
!ACTOR_LOOP_BACK = $30          ; frames the animation steps back at its end
!ACTOR_GFX_BASE = $31           ; its set's graphics, in the picture list (word)

; Tables the level loads.
!anim_set_of_kind = $7FE000     ; per kind, its animation set (byte)
!anim_set_gfx = $7FE100         ; per set, its graphics base (word)
!anim_set_table = $7FE300       ; per set, its animation table (word)
!anim_data = $7EB000            ; animation tables, then each animation's frames: word per frame, time then picture; time 0 ends
!picture_list = $7E2000         ; per picture, its graphics' offset

; Actors' ends (actor/actor_end.asm).
!actor_0a = $0A
!actor_13 = $13
!actor_2c = $2C
!actor_33 = $33
!actor_38 = $38
!kind_count = $7EFF00           ; by !actor_06, how many such actors are live
!actor_pool2 = $15DA            ; the second pool's free records (words), a stack
!actor_pool2_top = $15E6        ; byte offset of its top entry
; Animated-tile slots (video/tile_slots.asm): by slot 0-3, the map cell
; that claimed it; by cell, its attribute word and how many more slots it
; holds after its first (0-2).
!tile_slot_cell = $15ED
!cell_attrs = $7FE700
!cell_extra_slots = $7FE900

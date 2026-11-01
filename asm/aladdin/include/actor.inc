; actor.inc: an actor's record, as its code reaches it through the direct
; page (D = the record's address); the same names as the C's actor.h, in
; lower case. 35 records of $50 bytes from $092B. Positions and speeds are
; 24-bit, little-endian: a fraction byte, then a 16-bit whole part.

includeonce

; The records: the player's first, then the others.
!player = $08DB                 ; Aladdin's record
!actor_free_list = $15A2        ; addresses of the free records (words), a stack
!actor_free_top = $15E4         ; byte offset of its top entry; negative: none free

!actor_state = $00              ; what the actor is doing ($8C, $CC ...)
!actor_07 = $07
!actor_08 = $08                 ; low two bits: set from the frame's parity by some actors
!actor_09 = $09                 ; bits 4-5: the sprite priority some actors set by height
!actor_x = $0B                  ; x position: fraction, then pixels (word)
!actor_y = $0E                  ; y position: fraction, then pixels (word)
!actor_facing = $14             ; 0: facing right (x grows), else left
!actor_15 = $15                 ; the player's: nonzero, the reach test's box sits 10 pixels nearer
!actor_18 = $18                 ; zero: idle, for the list walk at $84:8A52
!actor_speed_x = $19            ; horizontal speed, 24-bit, along the facing
!actor_accel_x = $1C            ; added to or taken from the horizontal speed (fraction)
!actor_speed_y = $1D            ; vertical speed, 24-bit (down is positive)
!actor_accel_y = $20            ; added to or taken from the vertical speed: gravity (fraction)
!actor_next = $37               ; the next record in the actor's list (word; 0 ends it)
!actor_40 = $40                 ; word
!actor_44 = $44
!actor_45 = $45
!actor_46 = $46
!actor_47 = $47
!actor_48 = $48

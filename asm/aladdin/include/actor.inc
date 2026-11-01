; actor.inc: an actor's record, as its code reaches it through the direct
; page (D = the record's address); the same names as the C's actor.h, in
; lower case. 35 records of $50 bytes from $092B. Positions and speeds are
; 24-bit, little-endian: a fraction byte, then a 16-bit whole part.

includeonce

!actor_x = $0B                  ; x position: fraction, then pixels (word)
!actor_y = $0E                  ; y position: fraction, then pixels (word)
!actor_facing = $14             ; 0: facing right (x grows), else left
!actor_speed_x = $19            ; horizontal speed, 24-bit, along the facing
!actor_accel_x = $1C            ; added to or taken from the horizontal speed (fraction)
!actor_speed_y = $1D            ; vertical speed, 24-bit (down is positive)
!actor_accel_y = $20            ; added to or taken from the vertical speed: gravity (fraction)

; actor.inc: an actor's record, as its code reaches it through the direct
; page (D = the record's address); the same names as the C's actor.h, in
; lower case. 35 records of $50 bytes from $092B. Positions and speeds are
; 24-bit, little-endian: a fraction byte, then a 16-bit whole part.

includeonce

; The records: the player's first, then the others.
!player = $08DB                 ; Aladdin's record
!actor_free_list = $15A2        ; addresses of the free records (words), a stack
!actor_free_top = $15E4         ; byte offset of its top entry; negative: none free
!actor_tile_slot_users = $15E8  ; actors holding each animated-tile slot (byte per slot)
!actor_records = $08DB          ; every record, the player's first: $AF1 bytes
!actor_escort = $092B           ; the record of the level's escort actor
!actor_free_list_b = $15DA      ; a second free list: the 5 records from $09CB
!actor_free_top_b = $15E6       ; its top entry's offset
!actor_pool = $0B5B             ; the 28 records the first free list holds, $50 bytes apart

!actor_state = $00              ; 0 free, $0C new, then what the actor is doing ($8C, $CC ...)
!actor_01 = $01
!actor_03 = $03
!actor_04 = $04
!actor_06 = $06                 ; its hit box: an entry of the table at $C29A; a spawner sets its child's
!actor_07 = $07                 ; passed from a spawner to its child
!actor_08 = $08                 ; low two bits: set from the frame's parity by some actors
!actor_09 = $09                 ; bits 4-5: the sprite priority some actors set by height; bit 6 set: it can be hit
!actor_x = $0B                  ; x position: fraction, then pixels (word)
!actor_y = $0E                  ; y position: fraction, then pixels (word)
!actor_facing = $14             ; 0: facing right (x grows), else left
!actor_flip = $15               ; the facing's opposite: bit 0 set when facing right (the player's: nonzero, the reach test's box sits 10 pixels nearer)
!actor_12 = $12
!actor_18 = $18                 ; zero: idle, for the list walk at $84:8A52
!actor_speed_x = $19            ; horizontal speed, 24-bit, along the facing
!actor_accel_x = $1C            ; added to or taken from the horizontal speed (fraction)
!actor_speed_y = $1D            ; vertical speed, 24-bit (down is positive)
!actor_accel_y = $20            ; added to or taken from the vertical speed: gravity (fraction)
!actor_spawn_at = $21           ; where it spawns a child: index into the spawn offsets
!actor_look = $2B               ; sprite attributes (word): tile base, palette
!actor_33 = $33                 ; in the 28-record pool, its number (28 down to 1)
!actor_34 = $34                 ; $FF for a new child
!actor_35 = $35
!actor_36 = $36
!actor_next = $37               ; the next record in the actor's list (word; 0 ends it)
!actor_tile_slot = $39          ; the animated-tile slot it holds (1-4), 0 none
!actor_40 = $40                 ; word
!actor_41 = $41
!actor_42 = $42                 ; word
!actor_44 = $44
!actor_45 = $45
!actor_46 = $46
!actor_47 = $47
!actor_48 = $48                 ; word
!actor_49 = $49
!actor_stream = $4A             ; word: where its byte stream goes on
!actor_4c = $4C
!actor_4d = $4D

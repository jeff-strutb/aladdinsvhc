; object.inc: an object's record as its code reaches it, through the direct
; page (D = the record's address). Offsets whose purpose is not yet
; established keep their offset as their name (the same names as the C's
; object.h).

includeonce

!obj_00 = $00                   ; the object's state ($8C, $CC ...)
!obj_07 = $07
!obj_08 = $08                   ; low two bits: set from the frame's parity by some objects
!obj_09 = $09                   ; bits 4-5 set by height by some objects
!obj_0c = $0C                   ; word: X position
!obj_0f = $0F                   ; word: Y position
!obj_18 = $18                   ; zero: free for the list walk at $84:8A52
!obj_19 = $19                   ; three bytes, loaded together from a table
!obj_1d = $1D                   ; three bytes, loaded together from a table (bit 7 of $1F a sign)
!obj_20 = $20
!obj_37 = $37                   ; word: the next record in its list (0 ends it)
!obj_40 = $40                   ; word
!obj_44 = $44
!obj_45 = $45
!obj_46 = $46
!obj_47 = $47
!obj_48 = $48

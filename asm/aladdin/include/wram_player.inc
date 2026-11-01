; wram_player.inc: work RAM the bank-$83 player code keeps (the same names
; as the C's wram_player.h).

includeonce

!player_x = $08E7               ; word: Aladdin's X position
!player_y = $08EA               ; word: Aladdin's Y position
!player_08f0 = $08F0            ; nonzero: the near test's reach is 8, not 18
!point_145d = $145D             ; word: X of a point an object sets (Y at $145F)
!point_145f = $145F

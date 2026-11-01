; rom_video.inc: places in ROM the scroll layers' code names before their
; own area is filed (tables in bank $80).

includeonce

; Scroll layers (video/scroll_layer.asm): the drawn position's step for a
; column or row queued (+8, -8), and the level table.
!layer_draw_step = $80C666
!layer_setup = $80C648          ; 7 bytes per layer
!edge_place = $80C656           ; per edge: X, Y from the scroll (words)
!side_edge = $80C66A            ; per side, the edge a column (right, left) or row (bottom, top) is drawn at
!side_edge_bit = $80C66E        ; per side, that edge's bit: columns, then rows
!level_table = $80E460

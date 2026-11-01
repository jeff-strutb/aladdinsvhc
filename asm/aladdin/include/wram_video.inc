; wram_video.inc: work RAM the screen code keeps: VRAM upload requests, the
; text buffer, scroll positions (the same names as the C's wram_video.h, in
; lower case).

includeonce

; The text buffer (2 KB of BG tilemap at $7E:D800) and its upload.
!text_upload_pending = $0835    ; nonzero: upload the text buffer at the next NMI
!text_upload_vram = $03DF       ; VRAM word address it goes to (word)
!text_upload_size = $03E1       ; bytes to upload (word)
!text_buffer = $7ED800          ; the tilemap words
!wram_c800 = $7EC800            ; a 4 KB buffer, cleared before a level starts

; Scroll positions, each the integer part (word) of a 4-byte position.
!bg1_x = $19ED
!bg1_y = $19F1
!bg2_x = $19F5
!bg2_y = $19F9
!bg3_x = $19FD
!bg3_y = $1A01
; Added to the positions as they go to the PPU (words, 4 bytes apart).
!bg1_x_shift = $1A18
!bg1_y_shift = $1A1C
!bg2_x_shift = $1A20
!bg2_y_shift = $1A24
!bg2_x_held = $083C             ; nonzero: BG2's horizontal scroll is not written

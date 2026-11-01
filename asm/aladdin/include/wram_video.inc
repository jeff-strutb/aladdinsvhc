; wram_video.inc: work RAM the screen code keeps: VRAM upload requests, the
; text buffer, scroll positions (the same names as the C's wram_video.h, in
; lower case).

includeonce

; The text buffer (2 KB of BG tilemap at $7E:D800) and its upload.
!text_upload_vram = $03DF       ; VRAM word address it goes to (word)
!text_upload_size = $03E1       ; bytes to upload (word)
!tile_behaviour = $7EC800       ; per map tile, how it behaves (solid, slope ...); 4 KB, cleared before a level starts

; Upload requests the NMI serves (video/vram_queue.asm).
!text_parts_pending = $0836     ; 3 flags: nonzero, upload that part of the text buffer
!vram_upload_number = $0834     ; nonzero: the number of a fixed upload to make
!vram_queue_pending = $0858     ; nonzero: the queue below is ready
!vram_queue_count = $0859       ; records in it
!vram_queue = $085A             ; 8-byte records: VRAM address, source, bank, count, VMAIN
!tile_blocks_pending = $1BE6    ; nonzero: the block queue below is ready
!tile_blocks_length = $1BE4     ; its length in bytes
!tile_blocks = $1B44            ; 10-byte records: VRAM address, four tilemap words
!vram_list_pending = $1649      ; per slot: nonzero, upload the slot's list
!vram_list_pointer = $164D      ; per slot: the list's address (word)

; Scroll positions, each the integer part (word) of a 4-byte position.
!bg1_x = $19ED
!bg1_y = $19F1
!bg2_x = $19F5
!bg2_y = $19F9
!bg3_x = $19FD
!bg3_y = $1A01
; Added to the positions as they go to the PPU (words, 4 bytes apart).
!bg1_x_shift = $1A18
!bg2_x_shift = $1A20
!bg2_y_shift = $1A24
!bg2_x_held = $083C             ; nonzero: BG2's horizontal scroll is not written

; Scroll layers: records of $156 bytes, the play field's (layer 0) and the
; backdrop's (layer 1); the fields are offsets into a record.
!layer0 = $1738
!layer1 = $188E
!LAYER_SCROLL_X = $3B           ; scroll X, whole pixels (word); the fraction is the byte below
!LAYER_SCROLL_Y = $3F           ; scroll Y, whole pixels (word); the fraction is the byte below
!LAYER_COLUMN_LENGTH = $28       ; tiles a column draws (word)
!LAYER_ROW_LENGTH = $2A          ; tiles a row draws (word)
!LAYER_TILEMAP_BASE = $2E       ; bits every tilemap word of the layer carries (word)
!LAYER_TILEMAP_MASK = $30       ; the tilemap word bits a position gives (word)
!LAYER_SCREEN_FLIP = $32        ; tilemap bits a row step past the top or bottom flips (word)
!LAYER_DRAWN_X = $34            ; the X its drawn tiles reach to (word)
!LAYER_DRAWN_Y = $36            ; the Y its drawn tiles reach to (word)
!LAYER_COLUMN_SIDE = $38        ; the side of the column being drawn: 0 right, 1 left
!LAYER_ROW_SIDE = $39           ; the side of the row being drawn: 0 down, 1 up
!LAYER_MAP_BASE = $42           ; the layer's block map in $7F:F000 (word offset)
!LAYER_BLOCK_PTR = $44          ; a block's metatile list (long)
!LAYER_METATILE_PTR = $4A       ; a metatile's four tiles (long)
!LAYER_EDGES = $4F              ; the edges it draws: 1 right, 2 left, 4 bottom, 8 top
!LAYER_COLUMN_PARTS = $50       ; a column drawn, to copy to VRAM: its parts (1, or 2 past 32 tiles)
!LAYER_COLUMN_DEST = $51        ; its tilemap word (word)
!LAYER_COLUMN_BUF = $53         ; its tiles, a ring of 32 words from its row
!LAYER_ROW_PARTS = $D3          ; a row drawn, to copy to VRAM: its parts (2: the two screens)
!LAYER_ROW_DEST = $D4           ; its tilemap word (word)
!LAYER_ROW_BUF = $D6            ; its tiles, a ring of 64 words from its column

; Columns and rows of tiles queued to draw: rings of 8 entries of 4 bytes
; (the layer's record, the side: 0 right or down, 1 left or up).
!column_queue = $1A48
!row_queue = $1A68
!column_queue_head = $1A88      ; index of the next entry to draw
!column_queue_tail = $1A8A      ; index where the next entry goes
!row_queue_head = $1A8C
!row_queue_tail = $1A8E

; The level.
!level = $035E                  ; the level being played (word)
!scroll_limit_x = $1A04         ; the play field's scroll X limit (word)
!scroll_limit_y = $1A06         ; the play field's scroll Y limit (word)
!scroll_x_before = $19E5        ; the play field's scroll X before this frame's move (word)
!scroll_y_before = $19E9        ; the play field's scroll Y before this frame's move (word)
!camera_stopped_x = $1A0C       ; nonzero: an empty block stopped the scroll across this frame (word)
!camera_stopped_y = $1A0E       ; nonzero: an empty block stopped the scroll up or down (word)
!block_source_end = $0016       ; while a level loads: the byte offset past the last block source listed (word)
!block_sources = $1BF5          ; while a level loads: each block's source in its bank, a word per block
!level_metatile_bank = $005C    ; the bank of the level's metatiles


; The level's block map: per block a word, its metatile list (negative: empty).
!block_map = $7FF000            ; in play; while a level loads the same RAM holds !anim_set_of_type

; Backdrop bands that drift on their own: 11 positions (4 bytes, whole
; pixels in the middle two) and their speeds, and the HDMA table of 3-byte
; entries (lines, scroll) that scrolls each band.
!band_pos = $7EF800
!band_speed = $7EF900
!band_hdma = $7EF400

; While the layers' tilemaps fill as a level starts (video/scroll_layer.asm).
!fill_layers_left = $1A90       ; layers left to fill
!fill_columns_left = $1A91      ; columns left to draw in this one

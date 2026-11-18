; scene_steps.asm: three steps the code that places Aladdin in a scene
; (from $81:E154) calls each frame: his animation started or stepped and
; his picture sent, and a step across with the animation.
; C: src/aladdin/player/scene_steps.c.

incsrc "../include/macros.inc"

arch 65816

; @routine 81:E224 PlayerSceneMoveStep
; WHAT IT DOES: A frame of Aladdin walking in a scene: a step across, then
; his animation stepped and his picture sent.
; HOW: ActorMoveX, then PlayerSceneAnimStep.
; Entry: M 8-bit, X 8-bit; D = the player's record; DB a bank whose
; $0000-$1FFF is work RAM; called by JSR from the scene code. Exit: as
; PlayerSceneAnimStep.
%at($81E224)
PlayerSceneMoveStep:
    jsl.l ActorMoveX
    jsr.w PlayerSceneAnimStep
    rts

; @routine 81:E22C PlayerSceneAnimStart
; WHAT IT DOES: Starts Aladdin's animation A in a scene and sends his
; picture.
; HOW: AnimStart, then ActorPictureSend.
; Entry: M 8-bit, X 8-bit; A = the animation; D = the player's record;
; DB a bank whose $0000-$1FFF is work RAM; called by JSR from the scene
; code. Exit: as ActorPictureSend.
%at($81E22C)
PlayerSceneAnimStart:
    jsl.l AnimStart
    jsl.l ActorPictureSend
    rts

; @routine 81:E235 PlayerSceneAnimStep
; WHAT IT DOES: Steps Aladdin's animation in a scene and sends his picture.
; HOW: AnimStep, then ActorPictureSend.
; Entry: M 8-bit, X 8-bit; D = the player's record; DB a bank whose
; $0000-$1FFF is work RAM; called by JSR from the scene code and
; PlayerSceneMoveStep. Exit: as ActorPictureSend.
%at($81E235)
PlayerSceneAnimStep:
    jsl.l AnimStep
    jsl.l ActorPictureSend
    rts

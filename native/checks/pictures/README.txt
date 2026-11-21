Picture checks of screen effects: the original's pictures (snesbox `shot`) against the native game's (the Linux build's
--shots / ALADDIN_DEV_SHOTS). Pictures find bugs; they are not a target (PLAN.md decision 3): the engine draws natively,
time is not lined up, so a native picture is matched to the original's by content, and geometry (a window's box, the dark rows)
is compared in numbers. bsnes's colours go through a gamma of 1.5 (CHECK.md): the native colours are put through the same
function before comparing.

 original:  a script `load build/aladdin/states/stage_aX_sY.state`, `poke` the player and camera to where the native play
            starts (ALADDIN_DEV_AT=x,y puts the camera at x-128, y-112), then `wait N` / `shot NAME` lines; run
            snesbox (a private one: CHECK.md) with --core <main checkout>/tools/toolchains/bsnes/bsnes_libretro.so
            (the worktree has no tools/toolchains), through cpu-run. Its pictures are 512x448.
 native:    ALADDIN_DEV_AT=x,y native/checks/plays/run.sh native/checks/plays/end-P-S.txt UPDATES DIR EVERY FROM
 align.py ORIGDIR NATDIR   per original picture: the native picture that differs least, and the pixels that differ
 bbox.py DIR HALF          per picture the box that differs from the outside colour (HALF 1 for the original's 512x448)
 side.py ORIG NAT OUT.png  original, native and the differing pixels side by side
 sheet.py OUT.png FILES... contact sheet

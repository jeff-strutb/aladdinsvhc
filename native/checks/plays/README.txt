Plays by keys (the developer's tools, system/dev_bot.c; docs/aladdin/native/CHECK.md).
pPART-STAGE.txt   --input files: the level select by keys, then keys found by the developer's player from the level's start.
end-PART-STAGE.txt  the same, run WITH ALADDIN_DEV_AT=X,Y (the level's play starts there: its exit spot's x less 140, the spot's y;
                  see the list below), so the level's exit and the screens after it can be seen without playing all of it.
Replay: ALADDIN_CPU_RUN=... native/checks/plays/run.sh FILE UPDATES [PICTURES_DIR [EVERY [FROM]]]  (set ALADDIN_DEV_AT for end-*).
End starts: 0-1 2404,200; 0-2 4196,392; 1-0 3940,300; 1-1 3684,328; 2-0 4452,104; 2-1 7028,128; 3-0 3940,136; 3-1 3940,136;
3-2 3172,136; 4-0 2404,392; 4-1 2916,136; 4-2 2916,168; 5-0 3940,232; 5-2 4708,168 (Jafar's scene and fight); 5-3 2604,351.

Start to exit (2026-11-21, n5 play-through): the files pPART-STAGE.txt that end at the level's real exit are made by the archive search
(ALADDIN_DEV_BOT_GE=10, SEG 6, GE_LEN 60, make.sh: it cuts the file 120 updates after the exit); `ways.txt` lists the others'
settings. The whole table of what each play reaches is in NATIVE-FAULTS.txt ("PLAYS START TO EXIT").
3-0 p3-0.txt, 2-0 p2-0.txt: the search's furthest places (not the exits); the level's start-to-exit plays are the other files

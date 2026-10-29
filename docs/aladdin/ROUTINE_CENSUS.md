# Routine census

`tools/aladdin/census.py` finds the game's code that no script has run, by
following the code statically from every routine the catalog knows: calls,
jumps, jump tables, handler pointers stored in data, task entries named by
the task-create and task-exec calls' inline arguments, orphans after the end
of code, and dead code that decodes cleanly. Those bytes are in the progress
bars' denominator, so certified code is measured against the whole game, not
only what the scripts reach. Run it for the current counts.

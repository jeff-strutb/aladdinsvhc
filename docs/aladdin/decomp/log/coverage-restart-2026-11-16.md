# Covering actor variants by restarting a live actor, and branches no cart can take, 2026-11-16

## Restarting a live actor with another variant

An actor's start code branches on its `$07` (its variant), and a script
meets only the variants its level places. To run another variant's code, a
script can put a live actor of the kind back into its start state: poke its
record's state (+0) to `$0C` and its `$07` (+7) to the variant. On the next
frame the actor loop starts it again through its kind's table. The kind is
the record's `$06`. Records lie at `$08DB + n * $50`. To find one, dump work
RAM every 30 frames of the script (snesbox `dump`, names relative to
`--out`) and read the records.

- Kind $83 at `$0BAB` in `stage_a2_s1` from frame 240, restarted at frame
  300 with `$07 = $11`: every byte of `84:9538` runs, including the bit-4
  branch `84:9555` (`cov_kind83_variant.box`).
- Kind $51 at `$0B5B` in `level1.state`, restarted with `$07 = 1`, then
  `$07 = 2`: `83:D25A` and `83:D247` (`cov_kind51_variant.box`).

The restart reuses a live record, so the actor's other fields are what its
first start left. That is fine for start code, which sets what it reads.

## Branches the retail cart can never take

Several routines that otherwise run contain a check that no unmodified
cartridge can fail:

| Routine | Check | Never-run code |
|---|---|---|
| 84:8629 | cartridge RAM at $70:0080 (a copier's) | $84:8644 |
| 83:E6BB (kind $68) | cartridge RAM at $70:0040 | $83:E6F8 |
| 83:F049 (kind $6E) | the ROM's mirror and cartridge RAM | $83:F0CA, $83:F115 |
| 83:A82F (kind $2D) | NOPs at $83:989E or $83:98AC (a patched ROM) | $83:A83F: a random hit box |
| 84:AF7F, 84:AFAE | cartridge RAM; the region | (held C) |

The board has no RAM at bank $70: a read returns the bus's last byte, so a
write and read back never match. Gate C (every byte run) cannot pass these
routines, and EXCLUDED covers only whole routines. They stay T2 until the
standard says how such a branch is judged (question raised 2026-11-16).

A jump table can also have words no input selects: kind $36's start
(`83:AF8E`) indexes its table at `$83:AFA5` by `$07`, but `$07` values
with bit 4 set go elsewhere first, so the words for `$10`, `$12` and `$14`
are never read.

## A call whose callee writes its frame where a dropped return was

GuardLedgeJump (`A5:F19D`) drops its own return address, then calls
ActorMotionPreset by JSL. The JSL writes its return (`A5:F204` or `F23E`)
over the dropped one, in its caller's frame (`$0156-$0157` in the
scripts). A5 compared those bytes, and the C's call to C wrote nothing
there: DIVERGENT on 12 calls. The C now writes the JSL's frame
(`jsl_frame`) and steps S back up after the call.

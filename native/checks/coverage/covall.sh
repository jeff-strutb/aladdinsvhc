#!/bin/sh
W=/home/strutb/projects/strutb/aladdinsvhc/build/aladdin/native/coverage-work
while read s k; do $W/covrun.sh $s $k; done <<'L'
cov_kind22 22
cov_kind4e_bounce 4E
cov_kind5c_bob 5C
cov_kind5c_path 5C
cov_kind5d_go 5D
cov_kind5d_ride 5D
cov_kind5f_65_78 5F
cov_kind66_arms 66
cov_kind66_second 66
cov_kind6a_ways 6A
cov_kind71_ways 71
cov_kind84_start 84
cov_kind84_wait 84
cov_kind87_second 87
cov_kind87_variant 87
cov_kinda0_carry A0
cov_kinda0_fall A0
cov_kinda4_wait A4
cov_kinda5_wait A5
cov_kind_waits_83b 44
cov_guard_moves 08,69
cov_kind87_catch 87
cov_kinda7_chain A7
cov_kind91_variants 91
L

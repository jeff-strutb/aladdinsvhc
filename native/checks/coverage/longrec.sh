#!/bin/sh
# longrec.sh SCRIPT SPLIT: snesbox stops a run at 9999 frames. SCRIPT's replay (sd_SCRIPT.box) is run to frame SPLIT without dumps (a `save`),
# then from that state to its end with a dump after every frame (frames SPLIT+1 on): covrec_SCRIPT holds them; sect.py takes them from there.
# Needs M W WTREE ALADDIN_CPU_RUN; the sd_ file made by scriptdump.py.
set -e
s=$1; n=$2; mkdir -p $W/long_$s
awk -v n=$(printf 'f%05d' $n) '/^dump /{ if ($2==n) {print; print "save mid"; exit} next } {print}' $W/sd_$s.box > $W/long_$s/a.box
awk -v n=$(printf 'f%05d' $n) 'BEGIN{p=0} p{print} /^dump /{ if ($2==n) {p=1} }' $W/sd_$s.box > $W/long_$s/rest.box
{ echo "load $W/long_$s/a/mid.state"; cat $W/long_$s/rest.box; } > $W/long_$s/b.box
mkdir -p $W/long_$s/a; rm -rf $W/covrec_$s; mkdir -p $W/covrec_$s
cd $M
$ALADDIN_CPU_RUN -j 1 -m 2G -- timeout 1700 build/aladdin/box/snesbox --rom build/aladdin/baserom.sfc --script $W/long_$s/a.box --out $W/long_$s/a > $W/long_$s/a.log 2>&1
$ALADDIN_CPU_RUN -j 1 -m 2G -- timeout 1700 build/aladdin/box/snesbox --rom build/aladdin/baserom.sfc --script $W/long_$s/b.box --out $W/covrec_$s > $W/long_$s/b.log 2>&1
ls $W/covrec_$s | grep -c wram

#!/bin/bash

# all experiments follow the same execution format of
#  ./program N id other_args ...

n=$1
s=$2
d=$3
tag=$4

usage () {
    echo $@
    echo "usage: $0 [N] [size] [depth] [tag]"
    exit 0
}

if ! [[ $n =~ ^[0-9]+$ ]]; then
    usage "not a number:" $n
fi

echo "running: n=$n i s=$s d=$d"

logext="experiment"

logdir="logs/logs_${logext}_${n}_${s}_${d}_${tag}"

mkdir -p $logdir

# ( valgrind --tool=callgrind --dump-instr=yes --simulate-cache=yes --collect-jumps=yes ./$program $n 0 $s $d ) & #| tee "${logdir}/party_0.log" &

( ./build/exp_comp.x $n 0 $s $d | tee "${logdir}/party_0.log" ) &

# echo -n "starting "
for i in $(seq 1 $(($n - 1))); do
    # echo -n "$i "
    ( ./build/exp_comp.x $n $i $s $d &>"${logdir}/party_${i}.log" ) &
    # ( ./$program $n $i $s $d &>/dev/null ) &
    pid=$!
done
echo

echo "waiting for OUR experiment to finish ..."
# wait $pidzero
wait

sleep 2
echo "<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<"
echo "Starting ATLAS"

( ./build/atlas.x $n 0 $s $d | tee -a "${logdir}/party_0.log" ) &

# echo -n "starting "
for i in $(seq 1 $(($n - 1))); do
    # echo -n "$i "
    ( ./build/atlas.x $n $i $s $d &>>"${logdir}/party_${i}.log" ) &
done
echo

echo "waiting for ATLAS experiment to finish ..."
# wait $pidzero
wait

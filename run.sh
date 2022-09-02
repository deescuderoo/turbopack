#!/bin/bash
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

( ./build/ours.x $n 0 $s $d | tee "${logdir}/party_0.log" ) &

for i in $(seq 1 $(($n - 1))); do
    ( ./build/ours.x $n $i $s $d &>"${logdir}/party_${i}.log" ) &
    pid=$!
done
echo

echo "waiting for OUR experiment to finish ..."
wait

sleep 2
echo "<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<"
echo "Starting DN07"

( ./build/dn07.x $n 0 $s $d | tee -a "${logdir}/party_0.log" ) &

for i in $(seq 1 $(($n - 1))); do
    ( ./build/dn07.x $n $i $s $d &>>"${logdir}/party_${i}.log" ) &
done
echo

echo "waiting for DN07 experiment to finish ..."
wait

#!/bin/bash

# all experiments follow the same execution format of
#  ./program N id other_args ...

program=$1
n=$2
s=$3
d=$4

usage () {
    echo $@
    echo "usage: $0 [program name] [N] [size] [depth]"
    exit 0
}

if [ -z "${program}" ]; then
    usage "missing program argument"
fi

if ! [[ $n =~ ^[0-9]+$ ]]; then
    usage "not a number:" $n
fi

echo "running: $program $n i $s $d"

logext=$(basename $program .x)

logdir="logs/logs_${logext}_${n}_${m}_$(date +%s)"

mkdir -p $logdir

( ./$program $n 0 $s $d ) &
pidzero=$!

# echo -n "starting "
for i in $(seq 1 $(($n - 1))); do
    # echo -n "$i "
    ( ./$program $n $i $s $d &>"${logdir}/party_${i}.log" ) &
    # ( ./$program $n $i $s $d &>/dev/null ) &
    pid=$!
done
echo

echo "waiting for experiment to finish ..."
# wait $pidzero
wait

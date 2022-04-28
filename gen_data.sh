#!/bin/bash

num_it=$1
shift

echo "Removing old logs"
rm -rf ./logs/
echo "Done."
echo "Starting experiments n=["$@"] and $num_it iterations"

for n in $@; do
    for s in 1000 10000 100000; do
	for d in 10 100 1000; do
	    for it in $(seq 0 $(($num_it - 1))); do
		if [ $d -lt $s ]
		then
		    echo ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>"
		    echo "Running n=${n}, s=${s}, d=${d}, iteration=${it}"
		    ( ./run.sh $n $s $d $it )
		    sleep 2
		fi
	    done
	done
    done
done

echo
echo "DONE"
# wait $pidzero
wait

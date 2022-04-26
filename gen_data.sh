#!/bin/bash

num_it=$1


echo "Removing old logs"
rm -rf ./logs/
echo "Done. \nStarting experiments with $num_it iterations\n"

for n in 13 21 29 37 45; do
    for s in 1000 10000; do
	for d in 10 100 1000 10000; do
	    for it in $(seq 0 $(($num_it - 1))); do
		if [ $d -lt $s ]
		then
		    echo ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>"
		    echo "Running n=${n}, s=${s}, d=${d}, iteration=${it}"
		    ( ./run.sh build/exp_comp.x $n $s $d $it ) 
		fi
	    done
	done
    done
done

echo
echo "DONE"
# wait $pidzero
wait

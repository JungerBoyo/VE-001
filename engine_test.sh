#!/bin/bash

binary=$1
out_csv_dir=$2
shift
shift

t=4
W=(2 50 100 200)
XZ=(100 150 200 250 300 350 400 400)
Y=(50 100 100 200 200 300 300 350)

for w in "${W[@]}"; do
	for ((i=0; i<${#XZ[@]}; i++)); do
		echo "./$binary "$@" -t 4 -w ${w} -x 64 -y 64 -z 64 -X ${XZ[i]} -Y ${Y[i]} -Z ${XZ[i]}"
		"./$binary" "$@" -t 4 -w ${w} -x 64 -y 64 -z 64 -X ${XZ[i]} -Y ${Y[i]} -Z ${XZ[i]}
		mv ve001_meshing_samples.csv "$out_csv_dir/ve001_meshing_samples_w${w}_xz${XZ[i]}_y${Y[i]}.csv"
		mv ve001_frame_samples.csv "$out_csv_dir/ve001_frame_samples_w${w}_xz${XZ[i]}_y${Y[i]}.csv"
	done
done

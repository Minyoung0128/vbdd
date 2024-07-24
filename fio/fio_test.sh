RUNTIME=30s
NUMJOBS=4
SIZE=8m
RAMP=5s
IOENGINE=io_uring
DIRECT=1
VERIFY=0
FILE_NAME="/dev/CSL"

TYPE=("randwrite")
BLOCK_SIZES=("512B")
IO_DEPTHS=("16" "32" "64")

run_fio_test(){
	local bs=$1
	local output_file=$2
	local rwtype=$3
	local io_depth=$4

	sudo fio --filename=$FILE_NAME --name=fio_test --size=$SIZE --time_based --runtime=$RUNTIME --ramp_time=$RAMP \
		--ioengine=$IOENGINE --direct=$DIRECT --verify=$VERIFY --bs=$bs --iodepth=$io_depth \
		--rw=$rwtype --numjobs=$NUMJOBS --output-format=csv --output=$output_file
}

for bs in "${BLOCK_SIZES[@]}"; do
	for rwtype in "${TYPE[@]}"; do
		for io_depth in "${IO_DEPTHS[@]}"; do
	       		output_file="./result/fio_${bs}_${rwtype}_${io_depth}.csv"
			echo "Running fio test with blk size $bs and type $rwtype and depth $io_depth"
 			run_fio_test $bs $output_file $rwtype $io_depth
		done
	done
done

echo "FIO TEST COMPLETE"


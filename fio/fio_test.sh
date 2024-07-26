RUNTIME=30s
IO_DEPTH=32
SIZE=8m
RAMP=5s
IOENGINE=io_uring
DIRECT=1
VERIFY=0
FILE_NAME="/dev/CSL"

TYPE=("write" "read" "randread" "randwrite")
BLOCK_SIZES=("512B")
NUM_JOBS=("4")

run_fio_test(){
	local bs=$1
	local output_file=$2
	local rwtype=$3
	local num_jobs=$4

	sudo fio --filename=$FILE_NAME --name=fio_test --size=$SIZE --time_based --runtime=$RUNTIME --ramp_time=$RAMP \
		--ioengine=$IOENGINE --direct=$DIRECT --verify=$VERIFY --bs=$bs --iodepth=$IO_DEPTH \
		--rw=$rwtype --numjobs=$num_jobs --output-format=csv --output=$output_file
}

for bs in "${BLOCK_SIZES[@]}"; do
	for rwtype in "${TYPE[@]}"; do
		for num_jobs in "${NUM_JOBS[@]}"; do
	       		output_file="./result/fio_job${num_jobs}_${rwtype}.csv"
			echo "Running fio test with blk size $bs and type $rwtype and jobs $num_jobs"
 			run_fio_test $bs $output_file $rwtype $num_jobs
		done
	done
done

echo "FIO TEST COMPLETE"


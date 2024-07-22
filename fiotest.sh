sudo fio -direct=1 -iodepth=16 -rw=randread -ioengine=io_uring -bs=4k -size=8m -numjobs=4 -time_based -runtime=15s -group_reporting -filename=/dev/CSL -name=csl_test

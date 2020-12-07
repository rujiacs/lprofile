bin=/home/jr/profile/lprofile/build/utils/lprofile
testapp=/home/jr/profile/measure/instru/klargest

#testset=(100 500 1000 5000 10000)
##testset=(100 500)
#for size in ${testset[*]}
#do
##	output=`${bin} prof -f kth_largest_qs ${testapp} $size`
#	taskset -c 2 ${bin} prof -f kth_largest_qs ${testapp} $size &> out
##	echo $output
#	result=`cat out | awk '$1== "KLARGEST" {print $0}'`
#	echo $result >> $file
#	echo $result
#done
${bin} wrap -f kth_largest_qs ${testapp} 10
#${bin} prof -f kth_largest_qs ${testapp} 10

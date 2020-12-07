bin=/home/jr/profile/lprofile/build/utils/lprofile
testapp=/home/jr/profile/measure/instru/klargest

#export DYNINST_DEBUG_SPRINGBOARD=1
#export DYNINST_DEBUG_TRAP=1
#export DYNINST_DEBUG_STARTUP=1
#export DYNINST_DEBUG_INST=1
#export DYNINST_DEBUG_RELOC=1

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
#${bin} wrap -f kth_largest_qs ${testapp} 10
#${bin} prof -f kth_largest_qs ${testapp} 10
${bin} replace -f kth_largest_qs ${testapp} 10

export DYNINSTAPI_RT_LIB=/home/rujia/project/profile/lprofile/build/lprofile/dyninst/lib/libdyninstAPI_RT.so
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/rujia/project/profile/lprofile/build/lprofile/lib:/home/rujia/project/profile/lprofile/build/lprofile/dyninst/lib/lib

file="klargest_dyninst.txt"

bin=/home/rujia/project/profile/lprofile/build/utils/lprofile
testapp=/home/rujia/project/profile/tau/examples/dyninst/klargest

testset=(100 500 1000 5000 10000)
#testset=(100 500)
for size in ${testset[*]}
do
#	output=`${bin} prof -f kth_largest_qs ${testapp} $size`
	taskset -c 2 ${bin} prof -f kth_largest_qs ${testapp} $size &> out
#	echo $output
	result=`cat out | awk '$1== "KLARGEST" {print $0}'`
	echo $result >> $file
	echo $result
done
#${bin} prof -f kth_largest_qs ${testapp} 100

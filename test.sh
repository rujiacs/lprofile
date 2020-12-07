#export DYNINSTAPI_RT_LIB=/home/rujia/project/profile/lprofile/build/lprofile/dyninst/lib/libdyninstAPI_RT.so
#export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/rujia/project/profile/lprofile/build/lprofile/lib:/home/rujia/project/profile/lprofile/build/lprofile/dyninst/lib/lib

export DYNINST_DEBUG_SPRINGBOARD=1
export DYNINST_DEBUG_TRAP=1
export DYNINST_DEBUG_STARTUP=1
export DYNINST_DEBUG_PARSE=1
export DYNINST_DEBUG_INST=1
export DYNINST_DEBUG_RELOC=1

bin=/home/jr/profile/lprofile/build/utils/lprofile
testapp=/home/jr/profile/lprofile/test/test
testpath=/home/jr/profile/lprofile/test

${bin} replace -f func -w ${testpath}/test-replace ${testapp}
#${bin} simple
#${bin} funcmap -e ${testapp} -f

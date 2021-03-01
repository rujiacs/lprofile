#export DYNINSTAPI_RT_LIB=/home/rujia/project/profile/lprofile/build/lprofile/dyninst/lib/libdyninstAPI_RT.so
#export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/rujia/project/profile/lprofile/build/lprofile/lib:/home/rujia/project/profile/lprofile/build/lprofile/dyninst/lib/lib

export DYNINST_DEBUG_SPRINGBOARD=0
export DYNINST_DEBUG_TRAP=0
export DYNINST_DEBUG_STARTUP=0
export DYNINST_DEBUG_PARSE=0
export DYNINST_DEBUG_INST=0
export DYNINST_DEBUG_RELOC=0

bin=/home/rujia/project/profile/lprofile/build/utils/lprofile
testapp=/home/rujia/project/profile/lprofile/test/test
testpath=/home/rujia/project/profile/lprofile/test

#${bin} replace -f func -w ${testpath}/test-replace ${testapp}
${bin} simple
#${bin} funcmap -e ${testapp} -f

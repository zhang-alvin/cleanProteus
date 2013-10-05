export PROTEUS_ARCH=stampede
export PROTEUS_PREFIX=${PROTEUS}/${PROTEUS_ARCH}
export PROTEUS_PYTHON=${PROTEUS_PREFIX}/bin/python
export PATH=${PROTEUS_PREFIX}/bin:${PATH}
export LD_LIBRARY_PATH=${PROTEUS_PREFIX}/lib:${LD_LIBRARY_PATH}
export DYLD_LIBRARY_PATH=${PROTEUS_PREFIX}/lib:${DYLD_LIBRARY_PATH}
export TACC_MKL_LIB=${MKLROOT}/lib/intel64
export MV2_ON_DEMAND_THRESHOLD=2048
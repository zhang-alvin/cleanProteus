./config/configure.py --prefix=${PROTEUS_PREFIX} --with-pic --with-clanguage=C \
--with-cc=mpicc --with-cxx=mpicxx --with-fc=mpif90 --with-mpi-compilers --FFLAGS="-I." \
--with-shared --with-dynamic \
---with-blas-lapack-dir=${TACC_ACML_LIB} --with-blas-lapack-lib=acml \
--download-parmetis=ifneeded \
--download-spooles=ifneeded \
--download-superlu_dist=ifneeded

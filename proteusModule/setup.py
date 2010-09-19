from distutils.core import setup, Extension
import numpy

## \file setup.py setup.py
#  \brief The python script for building proteus
#
#  Set the DISTUTILS_DEBUG environment variable to print detailed information while setup.py is running.
#
try:
    from config import *
except:
    raise RuntimeError("Missing or invalid config.py file. See proteusConfig for examples")

###to turn on debugging in c++
##\todo Finishing cleaning up setup.py/setup.cfg, config.py...
from distutils import sysconfig
cv = sysconfig.get_config_vars()
cv["OPT"] = cv["OPT"].replace("-DNDEBUG","-DDEBUG")
cv["OPT"] = cv["OPT"].replace("-O3","-g")
cv["CFLAGS"] = cv["CFLAGS"].replace("-DNDEBUG","-DDEBUG")
cv["CFLAGS"] = cv["CFLAGS"].replace("-O3","-g")

setup(name='proteus',
      version='0.9.0',
      description='Python tools for multiphysics modeling',
      author='Chris Kees',
      author_email='christopher.e.kees@usace.army.mil',
      url='https://adh.usace.army.mil/proteus',
      packages = ['proteus'],
      ext_package='proteus',
      ext_modules=[Extension('cRANS2P',
                             ['proteus/cRANS2PModule.cpp','proteus/RANS2P.cpp'],
                             define_macros=[('PROTEUS_SUPERLU_H',PROTEUS_SUPERLU_H)],
                             include_dirs=[numpy.get_include(),'include',
                                           PROTEUS_SUPERLU_INCLUDE_DIR],
                             libraries=['m'],
                             extra_compile_args=PROTEUS_EXTRA_COMPILE_ARGS,
                             extra_link_args=PROTEUS_EXTRA_LINK_ARGS),
                   Extension('cRANS2PV2',
                             ['proteus/cRANS2PV2Module.cpp','proteus/RANS2PV2.cpp'],
                             define_macros=[('PROTEUS_SUPERLU_H',PROTEUS_SUPERLU_H)],
                             include_dirs=[numpy.get_include(),'include',
                                           PROTEUS_SUPERLU_INCLUDE_DIR],
                             libraries=['m'],
                             extra_compile_args=PROTEUS_EXTRA_COMPILE_ARGS,
                             extra_link_args=PROTEUS_EXTRA_LINK_ARGS),
                   Extension('cRANS2P2D',
                             ['proteus/cRANS2P2DModule.cpp','proteus/RANS2P2D.cpp'],
                             define_macros=[('PROTEUS_SUPERLU_H',PROTEUS_SUPERLU_H)],
                             include_dirs=[numpy.get_include(),'include',
                                           PROTEUS_SUPERLU_INCLUDE_DIR],
                             libraries=['m'],
                             extra_compile_args=PROTEUS_EXTRA_COMPILE_ARGS,
                             extra_link_args=PROTEUS_EXTRA_LINK_ARGS),
		   Extension('cRBLES2P',
                             ['proteus/cRBLES2PModule.cpp','proteus/RBLES2P.cpp'],
                             define_macros=[('PROTEUS_SUPERLU_H',PROTEUS_SUPERLU_H)],
                             include_dirs=[numpy.get_include(),'include',
                                           PROTEUS_SUPERLU_INCLUDE_DIR],
                             libraries=['m'],
                             extra_compile_args=PROTEUS_EXTRA_COMPILE_ARGS,
                             extra_link_args=PROTEUS_EXTRA_LINK_ARGS),
                   Extension('cVOF',
                             ['proteus/cVOFModule.cpp','proteus/VOF.cpp'],
                             define_macros=[('PROTEUS_SUPERLU_H',PROTEUS_SUPERLU_H)],
                             include_dirs=[numpy.get_include(),'include',
                                           PROTEUS_SUPERLU_INCLUDE_DIR],
                             libraries=['m'],
                             extra_compile_args=PROTEUS_EXTRA_COMPILE_ARGS,
                             extra_link_args=PROTEUS_EXTRA_LINK_ARGS),
                   Extension('cVOFV2',
                             ['proteus/cVOFV2Module.cpp','proteus/VOFV2.cpp'],
                             define_macros=[('PROTEUS_SUPERLU_H',PROTEUS_SUPERLU_H)],
                             include_dirs=[numpy.get_include(),'include',
                                           PROTEUS_SUPERLU_INCLUDE_DIR],
                             libraries=['m'],
                             extra_compile_args=PROTEUS_EXTRA_COMPILE_ARGS,
                             extra_link_args=PROTEUS_EXTRA_LINK_ARGS),
                   Extension('cVOF2D',
                             ['proteus/cVOF2DModule.cpp','proteus/VOF2D.cpp'],
                             define_macros=[('PROTEUS_SUPERLU_H',PROTEUS_SUPERLU_H)],
                             include_dirs=[numpy.get_include(),'include',
                                           PROTEUS_SUPERLU_INCLUDE_DIR],
                             libraries=['m'],
                             extra_compile_args=PROTEUS_EXTRA_COMPILE_ARGS,
                             extra_link_args=PROTEUS_EXTRA_LINK_ARGS),
                   Extension('cMCorr',
                             ['proteus/cMCorrModule.cpp','proteus/MCorr.cpp'],
                             define_macros=[('PROTEUS_SUPERLU_H',PROTEUS_SUPERLU_H)],
                             include_dirs=[numpy.get_include(),'include',
                                           PROTEUS_SUPERLU_INCLUDE_DIR],
                             libraries=['m'],
                             extra_compile_args=PROTEUS_EXTRA_COMPILE_ARGS,
                             extra_link_args=PROTEUS_EXTRA_LINK_ARGS),
                   Extension('cMCorr2D',
                             ['proteus/cMCorr2DModule.cpp','proteus/MCorr2D.cpp'],
                             define_macros=[('PROTEUS_SUPERLU_H',PROTEUS_SUPERLU_H)],
                             include_dirs=[numpy.get_include(),'include',
                                           PROTEUS_SUPERLU_INCLUDE_DIR],
                             libraries=['m'],
                             extra_compile_args=PROTEUS_EXTRA_COMPILE_ARGS,
                             extra_link_args=PROTEUS_EXTRA_LINK_ARGS),
                   Extension('cNCLS',
                             ['proteus/cNCLSModule.cpp','proteus/NCLS.cpp'],
                             define_macros=[('PROTEUS_SUPERLU_H',PROTEUS_SUPERLU_H)],
                             #               ('NCLS_SPACE_DIM',3)],
                             include_dirs=[numpy.get_include(),'include',
                                           PROTEUS_SUPERLU_INCLUDE_DIR],
                             libraries=['m'],
                             extra_compile_args=PROTEUS_EXTRA_COMPILE_ARGS,
                             extra_link_args=PROTEUS_EXTRA_LINK_ARGS),
                   Extension('cNCLSV2',
                             ['proteus/cNCLSV2Module.cpp','proteus/NCLSV2.cpp'],
                             define_macros=[('PROTEUS_SUPERLU_H',PROTEUS_SUPERLU_H)],
                             #               ('NCLS_SPACE_DIM',3)],
                             include_dirs=[numpy.get_include(),'include',
                                           PROTEUS_SUPERLU_INCLUDE_DIR],
                             libraries=['m'],
                             extra_compile_args=PROTEUS_EXTRA_COMPILE_ARGS,
                             extra_link_args=PROTEUS_EXTRA_LINK_ARGS),
                   Extension('cNCLS2D',
                             ['proteus/cNCLS2DModule.cpp','proteus/NCLS2D.cpp'],
                             define_macros=[('PROTEUS_SUPERLU_H',PROTEUS_SUPERLU_H)],
                             #               ('NCLS_SPACE_DIM',2)],
                             include_dirs=[numpy.get_include(),'include',
                                           PROTEUS_SUPERLU_INCLUDE_DIR],
                             libraries=['m'],
                             extra_compile_args=PROTEUS_EXTRA_COMPILE_ARGS,
                             extra_link_args=PROTEUS_EXTRA_LINK_ARGS),
                   Extension('cRDLS',
                             ['proteus/cRDLSModule.cpp','proteus/RDLS.cpp'],
                             define_macros=[('PROTEUS_SUPERLU_H',PROTEUS_SUPERLU_H)],
                             #               ('RDLS_SPACE_DIM',3)],
                             include_dirs=[numpy.get_include(),'include',
                                           PROTEUS_SUPERLU_INCLUDE_DIR],
                             libraries=['m'],
                             extra_compile_args=PROTEUS_EXTRA_COMPILE_ARGS,
                             extra_link_args=PROTEUS_EXTRA_LINK_ARGS),
                   Extension('cRDLS2D',
                             ['proteus/cRDLS2DModule.cpp','proteus/RDLS2D.cpp'],
                             define_macros=[('PROTEUS_SUPERLU_H',PROTEUS_SUPERLU_H)],
                             #               ('RDLS_SPACE_DIM',2)],
                             include_dirs=[numpy.get_include(),'include',
                                           PROTEUS_SUPERLU_INCLUDE_DIR],
                             libraries=['m'],
                             extra_compile_args=PROTEUS_EXTRA_COMPILE_ARGS,
                             extra_link_args=PROTEUS_EXTRA_LINK_ARGS),
                   Extension('cVANS2P',
                             ['proteus/cVANS2PModule.cpp','proteus/VANS2P.cpp'],
                             define_macros=[('PROTEUS_SUPERLU_H',PROTEUS_SUPERLU_H)],
                             include_dirs=[numpy.get_include(),'include',
                                           PROTEUS_SUPERLU_INCLUDE_DIR],
                             libraries=['m'],
                             extra_compile_args=PROTEUS_EXTRA_COMPILE_ARGS,
                             extra_link_args=PROTEUS_EXTRA_LINK_ARGS),
                   Extension('cRANS2PQ',
                             ['proteus/cRANS2PQModule.cpp','proteus/RANS2PQ.cpp'],
                             define_macros=[('PROTEUS_SUPERLU_H',PROTEUS_SUPERLU_H)],
                             include_dirs=[numpy.get_include(),'include',
                                           PROTEUS_SUPERLU_INCLUDE_DIR],
                             libraries=['m'],
                             extra_compile_args=PROTEUS_EXTRA_COMPILE_ARGS,
                             extra_link_args=PROTEUS_EXTRA_LINK_ARGS),
#                    Extension('cRANS2P2DQ',
#                              ['proteus/cRANS2P2DQModule.cpp','proteus/RANS2P2DQ.cpp'],
#                              define_macros=[('PROTEUS_SUPERLU_H',PROTEUS_SUPERLU_H)],
#                              include_dirs=[numpy.get_include(),'include',
#                                            PROTEUS_SUPERLU_INCLUDE_DIR],
#                              libraries=['m'],
#                              extra_compile_args=PROTEUS_EXTRA_COMPILE_ARGS,
#                              extra_link_args=PROTEUS_EXTRA_LINK_ARGS),
                   Extension('cVOFQ',
                             ['proteus/cVOFQModule.cpp','proteus/VOFQ.cpp'],
                             define_macros=[('PROTEUS_SUPERLU_H',PROTEUS_SUPERLU_H)],
                             include_dirs=[numpy.get_include(),'include',
                                           PROTEUS_SUPERLU_INCLUDE_DIR],
                             libraries=['m'],
                             extra_compile_args=PROTEUS_EXTRA_COMPILE_ARGS,
                             extra_link_args=PROTEUS_EXTRA_LINK_ARGS),
                   Extension('cVOF2DQ',
                             ['proteus/cVOF2DQModule.cpp','proteus/VOF2DQ.cpp'],
                             define_macros=[('PROTEUS_SUPERLU_H',PROTEUS_SUPERLU_H)],
                             include_dirs=[numpy.get_include(),'include',
                                           PROTEUS_SUPERLU_INCLUDE_DIR],
                             libraries=['m'],
                             extra_compile_args=PROTEUS_EXTRA_COMPILE_ARGS,
                             extra_link_args=PROTEUS_EXTRA_LINK_ARGS),
                   Extension('cMCorrQ',
                             ['proteus/cMCorrQModule.cpp','proteus/MCorrQ.cpp'],
                             define_macros=[('PROTEUS_SUPERLU_H',PROTEUS_SUPERLU_H)],
                             include_dirs=[numpy.get_include(),'include',
                                           PROTEUS_SUPERLU_INCLUDE_DIR],
                             libraries=['m'],
                             extra_compile_args=PROTEUS_EXTRA_COMPILE_ARGS,
                             extra_link_args=PROTEUS_EXTRA_LINK_ARGS),
                   Extension('cMCorr2DQ',
                             ['proteus/cMCorr2DQModule.cpp','proteus/MCorr2DQ.cpp'],
                             define_macros=[('PROTEUS_SUPERLU_H',PROTEUS_SUPERLU_H)],
                             include_dirs=[numpy.get_include(),'include',
                                           PROTEUS_SUPERLU_INCLUDE_DIR],
                             libraries=['m'],
                             extra_compile_args=PROTEUS_EXTRA_COMPILE_ARGS,
                             extra_link_args=PROTEUS_EXTRA_LINK_ARGS),
                   Extension('cNCLSQ',
                             ['proteus/cNCLSQModule.cpp','proteus/NCLSQ.cpp'],
                             define_macros=[('PROTEUS_SUPERLU_H',PROTEUS_SUPERLU_H)],
                             #               ('NCLS_SPACE_DIM',3)],
                             include_dirs=[numpy.get_include(),'include',
                                           PROTEUS_SUPERLU_INCLUDE_DIR],
                             libraries=['m'],
                             extra_compile_args=PROTEUS_EXTRA_COMPILE_ARGS,
                             extra_link_args=PROTEUS_EXTRA_LINK_ARGS),
                   Extension('cNCLS2DQ',
                             ['proteus/cNCLS2DQModule.cpp','proteus/NCLS2DQ.cpp'],
                             define_macros=[('PROTEUS_SUPERLU_H',PROTEUS_SUPERLU_H)],
                             #               ('NCLS_SPACE_DIM',2)],
                             include_dirs=[numpy.get_include(),'include',
                                           PROTEUS_SUPERLU_INCLUDE_DIR],
                             libraries=['m'],
                             extra_compile_args=PROTEUS_EXTRA_COMPILE_ARGS,
                             extra_link_args=PROTEUS_EXTRA_LINK_ARGS),
                   Extension('cRDLSQ',
                             ['proteus/cRDLSQModule.cpp','proteus/RDLSQ.cpp'],
                             define_macros=[('PROTEUS_SUPERLU_H',PROTEUS_SUPERLU_H)],
                             #               ('RDLS_SPACE_DIM',3)],
                             include_dirs=[numpy.get_include(),'include',
                                           PROTEUS_SUPERLU_INCLUDE_DIR],
                             libraries=['m'],
                             extra_compile_args=PROTEUS_EXTRA_COMPILE_ARGS,
                             extra_link_args=PROTEUS_EXTRA_LINK_ARGS),
                   Extension('cRDLS2DQ',
                             ['proteus/cRDLS2DQModule.cpp','proteus/RDLS2DQ.cpp'],
                             define_macros=[('PROTEUS_SUPERLU_H',PROTEUS_SUPERLU_H)],
                             #               ('RDLS_SPACE_DIM',2)],
                             include_dirs=[numpy.get_include(),'include',
                                           PROTEUS_SUPERLU_INCLUDE_DIR],
                             libraries=['m'],
                             extra_compile_args=PROTEUS_EXTRA_COMPILE_ARGS,
                             extra_link_args=PROTEUS_EXTRA_LINK_ARGS),
                   Extension('cVANS2PQ',
                             ['proteus/cVANS2PQModule.cpp','proteus/VANS2PQ.cpp'],
                             define_macros=[('PROTEUS_SUPERLU_H',PROTEUS_SUPERLU_H)],
                             include_dirs=[numpy.get_include(),'include',
                                           PROTEUS_SUPERLU_INCLUDE_DIR],
                             libraries=['m'],
                             extra_compile_args=PROTEUS_EXTRA_COMPILE_ARGS,
                             extra_link_args=PROTEUS_EXTRA_LINK_ARGS),
                   Extension('cVANS2P2D',
                             ['proteus/cVANS2P2DModule.cpp','proteus/VANS2P2D.cpp'],
                             define_macros=[('PROTEUS_SUPERLU_H',PROTEUS_SUPERLU_H)],
                             include_dirs=[numpy.get_include(),'include',
                                           PROTEUS_SUPERLU_INCLUDE_DIR],
                             libraries=['m'],
                             extra_compile_args=PROTEUS_EXTRA_COMPILE_ARGS,
                             extra_link_args=PROTEUS_EXTRA_LINK_ARGS),
                   Extension('cVolumeAveragedVOF',
                             ['proteus/cVolumeAveragedVOFModule.cpp','proteus/VolumeAveragedVOF.cpp'],
                             define_macros=[('PROTEUS_SUPERLU_H',PROTEUS_SUPERLU_H)],
                             include_dirs=[numpy.get_include(),'include',
                                           PROTEUS_SUPERLU_INCLUDE_DIR],
                             libraries=['m'],
                             extra_compile_args=PROTEUS_EXTRA_COMPILE_ARGS,
                             extra_link_args=PROTEUS_EXTRA_LINK_ARGS),
                   Extension('cVolumeAveragedVOF2D',
                             ['proteus/cVolumeAveragedVOF2DModule.cpp','proteus/VolumeAveragedVOF2D.cpp'],
                             define_macros=[('PROTEUS_SUPERLU_H',PROTEUS_SUPERLU_H)],
                             include_dirs=[numpy.get_include(),'include',
                                           PROTEUS_SUPERLU_INCLUDE_DIR],
                             libraries=['m'],
                             extra_compile_args=PROTEUS_EXTRA_COMPILE_ARGS,
                             extra_link_args=PROTEUS_EXTRA_LINK_ARGS),
                    Extension('cVolumeAveragedVOFQ',
                             ['proteus/cVolumeAveragedVOFQModule.cpp','proteus/VolumeAveragedVOFQ.cpp'],
                             define_macros=[('PROTEUS_SUPERLU_H',PROTEUS_SUPERLU_H)],
                             include_dirs=[numpy.get_include(),'include',
                                           PROTEUS_SUPERLU_INCLUDE_DIR],
                             libraries=['m'],
                             extra_compile_args=PROTEUS_EXTRA_COMPILE_ARGS,
                             extra_link_args=PROTEUS_EXTRA_LINK_ARGS),
                   Extension('cVolumeAveragedVOF2DQ',
                             ['proteus/cVolumeAveragedVOF2DQModule.cpp','proteus/VolumeAveragedVOF2DQ.cpp'],
                             define_macros=[('PROTEUS_SUPERLU_H',PROTEUS_SUPERLU_H)],
                             include_dirs=[numpy.get_include(),'include',
                                           PROTEUS_SUPERLU_INCLUDE_DIR],
                             libraries=['m'],
                             extra_compile_args=PROTEUS_EXTRA_COMPILE_ARGS,
                             extra_link_args=PROTEUS_EXTRA_LINK_ARGS),
                   Extension('cVANS2P2DQ',
                             ['proteus/cVANS2P2DQModule.cpp','proteus/VANS2P2DQ.cpp'],
                             define_macros=[('PROTEUS_SUPERLU_H',PROTEUS_SUPERLU_H)],
                             include_dirs=[numpy.get_include(),'include',
                                           PROTEUS_SUPERLU_INCLUDE_DIR],
                             libraries=['m'],
                             extra_compile_args=PROTEUS_EXTRA_COMPILE_ARGS,
                             extra_link_args=PROTEUS_EXTRA_LINK_ARGS),
                   Extension('ctracking',
                             ['proteus/ctrackingModule.cpp','proteus/tracking.cpp'],
                             define_macros=[('PROTEUS_SUPERLU_H',PROTEUS_SUPERLU_H)],
                             include_dirs=[numpy.get_include(),'include',
                                           PROTEUS_SUPERLU_INCLUDE_DIR],
                             libraries=['m'],
                             extra_compile_args=PROTEUS_EXTRA_COMPILE_ARGS,
                             extra_link_args=PROTEUS_EXTRA_LINK_ARGS),
                   Extension('cellam',
                             ['proteus/cellamModule.cpp','proteus/ellam.cpp'],
                             define_macros=[('PROTEUS_SUPERLU_H',PROTEUS_SUPERLU_H)],
                             include_dirs=[numpy.get_include(),'include',
                                           PROTEUS_SUPERLU_INCLUDE_DIR],
                             libraries=['m'],
                             extra_compile_args=PROTEUS_EXTRA_COMPILE_ARGS,
                             extra_link_args=PROTEUS_EXTRA_LINK_ARGS),
                   Extension('cfemIntegrals',
                             ['proteus/cfemIntegralsModule.c','proteus/femIntegrals.c','proteus/postprocessing.c'],
                             define_macros=[('PROTEUS_SUPERLU_H',PROTEUS_SUPERLU_H),
                                            ('PROTEUS_LAPACK_H',PROTEUS_LAPACK_H),
                                            ('PROTEUS_LAPACK_INTEGER',PROTEUS_LAPACK_INTEGER),
                                            ('PROTEUS_BLAS_H',PROTEUS_BLAS_H)],
                             include_dirs=[numpy.get_include(),'include',
                                           PROTEUS_SUPERLU_INCLUDE_DIR,
					   PROTEUS_LAPACK_INCLUDE_DIR,
					   PROTEUS_BLAS_INCLUDE_DIR],
                             library_dirs=[PROTEUS_LAPACK_LIB_DIR,
                                           PROTEUS_BLAS_LIB_DIR],
                             libraries=['m',PROTEUS_LAPACK_LIB,PROTEUS_BLAS_LIB],
                             extra_compile_args=PROTEUS_EXTRA_COMPILE_ARGS,
                             extra_link_args=PROTEUS_EXTRA_LINK_ARGS),
                   Extension('cmeshTools',
                             ['proteus/cmeshToolsModule.cpp','proteus/mesh.cpp','proteus/meshio.cpp'],
                             define_macros=[('PROTEUS_SUPERLU_H',PROTEUS_SUPERLU_H),
                                            ('PROTEUS_TRIANGLE_H',PROTEUS_TRIANGLE_H)],
                             include_dirs=([numpy.get_include(),'include']+
                                           [PROTEUS_TRIANGLE_INCLUDE_DIR]),
                             libraries=['m',PROTEUS_DAETK_LIB]+[PROTEUS_TRIANGLE_LIB],
                             library_dirs=[PROTEUS_DAETK_LIB_DIR]+[PROTEUS_TRIANGLE_LIB_DIR],
                             extra_compile_args=PROTEUS_EXTRA_COMPILE_ARGS,
                             extra_link_args=PROTEUS_EXTRA_LINK_ARGS),
                   Extension('ctransportCoefficients',
                             ['proteus/ctransportCoefficientsModule.c','proteus/transportCoefficients.c'],
                             include_dirs=[numpy.get_include(),'include'],
                             libraries=['m'],
                             extra_compile_args=PROTEUS_EXTRA_COMPILE_ARGS,
                             extra_link_args=PROTEUS_EXTRA_LINK_ARGS),
                   Extension('csubgridError',
                             ['proteus/csubgridErrorModule.c','proteus/subgridError.c'],
                             include_dirs=[numpy.get_include(),'include'],
                             libraries=['m'],
                             extra_compile_args=PROTEUS_EXTRA_COMPILE_ARGS,
                             extra_link_args=PROTEUS_EXTRA_LINK_ARGS),
                   Extension('cshockCapturing',
                             ['proteus/cshockCapturingModule.c','proteus/shockCapturing.c'],
                             include_dirs=[numpy.get_include(),'include'],
                             libraries=['m'],
                             extra_compile_args=PROTEUS_EXTRA_COMPILE_ARGS,
                             extra_link_args=PROTEUS_EXTRA_LINK_ARGS),
                   Extension('lapackWrappers',
                             ['proteus/lapackWrappersModule.c'],
                             define_macros=[('PROTEUS_LAPACK_H',PROTEUS_LAPACK_H),
                                            ('PROTEUS_LAPACK_INTEGER',PROTEUS_LAPACK_INTEGER),
                                            ('PROTEUS_BLAS_H',PROTEUS_BLAS_H)],
                             include_dirs=[numpy.get_include(),'include',
					   PROTEUS_LAPACK_INCLUDE_DIR,
					   PROTEUS_BLAS_INCLUDE_DIR],
			     library_dirs=[PROTEUS_LAPACK_LIB_DIR,PROTEUS_BLAS_LIB_DIR],
                             libraries=['m',
					PROTEUS_LAPACK_LIB,
                                        PROTEUS_BLAS_LIB],
                             extra_compile_args=PROTEUS_EXTRA_COMPILE_ARGS,
                             extra_link_args=PROTEUS_EXTRA_LINK_ARGS),
		   Extension('superluWrappers',
                             ['proteus/superluWrappersModule.c'],
                             define_macros=[('PROTEUS_SUPERLU_H',PROTEUS_SUPERLU_H),
                                            ('PROTEUS_BLAS_H',PROTEUS_BLAS_H)],
                             include_dirs=[numpy.get_include(),'include',PROTEUS_SUPERLU_INCLUDE_DIR],
                             library_dirs=[PROTEUS_SUPERLU_LIB_DIR,PROTEUS_LAPACK_LIB_DIR,PROTEUS_BLAS_LIB_DIR],
                             libraries=['m',PROTEUS_SUPERLU_LIB,PROTEUS_LAPACK_LIB,PROTEUS_BLAS_LIB],
                             extra_compile_args=PROTEUS_EXTRA_COMPILE_ARGS,
                             extra_link_args=PROTEUS_EXTRA_LINK_ARGS),
                   Extension('triangleWrappers',
                             ['proteus/triangleWrappersModule.c'],
                             define_macros=[('PROTEUS_TRIANGLE_H',
                                             PROTEUS_TRIANGLE_H),
                                            ('MWF_ADDED_FLAGS',
                                             1)],
                             include_dirs=[numpy.get_include(),PROTEUS_TRIANGLE_INCLUDE_DIR],
                             library_dirs=[PROTEUS_TRIANGLE_LIB_DIR],
                             libraries=['m',
                                        PROTEUS_TRIANGLE_LIB],
                             extra_compile_args=PROTEUS_EXTRA_COMPILE_ARGS,
                             extra_link_args=PROTEUS_EXTRA_LINK_ARGS),
                   Extension('testStuffImpl',
                             ['proteus/testStuffImplModule.c','proteus/testStuffImpl.c'],
                             define_macros=[('MWF_ADDED_FLAGS',
                                             1),
                                            ('PROTEUS_LAPACK_H',PROTEUS_LAPACK_H),
                                            ('PROTEUS_LAPACK_INTEGER',PROTEUS_LAPACK_INTEGER)
                                            ],
                             include_dirs=[numpy.get_include(),'include',
					   PROTEUS_LAPACK_INCLUDE_DIR
                                           ],
                             library_dirs=[PROTEUS_LAPACK_LIB_DIR],
                             libraries=['m',PROTEUS_LAPACK_LIB],
                             extra_link_args=PROTEUS_EXTRA_LINK_ARGS,
                             extra_compile_args=PROTEUS_EXTRA_COMPILE_ARGS),
                   Extension('csmoothers',
                             ['proteus/csmoothersModule.c', 'proteus/smoothers.c'],
                             define_macros=[('PROTEUS_SUPERLU_H',PROTEUS_SUPERLU_H),
                                            ('PROTEUS_LAPACK_H',PROTEUS_LAPACK_H),
                                            ('PROTEUS_LAPACK_INTEGER',PROTEUS_LAPACK_INTEGER),
                                            ('PROTEUS_BLAS_H',PROTEUS_BLAS_H)],
                             include_dirs=[numpy.get_include(),'include',
                                           PROTEUS_SUPERLU_INCLUDE_DIR,
					   PROTEUS_LAPACK_INCLUDE_DIR,
					   PROTEUS_BLAS_INCLUDE_DIR
                                           ],
                             library_dirs=[PROTEUS_SUPERLU_INCLUDE_DIR,
                                           PROTEUS_SUPERLU_LIB_DIR,
                                           PROTEUS_LAPACK_LIB_DIR,
                                           PROTEUS_BLAS_LIB_DIR],
                             libraries=['m',
                                        PROTEUS_SUPERLU_LIB,
                                        PROTEUS_LAPACK_LIB,
                                        PROTEUS_BLAS_LIB],
                             extra_compile_args=PROTEUS_EXTRA_COMPILE_ARGS,
                             extra_link_args=PROTEUS_EXTRA_LINK_ARGS),
                   Extension('ctimeIntegration',
                             ['proteus/ctimeIntegrationModule.c','proteus/timeIntegration.c'],
                             include_dirs=[numpy.get_include(),'include'],
                             libraries=['m'],
                             extra_compile_args=PROTEUS_EXTRA_COMPILE_ARGS,
                             extra_link_args=PROTEUS_EXTRA_LINK_ARGS),
                   Extension('canalyticalSolutions',
                             ['proteus/canalyticalSolutionsModule.c','proteus/analyticalSolutions.c'],
                             include_dirs=[numpy.get_include(),'include'],
                             libraries=['m'],
                             extra_compile_args=PROTEUS_EXTRA_COMPILE_ARGS,
                             extra_link_args=PROTEUS_EXTRA_LINK_ARGS),
                   Extension('flcbdfWrappers',
                             ['proteus/flcbdfWrappersModule.cpp','proteus/mesh.cpp','proteus/meshio.cpp'],
                             define_macros=[('PROTEUS_TRIANGLE_H',PROTEUS_TRIANGLE_H),
                                            ('PROTEUS_SUPERLU_H',PROTEUS_SUPERLU_H),
                                            ('CMRVEC_BOUNDS_CHECK',1),
                                            ('MV_VECTOR_BOUNDS_CHECK',1),
                                            ('PETSCVEC_BOUNDS_CHECK',1),
                                            ('F77_POST_UNDERSCORE',1),
                                            ('USE_BLAS',1)],
                             include_dirs=['include',numpy.get_include(),PROTEUS_SUPERLU_INCLUDE_DIR,PROTEUS_TRIANGLE_INCLUDE_DIR]+PROTEUS_DAETK_INCLUDE_DIR+PROTEUS_PETSC_INCLUDE_DIRS+
                                           [PROTEUS_MPI_INCLUDE_DIR],
                             library_dirs=[PROTEUS_DAETK_LIB_DIR]+PROTEUS_PETSC_LIB_DIRS+[PROTEUS_MPI_LIB_DIR],
                             libraries=['m',PROTEUS_DAETK_LIB,'metis']+PROTEUS_PETSC_LIBS+PROTEUS_MPI_LIBS,
                             extra_link_args=PROTEUS_EXTRA_LINK_ARGS,
                             extra_compile_args=PROTEUS_EXTRA_COMPILE_ARGS),
                   Extension('cpostprocessing',
                             ['proteus/cpostprocessingModule.c','proteus/postprocessing.c','proteus/femIntegrals.c'],
                             define_macros=[('MWF_ADDED_FLAGS',
                                             1),
                                            ('PROTEUS_LAPACK_H',PROTEUS_LAPACK_H),
                                            ('PROTEUS_LAPACK_INTEGER',PROTEUS_LAPACK_INTEGER)
                                            ],
                             include_dirs=[numpy.get_include(),'include',
					   PROTEUS_LAPACK_INCLUDE_DIR
                                           ],
                             library_dirs=[PROTEUS_LAPACK_LIB_DIR,PROTEUS_BLAS_LIB_DIR],
                             libraries=['m',PROTEUS_LAPACK_LIB,PROTEUS_BLAS_LIB],
                             extra_compile_args=PROTEUS_EXTRA_COMPILE_ARGS,
                             extra_link_args=PROTEUS_EXTRA_LINK_ARGS),
                   Extension('cnumericalFlux',
                             ['proteus/cnumericalFluxModule.c','proteus/numericalFlux.c'],
                             include_dirs=[numpy.get_include(),'include'],
                             libraries=['m'],
                             extra_compile_args=PROTEUS_EXTRA_COMPILE_ARGS,
                             extra_link_args=PROTEUS_EXTRA_LINK_ARGS),
                   Extension('cfmmfsw',
                             ['proteus/cfmmfswModule.cpp','proteus/cfmmfsw.cpp','proteus/stupidheap.cpp',
                              'proteus/FMMandFSW.cpp'],
                             define_macros=[('PROTEUS_SUPERLU_H',PROTEUS_SUPERLU_H)],
                             include_dirs=[numpy.get_include(),
                                           'include',
                                           PROTEUS_SUPERLU_INCLUDE_DIR]+PROTEUS_DAETK_INCLUDE_DIR+PROTEUS_PETSC_INCLUDE_DIRS,
                             libraries=['m'],
                             extra_compile_args=PROTEUS_EXTRA_COMPILE_ARGS,
                             extra_link_args=PROTEUS_EXTRA_LINK_ARGS),
                   Extension('cTwophaseDarcyCoefficients',
                             ['proteus/cTwophaseDarcyCoefficientsModule.cpp','proteus/SubsurfaceTransportCoefficients.cpp'],
                             include_dirs=[numpy.get_include(),'include'],
                             extra_link_args=PROTEUS_EXTRA_LINK_ARGS,
                             extra_compile_args=PROTEUS_EXTRA_COMPILE_ARGS),
                   Extension('cpskRelations',
                             ['proteus/cpskRelationsModule.cpp','proteus/SubsurfaceTransportCoefficients.cpp'],
                             include_dirs=['include'],
                             libraries=['m'],
                             extra_compile_args=PROTEUS_EXTRA_COMPILE_ARGS,
                             extra_link_args=PROTEUS_EXTRA_LINK_ARGS),
                   Extension('cSubsurfaceTransportCoefficients',
                             ['proteus/cSubsurfaceTransportCoefficientsModule.cpp','proteus/SubsurfaceTransportCoefficients.cpp'],
                             include_dirs=[numpy.get_include(),'include'],
                             libraries=['m'],
                             extra_link_args=PROTEUS_EXTRA_LINK_ARGS,
                             extra_compile_args=PROTEUS_EXTRA_COMPILE_ARGS),
                   #Cython generated modules with just c code
                   Extension("waveFunctions",['proteus/waveFunctions.c','proteus/transportCoefficients.c'],
                             include_dirs=[numpy.get_include(),'include'])                
                   ],

      scripts = ['scripts/parun','scripts/gf2poly'],
      requires=['numpy']
      )

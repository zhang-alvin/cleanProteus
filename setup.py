import sys
import setuptools
from distutils.core import setup, Extension
from petsc4py.conf.petscconf import Extension as PetscExtension

import numpy
from Cython.Distutils import build_ext

## \file setup.py setup.py
#  \brief The python script for building proteus
#
#  Set the DISTUTILS_DEBUG environment variable to print detailed information while setup.py is running.
#

from proteus import config
from proteus.config import *

###to turn on debugging in c++
##\todo Finishing cleaning up setup.py/setup.cfg, config.py...
from distutils import sysconfig
cv = sysconfig.get_config_vars()
cv["OPT"] = cv["OPT"].replace("-DNDEBUG","-DDEBUG")
cv["OPT"] = cv["OPT"].replace("-O3","-g")
cv["CFLAGS"] = cv["CFLAGS"].replace("-DNDEBUG","-DDEBUG")
cv["CFLAGS"] = cv["CFLAGS"].replace("-O3","-g")

PROTEUS_PETSC_EXTRA_LINK_ARGS = getattr(config, 'PROTEUS_PETSC_EXTRA_LINK_ARGS', [])
PROTEUS_PETSC_EXTRA_COMPILE_ARGS = getattr(config, 'PROTEUS_PETSC_EXTRA_COMPILE_ARGS', [])

proteus_install_path = os.path.join(sysconfig.get_python_lib(), 'proteus')

# handle non-system installations
for arg in sys.argv:
    if arg.startswith('--root'):
        proteus_install_path = proteus_install_path.partition(sys.prefix + '/')[-1]
        break
    if arg.startswith('--prefix'):
        proteus_install_path = proteus_install_path.partition(sys.prefix + '/')[-1]
        break

setup(name='proteus',
      version='1.0.0',
      description='Python tools for multiphysics modeling',
      author='Chris Kees, Matthew Farthing, et al.',
      author_email='chris.kees@us.army.mil',
      url='http://proteus.usace.army.mil',
      packages = ['proteus', 'proteus.config', 'proteus.tests', 'proteus.mprans'],
      cmdclass = {'build_ext':build_ext},
      ext_package='proteus',
      ext_modules=[Extension("WaveTools",['proteus/WaveTools.pyx'],
                             language='c',
                             include_dirs=[numpy.get_include(),'proteus']),
                   Extension("ADR",['proteus/ADR.pyx'],
                             depends=['proteus/ADR.h'],
                             language='c++',
                             include_dirs=[numpy.get_include(),'proteus']),
                   Extension("waveFunctions",['proteus/waveFunctions.pyx','proteus/transportCoefficients.c'],
                             include_dirs=[numpy.get_include(),'proteus'],
                             extra_link_args=PROTEUS_EXTRA_LINK_ARGS),
                   Extension("subsurfaceTransportFunctions",['proteus/subsurfaceTransportFunctions.pyx'],
                             include_dirs=[numpy.get_include(),'proteus'],
                             extra_link_args=PROTEUS_EXTRA_LINK_ARGS),
                   Extension('cfmmfsw', ['proteus/cfmmfswModule.cpp','proteus/cfmmfsw.cpp','proteus/stupidheap.cpp',
                             'proteus/FMMandFSW.cpp'],
                             define_macros=[('PROTEUS_SUPERLU_H',PROTEUS_SUPERLU_H)],
                             include_dirs=[numpy.get_include(),
                                           'proteus',
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
                             include_dirs=[numpy.get_include(),'proteus',
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
                             include_dirs=([numpy.get_include(),'proteus']+
                                           [PROTEUS_TRIANGLE_INCLUDE_DIR]),
                             libraries=['m',PROTEUS_DAETK_LIB]+[PROTEUS_TRIANGLE_LIB],
                             library_dirs=[PROTEUS_DAETK_LIB_DIR]+[PROTEUS_TRIANGLE_LIB_DIR],
                             extra_compile_args=PROTEUS_EXTRA_COMPILE_ARGS,
                             extra_link_args=PROTEUS_EXTRA_LINK_ARGS),
                   Extension('ctransportCoefficients',
                             ['proteus/ctransportCoefficientsModule.c','proteus/transportCoefficients.c'],
                             include_dirs=[numpy.get_include(),'proteus'],
                             libraries=['m'],
                             extra_compile_args=PROTEUS_EXTRA_COMPILE_ARGS,
                             extra_link_args=PROTEUS_EXTRA_LINK_ARGS),
                   Extension('csubgridError',
                             ['proteus/csubgridErrorModule.c','proteus/subgridError.c'],
                             include_dirs=[numpy.get_include(),'proteus'],
                             libraries=['m'],
                             extra_compile_args=PROTEUS_EXTRA_COMPILE_ARGS,
                             extra_link_args=PROTEUS_EXTRA_LINK_ARGS),
                   Extension('cshockCapturing',
                             ['proteus/cshockCapturingModule.c','proteus/shockCapturing.c'],
                             include_dirs=[numpy.get_include(),'proteus'],
                             libraries=['m'],
                             extra_compile_args=PROTEUS_EXTRA_COMPILE_ARGS,
                             extra_link_args=PROTEUS_EXTRA_LINK_ARGS),
                   Extension('lapackWrappers',
                             ['proteus/lapackWrappersModule.c'],
                             define_macros=[('PROTEUS_LAPACK_H',PROTEUS_LAPACK_H),
                                            ('PROTEUS_LAPACK_INTEGER',PROTEUS_LAPACK_INTEGER),
                                            ('PROTEUS_BLAS_H',PROTEUS_BLAS_H)],
                             include_dirs=[numpy.get_include(),'proteus',
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
                             include_dirs=[numpy.get_include(),'proteus',PROTEUS_SUPERLU_INCLUDE_DIR],
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
                             include_dirs=[numpy.get_include(),'proteus',
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
                             include_dirs=[numpy.get_include(),'proteus',
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
                             include_dirs=[numpy.get_include(),'proteus'],
                             libraries=['m'],
                             extra_compile_args=PROTEUS_EXTRA_COMPILE_ARGS,
                             extra_link_args=PROTEUS_EXTRA_LINK_ARGS),
                   Extension('canalyticalSolutions',
                             ['proteus/canalyticalSolutionsModule.c','proteus/analyticalSolutions.c'],
                             include_dirs=[numpy.get_include(),'proteus'],
                             libraries=['m'],
                             extra_compile_args=PROTEUS_EXTRA_COMPILE_ARGS,
                             extra_link_args=PROTEUS_EXTRA_LINK_ARGS),
                   Extension('cpostprocessing',
                             ['proteus/cpostprocessingModule.c','proteus/postprocessing.c','proteus/femIntegrals.c'],
                             define_macros=[('MWF_ADDED_FLAGS',
                                             1),
                                            ('PROTEUS_LAPACK_H',PROTEUS_LAPACK_H),
                                            ('PROTEUS_LAPACK_INTEGER',PROTEUS_LAPACK_INTEGER)
                                            ],
                             include_dirs=[numpy.get_include(),'proteus',
                                           PROTEUS_LAPACK_INCLUDE_DIR
                                           ],
                             library_dirs=[PROTEUS_LAPACK_LIB_DIR,PROTEUS_BLAS_LIB_DIR],
                             libraries=['m',PROTEUS_LAPACK_LIB,PROTEUS_BLAS_LIB],
                             extra_compile_args=PROTEUS_EXTRA_COMPILE_ARGS,
                             extra_link_args=PROTEUS_EXTRA_LINK_ARGS),
                   Extension('cnumericalFlux',
                             ['proteus/cnumericalFluxModule.c','proteus/numericalFlux.c'],
                             include_dirs=[numpy.get_include(),'proteus'],
                             libraries=['m'],
                             extra_compile_args=PROTEUS_EXTRA_COMPILE_ARGS,
                             extra_link_args=PROTEUS_EXTRA_LINK_ARGS),
                   Extension('cTwophaseDarcyCoefficients',
                             ['proteus/cTwophaseDarcyCoefficientsModule.cpp','proteus/SubsurfaceTransportCoefficients.cpp'],
                             include_dirs=[numpy.get_include(),'proteus'],
                             extra_link_args=PROTEUS_EXTRA_LINK_ARGS,
                             extra_compile_args=PROTEUS_EXTRA_COMPILE_ARGS),
                   Extension('cpskRelations',
                             ['proteus/cpskRelationsModule.cpp','proteus/SubsurfaceTransportCoefficients.cpp'],
                             include_dirs=['proteus'],
                             libraries=['m'],
                             extra_compile_args=PROTEUS_EXTRA_COMPILE_ARGS,
                             extra_link_args=PROTEUS_EXTRA_LINK_ARGS),
                   Extension('cSubsurfaceTransportCoefficients',
                             ['proteus/cSubsurfaceTransportCoefficientsModule.cpp','proteus/SubsurfaceTransportCoefficients.cpp'],
                             include_dirs=[numpy.get_include(),'proteus'],
                             libraries=['m'],
                             extra_link_args=PROTEUS_EXTRA_LINK_ARGS,
                             extra_compile_args=PROTEUS_EXTRA_COMPILE_ARGS),
                   PetscExtension('flcbdfWrappers',
                                  ['proteus/flcbdfWrappersModule.cpp','proteus/mesh.cpp','proteus/meshio.cpp'],
                                  define_macros=[('PROTEUS_TRIANGLE_H',PROTEUS_TRIANGLE_H),
                                                 ('PROTEUS_SUPERLU_H',PROTEUS_SUPERLU_H),
                                                 ('CMRVEC_BOUNDS_CHECK',1),
                                                 ('MV_VECTOR_BOUNDS_CHECK',1),
                                                 ('PETSCVEC_BOUNDS_CHECK',1),
                                                 ('F77_POST_UNDERSCORE',1),
                                                 ('USE_BLAS',1)],
                                  include_dirs=['proteus',
                                                numpy.get_include(),
                                                PROTEUS_SUPERLU_INCLUDE_DIR,
                                                PROTEUS_TRIANGLE_INCLUDE_DIR,
                                                PROTEUS_DAETK_INCLUDE_DIR] + \
                                      PROTEUS_PETSC_INCLUDE_DIRS + \
                                      PROTEUS_MPI_INCLUDE_DIRS,
                                  library_dirs=[PROTEUS_DAETK_LIB_DIR]+PROTEUS_PETSC_LIB_DIRS+PROTEUS_MPI_LIB_DIRS,
                                  libraries=['stdc++','m',PROTEUS_DAETK_LIB]+PROTEUS_PETSC_LIBS+PROTEUS_MPI_LIBS,
                                  extra_link_args=PROTEUS_EXTRA_LINK_ARGS + PROTEUS_PETSC_EXTRA_LINK_ARGS,
                                  extra_compile_args=PROTEUS_EXTRA_COMPILE_ARGS + PROTEUS_PETSC_EXTRA_COMPILE_ARGS),
                                  Extension("mprans.cNCLS",["proteus/mprans/cNCLS.pyx"],depends=["proteus/mprans/NCLS.h"], language="c++",
                             include_dirs=[numpy.get_include(), 'proteus']),
                   Extension("mprans.cMCorr",["proteus/mprans/cMCorr.pyx"],depends=["proteus/mprans/MCorr.h"], define_macros=[('PROTEUS_LAPACK_H',PROTEUS_LAPACK_H),
                                            ('PROTEUS_LAPACK_INTEGER',PROTEUS_LAPACK_INTEGER),
                                            ('PROTEUS_BLAS_H',PROTEUS_BLAS_H)],language="c++",
                             include_dirs=[numpy.get_include(), 'proteus'],
                             library_dirs=[PROTEUS_LAPACK_LIB_DIR,
                                           PROTEUS_BLAS_LIB_DIR],
                             libraries=['m',PROTEUS_LAPACK_LIB,PROTEUS_BLAS_LIB],
                             extra_compile_args=PROTEUS_EXTRA_COMPILE_ARGS,
                             extra_link_args=PROTEUS_EXTRA_LINK_ARGS),
                   Extension("mprans.cRANS2P",["proteus/mprans/cRANS2P.pyx"], depends=["proteus/mprans/RANS2P.h"] + ["proteus/ModelFactory.h","proteus/CompKernel.h"],
                             language="c++", include_dirs=[numpy.get_include(), 'proteus']),
                   Extension("mprans.cRANS2P2D",["proteus/mprans/cRANS2P2D.pyx"],
                             depends=["proteus/mprans/RANS2P2D.h"] + ["proteus/ModelFactory.h","proteus/CompKernel.h"],
                             language="c++",
                             include_dirs=[numpy.get_include(), 'proteus']),
                   Extension("mprans.cRDLS",["proteus/mprans/cRDLS.pyx"],
                             depends=["proteus/mprans/RDLS.h"] + ["proteus/ModelFactory.h","proteus/CompKernel.h"],
                             language="c++",
                             include_dirs=[numpy.get_include(), 'proteus']),
                   Extension("mprans.cVOF",["proteus/mprans/cVOF.pyx"],
                             depends=["proteus/mprans/VOF.h"] + ["proteus/ModelFactory.h","proteus/CompKernel.h"],
                             language="c++",
                             include_dirs=[numpy.get_include(), 'proteus']),
                   Extension("mprans.cMoveMesh",["proteus/mprans/cMoveMesh.pyx"],
                             depends=["proteus/mprans/MoveMesh.h"] + ["proteus/ModelFactory.h","proteus/CompKernel.h"],
                             language="c++",
                             include_dirs=[numpy.get_include(), 'proteus']),
                   Extension("mprans.cMoveMesh2D",["proteus/mprans/cMoveMesh2D.pyx"],
                             depends=["proteus/mprans/MoveMesh2D.h"] + ["proteus/ModelFactory.h","proteus/CompKernel.h"],
                             language="c++",
                             include_dirs=[numpy.get_include(), 'proteus']),
                   Extension("mprans.cSW2D",["proteus/mprans/cSW2D.pyx"],
                             depends=["proteus/mprans/SW2D.h"] + ["proteus/ModelFactory.h","proteus/CompKernel.h"],
                             language="c++",
                             include_dirs=[numpy.get_include(), 'proteus'],
                             extra_compile_args=PROTEUS_EXTRA_COMPILE_ARGS+['-g'],
                             extra_link_args=PROTEUS_EXTRA_LINK_ARGS+['-g']),
                   Extension("mprans.cSW2DCV",["proteus/mprans/cSW2DCV.pyx"],
                             depends=["proteus/mprans/SW2DCV.h"] + ["proteus/ModelFactory.h","proteus/CompKernel.h"],
                             language="c++",
                             include_dirs=[numpy.get_include(), 'proteus'],
                             extra_compile_args=PROTEUS_EXTRA_COMPILE_ARGS+['-g'],
                             extra_link_args=PROTEUS_EXTRA_LINK_ARGS+['-g']),
                   Extension("mprans.cKappa",["proteus/mprans/cKappa.pyx"],
                             depends=["proteus/mprans/Kappa.h"] + ["proteus/ModelFactory.h","proteus/CompKernel.h"],
                             language="c++",
                             include_dirs=[numpy.get_include(), 'proteus']),
                   Extension("mprans.cKappa2D",["proteus/mprans/cKappa2D.pyx"],
                             depends=["proteus/mprans/Kappa2D.h"] + ["proteus/ModelFactory.h","proteus/CompKernel.h"],
                             language="c++",
                             include_dirs=[numpy.get_include(), 'proteus']),
                   Extension("mprans.cDissipation",["proteus/mprans/cDissipation.pyx"],
                             depends=["proteus/mprans/Dissipation.h"] + ["proteus/ModelFactory.h","proteus/CompKernel.h"],
                             language="c++",
                             include_dirs=[numpy.get_include(), 'proteus']),
                   Extension("mprans.cDissipation2D",["proteus/mprans/cDissipation2D.pyx"],
                             depends=["proteus/mprans/Dissipation2D.h"] + ["proteus/ModelFactory.h","proteus/CompKernel.h"],
                             language="c++",
                             include_dirs=[numpy.get_include(), 'proteus']),
                   Extension("mprans.cRANS2P_IB",["proteus/mprans/cRANS2P_IB.pyx"],
                             depends=["proteus/mprans/RANS2P_IB.h"] + ["proteus/ModelFactory.h","proteus/CompKernel.h"],
                             language="c++",
                             include_dirs=[numpy.get_include(), 'proteus']),
                   Extension("mprans.cBEAMS",["proteus/mprans/cBEAMS.pyx"],
                             depends=["proteus/mprans/BEAMS.h"] + ["proteus/ModelFactory.h","proteus/CompKernel.h"],
                             language="c++",
                             include_dirs=[numpy.get_include(), 'proteus']),

                   ],
      data_files=[(proteus_install_path,['proteus/proteus_blas.h', 'proteus/proteus_lapack.h',
                                         'proteus/ModelFactory.h', 'proteus/CompKernel.h']),(os.path.join(proteus_install_path,'tests'),['proteus/tests/hex_cube_3x3.xmf','proteus/tests/hex_cube_3x3.h5'])],
      scripts = ['scripts/parun','scripts/gf2poly','scripts/gatherArchives.py','scripts/qtm','scripts/waves2xmf',
                 'scripts/velocity2xmf','scripts/run_script_garnet','scripts/run_script_diamond',
                 'scripts/run_script_lonestar','scripts/run_script_ranger','scripts/run_script_mpiexec','scripts/gatherTimes.py'],
      requires=['numpy']
      )

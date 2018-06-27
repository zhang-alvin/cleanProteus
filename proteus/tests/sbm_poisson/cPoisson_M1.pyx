# A type of -*- python -*- file
import numpy
cimport numpy
from proteus import *
from proteus.Transport import *
from proteus.Transport import OneLevelTransport

cdef extern from "Poisson_M1.h" namespace "proteus":
    cdef cppclass Poisson_base:
        void calculateResidual(double dt,
                               double * mesh_trial_ref,
                               double * mesh_grad_trial_ref,
                               double * mesh_dof,
                               double * mesh_velocity_dof,
                               int * mesh_l2g,
                               double * dV_ref,
                               double * u_trial_ref,
                               double * u_grad_trial_ref,
                               double * u_test_ref,
                               double * u_grad_test_ref,
                               double * mesh_trial_trace_ref,
                               double * mesh_grad_trial_trace_ref,
                               double * dS_ref,
                               double * u_trial_trace_ref,
                               double * u_grad_trial_trace_ref,
                               double * u_test_trace_ref,
                               double * u_grad_test_trace_ref,
                               double * normal_ref,
                               double * boundaryJac_ref,
                               int nElements_global,
                               double alphaBDF,
                               int * u_l2g,
                               double * elementDiameter,
                               double * nodeDiametersArray,
                               double * u_dof,
                               double * r,
                               int u_offset,
                               int u_stride,
                               int nExteriorElementBoundaries_global,
                               int * exteriorElementBoundariesArray,
                               int * elementBoundariesArray,
                               int * elementBoundaryElementsArray,
                               int * elementBoundaryLocalElementBoundariesArray,
                               int u_ndofs,
                               int NNZ,
                               int * csrRowIndeces_DofLoops,
                               int * csrColumnOffsets_DofLoops,
                               int * csrRowIndeces_CellLoops_rho,
                               int * csrColumnOffsets_CellLoops_rho,
                               int * csrColumnOffsets_eb_CellLoops_rho,
                               double * quantDOFs,
                               double * ML,
                               double* isActiveDOF,
                               int USE_SBM
                               )
        void calculateJacobian(double dt,
                               double * mesh_trial_ref,
                               double * mesh_grad_trial_ref,
                               double * mesh_dof,
                               double * mesh_velocity_dof,
                               int * mesh_l2g,
                               double * dV_ref,
                               double * u_trial_ref,
                               double * u_grad_trial_ref,
                               double * u_test_ref,
                               double * u_grad_test_ref,
                               double * mesh_trial_trace_ref,
                               double * mesh_grad_trial_trace_ref,
                               double * dS_ref,
                               double * u_trial_trace_ref,
                               double * u_grad_trial_trace_ref,
                               double * u_test_trace_ref,
                               double * u_grad_test_trace_ref,
                               double * normal_ref,
                               double * boundaryJac_ref,
                               int nElements_global,
                               int * u_l2g,
                               double * elementDiameter,
                               double * u_dof,
                               double * velocity,
                               double * q_m_betaBDF,
                               int* csrRowIndeces_u_u,
                               int* csrColumnOffsets_u_u,
                               int* csrColumnOffsets_eb_u_u,
                               double * globalJacobian,
                               int nExteriorElementBoundaries_global,
                               int * exteriorElementBoundariesArray,
                               int * elementBoundariesArray,
                               int * elementBoundaryElementsArray,
                               int * elementBoundaryLocalElementBoundariesArray,
                               int USE_SBM)
        void calculateMassMatrix(double dt,
                                 double * mesh_trial_ref,
                                 double * mesh_grad_trial_ref,
                                 double * mesh_dof,
                                 double * mesh_velocity_dof,
                                 double MOVING_DOMAIN,
                                 int * mesh_l2g,
                                 double * dV_ref,
                                 double * u_trial_ref,
                                 double * u_grad_trial_ref,
                                 double * u_test_ref,
                                 double * u_grad_test_ref,
                                 double * mesh_trial_trace_ref,
                                 double * mesh_grad_trial_trace_ref,
                                 double * dS_ref,
                                 double * u_trial_trace_ref,
                                 double * u_grad_trial_trace_ref,
                                 double * u_test_trace_ref,
                                 double * u_grad_test_trace_ref,
                                 double * normal_ref,
                                 double * boundaryJac_ref,
                                 int nElements_global,
                                 double useMetrics,
                                 double alphaBDF,
                                 int lag_shockCapturing,
                                 double shockCapturingDiffusion,
                                 int * u_l2g,
                                 double * elementDiameter,
                                 int degree_polynomial,
                                 double * u_dof,
                                 double * velocity,
                                 double * q_m_betaBDF,
                                 double * cfl,
                                 double * q_numDiff_u_last,
                                 int * csrRowIndeces_u_u, int * csrColumnOffsets_u_u,
                                 double * globalJacobian,
                                 int nExteriorElementBoundaries_global,
                                 int * exteriorElementBoundariesArray,
                                 int * elementBoundaryElementsArray,
                                 int * elementBoundaryLocalElementBoundariesArray,
                                 double * ebqe_velocity_ext,
                                 int * isDOFBoundary_u,
                                 double * ebqe_rd_u_ext,
                                 double * ebqe_bc_u_ext,
                                 int * csrColumnOffsets_eb_u_u,
                                 int PURE_BDF,
                                 int LUMPED_MASS_MATRIX)

    Poisson_base * newPoisson(int nSpaceIn,
                        int nQuadraturePoints_elementIn,
                        int nDOF_mesh_trial_elementIn,
                        int nDOF_trial_elementIn,
                        int nDOF_test_elementIn,
                        int nQuadraturePoints_elementBoundaryIn,
                        int CompKernelFlag)

cdef class cPoisson_base:
    cdef Poisson_base * thisptr

    def __cinit__(self,
                  int nSpaceIn,
                  int nQuadraturePoints_elementIn,
                  int nDOF_mesh_trial_elementIn,
                  int nDOF_trial_elementIn,
                  int nDOF_test_elementIn,
                  int nQuadraturePoints_elementBoundaryIn,
                  int CompKernelFlag):
        self.thisptr = newPoisson(nSpaceIn,
                               nQuadraturePoints_elementIn,
                               nDOF_mesh_trial_elementIn,
                               nDOF_trial_elementIn,
                               nDOF_test_elementIn,
                               nQuadraturePoints_elementBoundaryIn,
                               CompKernelFlag)

    def __dealloc__(self):
        del self.thisptr

    def calculateResidual(self,
                          double dt,
                          numpy.ndarray mesh_trial_ref,
                          numpy.ndarray mesh_grad_trial_ref,
                          numpy.ndarray mesh_dof,
                          numpy.ndarray mesh_velocity_dof,
                          numpy.ndarray mesh_l2g,
                          numpy.ndarray dV_ref,
                          numpy.ndarray u_trial_ref,
                          numpy.ndarray u_grad_trial_ref,
                          numpy.ndarray u_test_ref,
                          numpy.ndarray u_grad_test_ref,
                          numpy.ndarray mesh_trial_trace_ref,
                          numpy.ndarray mesh_grad_trial_trace_ref,
                          numpy.ndarray dS_ref,
                          numpy.ndarray u_trial_trace_ref,
                          numpy.ndarray u_grad_trial_trace_ref,
                          numpy.ndarray u_test_trace_ref,
                          numpy.ndarray u_grad_test_trace_ref,
                          numpy.ndarray normal_ref,
                          numpy.ndarray boundaryJac_ref,
                          int nElements_global,
                          double alphaBDF,
                          numpy.ndarray u_l2g,
                          numpy.ndarray elementDiameter,
                          numpy.ndarray nodeDiametersArray,
                          numpy.ndarray u_dof,
                          numpy.ndarray r,
                          int u_offset,
                          int u_stride,
                          int nExteriorElementBoundaries_global,
                          numpy.ndarray exteriorElementBoundariesArray,
                          numpy.ndarray elementBoundariesArray,
                          numpy.ndarray elementBoundaryElementsArray,
                          numpy.ndarray elementBoundaryLocalElementBoundariesArray,
                          int u_ndofs,
                          int NNZ,
                          numpy.ndarray csrRowIndeces_DofLoops,
                          numpy.ndarray csrColumnOffsets_DofLoops,
                          numpy.ndarray csrRowIndeces_CellLoops_rho,
                          numpy.ndarray csrColumnOffsets_CellLoops_rho,
                          numpy.ndarray csrColumnOffsets_eb_CellLoops_rho,
                          numpy.ndarray quantDOFs,
                          numpy.ndarray ML,
                          numpy.ndarray isActiveDOF,
                          int USE_SBM
                          ):
        self.thisptr.calculateResidual(dt,
                                       < double * >mesh_trial_ref.data,
                                       < double * >mesh_grad_trial_ref.data,
                                       < double * >mesh_dof.data,
                                       < double * >mesh_velocity_dof.data,
                                       < int * >mesh_l2g.data,
                                       < double * >dV_ref.data,
                                       < double * >u_trial_ref.data,
                                       < double * >u_grad_trial_ref.data,
                                       < double * >u_test_ref.data,
                                       < double * >u_grad_test_ref.data,
                                       < double * >mesh_trial_trace_ref.data,
                                       < double * >mesh_grad_trial_trace_ref.data,
                                       < double * >dS_ref.data,
                                       < double * >u_trial_trace_ref.data,
                                       < double * >u_grad_trial_trace_ref.data,
                                       < double * >u_test_trace_ref.data,
                                       < double * >u_grad_test_trace_ref.data,
                                       < double * >normal_ref.data,
                                       < double * >boundaryJac_ref.data,
                                       nElements_global,
                                       alphaBDF,
                                       < int * >u_l2g.data,
                                       < double * >elementDiameter.data,
                                       < double * >nodeDiametersArray.data,
                                       < double * >u_dof.data,
                                       < double * >r.data,
                                       u_offset,
                                       u_stride,
                                       nExteriorElementBoundaries_global,
                                       < int * >exteriorElementBoundariesArray.data,
                                       < int * >elementBoundariesArray.data,
                                       < int * >elementBoundaryElementsArray.data,
                                       < int * >elementBoundaryLocalElementBoundariesArray.data,
                                       u_ndofs,
                                       NNZ,
                                       < int * >csrRowIndeces_DofLoops.data,
                                       < int * >csrColumnOffsets_DofLoops.data,
                                       < int * >csrRowIndeces_CellLoops_rho.data,
                                       < int * >csrColumnOffsets_CellLoops_rho.data,
                                       < int * >csrColumnOffsets_eb_CellLoops_rho.data,
                                       < double * > quantDOFs.data,
                                       < double * > ML.data,
                                       < double * > isActiveDOF.data,
                                       USE_SBM)

    def calculateJacobian(self,
                          double dt,
                          numpy.ndarray mesh_trial_ref,
                          numpy.ndarray mesh_grad_trial_ref,
                          numpy.ndarray mesh_dof,
                          numpy.ndarray mesh_velocity_dof,
                          numpy.ndarray mesh_l2g,
                          numpy.ndarray dV_ref,
                          numpy.ndarray u_trial_ref,
                          numpy.ndarray u_grad_trial_ref,
                          numpy.ndarray u_test_ref,
                          numpy.ndarray u_grad_test_ref,
                          numpy.ndarray mesh_trial_trace_ref,
                          numpy.ndarray mesh_grad_trial_trace_ref,
                          numpy.ndarray dS_ref,
                          numpy.ndarray u_trial_trace_ref,
                          numpy.ndarray u_grad_trial_trace_ref,
                          numpy.ndarray u_test_trace_ref,
                          numpy.ndarray u_grad_test_trace_ref,
                          numpy.ndarray normal_ref,
                          numpy.ndarray boundaryJac_ref,
                          int nElements_global,
                          numpy.ndarray u_l2g,
                          numpy.ndarray elementDiameter,
                          numpy.ndarray u_dof,
                          numpy.ndarray velocity,
                          numpy.ndarray q_m_betaBDF,
                          numpy.ndarray csrRowIndeces_u_u, 
                          numpy.ndarray csrColumnOffsets_u_u,
                          numpy.ndarray csrColumnOffsets_eb_u_u,
                          globalJacobian,
                          int nExteriorElementBoundaries_global,
                          numpy.ndarray exteriorElementBoundariesArray,
                          numpy.ndarray elementBoundariesArray,
                          numpy.ndarray elementBoundaryElementsArray,
                          numpy.ndarray elementBoundaryLocalElementBoundariesArray,
                          int USE_SBM):
        cdef numpy.ndarray rowptr, colind, globalJacobian_a
        (rowptr, colind, globalJacobian_a) = globalJacobian.getCSRrepresentation()
        self.thisptr.calculateJacobian(dt,
                                       < double * >mesh_trial_ref.data,
                                       < double * >mesh_grad_trial_ref.data,
                                       < double * >mesh_dof.data,
                                       < double * >mesh_velocity_dof.data,
                                       < int * >mesh_l2g.data,
                                       < double * >dV_ref.data,
                                       < double * >u_trial_ref.data,
                                       < double * >u_grad_trial_ref.data,
                                       < double * >u_test_ref.data,
                                       < double * >u_grad_test_ref.data,
                                       < double * >mesh_trial_trace_ref.data,
                                       < double * >mesh_grad_trial_trace_ref.data,
                                       < double * >dS_ref.data,
                                       < double * >u_trial_trace_ref.data,
                                       < double * >u_grad_trial_trace_ref.data,
                                       < double * >u_test_trace_ref.data,
                                       < double * >u_grad_test_trace_ref.data,
                                       < double * >normal_ref.data,
                                       < double * >boundaryJac_ref.data,
                                       nElements_global,
                                       < int * >u_l2g.data,
                                       < double * >elementDiameter.data,
                                       < double * >u_dof.data,
                                       < double * >velocity.data,
                                       < double * >q_m_betaBDF.data,
                                       < int * >csrRowIndeces_u_u.data, 
                                       < int * >csrColumnOffsets_u_u.data,
                                       < int * >csrColumnOffsets_eb_u_u.data,
                                       < double * >globalJacobian_a.data,
                                       nExteriorElementBoundaries_global,
                                       < int * >exteriorElementBoundariesArray.data,
                                       < int * >elementBoundariesArray.data,
                                       < int * >elementBoundaryElementsArray.data,
                                       < int * >elementBoundaryLocalElementBoundariesArray.data,
                                       USE_SBM)

    def calculateMassMatrix(self,
                            double dt,
                            numpy.ndarray mesh_trial_ref,
                            numpy.ndarray mesh_grad_trial_ref,
                            numpy.ndarray mesh_dof,
                            numpy.ndarray mesh_velocity_dof,
                            double MOVING_DOMAIN,
                            numpy.ndarray mesh_l2g,
                            numpy.ndarray dV_ref,
                            numpy.ndarray u_trial_ref,
                            numpy.ndarray u_grad_trial_ref,
                            numpy.ndarray u_test_ref,
                            numpy.ndarray u_grad_test_ref,
                            numpy.ndarray mesh_trial_trace_ref,
                            numpy.ndarray mesh_grad_trial_trace_ref,
                            numpy.ndarray dS_ref,
                            numpy.ndarray u_trial_trace_ref,
                            numpy.ndarray u_grad_trial_trace_ref,
                            numpy.ndarray u_test_trace_ref,
                            numpy.ndarray u_grad_test_trace_ref,
                            numpy.ndarray normal_ref,
                            numpy.ndarray boundaryJac_ref,
                            int nElements_global,
                            double useMetrics,
                            double alphaBDF,
                            int lag_shockCapturing,
                            double shockCapturingDiffusion,
                            numpy.ndarray u_l2g,
                            numpy.ndarray elementDiameter,
                            int degree_polynomial,
                            numpy.ndarray u_dof,
                            numpy.ndarray velocity,
                            numpy.ndarray q_m_betaBDF,
                            numpy.ndarray cfl,
                            numpy.ndarray q_numDiff_u_last,
                            numpy.ndarray csrRowIndeces_u_u, numpy.ndarray csrColumnOffsets_u_u,
                            globalJacobian,
                            int nExteriorElementBoundaries_global,
                            numpy.ndarray exteriorElementBoundariesArray,
                            numpy.ndarray elementBoundaryElementsArray,
                            numpy.ndarray elementBoundaryLocalElementBoundariesArray,
                            numpy.ndarray ebqe_velocity_ext,
                            numpy.ndarray isDOFBoundary_u,
                            numpy.ndarray ebqe_rd_u_ext,
                            numpy.ndarray ebqe_bc_u_ext,
                            numpy.ndarray csrColumnOffsets_eb_u_u,
                            int PURE_BDF,
                            int LUMPED_MASS_MATRIX):
        cdef numpy.ndarray rowptr, colind, globalJacobian_a
        (rowptr, colind, globalJacobian_a) = globalJacobian.getCSRrepresentation()
        self.thisptr.calculateMassMatrix(dt,
                                         < double * >mesh_trial_ref.data,
                                         < double * >mesh_grad_trial_ref.data,
                                         < double * >mesh_dof.data,
                                         < double * >mesh_velocity_dof.data,
                                         MOVING_DOMAIN,
                                         < int * >mesh_l2g.data,
                                         < double * >dV_ref.data,
                                         < double * >u_trial_ref.data,
                                         < double * >u_grad_trial_ref.data,
                                         < double * >u_test_ref.data,
                                         < double * >u_grad_test_ref.data,
                                         < double * >mesh_trial_trace_ref.data,
                                         < double * >mesh_grad_trial_trace_ref.data,
                                         < double * >dS_ref.data,
                                         < double * >u_trial_trace_ref.data,
                                         < double * >u_grad_trial_trace_ref.data,
                                         < double * >u_test_trace_ref.data,
                                         < double * >u_grad_test_trace_ref.data,
                                         < double * >normal_ref.data,
                                         < double * >boundaryJac_ref.data,
                                         nElements_global,
                                         useMetrics,
                                         alphaBDF,
                                         lag_shockCapturing,
                                         shockCapturingDiffusion,
                                         < int * >u_l2g.data,
                                         < double * >elementDiameter.data,
                                         degree_polynomial,
                                         < double * >u_dof.data,
                                         < double * >velocity.data,
                                         < double * >q_m_betaBDF.data,
                                         < double * >cfl.data,
                                         < double * >q_numDiff_u_last.data,
                                         < int * >csrRowIndeces_u_u.data, < int * >csrColumnOffsets_u_u.data,
                                         < double * >globalJacobian_a.data,
                                         nExteriorElementBoundaries_global,
                                         < int * >exteriorElementBoundariesArray.data,
                                         < int * >elementBoundaryElementsArray.data,
                                         < int * >elementBoundaryLocalElementBoundariesArray.data,
                                         < double * >ebqe_velocity_ext.data,
                                         < int * >isDOFBoundary_u.data,
                                         < double * >ebqe_rd_u_ext.data,
                                         < double * >ebqe_bc_u_ext.data,
                                         < int * >csrColumnOffsets_eb_u_u.data,
                                         PURE_BDF,
                                         LUMPED_MASS_MATRIX)

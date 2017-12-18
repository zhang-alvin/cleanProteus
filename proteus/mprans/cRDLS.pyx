import numpy
cimport numpy
from proteus import *
from proteus.Transport import *
from proteus.Transport import OneLevelTransport

cdef extern from "RDLS.h" namespace "proteus":
    cdef cppclass RDLS_base:
        void calculateResidual(double* mesh_trial_ref,
                               double* mesh_grad_trial_ref,
                               double* mesh_dof,
                               int* mesh_l2g,
                               double* dV_ref,
                               double* u_trial_ref,
                               double* u_grad_trial_ref,
                               double* u_test_ref,
                               double* u_grad_test_ref,
                               double* mesh_trial_trace_ref,
                               double* mesh_grad_trial_trace_ref,
                               double* dS_ref,
                               double* u_trial_trace_ref,
                               double* u_grad_trial_trace_ref,
                               double* u_test_trace_ref,
                               double* u_grad_test_trace_ref,
                               double* normal_ref,
                               double* boundaryJac_ref,
                               int nElements_global,
                               double useMetrics,
                               double alphaBDF,
                               double epsFact_redist,
                               double backgroundDiffusionFactor,
                               double weakDirichletFactor,
                               int freezeLevelSet,
                               int useTimeIntegration,
                               int lag_shockCapturing,
                               int lag_subgridError,
                               double shockCapturingDiffusion,
                               int* u_l2g,
                               double* elementDiameter,
                               double* nodeDiametersArray,
                               double* u_dof,
                               double* phi_ls,
                               double* q_m,
                               double* q_u,
                               double* q_n,
                               double* q_dH,
                               double* u_weak_internal_bc_dofs,
                               double* q_m_betaBDF,
                               double* q_dH_last,
                               double* q_cfl,
                               double* q_numDiff_u,
                               double* q_numDiff_u_last,
                               int* weakDirichletConditionFlags,
                               int offset_u, int stride_u,
                               double* globalResidual,
                               int nExteriorElementBoundaries_global,
                               int* exteriorElementBoundariesArray,
                               int* elementBoundaryElementsArray,
                               int* elementBoundaryLocalElementBoundariesArray,
                               double* ebqe_phi_ls_ext,
                               int* isDOFBoundary_u,
                               double* ebqe_bc_u_ext,
                               double* ebqe_u,
                               double* ebqe_n)
        void calculateJacobian(double* mesh_trial_ref,
                               double* mesh_grad_trial_ref,
                               double* mesh_dof,
                               int* mesh_l2g,
                               double* dV_ref,
                               double* u_trial_ref,
                               double* u_grad_trial_ref,
                               double* u_test_ref,
                               double* u_grad_test_ref,
                               double* mesh_trial_trace_ref,
                               double* mesh_grad_trial_trace_ref,
                               double* dS_ref,
                               double* u_trial_trace_ref,
                               double* u_grad_trial_trace_ref,
                               double* u_test_trace_ref,
                               double* u_grad_test_trace_ref,
                               double* normal_ref,
                               double* boundaryJac_ref,
                               int nElements_global,
                               double useMetrics,
                               double alphaBDF,
                               double epsFact_redist,
                               double backgroundDiffusionFactor,
                               double weakDirichletFactor,
                               int freezeLevelSet,
                               int useTimeIntegration,
                               int lag_shockCapturing,
                               int lag_subgridError,
                               double shockCapturingDiffusion,
                               int* u_l2g,
                               double* elementDiameter,
                               double* nodeDiametersArray,
                               double* u_dof,
                               double* u_weak_internal_bc_dofs,
                               double* phi_ls,
                               double* q_m_betaBDF,
                               double* q_dH_last,
                               double* q_cfl,
                               double* q_numDiff_u,
                               double* q_numDiff_u_last,
                               int * weakDirichletConditionFlags,
                               int* csrRowIndeces_u_u,int* csrColumnOffsets_u_u,
                               double* globalJacobian,
                               int nExteriorElementBoundaries_global,
                               int* exteriorElementBoundariesArray,
                               int* elementBoundaryElementsArray,
                               int* elementBoundaryLocalElementBoundariesArray,
                               double* ebqe_phi_ls_ext,
                               int* isDOFBoundary_u,
                               double* ebqe_bc_u_ext,
                               int* csrColumnOffsets_eb_u_u)
    RDLS_base* newRDLS(int nSpaceIn,
                       int nQuadraturePoints_elementIn,
                       int nDOF_mesh_trial_elementIn,
                       int nDOF_trial_elementIn,
                       int nDOF_test_elementIn,
                       int nQuadraturePoints_elementBoundaryIn,
                       int CompKernelFlag)

cdef class cRDLS_base:
    cdef RDLS_base* thisptr
    def __cinit__(self,
                  int nSpaceIn,
                  int nQuadraturePoints_elementIn,
                  int nDOF_mesh_trial_elementIn,
                  int nDOF_trial_elementIn,
                  int nDOF_test_elementIn,
                  int nQuadraturePoints_elementBoundaryIn,
                  int CompKernelFlag):
        self.thisptr = newRDLS(nSpaceIn,
                               nQuadraturePoints_elementIn,
                               nDOF_mesh_trial_elementIn,
                               nDOF_trial_elementIn,
                               nDOF_test_elementIn,
                               nQuadraturePoints_elementBoundaryIn,
                               CompKernelFlag)
    def __dealloc__(self):
        del self.thisptr
    def calculateResidual(self,
                          numpy.ndarray mesh_trial_ref,
                          numpy.ndarray mesh_grad_trial_ref,
                          numpy.ndarray mesh_dof,
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
                          double epsFact_redist,
                          double backgroundDiffusionFactor,
                          double weakDirichletFactor,
                          int freezeLevelSet,
                          int useTimeIntegration,
                          int lag_shockCapturing,
                          int lag_subgridError,
                          double shockCapturingDiffusion,
                          numpy.ndarray u_l2g,
                          numpy.ndarray elementDiameter,
                          numpy.ndarray nodeDiametersArray,
                          numpy.ndarray u_dof,
                          numpy.ndarray phi_ls,
                          numpy.ndarray q_m,
                          numpy.ndarray q_u,
                          numpy.ndarray q_n,
                          numpy.ndarray q_dH,
                          numpy.ndarray u_weak_internal_bc_dofs,
                          numpy.ndarray q_m_betaBDF,
                          numpy.ndarray q_dH_last,
                          numpy.ndarray q_cfl,
                          numpy.ndarray q_numDiff_u,
                          numpy.ndarray q_numDiff_u_last,
                          numpy.ndarray weakDirichletConditionFlags,
                          int offset_u, int stride_u,
                          numpy.ndarray globalResidual,
                          int nExteriorElementBoundaries_global,
                          numpy.ndarray exteriorElementBoundariesArray,
                          numpy.ndarray elementBoundaryElementsArray,
                          numpy.ndarray elementBoundaryLocalElementBoundariesArray,
                          numpy.ndarray ebqe_phi_ls_ext,
                          numpy.ndarray isDOFBoundary_u,
                          numpy.ndarray ebqe_bc_u_ext,
                          numpy.ndarray ebqe_u,
                          numpy.ndarray ebqe_n):
        self.thisptr.calculateResidual(<double*> mesh_trial_ref.data,
                                    <double*> mesh_grad_trial_ref.data,
                                    <double*> mesh_dof.data,
                                    <int*> mesh_l2g.data,
                                    <double*> dV_ref.data,
                                    <double*> u_trial_ref.data,
                                    <double*> u_grad_trial_ref.data,
                                    <double*> u_test_ref.data,
                                    <double*> u_grad_test_ref.data,
                                    <double*> mesh_trial_trace_ref.data,
                                    <double*> mesh_grad_trial_trace_ref.data,
                                    <double*> dS_ref.data,
                                    <double*> u_trial_trace_ref.data,
                                    <double*> u_grad_trial_trace_ref.data,
                                    <double*> u_test_trace_ref.data,
                                    <double*> u_grad_test_trace_ref.data,
                                    <double*> normal_ref.data,
                                    <double*> boundaryJac_ref.data,
                                    nElements_global,
                                    useMetrics,
                                    alphaBDF,
                                    epsFact_redist,
                                    backgroundDiffusionFactor,
                                    weakDirichletFactor,
                                    freezeLevelSet,
                                    useTimeIntegration,
                                    lag_shockCapturing,
                                    lag_subgridError,
                                    shockCapturingDiffusion,
                                    <int*> u_l2g.data,
                                    <double*> elementDiameter.data,
                                    <double*> nodeDiametersArray.data,
                                    <double*> u_dof.data,
                                    <double*> phi_ls.data,
                                    <double*> q_m.data,
                                    <double*> q_u.data,
                                    <double*> q_n.data,
                                    <double*> q_dH.data,
                                    <double*> u_weak_internal_bc_dofs.data,
                                    <double*> q_m_betaBDF.data,
                                    <double*> q_dH_last.data,
                                    <double*> q_cfl.data,
                                    <double*> q_numDiff_u.data,
                                    <double*> q_numDiff_u_last.data,
                                    <int*> weakDirichletConditionFlags.data,
                                    offset_u, stride_u,
                                    <double*> globalResidual.data,
                                    nExteriorElementBoundaries_global,
                                    <int*> exteriorElementBoundariesArray.data,
                                    <int*> elementBoundaryElementsArray.data,
                                    <int*> elementBoundaryLocalElementBoundariesArray.data,
                                    <double*> ebqe_phi_ls_ext.data,
                                    <int*> isDOFBoundary_u.data,
                                    <double*> ebqe_bc_u_ext.data,
                                    <double*> ebqe_u.data,
                                    <double*> ebqe_n.data)
    def calculateJacobian(self,
                          numpy.ndarray mesh_trial_ref,
                          numpy.ndarray mesh_grad_trial_ref,
                          numpy.ndarray mesh_dof,
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
                          double epsFact_redist,
                          double backgroundDiffusionFactor,
                          double weakDirichletFactor,
                          int freezeLevelSet,
                          int useTimeIntegration,
                          int lag_shockCapturing,
                          int lag_subgridError,
                          double shockCapturingDiffusion,
                          numpy.ndarray u_l2g,
                          numpy.ndarray elementDiameter,
                          numpy.ndarray nodeDiametersArray,
                          numpy.ndarray u_dof,
                          numpy.ndarray u_weak_internal_bc_dofs,
                          numpy.ndarray phi_ls,
                          numpy.ndarray q_m_betaBDF,
                          numpy.ndarray q_dH_last,
                          numpy.ndarray q_cfl,
                          numpy.ndarray q_numDiff_u,
                          numpy.ndarray q_numDiff_u_last,
                          numpy.ndarray weakDirichletConditionFlags,
                          numpy.ndarray csrRowIndeces_u_u,numpy.ndarray csrColumnOffsets_u_u,
                          globalJacobian,
                          int nExteriorElementBoundaries_global,
                          numpy.ndarray exteriorElementBoundariesArray,
                          numpy.ndarray elementBoundaryElementsArray,
                          numpy.ndarray elementBoundaryLocalElementBoundariesArray,
                          numpy.ndarray ebqe_phi_ls_ext,
                          numpy.ndarray isDOFBoundary_u,
                          numpy.ndarray ebqe_bc_u_ext,
                          numpy.ndarray csrColumnOffsets_eb_u_u):
        cdef numpy.ndarray rowptr,colind,globalJacobian_a
        (rowptr,colind,globalJacobian_a) = globalJacobian.getCSRrepresentation()
        self.thisptr.calculateJacobian(<double*> mesh_trial_ref.data,
                                        <double*> mesh_grad_trial_ref.data,
                                        <double*> mesh_dof.data,
                                        <int*> mesh_l2g.data,
                                        <double*> dV_ref.data,
                                        <double*> u_trial_ref.data,
                                        <double*> u_grad_trial_ref.data,
                                        <double*> u_test_ref.data,
                                        <double*> u_grad_test_ref.data,
                                        <double*> mesh_trial_trace_ref.data,
                                        <double*> mesh_grad_trial_trace_ref.data,
                                        <double*> dS_ref.data,
                                        <double*> u_trial_trace_ref.data,
                                        <double*> u_grad_trial_trace_ref.data,
                                        <double*> u_test_trace_ref.data,
                                        <double*> u_grad_test_trace_ref.data,
                                        <double*> normal_ref.data,
                                        <double*> boundaryJac_ref.data,
                                        nElements_global,
                                        useMetrics,
                                        alphaBDF,
                                        epsFact_redist,
                                        backgroundDiffusionFactor,
                                        weakDirichletFactor,
                                        freezeLevelSet,
                                        useTimeIntegration,
                                        lag_shockCapturing,
                                        lag_subgridError,
                                        shockCapturingDiffusion,
                                        <int*> u_l2g.data,
                                        <double*> elementDiameter.data,
                                        <double*> nodeDiametersArray.data,
                                        <double*> u_dof.data,
                                        <double*> u_weak_internal_bc_dofs.data,
                                        <double*> phi_ls.data,
                                        <double*> q_m_betaBDF.data,
                                        <double*> q_dH_last.data,
                                        <double*> q_cfl.data,
                                        <double*> q_numDiff_u.data,
                                        <double*> q_numDiff_u_last.data,
                                        <int*> weakDirichletConditionFlags.data,
                                        <int*> csrRowIndeces_u_u.data,<int*> csrColumnOffsets_u_u.data,
                                        <double*> globalJacobian_a.data,
                                        nExteriorElementBoundaries_global,
                                        <int*> exteriorElementBoundariesArray.data,
                                        <int*> elementBoundaryElementsArray.data,
                                        <int*> elementBoundaryLocalElementBoundariesArray.data,
                                        <double*> ebqe_phi_ls_ext.data,
                                        <int*> isDOFBoundary_u.data,
                                        <double*> ebqe_bc_u_ext.data,
                                        <int*> csrColumnOffsets_eb_u_u.data)

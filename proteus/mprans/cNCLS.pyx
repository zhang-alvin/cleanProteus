import numpy
cimport numpy
from proteus import *
from proteus.Transport import *
from proteus.Transport import OneLevelTransport

cdef extern from "NCLS.h" namespace "proteus":
    cdef cppclass NCLS_base:
        void FCTStep(double dt, 
	             int NNZ,
		     int numDOFs,
		     double* lumped_mass_matrix, 
		     double* soln, 
		     double* solH, 
		     double* flux_plus_dLij_times_soln, 
		     int* csrRowIndeces_DofLoops, 
		     int* csrColumnOffsets_DofLoops, 
		     double* MassMatrix, 
		     double* dL_minus_dE,
                     double* min_u_bc,
                     double* max_u_bc) 
        void calculateResidual(double* mesh_trial_ref,
                               double* mesh_grad_trial_ref,
                               double* mesh_dof,
                               double* mesh_velocity_dof,
                               double MOVING_DOMAIN,
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
                               int lag_shockCapturing,
                               double shockCapturingDiffusion,
		               double sc_uref, double sc_alpha,
                               int* u_l2g, 
                               double* elementDiameter,
                               double* u_dof,
			       double* u_dof_old,
			       double* u_dof_old_old,
                               double* velocity,
                               double* q_m,
                               double* q_u,				   
			       double* q_n,
                               double* q_dH,
                               double* q_m_betaBDF,
                               double* q_dV,
                               double* q_dV_last,
                               double* cfl,
                               double* q_numDiff_u, 
                               double* q_numDiff_u_last, 
                               int offset_u, int stride_u, 
                               double* globalResidual,
                               int nExteriorElementBoundaries_global,
                               int* exteriorElementBoundariesArray,
                               int* elementBoundaryElementsArray,
                               int* elementBoundaryLocalElementBoundariesArray,
                               double* ebqe_velocity_ext,
                               int* isDOFBoundary_u,
                               double* ebqe_rd_u_ext,
                               double* ebqe_bc_u_ext,
                               double* ebqe_u, 
			       int EDGE_VISCOSITY, 
			       int ENTROPY_VISCOSITY, 
			       int numDOFs, 
			       int NNZ, 
			       int* csrRowIndeces_DofLoops, 
			       int* csrColumnOffsets_DofLoops,
			       int* csrRowIndeces_CellLoops, 
			       int* csrColumnOffsets_CellLoops,
			       int* csrColumnOffsets_eb_CellLoops,
			       int POWER_SMOOTHNESS_INDICATOR, 
			       int LUMPED_MASS_MATRIX, 
			       double* flux_plus_dLij_times_soln,
			       double* dL_minus_dE, 
			       double* min_u_bc,
			       double* max_u_bc, 
			       double* quantDOFs
			       )
        void calculateJacobian(double* mesh_trial_ref,
                               double* mesh_grad_trial_ref,
                               double* mesh_dof,
                               double* mesh_velocity_dof,
                               double MOVING_DOMAIN,
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
                               int lag_shockCapturing,
                               double shockCapturingDiffusion,
                               int* u_l2g,
                               double* elementDiameter,
                               double* u_dof, 
                               double* velocity,
                               double* q_m_betaBDF, 
                               double* cfl,
                               double* q_numDiff_u_last, 
                               int* csrRowIndeces_u_u,int* csrColumnOffsets_u_u,
                               double* globalJacobian,
                               int nExteriorElementBoundaries_global,
                               int* exteriorElementBoundariesArray,
                               int* elementBoundaryElementsArray,
                               int* elementBoundaryLocalElementBoundariesArray,
                               double* ebqe_velocity_ext,
                               int* isDOFBoundary_u,
                               double* ebqe_rd_u_ext,
                               double* ebqe_bc_u_ext,
                               int* csrColumnOffsets_eb_u_u, 
			       int EDGE_VISCOSITY, 
			       int ENTROPY_VISCOSITY,
			       int LUMPED_MASS_MATRIX)
        void calculateWaterline(
		               int*    wlc,
	                       double* waterline,    
	                       double* mesh_trial_ref,
                               double* mesh_grad_trial_ref,
                               double* mesh_dof,
                               double* mesh_velocity_dof,
                               double MOVING_DOMAIN,
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
                               int lag_shockCapturing,
                               double shockCapturingDiffusion,
		               double sc_uref, double sc_alpha,
                               int* u_l2g, 
                               double* elementDiameter,
                               double* u_dof,double* u_dof_old,
                               double* velocity,
                               double* q_m,
                               double* q_u,				   
			       double* q_n,
                               double* q_dH,
                               double* q_m_betaBDF,
                               double* cfl,
                               double* q_numDiff_u, 
                               double* q_numDiff_u_last, 
                               int offset_u, int stride_u, 
                               int nExteriorElementBoundaries_global,
                               int* exteriorElementBoundariesArray,
                               int* elementBoundaryElementsArray,
                               int* elementBoundaryLocalElementBoundariesArray,
                               int* elementBoundaryMaterialTypes,
                               double* ebqe_velocity_ext,
                               int* isDOFBoundary_u,
                               double* ebqe_bc_u_ext,
                               double* ebqe_u)


    NCLS_base* newNCLS(int nSpaceIn,
                       int nQuadraturePoints_elementIn,
                       int nDOF_mesh_trial_elementIn,
                       int nDOF_trial_elementIn,
                       int nDOF_test_elementIn,
                       int nQuadraturePoints_elementBoundaryIn,
                       int CompKernelFlag)

cdef class cNCLS_base:
   cdef NCLS_base* thisptr
   def __cinit__(self,
                 int nSpaceIn,
                 int nQuadraturePoints_elementIn,
                 int nDOF_mesh_trial_elementIn,
                 int nDOF_trial_elementIn,
                 int nDOF_test_elementIn,
                 int nQuadraturePoints_elementBoundaryIn,
                 int CompKernelFlag):
       self.thisptr = newNCLS(nSpaceIn,
                              nQuadraturePoints_elementIn,
                              nDOF_mesh_trial_elementIn,
                              nDOF_trial_elementIn,
                              nDOF_test_elementIn,
                              nQuadraturePoints_elementBoundaryIn,
                              CompKernelFlag)
   def __dealloc__(self):
       del self.thisptr
   def FCTStep(self, 
                double dt, 
                int NNZ,
                int numDOFs,
                numpy.ndarray lumped_mass_matrix, 
                numpy.ndarray soln, 
                numpy.ndarray solH, 
                numpy.ndarray flux_plus_dLij_times_soln, 
                numpy.ndarray csrRowIndeces_DofLoops, 
                numpy.ndarray csrColumnOffsets_DofLoops, 
                numpy.ndarray MassMatrix, 
                numpy.ndarray dL_minus_dE,
                numpy.ndarray min_u_bc,
                numpy.ndarray max_u_bc):
        self.thisptr.FCTStep(dt, 
                             NNZ,
                             numDOFs,
                             <double*> lumped_mass_matrix.data, 
                             <double*> soln.data, 
                             <double*> solH.data,
                             <double*> flux_plus_dLij_times_soln.data,
                             <int*> csrRowIndeces_DofLoops.data,
                             <int*> csrColumnOffsets_DofLoops.data,
                             <double*> MassMatrix.data,
                             <double*> dL_minus_dE.data,
                             <double*> min_u_bc.data,
                             <double*> max_u_bc.data)
   def calculateResidual(self,
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
		         double sc_uref, double sc_alpha,		 
                         numpy.ndarray u_l2g, 
                         numpy.ndarray elementDiameter,
                         numpy.ndarray u_dof,
			 numpy.ndarray u_dof_old,
			 numpy.ndarray u_dof_old_old,
                         numpy.ndarray velocity,
                         numpy.ndarray q_m,
                         numpy.ndarray q_u,
			 numpy.ndarray q_n,
                         numpy.ndarray q_dH,
                         numpy.ndarray q_m_betaBDF,
                         numpy.ndarray q_dV,
                         numpy.ndarray q_dV_last,
                         numpy.ndarray cfl,
                         numpy.ndarray q_numDiff_u, 
                         numpy.ndarray q_numDiff_u_last, 
                         int offset_u, int stride_u, 
                         numpy.ndarray globalResidual,
                         int nExteriorElementBoundaries_global,
                         numpy.ndarray exteriorElementBoundariesArray,
                         numpy.ndarray elementBoundaryElementsArray,
                         numpy.ndarray elementBoundaryLocalElementBoundariesArray,
                         numpy.ndarray ebqe_velocity_ext,
                         numpy.ndarray isDOFBoundary_u,
                         numpy.ndarray ebqe_rd_u_ext,
                         numpy.ndarray ebqe_bc_u_ext,
                         numpy.ndarray ebqe_u,
			 int EDGE_VISCOSITY, 
			 int ENTROPY_VISCOSITY,
			 int numDOFs, 
			 int NNZ, 
			 numpy.ndarray csrRowIndeces_DofLoops,
			 numpy.ndarray csrColumnOffsets_DofLoops,
			 numpy.ndarray csrRowIndeces_CellLoops,
			 numpy.ndarray csrColumnOffsets_CellLoops,
			 numpy.ndarray csrColumnOffsets_eb_CellLoops,
			 int POWER_SMOOTHNESS_INDICATOR, 
			 int LUMPED_MASS_MATRIX, 
			 numpy.ndarray flux_plus_dLij_times_soln,
			 numpy.ndarray dL_minus_dE, 
			 numpy.ndarray min_u_bc,
			 numpy.ndarray max_u_bc, 
			 numpy.ndarray quantDOFs):
       self.thisptr.calculateResidual(<double*>mesh_trial_ref.data,
                                       <double*>mesh_grad_trial_ref.data,
                                       <double*>mesh_dof.data,
                                       <double*>mesh_velocity_dof.data,
                                       MOVING_DOMAIN,
                                       <int*>mesh_l2g.data,
                                       <double*>dV_ref.data,
                                       <double*>u_trial_ref.data,
                                       <double*>u_grad_trial_ref.data,
                                       <double*>u_test_ref.data,
                                       <double*>u_grad_test_ref.data,
                                       <double*>mesh_trial_trace_ref.data,
                                       <double*>mesh_grad_trial_trace_ref.data,
                                       <double*>dS_ref.data,
                                       <double*>u_trial_trace_ref.data,
                                       <double*>u_grad_trial_trace_ref.data,
                                       <double*>u_test_trace_ref.data,
                                       <double*>u_grad_test_trace_ref.data,
                                       <double*>normal_ref.data,
                                       <double*>boundaryJac_ref.data,
                                       nElements_global,
			               useMetrics, 
                                       alphaBDF,
                                       lag_shockCapturing,
                                       shockCapturingDiffusion,
				       sc_uref, sc_alpha,
                                       <int*>u_l2g.data, 
                                       <double*>elementDiameter.data,
                                       <double*>u_dof.data,
				       <double*>u_dof_old.data,
				       <double*>u_dof_old_old.data,
                                       <double*>velocity.data,
                                       <double*>q_m.data,
                                       <double*>q_u.data,
                                       <double*>q_n.data,
                                       <double*>q_dH.data,
                                       <double*>q_m_betaBDF.data,
                                       <double*> q_dV.data,
                                       <double*> q_dV_last.data,
                                       <double*>cfl.data,
                                       <double*>q_numDiff_u.data, 
                                       <double*>q_numDiff_u_last.data, 
                                       offset_u, 
                                       stride_u, 
                                       <double*>globalResidual.data,
                                       nExteriorElementBoundaries_global,
                                       <int*>exteriorElementBoundariesArray.data,
                                       <int*>elementBoundaryElementsArray.data,
                                       <int*>elementBoundaryLocalElementBoundariesArray.data,
                                       <double*>ebqe_velocity_ext.data,
                                       <int*>isDOFBoundary_u.data,
                                       <double*>ebqe_rd_u_ext.data,
                                       <double*>ebqe_bc_u_ext.data,
                                       <double*>ebqe_u.data, 
				       EDGE_VISCOSITY, 
				       ENTROPY_VISCOSITY, 
				       numDOFs, 
				       NNZ, 
				       <int*>csrRowIndeces_DofLoops.data,
				       <int*>csrColumnOffsets_DofLoops.data, 
				       <int*>csrRowIndeces_CellLoops.data,
				       <int*>csrColumnOffsets_CellLoops.data,
				       <int*>csrColumnOffsets_eb_CellLoops.data, 
				       POWER_SMOOTHNESS_INDICATOR, 
				       LUMPED_MASS_MATRIX, 
				       <double*> flux_plus_dLij_times_soln.data,
				       <double*> dL_minus_dE.data, 
				       <double*> min_u_bc.data,
				       <double*> max_u_bc.data, 
				       <double*> quantDOFs.data)
   def calculateJacobian(self,
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
                         numpy.ndarray u_dof, 
                         numpy.ndarray velocity,
                         numpy.ndarray q_m_betaBDF, 
                         numpy.ndarray cfl,
                         numpy.ndarray q_numDiff_u_last, 
                         numpy.ndarray csrRowIndeces_u_u,numpy.ndarray csrColumnOffsets_u_u,
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
			 int EDGE_VISCOSITY, 
			 int ENTROPY_VISCOSITY,
			 int LUMPED_MASS_MATRIX):
       cdef numpy.ndarray rowptr,colind,globalJacobian_a
       (rowptr,colind,globalJacobian_a) = globalJacobian.getCSRrepresentation()
       self.thisptr.calculateJacobian(<double*>mesh_trial_ref.data,
                                       <double*>mesh_grad_trial_ref.data,
                                       <double*>mesh_dof.data,
                                       <double*>mesh_velocity_dof.data,
                                       MOVING_DOMAIN,
                                       <int*>mesh_l2g.data,
                                       <double*>dV_ref.data,
                                       <double*>u_trial_ref.data,
                                       <double*>u_grad_trial_ref.data,
                                       <double*>u_test_ref.data,
                                       <double*>u_grad_test_ref.data,
                                       <double*>mesh_trial_trace_ref.data,
                                       <double*>mesh_grad_trial_trace_ref.data,
                                       <double*>dS_ref.data,
                                       <double*>u_trial_trace_ref.data,
                                       <double*>u_grad_trial_trace_ref.data,
                                       <double*>u_test_trace_ref.data,
                                       <double*>u_grad_test_trace_ref.data,
                                       <double*>normal_ref.data,
                                       <double*>boundaryJac_ref.data,
                                       nElements_global,
			               useMetrics, 
                                       alphaBDF,
                                       lag_shockCapturing,
                                       shockCapturingDiffusion,
                                       <int*>u_l2g.data,
                                       <double*>elementDiameter.data,
                                       <double*>u_dof.data, 
                                       <double*>velocity.data,
                                       <double*>q_m_betaBDF.data, 
                                       <double*>cfl.data,
                                       <double*>q_numDiff_u_last.data, 
                                       <int*>csrRowIndeces_u_u.data,<int*>csrColumnOffsets_u_u.data,
                                       <double*>globalJacobian_a.data,
                                       nExteriorElementBoundaries_global,
                                       <int*>exteriorElementBoundariesArray.data,
                                       <int*>elementBoundaryElementsArray.data,
                                       <int*>elementBoundaryLocalElementBoundariesArray.data,
                                       <double*>ebqe_velocity_ext.data,
                                       <int*>isDOFBoundary_u.data,
                                       <double*>ebqe_rd_u_ext.data,
                                       <double*>ebqe_bc_u_ext.data,
                                       <int*>csrColumnOffsets_eb_u_u.data, 
				       EDGE_VISCOSITY, 
				       ENTROPY_VISCOSITY,
				       LUMPED_MASS_MATRIX)
   def calculateWaterline(self,
		         numpy.ndarray wlc,
	                 numpy.ndarray waterline, 
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
		         double sc_uref, double sc_alpha,		 
                         numpy.ndarray u_l2g, 
                         numpy.ndarray elementDiameter,
                         numpy.ndarray u_dof,
			 numpy.ndarray u_dof_old,
                         numpy.ndarray velocity,
                         numpy.ndarray q_m,
                         numpy.ndarray q_u,
			 numpy.ndarray q_n,
                         numpy.ndarray q_dH,
                         numpy.ndarray q_m_betaBDF,
                         numpy.ndarray cfl,
                         numpy.ndarray q_numDiff_u, 
                         numpy.ndarray q_numDiff_u_last, 
                         int offset_u, int stride_u, 
                         int nExteriorElementBoundaries_global,
                         numpy.ndarray exteriorElementBoundariesArray,
                         numpy.ndarray elementBoundaryElementsArray,
                         numpy.ndarray elementBoundaryLocalElementBoundariesArray,
                         numpy.ndarray elementBoundaryMaterialTypes,
                         numpy.ndarray ebqe_velocity_ext,
                         numpy.ndarray isDOFBoundary_u,
                         numpy.ndarray ebqe_bc_u_ext,
                         numpy.ndarray ebqe_u):
       self.thisptr.calculateWaterline(<int*>wlc.data,
	                               <double*>waterline.data,
			               <double*>mesh_trial_ref.data,
                                       <double*>mesh_grad_trial_ref.data,
                                       <double*>mesh_dof.data,
                                       <double*>mesh_velocity_dof.data,
                                       MOVING_DOMAIN,
                                       <int*>mesh_l2g.data,
                                       <double*>dV_ref.data,
                                       <double*>u_trial_ref.data,
                                       <double*>u_grad_trial_ref.data,
                                       <double*>u_test_ref.data,
                                       <double*>u_grad_test_ref.data,
                                       <double*>mesh_trial_trace_ref.data,
                                       <double*>mesh_grad_trial_trace_ref.data,
                                       <double*>dS_ref.data,
                                       <double*>u_trial_trace_ref.data,
                                       <double*>u_grad_trial_trace_ref.data,
                                       <double*>u_test_trace_ref.data,
                                       <double*>u_grad_test_trace_ref.data,
                                       <double*>normal_ref.data,
                                       <double*>boundaryJac_ref.data,
                                       nElements_global,
			               useMetrics, 
                                       alphaBDF,
                                       lag_shockCapturing,
                                       shockCapturingDiffusion,
				       sc_uref, sc_alpha,
                                       <int*>u_l2g.data, 
                                       <double*>elementDiameter.data,
                                       <double*>u_dof.data,
				       <double*>u_dof_old.data,
                                       <double*>velocity.data,
                                       <double*>q_m.data,
                                       <double*>q_u.data,
                                       <double*>q_n.data,
                                       <double*>q_dH.data,
                                       <double*>q_m_betaBDF.data,
                                       <double*>cfl.data,
                                       <double*>q_numDiff_u.data, 
                                       <double*>q_numDiff_u_last.data, 
                                       offset_u, 
                                       stride_u, 
                                       nExteriorElementBoundaries_global,
                                       <int*>exteriorElementBoundariesArray.data,
                                       <int*>elementBoundaryElementsArray.data,
                                       <int*>elementBoundaryLocalElementBoundariesArray.data,
				       <int*>elementBoundaryMaterialTypes.data,
                                       <double*>ebqe_velocity_ext.data,
                                       <int*>isDOFBoundary_u.data,
                                       <double*>ebqe_bc_u_ext.data,
                                       <double*>ebqe_u.data)

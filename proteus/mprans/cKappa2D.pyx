# A type of -*- python -*- file
import numpy
cimport numpy
from proteus import *
from proteus.Transport import *
from proteus.Transport import OneLevelTransport

cdef extern from "Kappa2D.h" namespace "proteus":
    cdef cppclass Kappa2D_base:
        void calculateResidual(double * mesh_trial_ref,
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
                               # diffusion terms
                               double nu_0,
                               double nu_1,
                               double sigma_k,
                               double c_mu,
                               double rho_0,
                               double rho_1,
#Sediment model
                                double sedFlag,
                                double* q_vos,
                                double *q_vos_gradc,
                                double* ebqe_q_vos,
                                double *ebqe_q_vos_gradc,
                                double rho_f,
                                double rho_s,
                                double* vs,
                                double* ebqe_vs,
                                double* g,
#end Sediment
                              int dissipation_model_flag,
                               # end diffusion
                               double useMetrics,
                               double alphaBDF,
                               int lag_shockCapturing,
                               double shockCapturingDiffusion,
                               double sc_uref, double sc_alpha,
                               int * u_l2g,
                               double * elementDiameter,
                               double * u_dof, double * u_dof_old,
                               double * velocity,
                               double * phi_ls,  # level set variable
                               double * q_dissipation,  # dissipation rate variable
                               double * q_grad_dissipation,
                               double * q_porosity,  # VRANS
                               # velocity dof
                               double * velocity_dof_u,
                               double * velocity_dof_v,
                               double * velocity_dof_w,
                               # end velocity dof
                               double * q_m,
                               double * q_u,
                               double * q_grad_u,
                               double * q_m_betaBDF,
                               double * cfl,
                               double * q_numDiff_u,
                               double * q_numDiff_u_last,
                               double * ebqe_penalty_ext,
                               int offset_u, int stride_u,
                               double * globalResidual,
                               int nExteriorElementBoundaries_global,
                               int * exteriorElementBoundariesArray,
                               int * elementBoundaryElementsArray,
                               int * elementBoundaryLocalElementBoundariesArray,
                               double * ebqe_velocity_ext,
                               int * isDOFBoundary_u,
                               double * ebqe_bc_u_ext,
                               int * isAdvectiveFluxBoundary_u,
                               double * ebqe_bc_advectiveFlux_u_ext,
                               int * isDiffusiveFluxBoundary_u,
                               double * ebqe_bc_diffusiveFlux_u_ext,
                               double * ebqe_phi, double epsFact,
                               double * ebqe_dissipation,  # dissipation rate
                               double * ebqe_porosity,  # VRANS
                               double * ebqe_u,
                               double * ebqe_flux)
        void calculateJacobian(double * mesh_trial_ref,
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
                               # diffusion
                               double nu_0,
                               double nu_1,
                               double sigma_k,
                               double c_mu,
                               double rho_0,
                               double rho_1,
                               int dissipation_model_flag,
                               # end diffuion
                               double useMetrics,
                               double alphaBDF,
                               int lag_shockCapturing,
                               double shockCapturingDiffusion,
                               int * u_l2g,
                               double * elementDiameter,
                               double * u_dof, double * u_dof_old,
                               double * velocity,
                               double * phi_ls,  # level set variable
                               double * q_dissipation,  # dissipation rate
                               double * q_grad_dissipation,
                               double * q_porosity,  # VRANS
#Sediment model
                                double sedFlag,
                                double* q_vos,
                                double *q_vos_gradc,
                                double* ebqe_q_vos,
                                double *ebqe_q_vos_gradc,
                                double rho_f,
                                double rho_s,
                                double* vs,
                                double* ebqe_vs,
                                double* g,
#end Sediment
                               # velocity dof
                               double * velocity_dof_u,
                               double * velocity_dof_v,
                               double * velocity_dof_w,
                               # end velocity dof
                               double * q_m_betaBDF,
                               double * cfl,
                               double * q_numDiff_u_last,
                               double * ebqe_penalty_ext,
                               int * csrRowIndeces_u_u, int * csrColumnOffsets_u_u,
                               double * globalJacobian,
                               int nExteriorElementBoundaries_global,
                               int * exteriorElementBoundariesArray,
                               int * elementBoundaryElementsArray,
                               int * elementBoundaryLocalElementBoundariesArray,
                               double * ebqe_velocity_ext,
                               int * isDOFBoundary_u,
                               double * ebqe_bc_u_ext,
                               int * isAdvectiveFluxBoundary_u,
                               double * ebqe_bc_advectiveFlux_u_ext,
                               int * isDiffusiveFluxBoundary_u,
                               double * ebqe_bc_diffusiveFlux_u_ext,
                               int * csrColumnOffsets_eb_u_u,
                               double * ebqe_phi, double epsFact,
                               double * ebqe_dissipation,  # dissipation rate
                               double * ebqe_porosity)  # VRANS
    Kappa2D_base * newKappa2D(int nSpaceIn,
                                          int nQuadraturePoints_elementIn,
                                          int nDOF_mesh_trial_elementIn,
                                          int nDOF_trial_elementIn,
                                          int nDOF_test_elementIn,
                                          int nQuadraturePoints_elementBoundaryIn,
                                          int CompKernelFlag,
                                          double aDarcy,
                                          double betaForch,
                                          double grain,
                                          double packFraction,
                                          double packMargin,
                                          double maxFraction,
                                                 double frFraction,
                                                 double sigmaC,
                                                 double C3e,
                                                 double C4e,
                                                 double eR,
                                                 double fContact,
                                                 double mContact,
                                                 double nContact,
                                                 double angFriction,
                                     double vos_limiter,
                                     double mu_fr_limiter)

cdef class cKappa2D_base:
    cdef Kappa2D_base * thisptr

    def __cinit__(self,
                  int nSpaceIn,
                  int nQuadraturePoints_elementIn,
                  int nDOF_mesh_trial_elementIn,
                  int nDOF_trial_elementIn,
                  int nDOF_test_elementIn,
                  int nQuadraturePoints_elementBoundaryIn,
                  int CompKernelFlag,
                  double aDarcy,
                  double betaForch,
                  double grain,
                  double packFraction,
                  double packMargin,
                  double maxFraction,
                  double frFraction,
                  double sigmaC,
                  double C3e,
                  double C4e,
                  double eR,
                  double fContact,
                  double mContact,
                  double nContact,
                  double angFriction,
                  double vos_limiter,
                  double mu_fr_limiter):

        self.thisptr = newKappa2D(nSpaceIn,
                                  nQuadraturePoints_elementIn,
                                  nDOF_mesh_trial_elementIn,
                                  nDOF_trial_elementIn,
                                  nDOF_test_elementIn,
                                  nQuadraturePoints_elementBoundaryIn,
                                        CompKernelFlag,
                          aDarcy,
                  betaForch,
                   grain,
                   packFraction,
                   packMargin,
                   maxFraction,
                   frFraction,
                   sigmaC,
                   C3e,
                   C4e,
                   eR,
                   fContact,
                   mContact,
                   nContact,
                   angFriction,
                   vos_limiter,
                   mu_fr_limiter)
    def __dealloc__(self):
        del self.thisptr

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
                          # diffusion terms
                          double nu_0,
                          double nu_1,
                          double sigma_k,
                          double c_mu,
                          double rho_0,
                          double rho_1,
  #                             Argumentlist for sediment
                           double sedFlag,
                           numpy.ndarray  q_vos,
                           numpy.ndarray q_vos_gradc,
                           numpy.ndarray  ebqe_q_vos,
                           numpy.ndarray ebqe_q_vos_gradc,
                           double rho_f,
                           double rho_s,
                           numpy.ndarray  vs,
                           numpy.ndarray  ebqe_vs,
                           numpy.ndarray g,
  #                             end for sediment
                          int dissipation_model_flag,
                          # end diffusion
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
                          numpy.ndarray phi_ls,  # level set variable
                          numpy.ndarray q_dissipation,  # dissipation rate
                          numpy.ndarray q_grad_dissipation,  # VRANS
                          numpy.ndarray q_porosity,  # VRANS
                          # velocity dof
                          numpy.ndarray velocity_dof_u,
                          numpy.ndarray velocity_dof_v,
                          numpy.ndarray velocity_dof_w,
                          # end velocity dof
                          numpy.ndarray q_m,
                          numpy.ndarray q_u,
                          numpy.ndarray q_grad_u,
                          numpy.ndarray q_m_betaBDF,
                          numpy.ndarray cfl,
                          numpy.ndarray q_numDiff_u,
                          numpy.ndarray q_numDiff_u_last,
                          numpy.ndarray ebqe_penalty_ext,
                          int offset_u, int stride_u,
                          numpy.ndarray globalResidual,
                          int nExteriorElementBoundaries_global,
                          numpy.ndarray exteriorElementBoundariesArray,
                          numpy.ndarray elementBoundaryElementsArray,
                          numpy.ndarray elementBoundaryLocalElementBoundariesArray,
                          numpy.ndarray ebqe_velocity_ext,
                          numpy.ndarray isDOFBoundary_u,
                          numpy.ndarray ebqe_bc_u_ext,
                          numpy.ndarray isAdvectiveFluxBoundary_u,
                          numpy.ndarray ebqe_bc_advectiveFlux_u_ext,
                          numpy.ndarray isDiffusiveFluxBoundary_u,
                          numpy.ndarray ebqe_bc_diffusiveFlux_u_ext,
                          numpy.ndarray ebqe_phi, double epsFact,
                          numpy.ndarray ebqe_dissipation,  # dissipation rate
                          numpy.ndarray ebqe_porosity,  # VRANS
                          numpy.ndarray ebqe_u,
                          numpy.ndarray ebqe_flux):
        self.thisptr.calculateResidual(< double*> mesh_trial_ref.data,
                                        < double * > mesh_grad_trial_ref.data,
                                        < double * > mesh_dof.data,
                                        < double * > mesh_velocity_dof.data,
                                        MOVING_DOMAIN,
                                        < int * > mesh_l2g.data,
                                        < double * > dV_ref.data,
                                        < double * > u_trial_ref.data,
                                        < double * > u_grad_trial_ref.data,
                                        < double * > u_test_ref.data,
                                        < double * > u_grad_test_ref.data,
                                        < double * > mesh_trial_trace_ref.data,
                                        < double * > mesh_grad_trial_trace_ref.data,
                                        < double * > dS_ref.data,
                                        < double * > u_trial_trace_ref.data,
                                        < double * > u_grad_trial_trace_ref.data,
                                        < double * > u_test_trace_ref.data,
                                        < double * > u_grad_test_trace_ref.data,
                                        < double * > normal_ref.data,
                                        < double * > boundaryJac_ref.data,
                                        nElements_global,
                                        # diffusion
                                        nu_0,
                                        nu_1,
                                        sigma_k,
                                        c_mu,
                                        rho_0,
                                        rho_1,
  #                             Argumentlist for sediment
                                        sedFlag,
                                       < double * >  q_vos.data,
                                       < double * > q_vos_gradc.data,
                                       < double * >  ebqe_q_vos.data,
                                       < double * > ebqe_q_vos_gradc.data,
                                       rho_f,
                                       rho_s,
                                       < double * >  vs.data,
                                       < double * >  ebqe_vs.data,
                                       < double * > g.data,
  #                             end for sediment
                                        dissipation_model_flag,
                                        # end diffuion
                                        useMetrics,
                                        alphaBDF,
                                        lag_shockCapturing,
                                        shockCapturingDiffusion,
                                        sc_uref, sc_alpha,
                                        < int * > u_l2g.data,
                                        < double * > elementDiameter.data,
                                        < double * > u_dof.data,
                                        < double * > u_dof_old.data,
                                        < double * > velocity.data,
                                        < double * > phi_ls.data,
                                        < double * > q_dissipation.data,  # dissipation rate
                                        < double * > q_grad_dissipation.data,  # VRANS
                                        < double * > q_porosity.data,  # VRANS
                                        # velocity dof
                                        < double * > velocity_dof_u.data,
                                        < double * > velocity_dof_v.data,
                                        < double * > velocity_dof_w.data,
                                        # end velocity dof
                                        < double * > q_m.data,
                                        < double * > q_u.data,
                                        < double * > q_grad_u.data,
                                        < double * > q_m_betaBDF.data,
                                        < double * > cfl.data,
                                        < double * > q_numDiff_u.data,
                                        < double * > q_numDiff_u_last.data,
                                        < double * > ebqe_penalty_ext.data,
                                        offset_u, stride_u,
                                        < double * > globalResidual.data,
                                        nExteriorElementBoundaries_global,
                                        < int * > exteriorElementBoundariesArray.data,
                                        < int * > elementBoundaryElementsArray.data,
                                        < int * > elementBoundaryLocalElementBoundariesArray.data,
                                        < double * > ebqe_velocity_ext.data,
                                        < int * > isDOFBoundary_u.data,
                                        < double * > ebqe_bc_u_ext.data,
                                        < int * > isAdvectiveFluxBoundary_u.data,
                                        < double * > ebqe_bc_advectiveFlux_u_ext.data,
                                        < int * > isDiffusiveFluxBoundary_u.data,
                                        < double * > ebqe_bc_diffusiveFlux_u_ext.data,
                                        < double * > ebqe_phi.data,
                                        epsFact,
                                        < double * > ebqe_dissipation.data,  # dissipation rate on boundary
                                        < double * > ebqe_porosity.data,  # VRANS
                                        < double * > ebqe_u.data,
                                        < double * > ebqe_flux.data)

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
                          # diffusion
                          double nu_0,
                          double nu_1,
                          double sigma_k,
                          double c_mu,
                          double rho_0,
                          double rho_1,
                          int dissipation_model_flag,
                          # end diffusion
                          double useMetrics,
                          double alphaBDF,
                          int lag_shockCapturing,
                          double shockCapturingDiffusion,
                          numpy.ndarray u_l2g,
                          numpy.ndarray elementDiameter,
                          numpy.ndarray u_dof, numpy.ndarray u_dof_old,
                          numpy.ndarray velocity,
                          numpy.ndarray phi_ls,  # level set variable
                          numpy.ndarray q_dissipation,  # dissipation rate
                          numpy.ndarray q_grad_dissipation,  # VRANS
                          numpy.ndarray q_porosity,  # VRANS
  #                             Argumentlist for sediment
                           double sedFlag,
                           numpy.ndarray  q_vos,
                           numpy.ndarray q_vos_gradc,
                           numpy.ndarray  ebqe_q_vos,
                           numpy.ndarray ebqe_q_vos_gradc,
                           double rho_f,
                           double rho_s,
                           numpy.ndarray  vs,
                           numpy.ndarray  ebqe_vs,
                           numpy.ndarray g,
  #                             end for sediment
                          # velocity dof
                          numpy.ndarray velocity_dof_u,
                          numpy.ndarray velocity_dof_v,
                          numpy.ndarray velocity_dof_w,
                          # end velocity dof
                          numpy.ndarray q_m_betaBDF,
                          numpy.ndarray cfl,
                          numpy.ndarray q_numDiff_u_last,
                          numpy.ndarray ebqe_penalty_ext,
                          numpy.ndarray csrRowIndeces_u_u, numpy.ndarray csrColumnOffsets_u_u,
                          globalJacobian,
                          int nExteriorElementBoundaries_global,
                          numpy.ndarray exteriorElementBoundariesArray,
                          numpy.ndarray elementBoundaryElementsArray,
                          numpy.ndarray elementBoundaryLocalElementBoundariesArray,
                          numpy.ndarray ebqe_velocity_ext,
                          numpy.ndarray isDOFBoundary_u,
                          numpy.ndarray ebqe_bc_u_ext,
                          numpy.ndarray isAdvectiveFluxBoundary_u,
                          numpy.ndarray ebqe_bc_advectiveFlux_u_ext,
                          numpy.ndarray isDiffusiveFluxBoundary_u,
                          numpy.ndarray ebqe_bc_diffusiveFlux_u_ext,
                          numpy.ndarray csrColumnOffsets_eb_u_u,
                          numpy.ndarray ebqe_phi,
                          double epsFact,
                          numpy.ndarray ebqe_dissipation,  # dissipation rate
                          numpy.ndarray ebqe_porosity):  # VRANS

        cdef numpy.ndarray rowptr, colind, globalJacobian_a
        (rowptr, colind, globalJacobian_a) = globalJacobian.getCSRrepresentation()
        self.thisptr.calculateJacobian(< double*> mesh_trial_ref.data,
                                        < double * > mesh_grad_trial_ref.data,
                                        < double * > mesh_dof.data,
                                        < double * > mesh_velocity_dof.data,
                                        MOVING_DOMAIN,
                                        < int * > mesh_l2g.data,
                                        < double * > dV_ref.data,
                                        < double * > u_trial_ref.data,
                                        < double * > u_grad_trial_ref.data,
                                        < double * > u_test_ref.data,
                                        < double * > u_grad_test_ref.data,
                                        < double * > mesh_trial_trace_ref.data,
                                        < double * > mesh_grad_trial_trace_ref.data,
                                        < double * > dS_ref.data,
                                        < double * > u_trial_trace_ref.data,
                                        < double * > u_grad_trial_trace_ref.data,
                                        < double * > u_test_trace_ref.data,
                                        < double * > u_grad_test_trace_ref.data,
                                        < double * > normal_ref.data,
                                        < double * > boundaryJac_ref.data,
                                        nElements_global,
                                        # diffusion
                                        nu_0,
                                        nu_1,
                                        sigma_k,
                                        c_mu,
                                        rho_0,
                                        rho_1,
                                        dissipation_model_flag,
                                        # end diffusion
                                        useMetrics,
                                        alphaBDF,
                                        lag_shockCapturing,
                                        shockCapturingDiffusion,
                                        < int * > u_l2g.data,
                                        < double * > elementDiameter.data,
                                        < double * > u_dof.data, < double * > u_dof_old.data,
                                        < double * > velocity.data,
                                        < double * > phi_ls.data,
                                        < double * > q_dissipation.data,
                                        < double * > q_grad_dissipation.data,
                                        < double * > q_porosity.data,  # VRANS
  #                             Argumentlist for sediment
                                        sedFlag,
                                       < double * >  q_vos.data,
                                       < double * > q_vos_gradc.data,
                                       < double * >  ebqe_q_vos.data,
                                       < double * > ebqe_q_vos_gradc.data,
                                       rho_f,
                                       rho_s,
                                       < double * >  vs.data,
                                       < double * >  ebqe_vs.data,
                                       < double * > g.data,
  #                             end for sediment
                                        # velocity dofs
                                        < double * > velocity_dof_u.data,
                                        < double * > velocity_dof_v.data,
                                        < double * > velocity_dof_w.data,
                                        # end velocity dofs
                                        < double * > q_m_betaBDF.data,
                                        < double * > cfl.data,
                                        < double * > q_numDiff_u_last.data,
                                        < double * > ebqe_penalty_ext.data,
                                        < int * > csrRowIndeces_u_u.data, < int * > csrColumnOffsets_u_u.data,
                                        < double * > globalJacobian_a.data,
                                        nExteriorElementBoundaries_global,
                                        < int * > exteriorElementBoundariesArray.data,
                                        < int * > elementBoundaryElementsArray.data,
                                        < int * > elementBoundaryLocalElementBoundariesArray.data,
                                        < double * > ebqe_velocity_ext.data,
                                        < int * > isDOFBoundary_u.data,
                                        < double * > ebqe_bc_u_ext.data,
                                        < int * > isAdvectiveFluxBoundary_u.data,
                                        < double * > ebqe_bc_advectiveFlux_u_ext.data,
                                        < int * > isDiffusiveFluxBoundary_u.data,
                                        < double * > ebqe_bc_diffusiveFlux_u_ext.data,
                                        < int * > csrColumnOffsets_eb_u_u.data,
                                        < double * > ebqe_phi.data, epsFact,
                                        < double * > ebqe_dissipation.data,
                                        < double * > ebqe_porosity.data)  # VRANS

#include "RANS2PQ.h"
#include <iostream>

extern "C" void calculateResidual_RANS2PQ(int nElements_global,
					  double alphaBDF,
					  double epsFact_rho,
					  double epsFact_mu, 
					  double sigma,
					  double rho_0,
					  double nu_0,
					  double rho_1,
					  double nu_1,
					  double hFactor,
					  double shockCapturingDiffusion,
					  int* p_l2g, int* vel_l2g, 
					  double* elementDiameter,
					  double* p_dof, double* u_dof, double* v_dof, double* w_dof,
					  double* p_trial, double* vel_trial,
					  double* p_grad_trial, double* vel_grad_trial,
					  double* p_test_dV, double* vel_test_dV,
					  double* p_grad_test_dV, double* vel_grad_test_dV,
					  double* vel_Hess_trial,double* vel_Hess_test_dV,
					  double* g,
					  double* phi,
					  double* n,
					  double* kappa,
					  double* q_mom_u_acc,
					  double* q_mom_v_acc,
					  double* q_mom_w_acc,
					  double* q_mass_adv,
					  double* q_mom_u_acc_beta_bdf, double* q_mom_v_acc_beta_bdf, double* q_mom_w_acc_beta_bdf,
					  double* q_velocity_last,
					  double* q_cfl,
					  double* q_numDiff_u, double* q_numDiff_v, double* q_numDiff_w,
					  double* q_numDiff_u_last, double* q_numDiff_v_last, double* q_numDiff_w_last,
					  double* q_elementResidual_p, double* q_elementResidual_u, double* q_elementResidual_v, double* q_elementResidual_w,
					  int* sdInfo_u_u_rowptr,int* sdInfo_u_u_colind,			      
					  int* sdInfo_u_v_rowptr,int* sdInfo_u_v_colind,
					  int* sdInfo_u_w_rowptr,int* sdInfo_u_w_colind,
					  int* sdInfo_v_v_rowptr,int* sdInfo_v_v_colind,
					  int* sdInfo_v_u_rowptr,int* sdInfo_v_u_colind,
					  int* sdInfo_v_w_rowptr,int* sdInfo_v_w_colind,
					  int* sdInfo_w_w_rowptr,int* sdInfo_w_w_colind,
					  int* sdInfo_w_u_rowptr,int* sdInfo_w_u_colind,
					  int* sdInfo_w_v_rowptr,int* sdInfo_w_v_colind,
					  int offset_p, int offset_u, int offset_v, int offset_w, int stride_p, 
					  int stride_u, int stride_v, int stride_w, double* globalResidual,
					  int nExteriorElementBoundaries_global,
					  int* exteriorElementBoundariesArray,
					  int* elementBoundaryElementsArray,
					  int* elementBoundaryLocalElementBoundariesArray,
					  double* p_trial_ext,
					  double* vel_trial_ext,
					  double* p_grad_trial_ext,
					  double* vel_grad_trial_ext,
					  double* ebqe_phi_ext,
					  double* ebqe_n_ext,
					  double* ebqe_kappa_ext,
					  int* isDOFBoundary_p,
					  int* isDOFBoundary_u,
					  int* isDOFBoundary_v,
					  int* isDOFBoundary_w,
					  int* isAdvectiveFluxBoundary_p,
					  int* isAdvectiveFluxBoundary_u,
					  int* isAdvectiveFluxBoundary_v,
					  int* isAdvectiveFluxBoundary_w,
					  int* isDiffusiveFluxBoundary_u,
					  int* isDiffusiveFluxBoundary_v,
					  int* isDiffusiveFluxBoundary_w,
					  double* ebqe_bc_p_ext,
					  double* ebqe_bc_flux_mass_ext,
					  double* ebqe_bc_flux_mom_u_adv_ext,
					  double* ebqe_bc_flux_mom_v_adv_ext,
					  double* ebqe_bc_flux_mom_w_adv_ext,
					  double* ebqe_bc_u_ext,
					  double* ebqe_bc_flux_u_diff_ext,
					  double* ebqe_penalty_ext,
					  double* ebqe_bc_v_ext,
					  double* ebqe_bc_flux_v_diff_ext,
					  double* ebqe_bc_w_ext,
					  double* ebqe_bc_flux_w_diff_ext,
					  double* p_test_dS_ext,
					  double* vel_test_dS_ext,
					  double* q_velocity,
					  double* ebqe_velocity,
					  double* flux)
{
  using namespace RANS2PQ;
  //
  //loop over elements to compute volume integrals and load them into element and global residual
  //
  //eN is the element index
  //eN_k is the quadrature point index for a scalar
  //eN_k_nSpace is the quadrature point index for a vector
  //eN_i is the element test function index
  //eN_j is the element trial function index
  //eN_k_j is the quadrature point index for a trial function
  //eN_k_i is the quadrature point index for a trial function
  double globalConservationError=0.0;
  for(int eN=0;eN<nElements_global;eN++)
    {
      //declare local storage for element residual and initialize
      register double elementResidual_p[nDOF_test_element],
	elementResidual_u[nDOF_test_element],
	elementResidual_v[nDOF_test_element],
	elementResidual_w[nDOF_test_element];
      const double eps_rho = epsFact_rho*elementDiameter[eN],
      	eps_mu = epsFact_mu*elementDiameter[eN];
      for (int i=0;i<nDOF_test_element;i++)
	{
	  elementResidual_p[i]=0.0;
	  elementResidual_u[i]=0.0;
	  elementResidual_v[i]=0.0;
	  elementResidual_w[i]=0.0;
	}//i
      //loop over quadrature points and compute integrands
      for  (int k=0;k<nQuadraturePoints_element;k++)
        {
	  //compute indeces and declare local storage
	  register int eN_k = eN*nQuadraturePoints_element+k,
	    eN_k_nSpace = eN_k*nSpace;
	  register double p=0.0,u=0.0,v=0.0,w=0.0,grad_p[nSpace],grad_u[nSpace],grad_v[nSpace],grad_w[nSpace],
	    Hess_u[nSpace2], Hess_v[nSpace2],Hess_w[nSpace2],
	    mom_u_acc=0.0,
	    dmom_u_acc_u=0.0,
	    mom_v_acc=0.0,
	    dmom_v_acc_v=0.0,
	    mom_w_acc=0.0,
	    dmom_w_acc_w=0.0,
	    mass_adv[nSpace],
	    dmass_adv_u[nSpace],
	    dmass_adv_v[nSpace],
	    dmass_adv_w[nSpace],
	    mom_u_adv[nSpace],
	    dmom_u_adv_u[nSpace],
	    dmom_u_adv_v[nSpace],
	    dmom_u_adv_w[nSpace],
	    mom_v_adv[nSpace],
	    dmom_v_adv_u[nSpace],
	    dmom_v_adv_v[nSpace],
	    dmom_v_adv_w[nSpace],
	    mom_w_adv[nSpace],
	    dmom_w_adv_u[nSpace],
	    dmom_w_adv_v[nSpace],
	    dmom_w_adv_w[nSpace],
	    mom_u_diff_ten[nSpace],
	    mom_v_diff_ten[nSpace],
	    mom_w_diff_ten[nSpace],
	    mom_uv_diff_ten[1],
	    mom_uw_diff_ten[1],
	    mom_vu_diff_ten[1],
	    mom_vw_diff_ten[1],
	    mom_wu_diff_ten[1],
	    mom_wv_diff_ten[1],
	    mom_u_source=0.0,
	    mom_v_source=0.0,
	    mom_w_source=0.0,
	    mom_u_ham=0.0,
	    dmom_u_ham_grad_p[nSpace],
	    mom_v_ham=0.0,
	    dmom_v_ham_grad_p[nSpace],
	    mom_w_ham=0.0,
	    dmom_w_ham_grad_p[nSpace],
	    mom_u_acc_t=0.0,
	    dmom_u_acc_u_t=0.0,
	    mom_v_acc_t=0.0,
	    dmom_v_acc_v_t=0.0,
	    mom_w_acc_t=0.0,
	    dmom_w_acc_w_t=0.0,
	    pdeResidual_p=0.0,
	    pdeResidual_u=0.0,
	    pdeResidual_v=0.0,
	    pdeResidual_w=0.0,
	    Lstar_u_p[nDOF_test_element],
	    Lstar_v_p[nDOF_test_element],
	    Lstar_w_p[nDOF_test_element],
	    Lstar_u_u[nDOF_test_element],
	    Lstar_v_v[nDOF_test_element],
	    Lstar_w_w[nDOF_test_element],
	    Lstar_p_u[nDOF_test_element],
	    Lstar_p_v[nDOF_test_element],
	    Lstar_p_w[nDOF_test_element],
	    subgridError_p=0.0,
	    subgridError_u=0.0,
	    subgridError_v=0.0,
	    subgridError_w=0.0,
	    tau_0=0.0,
	    tau_1=0.0;
          //
          //compute solution and gradients at quadrature points
          //
	  p=0.0;u=0.0;v=0.0;w=0.0;
	  for (int I=0;I<nSpace;I++)
	    {
	      grad_p[I]=0.0;grad_u[I]=0.0;grad_v[I]=0.0;grad_w[I]=0.0;
	      for (int J=0;J<nSpace;J++)
		{
		  Hess_u[I*nSpace+J] =0.0;
		  Hess_v[I*nSpace+J] =0.0;
		  Hess_w[I*nSpace+J] =0.0;
		}
	    }
          for (int j=0;j<nDOF_trial_element;j++)
            {
	      int eN_j=eN*nDOF_trial_element+j;
	      int eN_k_j=eN_k*nDOF_trial_element+j;
	      int eN_k_j_nSpace = eN_k_j*nSpace;
	      int eN_k_j_nSpace2 = eN_k_j*nSpace2;
              p += valFromDOF_c(p_dof[p_l2g[eN_j]],p_trial[eN_k_j]);
              u += valFromDOF_c(u_dof[vel_l2g[eN_j]],vel_trial[eN_k_j]);
              v += valFromDOF_c(v_dof[vel_l2g[eN_j]],vel_trial[eN_k_j]);
              w += valFromDOF_c(w_dof[vel_l2g[eN_j]],vel_trial[eN_k_j]);
	      
	      for (int I=0;I<nSpace;I++)
		{
		  grad_p[I] += gradFromDOF_c(p_dof[p_l2g[eN_j]],p_grad_trial[eN_k_j_nSpace+I]);
		  grad_u[I] += gradFromDOF_c(u_dof[vel_l2g[eN_j]],vel_grad_trial[eN_k_j_nSpace+I]);
		  grad_v[I] += gradFromDOF_c(v_dof[vel_l2g[eN_j]],vel_grad_trial[eN_k_j_nSpace+I]);
		  grad_w[I] += gradFromDOF_c(w_dof[vel_l2g[eN_j]],vel_grad_trial[eN_k_j_nSpace+I]);
		  for (int J=0;J<nSpace;J++)
		    {
		      Hess_u[I*nSpace+J] += HessFromDOF_c(u_dof[vel_l2g[eN_j]],vel_Hess_trial[eN_k_j_nSpace2+I*nSpace+J]);
		      Hess_v[I*nSpace+J] += HessFromDOF_c(v_dof[vel_l2g[eN_j]],vel_Hess_trial[eN_k_j_nSpace2+I*nSpace+J]);
		      Hess_w[I*nSpace+J] += HessFromDOF_c(w_dof[vel_l2g[eN_j]],vel_Hess_trial[eN_k_j_nSpace2+I*nSpace+J]);
		    }
		}
	    }
          //
          //calculate pde coefficients at quadrature points
          //
          evaluateCoefficients_c(eps_rho,
				 eps_mu,
				 sigma,
				 rho_0,
				 nu_0,
				 rho_1,
				 nu_1,
				 g,
				 phi[eN_k],
				 &n[eN_k_nSpace],
				 kappa[eN_k],
				 p,
				 grad_p,
				 u,
				 v,
				 w,
				 mom_u_acc,
				 dmom_u_acc_u,
				 mom_v_acc,
				 dmom_v_acc_v,
				 mom_w_acc,
				 dmom_w_acc_w,
				 mass_adv,
				 dmass_adv_u,
				 dmass_adv_v,
				 dmass_adv_w,
				 mom_u_adv,
				 dmom_u_adv_u,
				 dmom_u_adv_v,
				 dmom_u_adv_w,
				 mom_v_adv,
				 dmom_v_adv_u,
				 dmom_v_adv_v,
				 dmom_v_adv_w,
				 mom_w_adv,
				 dmom_w_adv_u,
				 dmom_w_adv_v,
				 dmom_w_adv_w,
				 mom_u_diff_ten,
				 mom_v_diff_ten,
				 mom_w_diff_ten,
				 mom_uv_diff_ten,
				 mom_uw_diff_ten,
				 mom_vu_diff_ten,
				 mom_vw_diff_ten,
				 mom_wu_diff_ten,
				 mom_wv_diff_ten,
				 mom_u_source,
				 mom_v_source,
				 mom_w_source,
				 mom_u_ham,
				 dmom_u_ham_grad_p,
				 mom_v_ham,
				 dmom_v_ham_grad_p,
				 mom_w_ham,
				 dmom_w_ham_grad_p);          
          //
          //moving mesh
          //
          //omit for now
          //
          //calculate time derivative at quadrature points
          //
          bdf_c(alphaBDF,
		q_mom_u_acc_beta_bdf[eN_k],
		mom_u_acc,
		dmom_u_acc_u,
		mom_u_acc_t,
		dmom_u_acc_u_t);
          bdf_c(alphaBDF,
		q_mom_v_acc_beta_bdf[eN_k],
		mom_v_acc,
		dmom_v_acc_v,
		mom_v_acc_t,
		dmom_v_acc_v_t);
          bdf_c(alphaBDF,
		q_mom_w_acc_beta_bdf[eN_k],
		mom_w_acc,
		dmom_w_acc_w,
		mom_w_acc_t,
		dmom_w_acc_w_t);
          //
          //calculate subgrid error (strong residual and adjoint)
          //
          //calculate strong residual
	  pdeResidual_p = Advection_strong_c(dmass_adv_u,grad_u) +
	    Advection_strong_c(dmass_adv_v,grad_v) +
	    Advection_strong_c(dmass_adv_w,grad_w);

	  pdeResidual_u = Mass_strong_c(mom_u_acc_t) +
	    Advection_strong_c(&q_velocity_last[eN_k_nSpace],grad_u) +
	    Hamiltonian_strong_c(dmom_u_ham_grad_p,grad_p) +
	    Reaction_strong_c(mom_u_source)+
	    Diffusion_strong_c(mom_u_diff_ten[1],Hess_u);//treat as diagona, grab nu from diffusion tensor
	  
	  pdeResidual_v = Mass_strong_c(mom_v_acc_t) +
	    Advection_strong_c(&q_velocity_last[eN_k_nSpace],grad_v) +
	    Hamiltonian_strong_c(dmom_v_ham_grad_p,grad_p) + 
	    Reaction_strong_c(mom_v_source)+
	    Diffusion_strong_c(mom_v_diff_ten[2],Hess_v);
	  
	  pdeResidual_w= Mass_strong_c(mom_w_acc_t) + 
	    Advection_strong_c(&q_velocity_last[eN_k_nSpace],grad_w) +
	    Hamiltonian_strong_c(dmom_w_ham_grad_p,grad_p) +
	    Reaction_strong_c(mom_w_source)+
	    Diffusion_strong_c(mom_w_diff_ten[0],Hess_w);

          //calculate adjoint
          for (int i=0;i<nDOF_test_element;i++)
            {
	      register int eN_k_i_nSpace = (eN_k*nDOF_trial_element+i)*nSpace;
	      register int eN_k_i_nSpace2 = (eN_k*nDOF_trial_element+i)*nSpace2;

	      Lstar_u_p[i]=Advection_adjoint_c(dmass_adv_u,&p_grad_test_dV[eN_k_i_nSpace]);
	      Lstar_v_p[i]=Advection_adjoint_c(dmass_adv_v,&p_grad_test_dV[eN_k_i_nSpace]);
	      Lstar_w_p[i]=Advection_adjoint_c(dmass_adv_w,&p_grad_test_dV[eN_k_i_nSpace]);
	      Lstar_u_u[i]=Advection_adjoint_c(&q_velocity_last[eN_k_nSpace],&vel_grad_test_dV[eN_k_i_nSpace])+
		Diffusion_adjoint_c(mom_u_diff_ten[1],&vel_Hess_test_dV[eN_k_i_nSpace2]);
	      Lstar_v_v[i]=Advection_adjoint_c(&q_velocity_last[eN_k_nSpace],&vel_grad_test_dV[eN_k_i_nSpace])+
		Diffusion_adjoint_c(mom_v_diff_ten[2],&vel_Hess_test_dV[eN_k_i_nSpace2]);
	      Lstar_w_w[i]=Advection_adjoint_c(&q_velocity_last[eN_k_nSpace],&vel_grad_test_dV[eN_k_i_nSpace]) +
		Diffusion_adjoint_c(mom_w_diff_ten[0],&vel_Hess_test_dV[eN_k_i_nSpace2]);
	      Lstar_p_u[i]=Hamiltonian_adjoint_c(dmom_u_ham_grad_p,&vel_grad_test_dV[eN_k_i_nSpace]);
	      Lstar_p_v[i]=Hamiltonian_adjoint_c(dmom_v_ham_grad_p,&vel_grad_test_dV[eN_k_i_nSpace]);
	      Lstar_p_w[i]=Hamiltonian_adjoint_c(dmom_w_ham_grad_p,&vel_grad_test_dV[eN_k_i_nSpace]);
            }
          //calculate tau and tau*Res
          calculateSubgridError_tau_c(hFactor,elementDiameter[eN],
				      dmom_u_acc_u_t,dmom_u_acc_u,
				      &q_velocity_last[eN_k_nSpace],mom_u_diff_ten[1],
				      tau_0,tau_1,q_cfl[eN_k]);
          calculateSubgridError_tauRes_c(tau_0,
					 tau_1,
					 pdeResidual_p,
					 pdeResidual_u,
					 pdeResidual_v,
					 pdeResidual_w,
					 subgridError_p,
					 subgridError_u,
					 subgridError_v,
					 subgridError_w);
          //
          //calcualte shock capturing diffusion
          //
          calculateNumericalDiffusion_c(shockCapturingDiffusion,elementDiameter[eN],pdeResidual_u,grad_u,q_numDiff_u[eN_k]);
          calculateNumericalDiffusion_c(shockCapturingDiffusion,elementDiameter[eN],pdeResidual_v,grad_v,q_numDiff_v[eN_k]);
          calculateNumericalDiffusion_c(shockCapturingDiffusion,elementDiameter[eN],pdeResidual_w,grad_w,q_numDiff_w[eN_k]);
          // 
          //update element residual 
          // 
          for(int i=0;i<nDOF_test_element;i++) 
	    { 
	      register int eN_k_i=eN_k*nDOF_test_element+i,
		eN_k_i_nSpace = eN_k_i*nSpace;

	      elementResidual_p[i] += Advection_weak_c(mass_adv,&p_grad_test_dV[eN_k_i_nSpace]) +
		SubgridError_c(subgridError_u,Lstar_u_p[i]) + 
		SubgridError_c(subgridError_v,Lstar_v_p[i]) + 
		SubgridError_c(subgridError_w,Lstar_w_p[i]);

	      elementResidual_u[i] += Mass_weak_c(mom_u_acc_t,vel_test_dV[eN_k_i]) + 
		Advection_weak_c(mom_u_adv,&vel_grad_test_dV[eN_k_i_nSpace]) +
		Diffusion_weak_c(sdInfo_u_u_rowptr,sdInfo_u_u_colind,mom_u_diff_ten,grad_u,&vel_grad_test_dV[eN_k_i_nSpace]) + 
		Diffusion_weak_c(sdInfo_u_v_rowptr,sdInfo_u_v_colind,mom_uv_diff_ten,grad_v,&vel_grad_test_dV[eN_k_i_nSpace]) + 
		Diffusion_weak_c(sdInfo_u_w_rowptr,sdInfo_u_w_colind,mom_uw_diff_ten,grad_w,&vel_grad_test_dV[eN_k_i_nSpace]) + 
		Reaction_weak_c(mom_u_source,vel_test_dV[eN_k_i]) + 
		Hamiltonian_weak_c(mom_u_ham,vel_test_dV[eN_k_i]) + 
		SubgridError_c(subgridError_p,Lstar_p_u[i]) + 
		SubgridError_c(subgridError_u,Lstar_u_u[i]) + 
		NumericalDiffusion_c(q_numDiff_u_last[eN_k],grad_u,&vel_grad_test_dV[eN_k_i_nSpace]); 
		 
	      elementResidual_v[i] += Mass_weak_c(mom_v_acc_t,vel_test_dV[eN_k_i]) + 
		Advection_weak_c(mom_v_adv,&vel_grad_test_dV[eN_k_i_nSpace]) +
		Diffusion_weak_c(sdInfo_v_v_rowptr,sdInfo_v_v_colind,mom_v_diff_ten,grad_v,&vel_grad_test_dV[eN_k_i_nSpace]) + 
		Diffusion_weak_c(sdInfo_v_u_rowptr,sdInfo_v_u_colind,mom_vu_diff_ten,grad_u,&vel_grad_test_dV[eN_k_i_nSpace]) + 
		Diffusion_weak_c(sdInfo_v_w_rowptr,sdInfo_v_w_colind,mom_vw_diff_ten,grad_w,&vel_grad_test_dV[eN_k_i_nSpace]) + 
		Reaction_weak_c(mom_v_source,vel_test_dV[eN_k_i]) + 
		Hamiltonian_weak_c(mom_v_ham,vel_test_dV[eN_k_i]) + 
		SubgridError_c(subgridError_p,Lstar_p_v[i]) + 
		SubgridError_c(subgridError_v,Lstar_v_v[i]) + 
		NumericalDiffusion_c(q_numDiff_v_last[eN_k],grad_v,&vel_grad_test_dV[eN_k_i_nSpace]); 

	      elementResidual_w[i] +=  Mass_weak_c(mom_w_acc_t,vel_test_dV[eN_k_i]) +
		Advection_weak_c(mom_w_adv,&vel_grad_test_dV[eN_k_i_nSpace]) + 
		Diffusion_weak_c(sdInfo_w_w_rowptr,sdInfo_w_w_colind,mom_w_diff_ten,grad_w,&vel_grad_test_dV[eN_k_i_nSpace]) + 
		Diffusion_weak_c(sdInfo_w_u_rowptr,sdInfo_w_u_colind,mom_wu_diff_ten,grad_u,&vel_grad_test_dV[eN_k_i_nSpace]) + 
		Diffusion_weak_c(sdInfo_w_v_rowptr,sdInfo_w_v_colind,mom_wv_diff_ten,grad_v,&vel_grad_test_dV[eN_k_i_nSpace]) + 
		Reaction_weak_c(mom_w_source,vel_test_dV[eN_k_i]) + 
		Hamiltonian_weak_c(mom_w_ham,vel_test_dV[eN_k_i]) + 
		SubgridError_c(subgridError_p,Lstar_p_w[i]) + 
		SubgridError_c(subgridError_w,Lstar_w_w[i]) + 
		NumericalDiffusion_c(q_numDiff_w_last[eN_k],grad_w,&vel_grad_test_dV[eN_k_i_nSpace]); 
            }//i
	  //save velocity at quadrature points for other models to use
	  // q_velocity[eN_k_nSpace+0]=u+subgridError_u;
	  // q_velocity[eN_k_nSpace+1]=v+subgridError_v;
	  // q_velocity[eN_k_nSpace+2]=w+subgridError_w;
	  //q_velocity[eN_k_nSpace+0]=u;
	  //q_velocity[eN_k_nSpace+1]=v;
	  //q_velocity[eN_k_nSpace+2]=w;
	  //
	  //save momentum for time history and velocity for subgrid error
	  //
	  q_mom_u_acc[eN_k] = mom_u_acc;                            
	  q_mom_v_acc[eN_k] = mom_v_acc;                            
	  q_mom_w_acc[eN_k] = mom_w_acc;
	  //subgrid error uses grid scale velocity
	  q_mass_adv[eN_k_nSpace+0] = u;
	  q_mass_adv[eN_k_nSpace+1] = v;
	  q_mass_adv[eN_k_nSpace+2] = w;
	}
      //
      //load element into global residual and save element residual
      //
      for(int i=0;i<nDOF_test_element;i++) 
        { 
          register int eN_i=eN*nDOF_test_element+i;
          
          q_elementResidual_p[eN_i]+=elementResidual_p[i];
          q_elementResidual_u[eN_i]+=elementResidual_u[i];
          q_elementResidual_v[eN_i]+=elementResidual_v[i];
          q_elementResidual_w[eN_i]+=elementResidual_w[i];

          globalResidual[offset_p+stride_p*p_l2g[eN_i]]+=elementResidual_p[i];
          globalResidual[offset_u+stride_u*vel_l2g[eN_i]]+=elementResidual_u[i];
          globalResidual[offset_v+stride_v*vel_l2g[eN_i]]+=elementResidual_v[i];
          globalResidual[offset_w+stride_w*vel_l2g[eN_i]]+=elementResidual_w[i];
        }//i
    }//elements
  //
  //loop over exterior element boundaries to calculate surface integrals and load into element and global residuals
  //
  //ebNE is the Exterior element boundary INdex
  //ebN is the element boundary INdex
  //eN is the element index
  for (int ebNE = 0; ebNE < nExteriorElementBoundaries_global; ebNE++) 
    { 
      register int ebN = exteriorElementBoundariesArray[ebNE], 
	eN  = elementBoundaryElementsArray[ebN*2+0];
      register double elementResidual_p[nDOF_test_element],
	elementResidual_u[nDOF_test_element],
	elementResidual_v[nDOF_test_element],
	elementResidual_w[nDOF_test_element];
      const double eps_rho = epsFact_rho*elementDiameter[eN],
      	eps_mu = epsFact_mu*elementDiameter[eN];
      for (int i=0;i<nDOF_test_element;i++)
	{
	  elementResidual_p[i]=0.0;
	  elementResidual_u[i]=0.0;
	  elementResidual_v[i]=0.0;
	  elementResidual_w[i]=0.0;
	}
      for  (int kb=0;kb<nQuadraturePoints_elementBoundary;kb++) 
	{ 
	  register int ebNE_kb = ebNE*nQuadraturePoints_elementBoundary+kb,
	    ebNE_kb_nSpace = ebNE_kb*nSpace;
	  register double p_ext=0.0,
	    u_ext=0.0,
	    v_ext=0.0,
	    w_ext=0.0,
	    grad_p_ext[nSpace],
	    grad_u_ext[nSpace],
	    grad_v_ext[nSpace],
	    grad_w_ext[nSpace],
	    mom_u_acc_ext=0.0,
	    dmom_u_acc_u_ext=0.0,
	    mom_v_acc_ext=0.0,
	    dmom_v_acc_v_ext=0.0,
	    mom_w_acc_ext=0.0,
	    dmom_w_acc_w_ext=0.0,
	    mass_adv_ext[nSpace],
	    dmass_adv_u_ext[nSpace],
	    dmass_adv_v_ext[nSpace],
	    dmass_adv_w_ext[nSpace],
	    mom_u_adv_ext[nSpace],
	    dmom_u_adv_u_ext[nSpace],
	    dmom_u_adv_v_ext[nSpace],
	    dmom_u_adv_w_ext[nSpace],
	    mom_v_adv_ext[nSpace],
	    dmom_v_adv_u_ext[nSpace],
	    dmom_v_adv_v_ext[nSpace],
	    dmom_v_adv_w_ext[nSpace],
	    mom_w_adv_ext[nSpace],
	    dmom_w_adv_u_ext[nSpace],
	    dmom_w_adv_v_ext[nSpace],
	    dmom_w_adv_w_ext[nSpace],
	    mom_u_diff_ten_ext[nSpace],
	    mom_v_diff_ten_ext[nSpace],
	    mom_w_diff_ten_ext[nSpace],
	    mom_uv_diff_ten_ext[1],
	    mom_uw_diff_ten_ext[1],
	    mom_vu_diff_ten_ext[1],
	    mom_vw_diff_ten_ext[1],
	    mom_wu_diff_ten_ext[1],
	    mom_wv_diff_ten_ext[1],
	    mom_u_source_ext=0.0,
	    mom_v_source_ext=0.0,
	    mom_w_source_ext=0.0,
	    mom_u_ham_ext=0.0,
	    dmom_u_ham_grad_p_ext[nSpace],
	    mom_v_ham_ext=0.0,
	    dmom_v_ham_grad_p_ext[nSpace],
	    mom_w_ham_ext=0.0,
	    dmom_w_ham_grad_p_ext[nSpace],
	    dmom_u_adv_p_ext[nSpace],
	    dmom_v_adv_p_ext[nSpace],
	    dmom_w_adv_p_ext[nSpace],
	    flux_mass_ext=0.0,
	    flux_mom_u_adv_ext=0.0,
	    flux_mom_v_adv_ext=0.0,
	    flux_mom_w_adv_ext=0.0,
	    flux_mom_u_diff_ext=0.0,
	    flux_mom_v_diff_ext=0.0,
	    flux_mom_w_diff_ext=0.0,
	    bc_p_ext=0.0,
	    bc_grad_p_ext[nSpace],
	    bc_grad_u_ext[nSpace],
	    bc_grad_v_ext[nSpace],
	    bc_grad_w_ext[nSpace],
	    bc_u_ext=0.0,
	    bc_v_ext=0.0,
	    bc_w_ext=0.0,
	    bc_mom_u_acc_ext=0.0,
	    bc_dmom_u_acc_u_ext=0.0,
	    bc_mom_v_acc_ext=0.0,
	    bc_dmom_v_acc_v_ext=0.0,
	    bc_mom_w_acc_ext=0.0,
	    bc_dmom_w_acc_w_ext=0.0,
	    bc_mass_adv_ext[nSpace],
	    bc_dmass_adv_u_ext[nSpace],
	    bc_dmass_adv_v_ext[nSpace],
	    bc_dmass_adv_w_ext[nSpace],
	    bc_mom_u_adv_ext[nSpace],
	    bc_dmom_u_adv_u_ext[nSpace],
	    bc_dmom_u_adv_v_ext[nSpace],
	    bc_dmom_u_adv_w_ext[nSpace],
	    bc_mom_v_adv_ext[nSpace],
	    bc_dmom_v_adv_u_ext[nSpace],
	    bc_dmom_v_adv_v_ext[nSpace],
	    bc_dmom_v_adv_w_ext[nSpace],
	    bc_mom_w_adv_ext[nSpace],
	    bc_dmom_w_adv_u_ext[nSpace],
	    bc_dmom_w_adv_v_ext[nSpace],
	    bc_dmom_w_adv_w_ext[nSpace],
	    bc_mom_u_diff_ten_ext[nSpace],
	    bc_mom_v_diff_ten_ext[nSpace],
	    bc_mom_w_diff_ten_ext[nSpace],
	    bc_mom_uv_diff_ten_ext[1],
	    bc_mom_uw_diff_ten_ext[1],
	    bc_mom_vu_diff_ten_ext[1],
	    bc_mom_vw_diff_ten_ext[1],
	    bc_mom_wu_diff_ten_ext[1],
	    bc_mom_wv_diff_ten_ext[1],
	    bc_mom_u_source_ext=0.0,
	    bc_mom_v_source_ext=0.0,
	    bc_mom_w_source_ext=0.0,
	    bc_mom_u_ham_ext=0.0,
	    bc_dmom_u_ham_grad_p_ext[nSpace],
	    bc_mom_v_ham_ext=0.0,
	    bc_dmom_v_ham_grad_p_ext[nSpace],
	    bc_mom_w_ham_ext=0.0,
	    bc_dmom_w_ham_grad_p_ext[nSpace];
	  // 
	  //calculate the solution and gradients at quadrature points 
	  // 
	  p_ext=0.0;u_ext=0.0;v_ext=0.0;w_ext=0.0;
	  for (int I=0;I<nSpace;I++)
	    {
	      grad_p_ext[I] = 0.0;
	      grad_u_ext[I] = 0.0;
	      grad_v_ext[I] = 0.0;
	      grad_w_ext[I] = 0.0;
	      bc_grad_p_ext[I] = 0.0;
	      bc_grad_u_ext[I] = 0.0;
	      bc_grad_v_ext[I] = 0.0;
	      bc_grad_w_ext[I] = 0.0;
	    }
	  for (int j=0;j<nDOF_trial_element;j++) 
	    { 
	      int eN_j = eN*nDOF_trial_element+j;
	      int ebNE_kb_j = ebNE_kb*nDOF_trial_element+j;
	      int ebNE_kb_j_nSpace= ebNE_kb_j*nSpace;
	      p_ext += valFromDOF_c(p_dof[p_l2g[eN_j]],p_trial_ext[ebNE_kb_j]); 
	      u_ext += valFromDOF_c(u_dof[vel_l2g[eN_j]],vel_trial_ext[ebNE_kb_j]); 
	      v_ext += valFromDOF_c(v_dof[vel_l2g[eN_j]],vel_trial_ext[ebNE_kb_j]); 
	      w_ext += valFromDOF_c(w_dof[vel_l2g[eN_j]],vel_trial_ext[ebNE_kb_j]); 
               
	      for (int I=0;I<nSpace;I++)
		{
		  grad_p_ext[I] += gradFromDOF_c(p_dof[p_l2g[eN_j]],p_grad_trial_ext[ebNE_kb_j_nSpace+I]); 
		  grad_u_ext[I] += gradFromDOF_c(u_dof[vel_l2g[eN_j]],vel_grad_trial_ext[ebNE_kb_j_nSpace+I]); 
		  grad_v_ext[I] += gradFromDOF_c(v_dof[vel_l2g[eN_j]],vel_grad_trial_ext[ebNE_kb_j_nSpace+I]); 
		  grad_w_ext[I] += gradFromDOF_c(w_dof[vel_l2g[eN_j]],vel_grad_trial_ext[ebNE_kb_j_nSpace+I]);
		} 
	    }
	  //
	  //load the boundary values
	  //
	  bc_p_ext = isDOFBoundary_p[ebNE_kb]*ebqe_bc_p_ext[ebNE_kb]+(1-isDOFBoundary_p[ebNE_kb])*p_ext;
	  bc_u_ext = isDOFBoundary_u[ebNE_kb]*ebqe_bc_u_ext[ebNE_kb]+(1-isDOFBoundary_u[ebNE_kb])*u_ext;
	  bc_v_ext = isDOFBoundary_v[ebNE_kb]*ebqe_bc_v_ext[ebNE_kb]+(1-isDOFBoundary_v[ebNE_kb])*v_ext;
	  bc_w_ext = isDOFBoundary_w[ebNE_kb]*ebqe_bc_w_ext[ebNE_kb]+(1-isDOFBoundary_w[ebNE_kb])*w_ext;
	  // 
	  //calculate the pde coefficients using the solution and the boundary values for the solution 
	  // 
	  evaluateCoefficients_c(eps_rho,
				 eps_mu,
				 sigma,
				 rho_0,
				 nu_0,
				 rho_1,
				 nu_1,
				 g,
				 ebqe_phi_ext[ebNE_kb],
				 &ebqe_n_ext[ebNE_kb_nSpace],
				 ebqe_kappa_ext[ebNE_kb],
				 p_ext,
				 grad_p_ext,
				 u_ext,
				 v_ext,
				 w_ext,
				 mom_u_acc_ext,
				 dmom_u_acc_u_ext,
				 mom_v_acc_ext,
				 dmom_v_acc_v_ext,
				 mom_w_acc_ext,
				 dmom_w_acc_w_ext,
				 mass_adv_ext,
				 dmass_adv_u_ext,
				 dmass_adv_v_ext,
				 dmass_adv_w_ext,
				 mom_u_adv_ext,
				 dmom_u_adv_u_ext,
				 dmom_u_adv_v_ext,
				 dmom_u_adv_w_ext,
				 mom_v_adv_ext,
				 dmom_v_adv_u_ext,
				 dmom_v_adv_v_ext,
				 dmom_v_adv_w_ext,
				 mom_w_adv_ext,
				 dmom_w_adv_u_ext,
				 dmom_w_adv_v_ext,
				 dmom_w_adv_w_ext,
				 mom_u_diff_ten_ext,
				 mom_v_diff_ten_ext,
				 mom_w_diff_ten_ext,
				 mom_uv_diff_ten_ext,
				 mom_uw_diff_ten_ext,
				 mom_vu_diff_ten_ext,
				 mom_vw_diff_ten_ext,
				 mom_wu_diff_ten_ext,
				 mom_wv_diff_ten_ext,
				 mom_u_source_ext,
				 mom_v_source_ext,
				 mom_w_source_ext,
				 mom_u_ham_ext,
				 dmom_u_ham_grad_p_ext,
				 mom_v_ham_ext,
				 dmom_v_ham_grad_p_ext,
				 mom_w_ham_ext,
				 dmom_w_ham_grad_p_ext);          
	  evaluateCoefficients_c(eps_rho,
				 eps_mu,
				 sigma,
				 rho_0,
				 nu_0,
				 rho_1,
				 nu_1,
				 g,
				 ebqe_phi_ext[ebNE_kb],
				 &ebqe_n_ext[ebNE_kb_nSpace],
				 ebqe_kappa_ext[ebNE_kb],
				 bc_p_ext,
				 bc_grad_p_ext,
				 bc_u_ext,
				 bc_v_ext,
				 bc_w_ext,
				 bc_mom_u_acc_ext,
				 bc_dmom_u_acc_u_ext,
				 bc_mom_v_acc_ext,
				 bc_dmom_v_acc_v_ext,
				 bc_mom_w_acc_ext,
				 bc_dmom_w_acc_w_ext,
				 bc_mass_adv_ext,
				 bc_dmass_adv_u_ext,
				 bc_dmass_adv_v_ext,
				 bc_dmass_adv_w_ext,
				 bc_mom_u_adv_ext,
				 bc_dmom_u_adv_u_ext,
				 bc_dmom_u_adv_v_ext,
				 bc_dmom_u_adv_w_ext,
				 bc_mom_v_adv_ext,
				 bc_dmom_v_adv_u_ext,
				 bc_dmom_v_adv_v_ext,
				 bc_dmom_v_adv_w_ext,
				 bc_mom_w_adv_ext,
				 bc_dmom_w_adv_u_ext,
				 bc_dmom_w_adv_v_ext,
				 bc_dmom_w_adv_w_ext,
				 bc_mom_u_diff_ten_ext,
				 bc_mom_v_diff_ten_ext,
				 bc_mom_w_diff_ten_ext,
				 bc_mom_uv_diff_ten_ext,
				 bc_mom_uw_diff_ten_ext,
				 bc_mom_vu_diff_ten_ext,
				 bc_mom_vw_diff_ten_ext,
				 bc_mom_wu_diff_ten_ext,
				 bc_mom_wv_diff_ten_ext,
				 bc_mom_u_source_ext,
				 bc_mom_v_source_ext,
				 bc_mom_w_source_ext,
				 bc_mom_u_ham_ext,
				 bc_dmom_u_ham_grad_p_ext,
				 bc_mom_v_ham_ext,
				 bc_dmom_v_ham_grad_p_ext,
				 bc_mom_w_ham_ext,
				 bc_dmom_w_ham_grad_p_ext);          
	  // 
	  //calculate the numerical fluxes 
	  // 
	  exteriorNumericalAdvectiveFlux_c(isDOFBoundary_p[ebNE_kb],
					   isDOFBoundary_u[ebNE_kb],
					   isDOFBoundary_v[ebNE_kb],
					   isDOFBoundary_w[ebNE_kb],
					   isAdvectiveFluxBoundary_p[ebNE_kb],
					   isAdvectiveFluxBoundary_u[ebNE_kb],
					   isAdvectiveFluxBoundary_v[ebNE_kb],
					   isAdvectiveFluxBoundary_w[ebNE_kb],
					   &ebqe_n_ext[ebNE_kb_nSpace],
					   bc_p_ext,
					   bc_mass_adv_ext,
					   bc_mom_u_adv_ext,
					   bc_mom_v_adv_ext,
					   bc_mom_w_adv_ext,
					   ebqe_bc_flux_mass_ext[ebNE_kb],
					   ebqe_bc_flux_mom_u_adv_ext[ebNE_kb],
					   ebqe_bc_flux_mom_v_adv_ext[ebNE_kb],
					   ebqe_bc_flux_mom_w_adv_ext[ebNE_kb],
					   p_ext,
					   mass_adv_ext,
					   mom_u_adv_ext,
					   mom_v_adv_ext,
					   mom_w_adv_ext,
					   dmass_adv_u_ext,
					   dmass_adv_v_ext,
					   dmass_adv_w_ext,
					   dmom_u_adv_p_ext,
					   dmom_u_adv_u_ext,
					   dmom_u_adv_v_ext,
					   dmom_u_adv_w_ext,
					   dmom_v_adv_p_ext,
					   dmom_v_adv_u_ext,
					   dmom_v_adv_v_ext,
					   dmom_v_adv_w_ext,
					   dmom_w_adv_p_ext,
					   dmom_w_adv_u_ext,
					   dmom_w_adv_v_ext,
					   dmom_w_adv_w_ext,
					   flux_mass_ext,
					   flux_mom_u_adv_ext,
					   flux_mom_v_adv_ext,
					   flux_mom_w_adv_ext,
					   &ebqe_velocity[ebNE_kb_nSpace]);
	  exteriorNumericalDiffusiveFlux_c(eps_rho,
					   ebqe_phi_ext[ebNE_kb],
					   sdInfo_u_u_rowptr,
					   sdInfo_u_u_colind,
					   isDOFBoundary_u[ebNE_kb],
					   isDiffusiveFluxBoundary_u[ebNE_kb],
					   &ebqe_n_ext[ebNE_kb_nSpace],
					   bc_mom_u_diff_ten_ext,
					   bc_u_ext,
					   ebqe_bc_flux_u_diff_ext[ebNE_kb],
					   mom_u_diff_ten_ext,
					   grad_u_ext,
					   u_ext,
					   ebqe_penalty_ext[ebNE_kb],
					   flux_mom_u_diff_ext);
	  exteriorNumericalDiffusiveFlux_c(eps_rho,
					   ebqe_phi_ext[ebNE_kb],
					   sdInfo_v_v_rowptr,
					   sdInfo_v_v_colind,
					   isDOFBoundary_v[ebNE_kb],
					   isDiffusiveFluxBoundary_v[ebNE_kb],
					   &ebqe_n_ext[ebNE_kb_nSpace],
					   bc_mom_v_diff_ten_ext,
					   bc_v_ext,
					   ebqe_bc_flux_v_diff_ext[ebNE_kb],
					   mom_v_diff_ten_ext,
					   grad_v_ext,
					   v_ext,
					   ebqe_penalty_ext[ebNE_kb],
					   flux_mom_v_diff_ext);
	  exteriorNumericalDiffusiveFlux_c(eps_rho,
					   ebqe_phi_ext[ebNE_kb],
					   sdInfo_w_w_rowptr,
					   sdInfo_w_w_colind,
					   isDOFBoundary_w[ebNE_kb],
					   isDiffusiveFluxBoundary_w[ebNE_kb],
					   &ebqe_n_ext[ebNE_kb_nSpace],
					   bc_mom_w_diff_ten_ext,
					   bc_w_ext,
					   ebqe_bc_flux_w_diff_ext[ebNE_kb],
					   mom_w_diff_ten_ext,
					   grad_w_ext,
					   w_ext,
					   ebqe_penalty_ext[ebNE_kb],
					   flux_mom_w_diff_ext);
	  flux[ebN*nQuadraturePoints_elementBoundary+kb] = flux_mass_ext;
	  //
	  //update residuals
	  //
	  for (int i=0;i<nDOF_test_element;i++)
	    {
	      int ebNE_kb_i = ebNE_kb*nDOF_test_element+i;

	      elementResidual_p[i] += ExteriorElementBoundaryFlux_c(flux_mass_ext,p_test_dS_ext[ebNE_kb_i]);
	      globalConservationError += ExteriorElementBoundaryFlux_c(flux_mass_ext,p_test_dS_ext[ebNE_kb_i]);
	      elementResidual_u[i] += ExteriorElementBoundaryFlux_c(flux_mom_u_adv_ext,vel_test_dS_ext[ebNE_kb_i])+
	      	ExteriorElementBoundaryFlux_c(flux_mom_u_diff_ext,vel_test_dS_ext[ebNE_kb_i]); 

	      elementResidual_v[i] += ExteriorElementBoundaryFlux_c(flux_mom_v_adv_ext,vel_test_dS_ext[ebNE_kb_i]) +
	      	ExteriorElementBoundaryFlux_c(flux_mom_v_diff_ext,vel_test_dS_ext[ebNE_kb_i]); 
	       
	      elementResidual_w[i] += ExteriorElementBoundaryFlux_c(flux_mom_w_adv_ext,vel_test_dS_ext[ebNE_kb_i]) +
	      	ExteriorElementBoundaryFlux_c(flux_mom_w_diff_ext,vel_test_dS_ext[ebNE_kb_i]); 
	    }//i
	}//kb
      //
      //update the element and global residual storage
      //
      for (int i=0;i<nDOF_test_element;i++)
	{
	  int eN_i = eN*nDOF_test_element+i;
	  q_elementResidual_p[eN_i]+=elementResidual_p[i];
	  q_elementResidual_u[eN_i]+=elementResidual_u[i];
	  q_elementResidual_v[eN_i]+=elementResidual_v[i];
	  q_elementResidual_w[eN_i]+=elementResidual_w[i];

	  globalResidual[offset_p+stride_p*p_l2g[eN_i]]+=elementResidual_p[i];
	  globalResidual[offset_u+stride_u*vel_l2g[eN_i]]+=elementResidual_u[i];
	  globalResidual[offset_v+stride_v*vel_l2g[eN_i]]+=elementResidual_v[i];
	  globalResidual[offset_w+stride_w*vel_l2g[eN_i]]+=elementResidual_w[i];
	}//i
    }//ebNE
  std::cout<<"Global Conservation========================================================================="<<globalConservationError<<std::endl;
}

extern "C" void calculateJacobian_RANS2PQ(int nElements_global,
					  double alphaBDF,
					  double epsFact_rho,
					  double epsFact_mu,
					  double sigma,
					  double rho_0,
					  double nu_0,
					  double rho_1,
					  double nu_1,
					  double hFactor,
					  double shockCapturingDiffusion,
					  int* p_l2g, int* vel_l2g,
					  double* elementDiameter,
					  double* p_dof, double* u_dof, double* v_dof, double* w_dof,
					  double* p_trial, double* vel_trial,
					  double* p_grad_trial, double* vel_grad_trial,
					  double* p_test_dV, double* vel_test_dV, 
					  double* p_grad_test_dV, double* vel_grad_test_dV,
					  double* vel_Hess_trial,double* vel_Hess_test_dV,
					  double* g,
					  double* phi,
					  double* n,
					  double* kappa,
					  double* q_mom_u_acc_beta_bdf, double* q_mom_v_acc_beta_bdf, double* q_mom_w_acc_beta_bdf,
					  double* q_velocity_last,
					  double* q_cfl,
					  double* q_numDiff_u_last, double* q_numDiff_v_last, double* q_numDiff_w_last,
					  int* sdInfo_u_u_rowptr,int* sdInfo_u_u_colind,			      
					  int* sdInfo_u_v_rowptr,int* sdInfo_u_v_colind,
					  int* sdInfo_u_w_rowptr,int* sdInfo_u_w_colind,
					  int* sdInfo_v_v_rowptr,int* sdInfo_v_v_colind,
					  int* sdInfo_v_u_rowptr,int* sdInfo_v_u_colind,
					  int* sdInfo_v_w_rowptr,int* sdInfo_v_w_colind,
					  int* sdInfo_w_w_rowptr,int* sdInfo_w_w_colind,
					  int* sdInfo_w_u_rowptr,int* sdInfo_w_u_colind,
					  int* sdInfo_w_v_rowptr,int* sdInfo_w_v_colind,
					  int* csrRowIndeces_p_p,int* csrColumnOffsets_p_p,
					  int* csrRowIndeces_p_u,int* csrColumnOffsets_p_u,
					  int* csrRowIndeces_p_v,int* csrColumnOffsets_p_v,
					  int* csrRowIndeces_p_w,int* csrColumnOffsets_p_w,
					  int* csrRowIndeces_u_p,int* csrColumnOffsets_u_p,
					  int* csrRowIndeces_u_u,int* csrColumnOffsets_u_u,
					  int* csrRowIndeces_u_v,int* csrColumnOffsets_u_v,
					  int* csrRowIndeces_u_w,int* csrColumnOffsets_u_w,
					  int* csrRowIndeces_v_p,int* csrColumnOffsets_v_p,
					  int* csrRowIndeces_v_u,int* csrColumnOffsets_v_u,
					  int* csrRowIndeces_v_v,int* csrColumnOffsets_v_v,
					  int* csrRowIndeces_v_w,int* csrColumnOffsets_v_w,
					  int* csrRowIndeces_w_p,int* csrColumnOffsets_w_p,
					  int* csrRowIndeces_w_u,int* csrColumnOffsets_w_u,
					  int* csrRowIndeces_w_v,int* csrColumnOffsets_w_v,
					  int* csrRowIndeces_w_w,int* csrColumnOffsets_w_w,
					  double* globalJacobian,
					  int nExteriorElementBoundaries_global,
					  int* exteriorElementBoundariesArray,
					  int* elementBoundaryElementsArray,
					  int* elementBoundaryLocalElementBoundariesArray,
					  double* p_trial_ext,
					  double* vel_trial_ext,
					  double* p_grad_trial_ext,
					  double* vel_grad_trial_ext,
					  double* ebqe_phi_ext,
					  double* ebqe_n_ext,
					  double* ebqe_kappa_ext,
					  int* isDOFBoundary_p,
					  int* isDOFBoundary_u,
					  int* isDOFBoundary_v,
					  int* isDOFBoundary_w,
					  int* isAdvectiveFluxBoundary_p,
					  int* isAdvectiveFluxBoundary_u,
					  int* isAdvectiveFluxBoundary_v,
					  int* isAdvectiveFluxBoundary_w,
					  int* isDiffusiveFluxBoundary_u,
					  int* isDiffusiveFluxBoundary_v,
					  int* isDiffusiveFluxBoundary_w,
					  double* ebqe_bc_p_ext,
					  double* ebqe_bc_flux_mass_ext,
					  double* ebqe_bc_flux_mom_u_adv_ext,
					  double* ebqe_bc_flux_mom_v_adv_ext,
					  double* ebqe_bc_flux_mom_w_adv_ext,
					  double* ebqe_bc_u_ext,
					  double* ebqe_bc_flux_u_diff_ext,
					  double* ebqe_penalty_ext,
					  double* ebqe_bc_v_ext,
					  double* ebqe_bc_flux_v_diff_ext,
					  double* ebqe_bc_w_ext,
					  double* ebqe_bc_flux_w_diff_ext,
					  double* p_test_dS_ext,
					  double* vel_test_dS_ext,
					  int* csrColumnOffsets_eb_p_p,
					  int* csrColumnOffsets_eb_p_u,
					  int* csrColumnOffsets_eb_p_v,
					  int* csrColumnOffsets_eb_p_w,
					  int* csrColumnOffsets_eb_u_p,
					  int* csrColumnOffsets_eb_u_u,
					  int* csrColumnOffsets_eb_u_v,
					  int* csrColumnOffsets_eb_u_w,
					  int* csrColumnOffsets_eb_v_p,
					  int* csrColumnOffsets_eb_v_u,
					  int* csrColumnOffsets_eb_v_v,
					  int* csrColumnOffsets_eb_v_w,
					  int* csrColumnOffsets_eb_w_p,
					  int* csrColumnOffsets_eb_w_u,
					  int* csrColumnOffsets_eb_w_v,
					  int* csrColumnOffsets_eb_w_w)
{
  using namespace RANS2PQ;
  //
  //loop over elements to compute volume integrals and load them into the element Jacobians and global Jacobian
  //
  for(int eN=0;eN<nElements_global;eN++)
    {
      const double eps_rho = epsFact_rho*elementDiameter[eN],
      	eps_mu = epsFact_mu*elementDiameter[eN];

      register double  elementJacobian_p_p[nDOF_test_element][nDOF_trial_element],
	elementJacobian_p_u[nDOF_test_element][nDOF_trial_element],
	elementJacobian_p_v[nDOF_test_element][nDOF_trial_element],
	elementJacobian_p_w[nDOF_test_element][nDOF_trial_element],
	elementJacobian_u_p[nDOF_test_element][nDOF_trial_element],
	elementJacobian_u_u[nDOF_test_element][nDOF_trial_element],
	elementJacobian_u_v[nDOF_test_element][nDOF_trial_element],
	elementJacobian_u_w[nDOF_test_element][nDOF_trial_element],
	elementJacobian_v_p[nDOF_test_element][nDOF_trial_element],
	elementJacobian_v_u[nDOF_test_element][nDOF_trial_element],
	elementJacobian_v_v[nDOF_test_element][nDOF_trial_element],
	elementJacobian_v_w[nDOF_test_element][nDOF_trial_element],
	elementJacobian_w_p[nDOF_test_element][nDOF_trial_element],
	elementJacobian_w_u[nDOF_test_element][nDOF_trial_element],
	elementJacobian_w_v[nDOF_test_element][nDOF_trial_element],
	elementJacobian_w_w[nDOF_test_element][nDOF_trial_element];
      for (int i=0;i<nDOF_test_element;i++)
	for (int j=0;j<nDOF_trial_element;j++)
	  {
	    elementJacobian_p_p[i][j]=0.0;
	    elementJacobian_p_u[i][j]=0.0;
	    elementJacobian_p_v[i][j]=0.0;
	    elementJacobian_p_w[i][j]=0.0;
	    elementJacobian_u_p[i][j]=0.0;
	    elementJacobian_u_u[i][j]=0.0;
	    elementJacobian_u_v[i][j]=0.0;
	    elementJacobian_u_w[i][j]=0.0;
	    elementJacobian_v_p[i][j]=0.0;
	    elementJacobian_v_u[i][j]=0.0;
	    elementJacobian_v_v[i][j]=0.0;
	    elementJacobian_v_w[i][j]=0.0;
	    elementJacobian_w_p[i][j]=0.0;
	    elementJacobian_w_u[i][j]=0.0;
	    elementJacobian_w_v[i][j]=0.0;
	    elementJacobian_w_w[i][j]=0.0;
	  }
      for  (int k=0;k<nQuadraturePoints_element;k++)
        {
	  int eN_k = eN*nQuadraturePoints_element+k, //index to a scalar at a quadrature point
	    eN_k_nSpace = eN_k*nSpace; //index to a vector at a quadrature point

	  //declare local storage
	  register double p=0.0,u=0.0,v=0.0,w=0.0,
	    grad_p[nSpace],grad_u[nSpace],grad_v[nSpace],grad_w[nSpace],
	    mom_u_acc=0.0,
	    dmom_u_acc_u=0.0,
	    mom_v_acc=0.0,
	    dmom_v_acc_v=0.0,
	    mom_w_acc=0.0,
	    dmom_w_acc_w=0.0,
	    mass_adv[nSpace],
	    dmass_adv_u[nSpace],
	    dmass_adv_v[nSpace],
	    dmass_adv_w[nSpace],
	    mom_u_adv[nSpace],
	    dmom_u_adv_u[nSpace],
	    dmom_u_adv_v[nSpace],
	    dmom_u_adv_w[nSpace],
	    mom_v_adv[nSpace],
	    dmom_v_adv_u[nSpace],
	    dmom_v_adv_v[nSpace],
	    dmom_v_adv_w[nSpace],
	    mom_w_adv[nSpace],
	    dmom_w_adv_u[nSpace],
	    dmom_w_adv_v[nSpace],
	    dmom_w_adv_w[nSpace],
	    mom_u_diff_ten[nSpace],
	    mom_v_diff_ten[nSpace],
	    mom_w_diff_ten[nSpace],
	    mom_uv_diff_ten[1],
	    mom_uw_diff_ten[1],
	    mom_vu_diff_ten[1],
	    mom_vw_diff_ten[1],
	    mom_wu_diff_ten[1],
	    mom_wv_diff_ten[1],
	    mom_u_source=0.0,
	    mom_v_source=0.0,
	    mom_w_source=0.0,
	    mom_u_ham=0.0,
	    dmom_u_ham_grad_p[nSpace],
	    mom_v_ham=0.0,
	    dmom_v_ham_grad_p[nSpace],
	    mom_w_ham=0.0,
	    dmom_w_ham_grad_p[nSpace],
	    mom_u_acc_t=0.0,
	    dmom_u_acc_u_t=0.0,
	    mom_v_acc_t=0.0,
	    dmom_v_acc_v_t=0.0,
	    mom_w_acc_t=0.0,
	    dmom_w_acc_w_t=0.0,
	    dpdeResidual_p_u[nDOF_trial_element],dpdeResidual_p_v[nDOF_trial_element],dpdeResidual_p_w[nDOF_trial_element],
	    dpdeResidual_u_p[nDOF_trial_element],dpdeResidual_u_u[nDOF_trial_element],
	    dpdeResidual_v_p[nDOF_trial_element],dpdeResidual_v_v[nDOF_trial_element],
	    dpdeResidual_w_p[nDOF_trial_element],dpdeResidual_w_w[nDOF_trial_element],
	    Lstar_u_p[nDOF_test_element],
	    Lstar_v_p[nDOF_test_element],
	    Lstar_w_p[nDOF_test_element],
	    Lstar_u_u[nDOF_test_element],
	    Lstar_v_v[nDOF_test_element],
	    Lstar_w_w[nDOF_test_element],
	    Lstar_p_u[nDOF_test_element],
	    Lstar_p_v[nDOF_test_element],
	    Lstar_p_w[nDOF_test_element],
	    dsubgridError_p_u[nDOF_trial_element],
	    dsubgridError_p_v[nDOF_trial_element],
	    dsubgridError_p_w[nDOF_trial_element],
	    dsubgridError_u_p[nDOF_trial_element],
	    dsubgridError_u_u[nDOF_trial_element],
	    dsubgridError_v_p[nDOF_trial_element],
	    dsubgridError_v_v[nDOF_trial_element],
	    dsubgridError_w_p[nDOF_trial_element],
	    dsubgridError_w_w[nDOF_trial_element],
	    tau_0=0.0,
	    tau_1=0.0;
          //
          //calculate solution and gradients at quadrature points
          //
	  p=0.0;u=0.0;v=0.0;w=0.0;
	  for (int I=0;I<nSpace;I++)
	    {
	      grad_p[I]=0.0;grad_u[I]=0.0;grad_v[I]=0.0;grad_w[I]=0.0;
	    }
          for (int j=0;j<nDOF_trial_element;j++)
            {
	      int eN_j=eN*nDOF_trial_element+j;
	      int eN_k_j=eN_k*nDOF_trial_element+j;
	      int eN_k_j_nSpace = eN_k_j*nSpace;
              p += valFromDOF_c(p_dof[p_l2g[eN_j]],p_trial[eN_k_j]);
              u += valFromDOF_c(u_dof[vel_l2g[eN_j]],vel_trial[eN_k_j]);
              v += valFromDOF_c(v_dof[vel_l2g[eN_j]],vel_trial[eN_k_j]);
              w += valFromDOF_c(w_dof[vel_l2g[eN_j]],vel_trial[eN_k_j]);

	      for (int I=0;I<nSpace;I++)
		{
		  grad_p[I] += gradFromDOF_c(p_dof[p_l2g[eN_j]],p_grad_trial[eN_k_j_nSpace+I]);
		  grad_u[I] += gradFromDOF_c(u_dof[vel_l2g[eN_j]],vel_grad_trial[eN_k_j_nSpace+I]);
		  grad_v[I] += gradFromDOF_c(v_dof[vel_l2g[eN_j]],vel_grad_trial[eN_k_j_nSpace+I]);
		  grad_w[I] += gradFromDOF_c(w_dof[vel_l2g[eN_j]],vel_grad_trial[eN_k_j_nSpace+I]);
		}
	    }
          //
          //calculate pde coefficients and derivatives at quadrature points
          //
          evaluateCoefficients_c(eps_rho,
				 eps_mu,
				 sigma,
				 rho_0,
				 nu_0,
				 rho_1,
				 nu_1,
				 g,
				 phi[eN_k],
				 &n[eN_k_nSpace],
				 kappa[eN_k],
				 p,
				 grad_p,
				 u,
				 v,
				 w,
				 mom_u_acc,
				 dmom_u_acc_u,
				 mom_v_acc,
				 dmom_v_acc_v,
				 mom_w_acc,
				 dmom_w_acc_w,
				 mass_adv,
				 dmass_adv_u,
				 dmass_adv_v,
				 dmass_adv_w,
				 mom_u_adv,
				 dmom_u_adv_u,
				 dmom_u_adv_v,
				 dmom_u_adv_w,
				 mom_v_adv,
				 dmom_v_adv_u,
				 dmom_v_adv_v,
				 dmom_v_adv_w,
				 mom_w_adv,
				 dmom_w_adv_u,
				 dmom_w_adv_v,
				 dmom_w_adv_w,
				 mom_u_diff_ten,
				 mom_v_diff_ten,
				 mom_w_diff_ten,
				 mom_uv_diff_ten,
				 mom_uw_diff_ten,
				 mom_vu_diff_ten,
				 mom_vw_diff_ten,
				 mom_wu_diff_ten,
				 mom_wv_diff_ten,
				 mom_u_source,
				 mom_v_source,
				 mom_w_source,
				 mom_u_ham,
				 dmom_u_ham_grad_p,
				 mom_v_ham,
				 dmom_v_ham_grad_p,
				 mom_w_ham,
				 dmom_w_ham_grad_p);          
          //
          //moving mesh
          //
          //omit for now
          //
          //calculate time derivatives
          //
          bdf_c(alphaBDF,
		q_mom_u_acc_beta_bdf[eN_k],
		mom_u_acc,
		dmom_u_acc_u,
		mom_u_acc_t,
		dmom_u_acc_u_t);
          bdf_c(alphaBDF,
		q_mom_v_acc_beta_bdf[eN_k],
		mom_v_acc,
		dmom_v_acc_v,
		mom_v_acc_t,
		dmom_v_acc_v_t);
          bdf_c(alphaBDF,
		q_mom_w_acc_beta_bdf[eN_k],
		mom_w_acc,
		dmom_w_acc_w,
		mom_w_acc_t,
		dmom_w_acc_w_t);
          //
          //calculate subgrid error contribution to the Jacobian (strong residual, adjoint, jacobian of strong residual)
          //
          //calculate the adjoint times the test functions
          for (int i=0;i<nDOF_test_element;i++)
            {
	      int eN_k_i_nSpace = (eN_k*nDOF_trial_element+i)*nSpace;
	      int eN_k_i_nSpace2 = (eN_k*nDOF_trial_element+i)*nSpace2;

	      Lstar_u_p[i]=Advection_adjoint_c(dmass_adv_u,&p_grad_test_dV[eN_k_i_nSpace]);
	      Lstar_v_p[i]=Advection_adjoint_c(dmass_adv_v,&p_grad_test_dV[eN_k_i_nSpace]);
	      Lstar_w_p[i]=Advection_adjoint_c(dmass_adv_w,&p_grad_test_dV[eN_k_i_nSpace]);
	      Lstar_u_u[i]=Advection_adjoint_c(&q_velocity_last[eN_k_nSpace],&vel_grad_test_dV[eN_k_i_nSpace])+
		Diffusion_adjoint_c(mom_u_diff_ten[1],&vel_Hess_test_dV[eN_k_i_nSpace2]);
	      Lstar_v_v[i]=Advection_adjoint_c(&q_velocity_last[eN_k_nSpace],&vel_grad_test_dV[eN_k_i_nSpace])+
		Diffusion_adjoint_c(mom_v_diff_ten[2],&vel_Hess_test_dV[eN_k_i_nSpace2]);
	      Lstar_w_w[i]=Advection_adjoint_c(&q_velocity_last[eN_k_nSpace],&vel_grad_test_dV[eN_k_i_nSpace])+
		Diffusion_adjoint_c(mom_w_diff_ten[0],&vel_Hess_test_dV[eN_k_i_nSpace2]);
	      Lstar_p_u[i]=Hamiltonian_adjoint_c(dmom_u_ham_grad_p,&vel_grad_test_dV[eN_k_i_nSpace]);
	      Lstar_p_v[i]=Hamiltonian_adjoint_c(dmom_v_ham_grad_p,&vel_grad_test_dV[eN_k_i_nSpace]);
	      Lstar_p_w[i]=Hamiltonian_adjoint_c(dmom_w_ham_grad_p,&vel_grad_test_dV[eN_k_i_nSpace]);
            }
          //calculate the Jacobian of strong residual
          for (int j=0;j<nDOF_trial_element;j++)
            {
	      int eN_k_j=eN_k*nDOF_trial_element+j;
	      int eN_k_j_nSpace = eN_k_j*nSpace;
	      int eN_k_j_nSpace2 = eN_k_j*nSpace2;

	      dpdeResidual_p_u[j]=AdvectionJacobian_strong_c(dmass_adv_u,&vel_grad_trial[eN_k_j_nSpace]);
	      dpdeResidual_p_v[j]=AdvectionJacobian_strong_c(dmass_adv_v,&vel_grad_trial[eN_k_j_nSpace]);
	      dpdeResidual_p_w[j]=AdvectionJacobian_strong_c(dmass_adv_w,&vel_grad_trial[eN_k_j_nSpace]);

	      dpdeResidual_u_p[j]=HamiltonianJacobian_strong_c(dmom_u_ham_grad_p,&p_grad_trial[eN_k_j_nSpace]);
	      dpdeResidual_u_u[j]=MassJacobian_strong_c(dmom_u_acc_u_t,vel_trial[eN_k_j]) +
		AdvectionJacobian_strong_c(&q_velocity_last[eN_k_nSpace],&vel_grad_trial[eN_k_j_nSpace])+
		DiffusionJacobian_strong_c(mom_u_diff_ten[1],&vel_Hess_trial[eN_k_j_nSpace2]);
	      
	      dpdeResidual_v_p[j]=HamiltonianJacobian_strong_c(dmom_v_ham_grad_p,&p_grad_trial[eN_k_j_nSpace]);
	      dpdeResidual_v_v[j]=MassJacobian_strong_c(dmom_v_acc_v_t,vel_trial[eN_k_j]) +
		AdvectionJacobian_strong_c(&q_velocity_last[eN_k_nSpace],&vel_grad_trial[eN_k_j_nSpace])+
		DiffusionJacobian_strong_c(mom_v_diff_ten[2],&vel_Hess_trial[eN_k_j_nSpace2]);

	      dpdeResidual_w_p[j]=HamiltonianJacobian_strong_c(dmom_w_ham_grad_p,&p_grad_trial[eN_k_j_nSpace]);
	      dpdeResidual_w_w[j]=MassJacobian_strong_c(dmom_w_acc_w_t,vel_trial[eN_k_j]) + 
		AdvectionJacobian_strong_c(&q_velocity_last[eN_k_nSpace],&vel_grad_trial[eN_k_j_nSpace])+
		DiffusionJacobian_strong_c(mom_w_diff_ten[0],&vel_Hess_trial[eN_k_j_nSpace2]);
            }
          //tau and tau*Res
          calculateSubgridError_tau_c(hFactor,elementDiameter[eN],
				      dmom_u_acc_u_t,dmom_u_acc_u,
				      &q_velocity_last[eN_k_nSpace],mom_u_diff_ten[1],
				      tau_0,tau_1,q_cfl[eN_k]);
          calculateSubgridErrorDerivatives_tauRes_c(tau_0,
						    tau_1,
						    dpdeResidual_p_u,
						    dpdeResidual_p_v,
						    dpdeResidual_p_w,
						    dpdeResidual_u_p,
						    dpdeResidual_u_u,
						    dpdeResidual_v_p,
						    dpdeResidual_v_v,
						    dpdeResidual_w_p,
						    dpdeResidual_w_w,
						    dsubgridError_p_u,
						    dsubgridError_p_v,
						    dsubgridError_p_w,
						    dsubgridError_u_p,
						    dsubgridError_u_u,
						    dsubgridError_v_p,
						    dsubgridError_v_v,
						    dsubgridError_w_p,
						    dsubgridError_w_w);
  	  for(int i=0;i<nDOF_test_element;i++)
	    {
	      int eN_k_i=eN_k*nDOF_test_element+i;
	      int eN_k_i_nSpace=eN_k_i*nSpace;
	      for(int j=0;j<nDOF_trial_element;j++) 
		{ 
		  int eN_k_j=eN_k*nDOF_trial_element+j;
		  int eN_k_j_nSpace = eN_k_j*nSpace;

		  elementJacobian_p_p[i][j] += SubgridErrorJacobian_c(dsubgridError_u_p[j],Lstar_u_p[i]) + 
		    SubgridErrorJacobian_c(dsubgridError_v_p[j],Lstar_v_p[i]) + 
		    SubgridErrorJacobian_c(dsubgridError_w_p[j],Lstar_w_p[i]); 

		  elementJacobian_p_u[i][j] += AdvectionJacobian_weak_c(dmass_adv_u,vel_trial[eN_k_j],&p_grad_test_dV[eN_k_i_nSpace]) + 
		    SubgridErrorJacobian_c(dsubgridError_u_u[j],Lstar_u_p[i]); 
		  elementJacobian_p_v[i][j] += AdvectionJacobian_weak_c(dmass_adv_v,vel_trial[eN_k_j],&p_grad_test_dV[eN_k_i_nSpace]) + 
		    SubgridErrorJacobian_c(dsubgridError_v_v[j],Lstar_v_p[i]); 
		  elementJacobian_p_w[i][j] += AdvectionJacobian_weak_c(dmass_adv_w,vel_trial[eN_k_j],&p_grad_test_dV[eN_k_i_nSpace]) + 
		    SubgridErrorJacobian_c(dsubgridError_w_w[j],Lstar_w_p[i]); 

		  elementJacobian_u_p[i][j] += HamiltonianJacobian_weak_c(dmom_u_ham_grad_p,&p_grad_trial[eN_k_j_nSpace],vel_test_dV[eN_k_i]) + 
		    SubgridErrorJacobian_c(dsubgridError_u_p[j],Lstar_u_u[i]); 
		  elementJacobian_u_u[i][j] += MassJacobian_weak_c(dmom_u_acc_u_t,vel_trial[eN_k_j],vel_test_dV[eN_k_i]) + 
		    AdvectionJacobian_weak_c(dmom_u_adv_u,vel_trial[eN_k_j],&vel_grad_test_dV[eN_k_i_nSpace]) +
		    SimpleDiffusionJacobian_weak_c(sdInfo_u_u_rowptr,sdInfo_u_u_colind,mom_u_diff_ten,&vel_grad_trial[eN_k_j_nSpace],&vel_grad_test_dV[eN_k_i_nSpace]) + 
		    SubgridErrorJacobian_c(dsubgridError_p_u[j],Lstar_p_u[i]) + 
		    SubgridErrorJacobian_c(dsubgridError_u_u[j],Lstar_u_u[i]) + 
		    NumericalDiffusionJacobian_c(q_numDiff_u_last[eN_k],&vel_grad_trial[eN_k_j_nSpace],&vel_grad_test_dV[eN_k_i_nSpace]); 
		  elementJacobian_u_v[i][j] += AdvectionJacobian_weak_c(dmom_u_adv_v,vel_trial[eN_k_j],&vel_grad_test_dV[eN_k_i_nSpace]) + 
		    SimpleDiffusionJacobian_weak_c(sdInfo_u_v_rowptr,sdInfo_u_v_colind,mom_uv_diff_ten,&vel_grad_trial[eN_k_j_nSpace],&vel_grad_test_dV[eN_k_i_nSpace]) + 
		    SubgridErrorJacobian_c(dsubgridError_p_v[j],Lstar_p_u[i]); 
		  elementJacobian_u_w[i][j] += AdvectionJacobian_weak_c(dmom_u_adv_w,vel_trial[eN_k_j],&vel_grad_test_dV[eN_k_i_nSpace]) + 
		    SimpleDiffusionJacobian_weak_c(sdInfo_u_w_rowptr,sdInfo_u_w_colind,mom_uw_diff_ten,&vel_grad_trial[eN_k_j_nSpace],&vel_grad_test_dV[eN_k_i_nSpace]) + 
		    SubgridErrorJacobian_c(dsubgridError_p_w[j],Lstar_p_u[i]); 

		  elementJacobian_v_p[i][j] += HamiltonianJacobian_weak_c(dmom_v_ham_grad_p,&p_grad_trial[eN_k_j_nSpace],vel_test_dV[eN_k_i]) + 
		    SubgridErrorJacobian_c(dsubgridError_v_p[j],Lstar_v_v[i]); 
		  elementJacobian_v_u[i][j] += AdvectionJacobian_weak_c(dmom_v_adv_u,vel_trial[eN_k_j],&vel_grad_test_dV[eN_k_i_nSpace]) + 
		    SimpleDiffusionJacobian_weak_c(sdInfo_v_u_rowptr,sdInfo_v_u_colind,mom_vu_diff_ten,&vel_grad_trial[eN_k_j_nSpace],&vel_grad_test_dV[eN_k_i_nSpace]) + 
		    SubgridErrorJacobian_c(dsubgridError_p_u[j],Lstar_p_v[i]);
		  elementJacobian_v_v[i][j] += MassJacobian_weak_c(dmom_v_acc_v_t,vel_trial[eN_k_j],vel_test_dV[eN_k_i]) + 
		    AdvectionJacobian_weak_c(dmom_v_adv_v,vel_trial[eN_k_j],&vel_grad_test_dV[eN_k_i_nSpace]) +
		    SimpleDiffusionJacobian_weak_c(sdInfo_v_v_rowptr,sdInfo_v_v_colind,mom_v_diff_ten,&vel_grad_trial[eN_k_j_nSpace],&vel_grad_test_dV[eN_k_i_nSpace]) + 
		    SubgridErrorJacobian_c(dsubgridError_p_v[j],Lstar_p_v[i]) +
		    SubgridErrorJacobian_c(dsubgridError_v_v[j],Lstar_v_v[i]) + 
		    NumericalDiffusionJacobian_c(q_numDiff_v_last[eN_k],&vel_grad_trial[eN_k_j_nSpace],&vel_grad_test_dV[eN_k_i_nSpace]); 
		  elementJacobian_v_w[i][j] += AdvectionJacobian_weak_c(dmom_v_adv_w,vel_trial[eN_k_j],&vel_grad_test_dV[eN_k_i_nSpace]) +  
		    SimpleDiffusionJacobian_weak_c(sdInfo_v_w_rowptr,sdInfo_v_w_colind,mom_vw_diff_ten,&vel_grad_trial[eN_k_j_nSpace],&vel_grad_test_dV[eN_k_i_nSpace]) + 
		    SubgridErrorJacobian_c(dsubgridError_p_w[j],Lstar_p_v[i]);

		  elementJacobian_w_p[i][j] += HamiltonianJacobian_weak_c(dmom_w_ham_grad_p,&p_grad_trial[eN_k_j_nSpace],vel_test_dV[eN_k_i]) + 
		    SubgridErrorJacobian_c(dsubgridError_w_p[j],Lstar_w_w[i]); 
		  elementJacobian_w_u[i][j] += AdvectionJacobian_weak_c(dmom_w_adv_u,vel_trial[eN_k_j],&vel_grad_test_dV[eN_k_i_nSpace]) +  
		    SimpleDiffusionJacobian_weak_c(sdInfo_w_u_rowptr,sdInfo_w_u_colind,mom_wu_diff_ten,&vel_grad_trial[eN_k_j_nSpace],&vel_grad_test_dV[eN_k_i_nSpace]) + 
		    SubgridErrorJacobian_c(dsubgridError_p_u[j],Lstar_p_w[i]); 
		  elementJacobian_w_v[i][j] += AdvectionJacobian_weak_c(dmom_w_adv_v,vel_trial[eN_k_j],&vel_grad_test_dV[eN_k_i_nSpace]) + 
		    SimpleDiffusionJacobian_weak_c(sdInfo_w_v_rowptr,sdInfo_w_v_colind,mom_wv_diff_ten,&vel_grad_trial[eN_k_j_nSpace],&vel_grad_test_dV[eN_k_i_nSpace]) + 
		    SubgridErrorJacobian_c(dsubgridError_p_v[j],Lstar_p_w[i]); 
		  elementJacobian_w_w[i][j] += MassJacobian_weak_c(dmom_w_acc_w_t,vel_trial[eN_k_j],vel_test_dV[eN_k_i]) + 
		    AdvectionJacobian_weak_c(dmom_w_adv_w,vel_trial[eN_k_j],&vel_grad_test_dV[eN_k_i_nSpace]) +  
		    SimpleDiffusionJacobian_weak_c(sdInfo_w_w_rowptr,sdInfo_w_w_colind,mom_w_diff_ten,&vel_grad_trial[eN_k_j_nSpace],&vel_grad_test_dV[eN_k_i_nSpace]) + 
		    SubgridErrorJacobian_c(dsubgridError_p_w[j],Lstar_p_w[i]) + 
		    SubgridErrorJacobian_c(dsubgridError_w_w[j],Lstar_w_w[i]) + 
		    NumericalDiffusionJacobian_c(q_numDiff_w_last[eN_k],&vel_grad_trial[eN_k_j_nSpace],&vel_grad_test_dV[eN_k_i_nSpace]); 
		}//j
            }//i
	}//k
      //
      //load into element Jacobian into global Jacobian
      //
      for (int i=0;i<nDOF_test_element;i++)
	{
	  int eN_i = eN*nDOF_test_element+i;
	  for (int j=0;j<nDOF_trial_element;j++)
	    {
	      int eN_i_j = eN_i*nDOF_trial_element+j;
	      globalJacobian[csrRowIndeces_p_p[eN_i] + csrColumnOffsets_p_p[eN_i_j]] += elementJacobian_p_p[i][j];
	      globalJacobian[csrRowIndeces_p_u[eN_i] + csrColumnOffsets_p_u[eN_i_j]] += elementJacobian_p_u[i][j];
	      globalJacobian[csrRowIndeces_p_v[eN_i] + csrColumnOffsets_p_v[eN_i_j]] += elementJacobian_p_v[i][j];
	      globalJacobian[csrRowIndeces_p_w[eN_i] + csrColumnOffsets_p_w[eN_i_j]] += elementJacobian_p_w[i][j];

	      globalJacobian[csrRowIndeces_u_p[eN_i] + csrColumnOffsets_u_p[eN_i_j]] += elementJacobian_u_p[i][j];
	      globalJacobian[csrRowIndeces_u_u[eN_i] + csrColumnOffsets_u_u[eN_i_j]] += elementJacobian_u_u[i][j];
	      globalJacobian[csrRowIndeces_u_v[eN_i] + csrColumnOffsets_u_v[eN_i_j]] += elementJacobian_u_v[i][j];
	      globalJacobian[csrRowIndeces_u_w[eN_i] + csrColumnOffsets_u_w[eN_i_j]] += elementJacobian_u_w[i][j];

	      globalJacobian[csrRowIndeces_v_p[eN_i] + csrColumnOffsets_v_p[eN_i_j]] += elementJacobian_v_p[i][j];
	      globalJacobian[csrRowIndeces_v_u[eN_i] + csrColumnOffsets_v_u[eN_i_j]] += elementJacobian_v_u[i][j];
	      globalJacobian[csrRowIndeces_v_v[eN_i] + csrColumnOffsets_v_v[eN_i_j]] += elementJacobian_v_v[i][j];
	      globalJacobian[csrRowIndeces_v_w[eN_i] + csrColumnOffsets_v_w[eN_i_j]] += elementJacobian_v_w[i][j];

	      globalJacobian[csrRowIndeces_w_p[eN_i] + csrColumnOffsets_w_p[eN_i_j]] += elementJacobian_w_p[i][j];
	      globalJacobian[csrRowIndeces_w_u[eN_i] + csrColumnOffsets_w_u[eN_i_j]] += elementJacobian_w_u[i][j];
	      globalJacobian[csrRowIndeces_w_v[eN_i] + csrColumnOffsets_w_v[eN_i_j]] += elementJacobian_w_v[i][j];
	      globalJacobian[csrRowIndeces_w_w[eN_i] + csrColumnOffsets_w_w[eN_i_j]] += elementJacobian_w_w[i][j];
	    }//j
	}//i
    }//elements
  //
  //loop over exterior element boundaries to compute the surface integrals and load them into the global Jacobian
  //
  for (int ebNE = 0; ebNE < nExteriorElementBoundaries_global; ebNE++) 
    { 
      register int ebN = exteriorElementBoundariesArray[ebNE]; 
      register int eN  = elementBoundaryElementsArray[ebN*2+0];
      const double eps_rho = epsFact_rho*elementDiameter[eN],
      	eps_mu = epsFact_mu*elementDiameter[eN];
      for  (int kb=0;kb<nQuadraturePoints_elementBoundary;kb++) 
	{ 
	  register int ebNE_kb = ebNE*nQuadraturePoints_elementBoundary+kb,
	    ebNE_kb_nSpace = ebNE_kb*nSpace;

	  register double p_ext=0.0,
	    u_ext=0.0,
	    v_ext=0.0,
	    w_ext=0.0,
	    grad_p_ext[nSpace],
	    grad_u_ext[nSpace],
	    grad_v_ext[nSpace],
	    grad_w_ext[nSpace],
	    mom_u_acc_ext=0.0,
	    dmom_u_acc_u_ext=0.0,
	    mom_v_acc_ext=0.0,
	    dmom_v_acc_v_ext=0.0,
	    mom_w_acc_ext=0.0,
	    dmom_w_acc_w_ext=0.0,
	    mass_adv_ext[nSpace],
	    dmass_adv_u_ext[nSpace],
	    dmass_adv_v_ext[nSpace],
	    dmass_adv_w_ext[nSpace],
	    mom_u_adv_ext[nSpace],
	    dmom_u_adv_u_ext[nSpace],
	    dmom_u_adv_v_ext[nSpace],
	    dmom_u_adv_w_ext[nSpace],
	    mom_v_adv_ext[nSpace],
	    dmom_v_adv_u_ext[nSpace],
	    dmom_v_adv_v_ext[nSpace],
	    dmom_v_adv_w_ext[nSpace],
	    mom_w_adv_ext[nSpace],
	    dmom_w_adv_u_ext[nSpace],
	    dmom_w_adv_v_ext[nSpace],
	    dmom_w_adv_w_ext[nSpace],
	    mom_u_diff_ten_ext[nSpace],
	    mom_v_diff_ten_ext[nSpace],
	    mom_w_diff_ten_ext[nSpace],
	    mom_uv_diff_ten_ext[1],
	    mom_uw_diff_ten_ext[1],
	    mom_vu_diff_ten_ext[1],
	    mom_vw_diff_ten_ext[1],
	    mom_wu_diff_ten_ext[1],
	    mom_wv_diff_ten_ext[1],
	    mom_u_source_ext=0.0,
	    mom_v_source_ext=0.0,
	    mom_w_source_ext=0.0,
	    mom_u_ham_ext=0.0,
	    dmom_u_ham_grad_p_ext[nSpace],
	    mom_v_ham_ext=0.0,
	    dmom_v_ham_grad_p_ext[nSpace],
	    mom_w_ham_ext=0.0,
	    dmom_w_ham_grad_p_ext[nSpace],
	    dmom_u_adv_p_ext[nSpace],
	    dmom_v_adv_p_ext[nSpace],
	    dmom_w_adv_p_ext[nSpace],
	    dflux_mass_u_ext=0.0,
	    dflux_mass_v_ext=0.0,
	    dflux_mass_w_ext=0.0,
	    dflux_mom_u_adv_p_ext=0.0,
	    dflux_mom_u_adv_u_ext=0.0,
	    dflux_mom_u_adv_v_ext=0.0,
	    dflux_mom_u_adv_w_ext=0.0,
	    dflux_mom_v_adv_p_ext=0.0,
	    dflux_mom_v_adv_u_ext=0.0,
	    dflux_mom_v_adv_v_ext=0.0,
	    dflux_mom_v_adv_w_ext=0.0,
	    dflux_mom_w_adv_p_ext=0.0,
	    dflux_mom_w_adv_u_ext=0.0,
	    dflux_mom_w_adv_v_ext=0.0,
	    dflux_mom_w_adv_w_ext=0.0,
	    bc_p_ext=0.0,
	    bc_grad_p_ext[nSpace],
	    bc_grad_u_ext[nSpace],
	    bc_grad_v_ext[nSpace],
	    bc_grad_w_ext[nSpace],
	    bc_u_ext=0.0,
	    bc_v_ext=0.0,
	    bc_w_ext=0.0,
	    bc_mom_u_acc_ext=0.0,
	    bc_dmom_u_acc_u_ext=0.0,
	    bc_mom_v_acc_ext=0.0,
	    bc_dmom_v_acc_v_ext=0.0,
	    bc_mom_w_acc_ext=0.0,
	    bc_dmom_w_acc_w_ext=0.0,
	    bc_mass_adv_ext[nSpace],
	    bc_dmass_adv_u_ext[nSpace],
	    bc_dmass_adv_v_ext[nSpace],
	    bc_dmass_adv_w_ext[nSpace],
	    bc_mom_u_adv_ext[nSpace],
	    bc_dmom_u_adv_u_ext[nSpace],
	    bc_dmom_u_adv_v_ext[nSpace],
	    bc_dmom_u_adv_w_ext[nSpace],
	    bc_mom_v_adv_ext[nSpace],
	    bc_dmom_v_adv_u_ext[nSpace],
	    bc_dmom_v_adv_v_ext[nSpace],
	    bc_dmom_v_adv_w_ext[nSpace],
	    bc_mom_w_adv_ext[nSpace],
	    bc_dmom_w_adv_u_ext[nSpace],
	    bc_dmom_w_adv_v_ext[nSpace],
	    bc_dmom_w_adv_w_ext[nSpace],
	    bc_mom_u_diff_ten_ext[nSpace],
	    bc_mom_v_diff_ten_ext[nSpace],
	    bc_mom_w_diff_ten_ext[nSpace],
	    bc_mom_uv_diff_ten_ext[1],
	    bc_mom_uw_diff_ten_ext[1],
	    bc_mom_vu_diff_ten_ext[1],
	    bc_mom_vw_diff_ten_ext[1],
	    bc_mom_wu_diff_ten_ext[1],
	    bc_mom_wv_diff_ten_ext[1],
	    bc_mom_u_source_ext=0.0,
	    bc_mom_v_source_ext=0.0,
	    bc_mom_w_source_ext=0.0,
	    bc_mom_u_ham_ext=0.0,
	    bc_dmom_u_ham_grad_p_ext[nSpace],
	    bc_mom_v_ham_ext=0.0,
	    bc_dmom_v_ham_grad_p_ext[nSpace],
	    bc_mom_w_ham_ext=0.0,
	    bc_dmom_w_ham_grad_p_ext[nSpace],
	    fluxJacobian_p_p[nDOF_trial_element],
	    fluxJacobian_p_u[nDOF_trial_element],
	    fluxJacobian_p_v[nDOF_trial_element],
	    fluxJacobian_p_w[nDOF_trial_element],
	    fluxJacobian_u_p[nDOF_trial_element],
	    fluxJacobian_u_u[nDOF_trial_element],
	    fluxJacobian_u_v[nDOF_trial_element],
	    fluxJacobian_u_w[nDOF_trial_element],
	    fluxJacobian_v_p[nDOF_trial_element],
	    fluxJacobian_v_u[nDOF_trial_element],
	    fluxJacobian_v_v[nDOF_trial_element],
	    fluxJacobian_v_w[nDOF_trial_element],
	    fluxJacobian_w_p[nDOF_trial_element],
	    fluxJacobian_w_u[nDOF_trial_element],
	    fluxJacobian_w_v[nDOF_trial_element],
	    fluxJacobian_w_w[nDOF_trial_element];
	  // 
	  //calculate the solution and gradients at quadrature points 
	  // 
	  p_ext=0.0;u_ext=0.0;v_ext=0.0;w_ext=0.0;
	  for (int I=0;I<nSpace;I++)
	    {
	      grad_p_ext[I] = 0.0;
	      grad_u_ext[I] = 0.0;
	      grad_v_ext[I] = 0.0;
	      grad_w_ext[I] = 0.0;
	      bc_grad_p_ext[I] = 0.0;
	      bc_grad_u_ext[I] = 0.0;
	      bc_grad_v_ext[I] = 0.0;
	      bc_grad_w_ext[I] = 0.0;
	    }
	  for (int j=0;j<nDOF_trial_element;j++) 
	    { 
	      register int eN_j = eN*nDOF_trial_element+j,
		ebNE_kb_j = ebNE_kb*nDOF_trial_element+j,
		ebNE_kb_j_nSpace= ebNE_kb_j*nSpace;
	      p_ext += valFromDOF_c(p_dof[p_l2g[eN_j]],p_trial_ext[ebNE_kb_j]); 
	      u_ext += valFromDOF_c(u_dof[vel_l2g[eN_j]],vel_trial_ext[ebNE_kb_j]); 
	      v_ext += valFromDOF_c(v_dof[vel_l2g[eN_j]],vel_trial_ext[ebNE_kb_j]); 
	      w_ext += valFromDOF_c(w_dof[vel_l2g[eN_j]],vel_trial_ext[ebNE_kb_j]); 
               
	      for (int I=0;I<nSpace;I++)
		{
		  grad_p_ext[I] += gradFromDOF_c(p_dof[p_l2g[eN_j]],p_grad_trial_ext[ebNE_kb_j_nSpace+I]); 
		  grad_u_ext[I] += gradFromDOF_c(u_dof[vel_l2g[eN_j]],vel_grad_trial_ext[ebNE_kb_j_nSpace+I]); 
		  grad_v_ext[I] += gradFromDOF_c(v_dof[vel_l2g[eN_j]],vel_grad_trial_ext[ebNE_kb_j_nSpace+I]); 
		  grad_w_ext[I] += gradFromDOF_c(w_dof[vel_l2g[eN_j]],vel_grad_trial_ext[ebNE_kb_j_nSpace+I]);
		} 
	    }
	  //
	  //load the boundary values
	  //
	  bc_p_ext = isDOFBoundary_p[ebNE_kb]*ebqe_bc_p_ext[ebNE_kb]+(1-isDOFBoundary_p[ebNE_kb])*p_ext;
	  bc_u_ext = isDOFBoundary_u[ebNE_kb]*ebqe_bc_u_ext[ebNE_kb]+(1-isDOFBoundary_u[ebNE_kb])*u_ext;
	  bc_v_ext = isDOFBoundary_v[ebNE_kb]*ebqe_bc_v_ext[ebNE_kb]+(1-isDOFBoundary_v[ebNE_kb])*v_ext;
	  bc_w_ext = isDOFBoundary_w[ebNE_kb]*ebqe_bc_w_ext[ebNE_kb]+(1-isDOFBoundary_w[ebNE_kb])*w_ext;
	  // 
	  //calculate the internal and external trace of the pde coefficients 
	  // 
	  evaluateCoefficients_c(eps_rho,
				 eps_mu,
				 sigma,
				 rho_0,
				 nu_0,
				 rho_1,
				 nu_1,
				 g,
				 ebqe_phi_ext[ebNE_kb],
				 &ebqe_n_ext[ebNE_kb_nSpace],
				 ebqe_kappa_ext[ebNE_kb],
				 p_ext,
				 grad_p_ext,
				 u_ext,
				 v_ext,
				 w_ext,
				 mom_u_acc_ext,
				 dmom_u_acc_u_ext,
				 mom_v_acc_ext,
				 dmom_v_acc_v_ext,
				 mom_w_acc_ext,
				 dmom_w_acc_w_ext,
				 mass_adv_ext,
				 dmass_adv_u_ext,
				 dmass_adv_v_ext,
				 dmass_adv_w_ext,
				 mom_u_adv_ext,
				 dmom_u_adv_u_ext,
				 dmom_u_adv_v_ext,
				 dmom_u_adv_w_ext,
				 mom_v_adv_ext,
				 dmom_v_adv_u_ext,
				 dmom_v_adv_v_ext,
				 dmom_v_adv_w_ext,
				 mom_w_adv_ext,
				 dmom_w_adv_u_ext,
				 dmom_w_adv_v_ext,
				 dmom_w_adv_w_ext,
				 mom_u_diff_ten_ext,
				 mom_v_diff_ten_ext,
				 mom_w_diff_ten_ext,
				 mom_uv_diff_ten_ext,
				 mom_uw_diff_ten_ext,
				 mom_vu_diff_ten_ext,
				 mom_vw_diff_ten_ext,
				 mom_wu_diff_ten_ext,
				 mom_wv_diff_ten_ext,
				 mom_u_source_ext,
				 mom_v_source_ext,
				 mom_w_source_ext,
				 mom_u_ham_ext,
				 dmom_u_ham_grad_p_ext,
				 mom_v_ham_ext,
				 dmom_v_ham_grad_p_ext,
				 mom_w_ham_ext,
				 dmom_w_ham_grad_p_ext);          
	  evaluateCoefficients_c(eps_rho,
				 eps_mu,
				 sigma,
				 rho_0,
				 nu_0,
				 rho_1,
				 nu_1,
				 g,
				 ebqe_phi_ext[ebNE_kb],
				 &ebqe_n_ext[ebNE_kb_nSpace],
				 ebqe_kappa_ext[ebNE_kb],
				 bc_p_ext,
				 bc_grad_p_ext,
				 bc_u_ext,
				 bc_v_ext,
				 bc_w_ext,
				 bc_mom_u_acc_ext,
				 bc_dmom_u_acc_u_ext,
				 bc_mom_v_acc_ext,
				 bc_dmom_v_acc_v_ext,
				 bc_mom_w_acc_ext,
				 bc_dmom_w_acc_w_ext,
				 bc_mass_adv_ext,
				 bc_dmass_adv_u_ext,
				 bc_dmass_adv_v_ext,
				 bc_dmass_adv_w_ext,
				 bc_mom_u_adv_ext,
				 bc_dmom_u_adv_u_ext,
				 bc_dmom_u_adv_v_ext,
				 bc_dmom_u_adv_w_ext,
				 bc_mom_v_adv_ext,
				 bc_dmom_v_adv_u_ext,
				 bc_dmom_v_adv_v_ext,
				 bc_dmom_v_adv_w_ext,
				 bc_mom_w_adv_ext,
				 bc_dmom_w_adv_u_ext,
				 bc_dmom_w_adv_v_ext,
				 bc_dmom_w_adv_w_ext,
				 bc_mom_u_diff_ten_ext,
				 bc_mom_v_diff_ten_ext,
				 bc_mom_w_diff_ten_ext,
				 bc_mom_uv_diff_ten_ext,
				 bc_mom_uw_diff_ten_ext,
				 bc_mom_vu_diff_ten_ext,
				 bc_mom_vw_diff_ten_ext,
				 bc_mom_wu_diff_ten_ext,
				 bc_mom_wv_diff_ten_ext,
				 bc_mom_u_source_ext,
				 bc_mom_v_source_ext,
				 bc_mom_w_source_ext,
				 bc_mom_u_ham_ext,
				 bc_dmom_u_ham_grad_p_ext,
				 bc_mom_v_ham_ext,
				 bc_dmom_v_ham_grad_p_ext,
				 bc_mom_w_ham_ext,
				 bc_dmom_w_ham_grad_p_ext);          
	  // 
	  //calculate the numerical fluxes 
	  // 
	  exteriorNumericalAdvectiveFluxDerivatives_c(isDOFBoundary_p[ebNE_kb],
						      isDOFBoundary_u[ebNE_kb],
						      isDOFBoundary_v[ebNE_kb],
						      isDOFBoundary_w[ebNE_kb],
						      isAdvectiveFluxBoundary_p[ebNE_kb],
						      isAdvectiveFluxBoundary_u[ebNE_kb],
						      isAdvectiveFluxBoundary_v[ebNE_kb],
						      isAdvectiveFluxBoundary_w[ebNE_kb],
						      &ebqe_n_ext[ebNE_kb_nSpace],
						      bc_p_ext,
						      bc_mass_adv_ext,
						      bc_mom_u_adv_ext,
						      bc_mom_v_adv_ext,
						      bc_mom_w_adv_ext,
						      ebqe_bc_flux_mass_ext[ebNE_kb],
						      ebqe_bc_flux_mom_u_adv_ext[ebNE_kb],
						      ebqe_bc_flux_mom_v_adv_ext[ebNE_kb],
						      ebqe_bc_flux_mom_w_adv_ext[ebNE_kb],
						      p_ext,
						      mass_adv_ext,
						      mom_u_adv_ext,
						      mom_v_adv_ext,
						      mom_w_adv_ext,
						      dmass_adv_u_ext,
						      dmass_adv_v_ext,
						      dmass_adv_w_ext,
						      dmom_u_adv_p_ext,
						      dmom_u_adv_u_ext,
						      dmom_u_adv_v_ext,
						      dmom_u_adv_w_ext,
						      dmom_v_adv_p_ext,
						      dmom_v_adv_u_ext,
						      dmom_v_adv_v_ext,
						      dmom_v_adv_w_ext,
						      dmom_w_adv_p_ext,
						      dmom_w_adv_u_ext,
						      dmom_w_adv_v_ext,
						      dmom_w_adv_w_ext,
						      dflux_mass_u_ext,
						      dflux_mass_v_ext,
						      dflux_mass_w_ext,
						      dflux_mom_u_adv_p_ext,
						      dflux_mom_u_adv_u_ext,
						      dflux_mom_u_adv_v_ext,
						      dflux_mom_u_adv_w_ext,
						      dflux_mom_v_adv_p_ext,
						      dflux_mom_v_adv_u_ext,
						      dflux_mom_v_adv_v_ext,
						      dflux_mom_v_adv_w_ext,
						      dflux_mom_w_adv_p_ext,
						      dflux_mom_w_adv_u_ext,
						      dflux_mom_w_adv_v_ext,
						      dflux_mom_w_adv_w_ext);
	  //
	  //calculate the flux jacobian
	  //
	  for (int j=0;j<nDOF_trial_element;j++)
	    {
	      register int ebNE_kb_j = ebNE_kb*nDOF_trial_element+j,
		ebNE_kb_j_nSpace = ebNE_kb_j*nSpace;

	      fluxJacobian_p_p[j]=0.0;
	      fluxJacobian_p_u[j]=ExteriorNumericalAdvectiveFluxJacobian_c(dflux_mass_u_ext,vel_trial_ext[ebNE_kb_j]);
	      fluxJacobian_p_v[j]=ExteriorNumericalAdvectiveFluxJacobian_c(dflux_mass_v_ext,vel_trial_ext[ebNE_kb_j]);
	      fluxJacobian_p_w[j]=ExteriorNumericalAdvectiveFluxJacobian_c(dflux_mass_w_ext,vel_trial_ext[ebNE_kb_j]);

	      fluxJacobian_u_p[j]=ExteriorNumericalAdvectiveFluxJacobian_c(dflux_mom_u_adv_p_ext,p_trial_ext[ebNE_kb_j]);
	      fluxJacobian_u_u[j]=ExteriorNumericalAdvectiveFluxJacobian_c(dflux_mom_u_adv_u_ext,vel_trial_ext[ebNE_kb_j]) +
		ExteriorNumericalDiffusiveFluxJacobian_c(eps_rho,
							 ebqe_phi_ext[ebNE_kb],
							 sdInfo_u_u_rowptr,
							 sdInfo_u_u_colind,
							 isDOFBoundary_u[ebNE_kb],
							 &ebqe_n_ext[ebNE_kb_nSpace],
							 mom_u_diff_ten_ext,
							 vel_trial_ext[ebNE_kb_j],
							 &vel_grad_trial_ext[ebNE_kb_j_nSpace],
							 ebqe_penalty_ext[ebNE_kb]);
	      fluxJacobian_u_v[j]=ExteriorNumericalAdvectiveFluxJacobian_c(dflux_mom_u_adv_v_ext,vel_trial_ext[ebNE_kb_j]);
	      fluxJacobian_u_w[j]=ExteriorNumericalAdvectiveFluxJacobian_c(dflux_mom_u_adv_w_ext,vel_trial_ext[ebNE_kb_j]);

	      fluxJacobian_v_p[j]=ExteriorNumericalAdvectiveFluxJacobian_c(dflux_mom_v_adv_p_ext,p_trial_ext[ebNE_kb_j]);
	      fluxJacobian_v_u[j]=ExteriorNumericalAdvectiveFluxJacobian_c(dflux_mom_v_adv_u_ext,vel_trial_ext[ebNE_kb_j]);
	      fluxJacobian_v_v[j]=ExteriorNumericalAdvectiveFluxJacobian_c(dflux_mom_v_adv_v_ext,vel_trial_ext[ebNE_kb_j]) +
		ExteriorNumericalDiffusiveFluxJacobian_c(eps_rho,
							 ebqe_phi_ext[ebNE_kb],
							 sdInfo_v_v_rowptr,
							 sdInfo_v_v_colind,
							 isDOFBoundary_v[ebNE_kb],
							 &ebqe_n_ext[ebNE_kb_nSpace],
							 mom_v_diff_ten_ext,
							 vel_trial_ext[ebNE_kb_j],
							 &vel_grad_trial_ext[ebNE_kb_j_nSpace],
							 ebqe_penalty_ext[ebNE_kb]);
	      fluxJacobian_v_w[j]=ExteriorNumericalAdvectiveFluxJacobian_c(dflux_mom_v_adv_w_ext,vel_trial_ext[ebNE_kb_j]);

	      fluxJacobian_w_p[j]=ExteriorNumericalAdvectiveFluxJacobian_c(dflux_mom_w_adv_p_ext,p_trial_ext[ebNE_kb_j]);
	      fluxJacobian_w_u[j]=ExteriorNumericalAdvectiveFluxJacobian_c(dflux_mom_w_adv_u_ext,vel_trial_ext[ebNE_kb_j]);
	      fluxJacobian_w_v[j]=ExteriorNumericalAdvectiveFluxJacobian_c(dflux_mom_w_adv_v_ext,vel_trial_ext[ebNE_kb_j]);
	      fluxJacobian_w_w[j]=ExteriorNumericalAdvectiveFluxJacobian_c(dflux_mom_w_adv_w_ext,vel_trial_ext[ebNE_kb_j]) +
		ExteriorNumericalDiffusiveFluxJacobian_c(eps_rho,
							 ebqe_phi_ext[ebNE_kb],
							 sdInfo_w_w_rowptr,
							 sdInfo_w_w_colind,
							 isDOFBoundary_w[ebNE_kb],
							 &ebqe_n_ext[ebNE_kb_nSpace],
							 mom_w_diff_ten_ext,
							 vel_trial_ext[ebNE_kb_j],
							 &vel_grad_trial_ext[ebNE_kb_j_nSpace],
							 ebqe_penalty_ext[ebNE_kb]);
	    }//j
	  //
	  //update the global Jacobian from the flux Jacobian
	  //
	  for (int i=0;i<nDOF_test_element;i++)
	    {
	      register int eN_i = eN*nDOF_test_element+i,
		ebNE_kb_i = ebNE_kb*nDOF_test_element+i;
	      for (int j=0;j<nDOF_trial_element;j++)
		{
		  register int ebN_i_j = ebN*4*nDOF_test_X_trial_element + i*nDOF_trial_element + j;
		  
		  globalJacobian[csrRowIndeces_p_p[eN_i] + csrColumnOffsets_eb_p_p[ebN_i_j]] += fluxJacobian_p_p[j]*p_test_dS_ext[ebNE_kb_i];
		  globalJacobian[csrRowIndeces_p_u[eN_i] + csrColumnOffsets_eb_p_u[ebN_i_j]] += fluxJacobian_p_u[j]*p_test_dS_ext[ebNE_kb_i];
		  globalJacobian[csrRowIndeces_p_v[eN_i] + csrColumnOffsets_eb_p_v[ebN_i_j]] += fluxJacobian_p_v[j]*p_test_dS_ext[ebNE_kb_i];
		  globalJacobian[csrRowIndeces_p_w[eN_i] + csrColumnOffsets_eb_p_w[ebN_i_j]] += fluxJacobian_p_w[j]*p_test_dS_ext[ebNE_kb_i];
		   
		  globalJacobian[csrRowIndeces_u_p[eN_i] + csrColumnOffsets_eb_u_p[ebN_i_j]] += fluxJacobian_u_p[j]*vel_test_dS_ext[ebNE_kb_i];
		  globalJacobian[csrRowIndeces_u_u[eN_i] + csrColumnOffsets_eb_u_u[ebN_i_j]] += fluxJacobian_u_u[j]*vel_test_dS_ext[ebNE_kb_i];
		  globalJacobian[csrRowIndeces_u_v[eN_i] + csrColumnOffsets_eb_u_v[ebN_i_j]] += fluxJacobian_u_v[j]*vel_test_dS_ext[ebNE_kb_i];
		  globalJacobian[csrRowIndeces_u_w[eN_i] + csrColumnOffsets_eb_u_w[ebN_i_j]] += fluxJacobian_u_w[j]*vel_test_dS_ext[ebNE_kb_i];
		   
		  globalJacobian[csrRowIndeces_v_p[eN_i] + csrColumnOffsets_eb_v_p[ebN_i_j]] += fluxJacobian_v_p[j]*vel_test_dS_ext[ebNE_kb_i];
		  globalJacobian[csrRowIndeces_v_u[eN_i] + csrColumnOffsets_eb_v_u[ebN_i_j]] += fluxJacobian_v_u[j]*vel_test_dS_ext[ebNE_kb_i];
		  globalJacobian[csrRowIndeces_v_v[eN_i] + csrColumnOffsets_eb_v_v[ebN_i_j]] += fluxJacobian_v_v[j]*vel_test_dS_ext[ebNE_kb_i];
		  globalJacobian[csrRowIndeces_v_w[eN_i] + csrColumnOffsets_eb_v_w[ebN_i_j]] += fluxJacobian_v_w[j]*vel_test_dS_ext[ebNE_kb_i];
		   
		  globalJacobian[csrRowIndeces_w_p[eN_i] + csrColumnOffsets_eb_w_p[ebN_i_j]] += fluxJacobian_w_p[j]*vel_test_dS_ext[ebNE_kb_i];
		  globalJacobian[csrRowIndeces_w_u[eN_i] + csrColumnOffsets_eb_w_u[ebN_i_j]] += fluxJacobian_w_u[j]*vel_test_dS_ext[ebNE_kb_i];
		  globalJacobian[csrRowIndeces_w_v[eN_i] + csrColumnOffsets_eb_w_v[ebN_i_j]] += fluxJacobian_w_v[j]*vel_test_dS_ext[ebNE_kb_i];
		  globalJacobian[csrRowIndeces_w_w[eN_i] + csrColumnOffsets_eb_w_w[ebN_i_j]] += fluxJacobian_w_w[j]*vel_test_dS_ext[ebNE_kb_i];
		}//j
	    }//i
	}//kb
    }//ebNE
}//computeJacobian

extern "C" void calculateVelocityAverage_RANS2PQ(int nExteriorElementBoundaries_global,
						 int* exteriorElementBoundariesArray,
						 int nInteriorElementBoundaries_global,
						 int* interiorElementBoundariesArray,
						 int* elementBoundaryElementsArray,
						 int* elementBoundaryLocalElementBoundariesArray,
						 int* vel_l2g, 
						 double* u_dof, double* v_dof, double* w_dof,
						 double* vel_trial,
						 double* ebqe_velocity,
						 double* velocityAverage)
{
  using namespace RANS2PQ;
  for (int ebNE = 0; ebNE < nExteriorElementBoundaries_global; ebNE++) 
    { 
      register int ebN = exteriorElementBoundariesArray[ebNE],
	eN_global   = elementBoundaryElementsArray[ebN*2+0],
	ebN_element  = elementBoundaryLocalElementBoundariesArray[ebN*2+0];
      for  (int kb=0;kb<nQuadraturePoints_elementBoundary;kb++) 
	{ 
	  register int ebN_kb_nSpace = ebN*nQuadraturePoints_elementBoundary*nSpace+kb*nSpace,
	    ebNE_kb_nSpace = ebNE*nQuadraturePoints_elementBoundary*nSpace+kb*nSpace;
	  velocityAverage[ebN_kb_nSpace+0]=ebqe_velocity[ebNE_kb_nSpace+0];
	  velocityAverage[ebN_kb_nSpace+1]=ebqe_velocity[ebNE_kb_nSpace+1];
	  velocityAverage[ebN_kb_nSpace+2]=ebqe_velocity[ebNE_kb_nSpace+2];
	}//ebNE
    }
  for (int ebNI = 0; ebNI < nInteriorElementBoundaries_global; ebNI++) 
    { 
      register int ebN = interiorElementBoundariesArray[ebNI],
	left_eN_global   = elementBoundaryElementsArray[ebN*2+0],
	left_ebN_element  = elementBoundaryLocalElementBoundariesArray[ebN*2+0],
	right_eN_global  = elementBoundaryElementsArray[ebN*2+1],
	right_ebN_element = elementBoundaryLocalElementBoundariesArray[ebN*2+1];

      for  (int kb=0;kb<nQuadraturePoints_elementBoundary;kb++) 
	{ 
	  register int ebN_kb_nSpace = ebN*nQuadraturePoints_elementBoundary*nSpace+kb*nSpace;
	  register double u_left=0.0,
	    v_left=0.0,
	    w_left=0.0,
	    u_right=0.0,
	    v_right=0.0,
	    w_right=0.0;
	  // 
	  //calculate the velocity solution at quadrature points on left and right
	  // 
	  for (int j=0;j<nDOF_trial_element;j++) 
	    { 
	      int left_eN_j = left_eN_global*nDOF_trial_element+j;
	      int left_eN_ebN_kb_j = left_eN_global*nElementBoundaries_element*nQuadraturePoints_elementBoundary*nDOF_trial_element + 
		left_ebN_element*nQuadraturePoints_elementBoundary*nDOF_trial_element + 
		kb*nDOF_trial_element +
		j;
	      int right_eN_j = right_eN_global*nDOF_trial_element+j;
	      int right_eN_ebN_kb_j = right_eN_global*nElementBoundaries_element*nQuadraturePoints_elementBoundary*nDOF_trial_element + 
		right_ebN_element*nQuadraturePoints_elementBoundary*nDOF_trial_element + 
		kb*nDOF_trial_element +
		j;
	      u_left += valFromDOF_c(u_dof[vel_l2g[left_eN_j]],vel_trial[left_eN_ebN_kb_j]); 
	      v_left += valFromDOF_c(v_dof[vel_l2g[left_eN_j]],vel_trial[left_eN_ebN_kb_j]); 
	      w_left += valFromDOF_c(w_dof[vel_l2g[left_eN_j]],vel_trial[left_eN_ebN_kb_j]); 
	      u_right += valFromDOF_c(u_dof[vel_l2g[right_eN_j]],vel_trial[right_eN_ebN_kb_j]); 
	      v_right += valFromDOF_c(v_dof[vel_l2g[right_eN_j]],vel_trial[right_eN_ebN_kb_j]); 
	      w_right += valFromDOF_c(w_dof[vel_l2g[right_eN_j]],vel_trial[right_eN_ebN_kb_j]); 
	    }
	  velocityAverage[ebN_kb_nSpace+0]=0.5*(u_left + u_right);
	  velocityAverage[ebN_kb_nSpace+1]=0.5*(v_left + v_right);
	  velocityAverage[ebN_kb_nSpace+2]=0.5*(w_left + w_right);
	}//ebNI
    }
}

#include "RANS2PV2.h"
#include <iostream>
#include <cassert>

inline 
void calculateMapping_element(const int eN,
			      const int k,
			      double* mesh_dof,
			      int* mesh_l2g,
			      double* mesh_trial_ref,
			      double* mesh_grad_trial_ref,
			      double* jac,
			      double& jacDet,
			      double* jacInv,
			      double& x,
			      double& y,
			      double& z)
{
  using namespace RANS2PV2;
  const int X=0,Y=1,Z=2,
    XX=0,XY=1,XZ=2,
    YX=3,YY=4,YZ=5,
    ZX=6,ZY=7,ZZ=8;
  
  register double Grad_x[nSpace],Grad_y[nSpace],Grad_z[nSpace],oneOverJacDet;

  //
  //mapping of reference element to physical element
  //
  x=0.0;y=0.0;z=0.0;
  for (int I=0;I<nSpace;I++)
    {
      Grad_x[I]=0.0;Grad_y[I]=0.0;Grad_z[I]=0.0;
    }
  for (int j=0;j<nDOF_mesh_trial_element;j++)
    {
      int eN_j=eN*nDOF_mesh_trial_element+j;
      x += valFromDOF_c(mesh_dof[mesh_l2g[eN_j]*nSpace+0],mesh_trial_ref[k*nDOF_mesh_trial_element+j]);
      y += valFromDOF_c(mesh_dof[mesh_l2g[eN_j]*nSpace+1],mesh_trial_ref[k*nDOF_mesh_trial_element+j]);
      z += valFromDOF_c(mesh_dof[mesh_l2g[eN_j]*nSpace+2],mesh_trial_ref[k*nDOF_mesh_trial_element+j]);	      
      for (int I=0;I<nSpace;I++)
	{
	  Grad_x[I] += gradFromDOF_c(mesh_dof[mesh_l2g[eN_j]*nSpace+0],mesh_grad_trial_ref[k*nDOF_mesh_trial_element*nSpace+j*nSpace+I]);
	  Grad_y[I] += gradFromDOF_c(mesh_dof[mesh_l2g[eN_j]*nSpace+1],mesh_grad_trial_ref[k*nDOF_mesh_trial_element*nSpace+j*nSpace+I]);
	  Grad_z[I] += gradFromDOF_c(mesh_dof[mesh_l2g[eN_j]*nSpace+2],mesh_grad_trial_ref[k*nDOF_mesh_trial_element*nSpace+j*nSpace+I]);
	}
    }
  jac[XX] = Grad_x[X];//node[X]*grad[X];
  jac[XY] = Grad_x[Y];//node[X]*grad[Y];
  jac[XZ] = Grad_x[Z];//node[X]*grad[Z];
  jac[YX] = Grad_y[X];//node[Y]*grad[X];
  jac[YY] = Grad_y[Y];//node[Y]*grad[Y];
  jac[YZ] = Grad_y[Z];//node[Y]*grad[Z];
  jac[ZX] = Grad_z[X];//node[Z]*grad[X];
  jac[ZY] = Grad_z[Y];//node[Z]*grad[Y];
  jac[ZZ] = Grad_z[Z];//node[Z]*grad[Z];
  jacDet = 
    jac[XX]*(jac[YY]*jac[ZZ] - jac[YZ]*jac[ZY]) -
    jac[XY]*(jac[YX]*jac[ZZ] - jac[YZ]*jac[ZX]) +
    jac[XZ]*(jac[YX]*jac[ZY] - jac[YY]*jac[ZX]);
  oneOverJacDet = 1.0/jacDet;
  jacInv[XX] = oneOverJacDet*(jac[YY]*jac[ZZ] - jac[YZ]*jac[ZY]);
  jacInv[YX] = oneOverJacDet*(jac[YZ]*jac[ZX] - jac[YX]*jac[ZZ]);
  jacInv[ZX] = oneOverJacDet*(jac[YX]*jac[ZY] - jac[YY]*jac[ZX]);
  jacInv[XY] = oneOverJacDet*(jac[ZY]*jac[XZ] - jac[ZZ]*jac[XY]);
  jacInv[YY] = oneOverJacDet*(jac[ZZ]*jac[XX] - jac[ZX]*jac[XZ]);
  jacInv[ZY] = oneOverJacDet*(jac[ZX]*jac[XY] - jac[ZY]*jac[XX]);
  jacInv[XZ] = oneOverJacDet*(jac[XY]*jac[YZ] - jac[XZ]*jac[YY]);
  jacInv[YZ] = oneOverJacDet*(jac[XZ]*jac[YX] - jac[XX]*jac[YZ]);
  jacInv[ZZ] = oneOverJacDet*(jac[XX]*jac[YY] - jac[XY]*jac[YX]);
}

inline 
void calculateMapping_elementBoundary(const int eN,
				      const int ebN_local,
				      const int kb,
				      double* mesh_dof,
				      int* mesh_l2g,
				      double* mesh_trial_trace_ref,
				      double* mesh_grad_trial_trace_ref,
				      double* boundaryJac_ref,
				      double* jac,
				      double& jacDet,
				      double* jacInv,
				      double* boundaryJac,
				      double* metricTensor,
				      double& metricTensorDetSqrt,
				      double* normal_ref,
				      double* normal,
				      double& x,
				      double& y,
				      double& z)
{
  using namespace RANS2PV2;
  const int X=0,Y=1,Z=2,
    XX=0,XY=1,XZ=2,
    YX=3,YY=4,YZ=5,
    ZX=6,ZY=7,ZZ=8,
    XHX=0,XHY=1,
    YHX=2,YHY=3,
    ZHX=4,ZHY=5,
    HXHX=0,HXHY=1,
    HYHX=2,HYHY=3;
  const int ebN_local_kb = ebN_local*nQuadraturePoints_elementBoundary+kb,
    ebN_local_kb_nSpace = ebN_local_kb*nSpace;
  
  register double Grad_x_ext[nSpace],Grad_y_ext[nSpace],Grad_z_ext[nSpace],oneOverJacDet,norm_normal=0.0;
  // 
  //calculate mapping from the reference element to the physical element
  // 
  x=0.0;y=0.0;z=0.0;
  for (int I=0;I<nSpace;I++)
    {
      Grad_x_ext[I] = 0.0;
      Grad_y_ext[I] = 0.0;
      Grad_z_ext[I] = 0.0;
    }
  for (int j=0;j<nDOF_mesh_trial_element;j++) 
    { 
      int eN_j = eN*nDOF_trial_element+j;
      int ebN_local_kb_j = ebN_local_kb*nDOF_trial_element+j;
      int ebN_local_kb_j_nSpace = ebN_local_kb_j*nSpace;
      x += valFromDOF_c(mesh_dof[mesh_l2g[eN_j]*nSpace+0],mesh_trial_trace_ref[ebN_local_kb_j]); 
      y += valFromDOF_c(mesh_dof[mesh_l2g[eN_j]*nSpace+1],mesh_trial_trace_ref[ebN_local_kb_j]); 
      z += valFromDOF_c(mesh_dof[mesh_l2g[eN_j]*nSpace+2],mesh_trial_trace_ref[ebN_local_kb_j]); 
      for (int I=0;I<nSpace;I++)
	{
	  Grad_x_ext[I] += gradFromDOF_c(mesh_dof[mesh_l2g[eN_j]*nSpace+0],mesh_grad_trial_trace_ref[ebN_local_kb_j_nSpace+I]);
	  Grad_y_ext[I] += gradFromDOF_c(mesh_dof[mesh_l2g[eN_j]*nSpace+1],mesh_grad_trial_trace_ref[ebN_local_kb_j_nSpace+I]); 
	  Grad_z_ext[I] += gradFromDOF_c(mesh_dof[mesh_l2g[eN_j]*nSpace+2],mesh_grad_trial_trace_ref[ebN_local_kb_j_nSpace+I]);
	} 
    }
  //Space Mapping Jacobian
  jac[XX] = Grad_x_ext[X];//node[X]*grad[X];
  jac[XY] = Grad_x_ext[Y];//node[X]*grad[Y];
  jac[XZ] = Grad_x_ext[Z];//node[X]*grad[Z];
  jac[YX] = Grad_y_ext[X];//node[Y]*grad[X];
  jac[YY] = Grad_y_ext[Y];//node[Y]*grad[Y];
  jac[YZ] = Grad_y_ext[Z];//node[Y]*grad[Z];
  jac[ZX] = Grad_z_ext[X];//node[Z]*grad[X];
  jac[ZY] = Grad_z_ext[Y];//node[Z]*grad[Y];
  jac[ZZ] = Grad_z_ext[Z];//node[Z]*grad[Z];
  jacDet = 
    jac[XX]*(jac[YY]*jac[ZZ] - jac[YZ]*jac[ZY]) -
    jac[XY]*(jac[YX]*jac[ZZ] - jac[YZ]*jac[ZX]) +
    jac[XZ]*(jac[YX]*jac[ZY] - jac[YY]*jac[ZX]);
  oneOverJacDet = 1.0/jacDet;
  jacInv[XX] = oneOverJacDet*(jac[YY]*jac[ZZ] - jac[YZ]*jac[ZY]);
  jacInv[YX] = oneOverJacDet*(jac[YZ]*jac[ZX] - jac[YX]*jac[ZZ]);
  jacInv[ZX] = oneOverJacDet*(jac[YX]*jac[ZY] - jac[YY]*jac[ZX]);
  jacInv[XY] = oneOverJacDet*(jac[ZY]*jac[XZ] - jac[ZZ]*jac[XY]);
  jacInv[YY] = oneOverJacDet*(jac[ZZ]*jac[XX] - jac[ZX]*jac[XZ]);
  jacInv[ZY] = oneOverJacDet*(jac[ZX]*jac[XY] - jac[ZY]*jac[XX]);
  jacInv[XZ] = oneOverJacDet*(jac[XY]*jac[YZ] - jac[XZ]*jac[YY]);
  jacInv[YZ] = oneOverJacDet*(jac[XZ]*jac[YX] - jac[XX]*jac[YZ]);
  jacInv[ZZ] = oneOverJacDet*(jac[XX]*jac[YY] - jac[XY]*jac[YX]);
  //normal
  norm_normal=0.0;
  for (int I=0;I<nSpace;I++)
    normal[I] = 0.0;
  for (int I=0;I<nSpace;I++)
    {
      for (int J=0;J<nSpace;J++)
	normal[I] += jacInv[J*nSpace+I]*normal_ref[ebN_local_kb_nSpace+J];
      norm_normal+=normal[I]*normal[I];
    }
  norm_normal = sqrt(norm_normal);
  for (int I=0;I<nSpace;I++)
    normal[I] /= norm_normal;
  //metric tensor and determinant
  boundaryJac[XHX] = jac[XX]*boundaryJac_ref[ebN_local*6+XHX]+jac[XY]*boundaryJac_ref[ebN_local*6+YHX]+jac[XZ]*boundaryJac_ref[ebN_local*6+ZHX];
  boundaryJac[XHY] = jac[XX]*boundaryJac_ref[ebN_local*6+XHY]+jac[XY]*boundaryJac_ref[ebN_local*6+YHY]+jac[XZ]*boundaryJac_ref[ebN_local*6+ZHY];
  boundaryJac[YHX] = jac[YX]*boundaryJac_ref[ebN_local*6+XHX]+jac[YY]*boundaryJac_ref[ebN_local*6+YHX]+jac[YZ]*boundaryJac_ref[ebN_local*6+ZHX];
  boundaryJac[YHY] = jac[YX]*boundaryJac_ref[ebN_local*6+XHY]+jac[YY]*boundaryJac_ref[ebN_local*6+YHY]+jac[YZ]*boundaryJac_ref[ebN_local*6+ZHY];
  boundaryJac[ZHX] = jac[ZX]*boundaryJac_ref[ebN_local*6+XHX]+jac[ZY]*boundaryJac_ref[ebN_local*6+YHX]+jac[ZZ]*boundaryJac_ref[ebN_local*6+ZHX];
  boundaryJac[ZHY] = jac[ZX]*boundaryJac_ref[ebN_local*6+XHY]+jac[ZY]*boundaryJac_ref[ebN_local*6+YHY]+jac[ZZ]*boundaryJac_ref[ebN_local*6+ZHY];
  
  metricTensor[HXHX] = boundaryJac[XHX]*boundaryJac[XHX]+boundaryJac[YHX]*boundaryJac[YHX]+boundaryJac[ZHX]*boundaryJac[ZHX];
  metricTensor[HXHY] = boundaryJac[XHX]*boundaryJac[XHY]+boundaryJac[YHX]*boundaryJac[YHY]+boundaryJac[ZHX]*boundaryJac[ZHY];
  metricTensor[HYHX] = boundaryJac[XHY]*boundaryJac[XHX]+boundaryJac[YHY]*boundaryJac[YHX]+boundaryJac[ZHY]*boundaryJac[ZHX];
  metricTensor[HYHY] = boundaryJac[XHY]*boundaryJac[XHY]+boundaryJac[YHY]*boundaryJac[YHY]+boundaryJac[ZHY]*boundaryJac[ZHY];
  
  metricTensorDetSqrt=sqrt(metricTensor[HXHX]*metricTensor[HYHY]- metricTensor[HXHY]*metricTensor[HYHX]);
}

extern "C" void calculateResidual_RANS2PV2(//testing mesh replacement
					 double* mesh_trial_ref,
					 double* mesh_grad_trial_ref,
					 double* mesh_dof,
					 int* mesh_l2g,
					 double* dV_ref,
					 double* p_trial_ref,
					 double* p_grad_trial_ref,
					 double* p_test_ref,
					 double* p_grad_test_ref,
					 double* vel_trial_ref,
					 double* vel_grad_trial_ref,
					 double* vel_test_ref,
					 double* vel_grad_test_ref,
					 //element boundary
					 double* mesh_trial_trace_ref,
					 double* mesh_grad_trial_trace_ref,
					 double* dS_ref,
					 double* p_trial_trace_ref,
					 double* p_grad_trial_trace_ref,
					 double* p_test_trace_ref,
					 double* p_grad_test_trace_ref,
					 double* vel_trial_trace_ref,
					 double* vel_grad_trial_trace_ref,
					 double* vel_test_trace_ref,
					 double* vel_grad_test_trace_ref,					 
					 double* normal_ref,
					 double* boundaryJac_ref,
					 //end testing meshreplacement
					 int nElements_global,
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
  using namespace RANS2PV2;
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
	  register double p=0.0,u=0.0,v=0.0,w=0.0,grad_p[nSpace],grad_u[nSpace],grad_v[nSpace],grad_w[nSpace],mom_u_acc=0.0,
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
	    tau_1=0.0,
	    jac[nSpace*nSpace],
	    jacDet,
	    jacInv[nSpace*nSpace],
	    Grad_p[nSpace],Grad_u[nSpace],Grad_v[nSpace],Grad_w[nSpace],dV,p_test_dV_new[nDOF_test_element],vel_test_dV_new[nDOF_test_element],
	    p_grad_test_dV_new[nDOF_test_element*nSpace],vel_grad_test_dV_new[nDOF_test_element*nSpace],p_new,u_new,v_new,w_new,
	    grad_p_new[nSpace],grad_u_new[nSpace],grad_v_new[nSpace],grad_w_new[nSpace],x,y,z;
	  //get jacobian, etc for mapping reference element
	  calculateMapping_element(eN,
				   k,
				   mesh_dof,
				   mesh_l2g,
				   mesh_trial_ref,
				   mesh_grad_trial_ref,
				   jac,
				   jacDet,
				   jacInv,
				   x,y,z);
	  //get the physical integration weight
	  dV = fabs(jacDet)*dV_ref[k];
	  //get the solution and gradients
	  p=0.0;u=0.0;v=0.0;w=0.0;
	  for (int I=0;I<nSpace;I++)
	    {
	      Grad_p[I]=0.0;Grad_u[I]=0.0;Grad_v[I]=0.0;Grad_w[I]=0.0;
	      grad_p[I]=0.0;grad_u[I]=0.0;grad_v[I]=0.0;grad_w[I]=0.0;
	    }
	  for (int j=0;j<nDOF_trial_element;j++)
	    {
	      for (int I=0;I<nSpace;I++)
		{
		  p_grad_test_dV_new[j*nSpace+I]   = 0.0;
		  vel_grad_test_dV_new[j*nSpace+I] = 0.0;
		}
	      int eN_j=eN*nDOF_trial_element+j;

	      p += valFromDOF_c(p_dof[p_l2g[eN_j]],p_trial_ref[k*nDOF_trial_element+j]);
	      u += valFromDOF_c(u_dof[vel_l2g[eN_j]],vel_trial_ref[k*nDOF_trial_element+j]);
	      v += valFromDOF_c(v_dof[vel_l2g[eN_j]],vel_trial_ref[k*nDOF_trial_element+j]);
	      w += valFromDOF_c(w_dof[vel_l2g[eN_j]],vel_trial_ref[k*nDOF_trial_element+j]);	      
	      for (int I=0;I<nSpace;I++)
		{
		  Grad_p[I] += gradFromDOF_c(p_dof[p_l2g[eN_j]],p_grad_trial_ref[k*nDOF_trial_element*nSpace+j*nSpace+I]);
		  Grad_u[I] += gradFromDOF_c(u_dof[vel_l2g[eN_j]],vel_grad_trial_ref[k*nDOF_trial_element*nSpace+j*nSpace+I]);
		  Grad_v[I] += gradFromDOF_c(v_dof[vel_l2g[eN_j]],vel_grad_trial_ref[k*nDOF_trial_element*nSpace+j*nSpace+I]);
		  Grad_w[I] += gradFromDOF_c(w_dof[vel_l2g[eN_j]],vel_grad_trial_ref[k*nDOF_trial_element*nSpace+j*nSpace+I]);
		}
	      for (int I=0;I<nSpace;I++)
		{
		  for (int J=0;J<nSpace;J++)
		    {
		      p_grad_test_dV_new[j*nSpace+I] += jacInv[J*nSpace+I]*p_grad_test_ref[k*nDOF_trial_element*nSpace+j*nSpace+J];
		      vel_grad_test_dV_new[j*nSpace+I] += jacInv[J*nSpace+I]*vel_grad_test_ref[k*nDOF_trial_element*nSpace+j*nSpace+J];
		    }
		  p_grad_test_dV_new[j*nSpace+I]*=dV;
		  vel_grad_test_dV_new[j*nSpace+I]*=dV;
		}
	      p_test_dV_new[j] = p_test_ref[k*nDOF_test_element+j]*dV;
	      vel_test_dV_new[j] = vel_test_ref[k*nDOF_test_element+j]*dV;
	    }
	  //convert gradients to physical space with J^{-t} 
	  for (int I=0;I<nSpace;I++)
	    for (int J=0;J<nSpace;J++)
	      {
		grad_p[I] += jacInv[J*nSpace+I]*Grad_p[J];
		grad_u[I] += jacInv[J*nSpace+I]*Grad_u[J];
		grad_v[I] += jacInv[J*nSpace+I]*Grad_v[J];
		grad_w[I] += jacInv[J*nSpace+I]*Grad_w[J];
	      }
	  p_new = p;
	  u_new = u;
	  v_new = v;
	  w_new = w;
	  for (int I=0;I<nSpace;I++)
	    {
	      grad_p_new[I] = grad_p[I];
	      grad_u_new[I] = grad_u[I];
	      grad_v_new[I] = grad_v[I];
	      grad_w_new[I] = grad_w[I];
	    }
	  //
	  //end testing of pre-computed mesh quantities
	  // 
          //
          //compute solution and gradients at quadrature points
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
	  //save velocity at quadrature points for other models to use
	  // q_velocity[eN_k_nSpace+0]=u+subgridError_u;
	  // q_velocity[eN_k_nSpace+1]=v+subgridError_v;
	  // q_velocity[eN_k_nSpace+2]=w+subgridError_w;
// 	  q_velocity[eN_k_nSpace+0]=u;
// 	  q_velocity[eN_k_nSpace+1]=v;
// 	  q_velocity[eN_k_nSpace+2]=w;
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
	  //save momentum for time history and velocity for subgrid error
	  //
	  q_mom_u_acc[eN_k] = mom_u_acc;                            
	  q_mom_v_acc[eN_k] = mom_v_acc;                            
	  q_mom_w_acc[eN_k] = mom_w_acc;
	  //subgrid error uses grid scale velocity
	  q_mass_adv[eN_k_nSpace+0] = u;
	  q_mass_adv[eN_k_nSpace+1] = v;
	  q_mass_adv[eN_k_nSpace+2] = w;
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
	    Reaction_strong_c(mom_u_source);

	  pdeResidual_v = Mass_strong_c(mom_v_acc_t) +
	    Advection_strong_c(&q_velocity_last[eN_k_nSpace],grad_v) +
	    Hamiltonian_strong_c(dmom_v_ham_grad_p,grad_p) + 
	    Reaction_strong_c(mom_v_source);

	  pdeResidual_w= Mass_strong_c(mom_w_acc_t) + 
	    Advection_strong_c(&q_velocity_last[eN_k_nSpace],grad_w) +
	    Hamiltonian_strong_c(dmom_w_ham_grad_p,grad_p) +
	    Reaction_strong_c(mom_w_source);

          //calculate adjoint
          for (int i=0;i<nDOF_test_element;i++)
            {
	      register int eN_k_i_nSpace = (eN_k*nDOF_trial_element+i)*nSpace;

	      Lstar_u_p[i]=Advection_adjoint_c(dmass_adv_u,&p_grad_test_dV[eN_k_i_nSpace]);
	      Lstar_v_p[i]=Advection_adjoint_c(dmass_adv_v,&p_grad_test_dV[eN_k_i_nSpace]);
	      Lstar_w_p[i]=Advection_adjoint_c(dmass_adv_w,&p_grad_test_dV[eN_k_i_nSpace]);
	      Lstar_u_u[i]=Advection_adjoint_c(&q_velocity_last[eN_k_nSpace],&vel_grad_test_dV[eN_k_i_nSpace]);
	      Lstar_v_v[i]=Advection_adjoint_c(&q_velocity_last[eN_k_nSpace],&vel_grad_test_dV[eN_k_i_nSpace]);
	      Lstar_w_w[i]=Advection_adjoint_c(&q_velocity_last[eN_k_nSpace],&vel_grad_test_dV[eN_k_i_nSpace]);
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
	      double epsTest=1.0e-10;
	      if(fabs(p - p_new) > epsTest)
		std::cout<<"p"<<std::endl;
	      if(fabs(u - u_new) > epsTest)
		std::cout<<"u"<<std::endl;
	      if(fabs(v - v_new) > epsTest)
		std::cout<<"v"<<std::endl;
	      if(fabs(w - w_new) > epsTest)
		std::cout<<"w"<<std::endl;
	      if(fabs(p_test_dV[eN_k_i]-p_test_dV_new[i]) > epsTest)
		std::cout<<"p_test_dV"<<p_test_dV[eN_k_i]<<'\t'<<p_test_dV_new[i]<<std::endl;
	      if(fabs(vel_test_dV[eN_k_i] - vel_test_dV_new[i]) > epsTest)
		std::cout<<"vel_test_dV"<<vel_test_dV[eN_k_i]<<'\t'<<vel_test_dV_new[i]<<std::endl;
	      for (int I=0;I<nSpace;I++)
		{
		  if(fabs(grad_p[I] - grad_p_new[I]) > epsTest)
		    std::cout<<"grad_p"<<(grad_p[I] - grad_p_new[I])<<std::endl;
		  if(fabs(grad_u[I] - grad_u_new[I]) > epsTest)
		    std::cout<<"grad_u"<<(grad_u[I] - grad_u_new[I])<<std::endl;
		  if(fabs(grad_v[I] - grad_v_new[I]) > epsTest)
		    std::cout<<"grad_v"<<(grad_v[I] - grad_v_new[I])<<std::endl;
		  if(fabs(grad_w[I] - grad_w_new[I]) > epsTest)
		    std::cout<<"grad_w"<<(grad_w[I] - grad_w_new[I])<<std::endl;
		  if (fabs(p_grad_test_dV[eN_k_i_nSpace+I] - p_grad_test_dV_new[i*nSpace+I]) > epsTest)
		    std::cout<<"p_grad_test_dV"<<p_grad_test_dV[eN_k_i_nSpace+I]<<'\t'<<p_grad_test_dV_new[i*nSpace+I]<<std::endl;
		  if (fabs(vel_grad_test_dV[eN_k_i_nSpace+I] - vel_grad_test_dV_new[i*nSpace+I]) > epsTest)
		    std::cout<<"vel_grad_test_dV"<<vel_grad_test_dV[eN_k_i_nSpace+I]<<'\t'<<vel_grad_test_dV_new[i*nSpace+I]<<std::endl;
		}
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
	eN  = elementBoundaryElementsArray[ebN*2+0],
	ebN_local = elementBoundaryLocalElementBoundariesArray[ebN*2+0];
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
	    ebNE_kb_nSpace = ebNE_kb*nSpace,
	    ebN_local_kb = ebN_local*nQuadraturePoints_elementBoundary+kb;
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
	    bc_dmom_w_ham_grad_p_ext[nSpace],
	    jac_ext[nSpace*nSpace],
	    jacDet_ext,
	    jacInv_ext[nSpace*nSpace],
	    boundaryJac[nSpace*(nSpace-1)],
	    metricTensor[(nSpace-1)*(nSpace-1)],
	    metricTensorDetSqrt,
	    Grad_p_ext[nSpace],
	    Grad_u_ext[nSpace],
	    Grad_v_ext[nSpace],
	    Grad_w_ext[nSpace],dS,p_test_dS_new[nDOF_test_element],vel_test_dS_new[nDOF_test_element],
	    p_ext_new,u_ext_new,v_ext_new,w_ext_new,grad_p_ext_new[nSpace],grad_u_ext_new[nSpace],grad_v_ext_new[nSpace],grad_w_ext_new[nSpace],normal[3],x_ext,y_ext,z_ext;

	  //
	  //start testing replacement for precomputed quanties
	  //
	  //std::cout<<"computing mesh quantities------------------elementBoundary"<<std::endl;
	  calculateMapping_elementBoundary(eN,
					   ebN_local,
					   kb,
					   mesh_dof,
					   mesh_l2g,
					   mesh_trial_trace_ref,
					   mesh_grad_trial_trace_ref,
					   boundaryJac_ref,
					   jac_ext,
					   jacDet_ext,
					   jacInv_ext,
					   boundaryJac,
					   metricTensor,
					   metricTensorDetSqrt,
					   normal_ref,
					   normal,
					   x_ext,y_ext,z_ext);
	  dS = metricTensorDetSqrt*dS_ref[kb];

	  p_ext=0.0;u_ext=0.0;v_ext=0.0;w_ext=0.0;
	  for (int I=0;I<nSpace;I++)
	    {
	      grad_p_ext[I] = 0.0;
	      grad_u_ext[I] = 0.0;
	      grad_v_ext[I] = 0.0;
	      grad_w_ext[I] = 0.0;
	      Grad_p_ext[I] = 0.0;
	      Grad_u_ext[I] = 0.0;
	      Grad_v_ext[I] = 0.0;
	      Grad_w_ext[I] = 0.0;
	      bc_grad_p_ext[I] = 0.0;
	      bc_grad_u_ext[I] = 0.0;
	      bc_grad_v_ext[I] = 0.0;
	      bc_grad_w_ext[I] = 0.0;
	    }
	  for (int j=0;j<nDOF_trial_element;j++) 
	    { 
	      int eN_j = eN*nDOF_trial_element+j;
	      int ebN_local_kb_j = ebN_local_kb*nDOF_trial_element+j;
	      int ebN_local_kb_j_nSpace = ebN_local_kb_j*nSpace;
	      p_ext += valFromDOF_c(p_dof[p_l2g[eN_j]],p_trial_trace_ref[ebN_local_kb_j]); 
	      u_ext += valFromDOF_c(u_dof[vel_l2g[eN_j]],vel_trial_trace_ref[ebN_local_kb_j]); 
	      v_ext += valFromDOF_c(v_dof[vel_l2g[eN_j]],vel_trial_trace_ref[ebN_local_kb_j]); 
	      w_ext += valFromDOF_c(w_dof[vel_l2g[eN_j]],vel_trial_trace_ref[ebN_local_kb_j]); 
               
	      for (int I=0;I<nSpace;I++)
		{
		  Grad_p_ext[I] += gradFromDOF_c(p_dof[p_l2g[eN_j]],p_grad_trial_trace_ref[ebN_local_kb_j_nSpace+I]); 
		  Grad_u_ext[I] += gradFromDOF_c(u_dof[vel_l2g[eN_j]],vel_grad_trial_trace_ref[ebN_local_kb_j_nSpace+I]); 
		  Grad_v_ext[I] += gradFromDOF_c(v_dof[vel_l2g[eN_j]],vel_grad_trial_trace_ref[ebN_local_kb_j_nSpace+I]); 
		  Grad_w_ext[I] += gradFromDOF_c(w_dof[vel_l2g[eN_j]],vel_grad_trial_trace_ref[ebN_local_kb_j_nSpace+I]);
		} 
	      p_test_dS_new[j] = p_test_trace_ref[ebN_local_kb_j]*dS;
	      vel_test_dS_new[j] = vel_test_trace_ref[ebN_local_kb_j]*dS;
	    }
	  //convert gradients to physical space with J^{-t} 
	  for (int I=0;I<nSpace;I++)
	    for (int J=0;J<nSpace;J++)
	      {
		grad_p_ext[I] += jacInv_ext[J*nSpace+I]*Grad_p_ext[J];
		grad_u_ext[I] += jacInv_ext[J*nSpace+I]*Grad_u_ext[J];
		grad_v_ext[I] += jacInv_ext[J*nSpace+I]*Grad_v_ext[J];
		grad_w_ext[I] += jacInv_ext[J*nSpace+I]*Grad_w_ext[J];
	      }
	  p_ext_new = p_ext;
	  u_ext_new = u_ext;
	  v_ext_new = v_ext;
	  w_ext_new = w_ext;
	  for (int I=0;I<nSpace;I++)
	    {
	      grad_p_ext_new[I] = grad_p_ext[I];
	      grad_u_ext_new[I] = grad_u_ext[I];
	      grad_v_ext_new[I] = grad_v_ext[I];
	      grad_w_ext_new[I] = grad_w_ext[I];
	    }	  
	  //end testing
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

	      double epsTest=1.0e-10;
	      if(fabs(p_ext - p_ext_new) > epsTest)
		std::cout<<"p_ext"<<fabs(p_ext - p_ext_new)<<std::endl;
	      if(fabs(u_ext - u_ext_new) > epsTest)
		std::cout<<"u_ext"<<fabs(u_ext - u_ext_new)<<std::endl;
	      if(fabs(v_ext - v_ext_new) > epsTest)
		std::cout<<"v_ext"<<fabs(v_ext - v_ext_new)<<std::endl;
	      if(fabs(w_ext - w_ext_new) > epsTest)
		std::cout<<"w_ext"<<fabs(w_ext - w_ext_new)<<std::endl;
	      if(fabs(p_test_dS_ext[ebNE_kb_i]-p_test_dS_new[i]) > epsTest)
		std::cout<<"p_test_dS"<<p_test_dS_ext[ebNE_kb_i]<<'\t'<<p_test_dS_new[i]<<std::endl;
	      if(fabs(vel_test_dS_ext[ebNE_kb_i] - vel_test_dS_new[i]) > epsTest)
		std::cout<<"vel_test_dS"<<vel_test_dS_ext[ebNE_kb_i]<<'\t'<<vel_test_dS_new[i]<<std::endl;
	      for (int I=0;I<nSpace;I++)
		{
		  if(fabs(grad_p_ext[I] - grad_p_ext_new[I]) > epsTest)
		    std::cout<<"grad_p_ext"<<(grad_p_ext[I] - grad_p_ext_new[I])<<std::endl;
		  if(fabs(grad_u_ext[I] - grad_u_ext_new[I]) > epsTest)
		    std::cout<<"grad_u_ext"<<(grad_u_ext[I] - grad_u_ext_new[I])<<std::endl;
		  if(fabs(grad_v_ext[I] - grad_v_ext_new[I]) > epsTest)
		    std::cout<<"grad_v_ext"<<(grad_v_ext[I] - grad_v_ext_new[I])<<std::endl;
		  if(fabs(grad_w_ext[I] - grad_w_ext_new[I]) > epsTest)
		    std::cout<<"grad_w_ext"<<(grad_w_ext[I] - grad_w_ext_new[I])<<std::endl;
		  // if (fabs(p_grad_test_dS[ebNE_kb_i_nSpace+I] - p_grad_test_dS_new[i*nSpace+I]) > epsTest)
		  //   std::cout<<p_grad_test_dS[ebNE_kb_i_nSpace+I]<<'\t'<<p_grad_test_dS_new[i*nSpace+I]<<std::endl;
		  // if (fabs(vel_grad_test_dS[ebNE_kb_i_nSpace+I] - vel_grad_test_dS_new[i*nSpace+I]) > epsTest)
		  //   std::cout<<vel_grad_test_dS[ebNE_kb_i_nSpace+I]<<'\t'<<vel_grad_test_dS_new[i*nSpace+I]<<std::endl;
		}
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
}

extern "C" void calculateJacobian_RANS2PV2(int nElements_global,
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
  using namespace RANS2PV2;
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

	      Lstar_u_p[i]=Advection_adjoint_c(dmass_adv_u,&p_grad_test_dV[eN_k_i_nSpace]);
	      Lstar_v_p[i]=Advection_adjoint_c(dmass_adv_v,&p_grad_test_dV[eN_k_i_nSpace]);
	      Lstar_w_p[i]=Advection_adjoint_c(dmass_adv_w,&p_grad_test_dV[eN_k_i_nSpace]);
	      Lstar_u_u[i]=Advection_adjoint_c(&q_velocity_last[eN_k_nSpace],&vel_grad_test_dV[eN_k_i_nSpace]);
	      Lstar_v_v[i]=Advection_adjoint_c(&q_velocity_last[eN_k_nSpace],&vel_grad_test_dV[eN_k_i_nSpace]);
	      Lstar_w_w[i]=Advection_adjoint_c(&q_velocity_last[eN_k_nSpace],&vel_grad_test_dV[eN_k_i_nSpace]);
	      Lstar_p_u[i]=Hamiltonian_adjoint_c(dmom_u_ham_grad_p,&vel_grad_test_dV[eN_k_i_nSpace]);
	      Lstar_p_v[i]=Hamiltonian_adjoint_c(dmom_v_ham_grad_p,&vel_grad_test_dV[eN_k_i_nSpace]);
	      Lstar_p_w[i]=Hamiltonian_adjoint_c(dmom_w_ham_grad_p,&vel_grad_test_dV[eN_k_i_nSpace]);
            }
          //calculate the Jacobian of strong residual
          for (int j=0;j<nDOF_trial_element;j++)
            {
	      int eN_k_j=eN_k*nDOF_trial_element+j;
	      int eN_k_j_nSpace = eN_k_j*nSpace;

	      dpdeResidual_p_u[j]=AdvectionJacobian_strong_c(dmass_adv_u,&vel_grad_trial[eN_k_j_nSpace]);
	      dpdeResidual_p_v[j]=AdvectionJacobian_strong_c(dmass_adv_v,&vel_grad_trial[eN_k_j_nSpace]);
	      dpdeResidual_p_w[j]=AdvectionJacobian_strong_c(dmass_adv_w,&vel_grad_trial[eN_k_j_nSpace]);

	      dpdeResidual_u_p[j]=HamiltonianJacobian_strong_c(dmom_u_ham_grad_p,&p_grad_trial[eN_k_j_nSpace]);
	      dpdeResidual_u_u[j]=MassJacobian_strong_c(dmom_u_acc_u_t,vel_trial[eN_k_j]) +
		AdvectionJacobian_strong_c(&q_velocity_last[eN_k_nSpace],&vel_grad_trial[eN_k_j_nSpace]);

	      dpdeResidual_v_p[j]=HamiltonianJacobian_strong_c(dmom_v_ham_grad_p,&p_grad_trial[eN_k_j_nSpace]);
	      dpdeResidual_v_v[j]=MassJacobian_strong_c(dmom_v_acc_v_t,vel_trial[eN_k_j]) +
		AdvectionJacobian_strong_c(&q_velocity_last[eN_k_nSpace],&vel_grad_trial[eN_k_j_nSpace]);

	      dpdeResidual_w_p[j]=HamiltonianJacobian_strong_c(dmom_w_ham_grad_p,&p_grad_trial[eN_k_j_nSpace]);
	      dpdeResidual_w_w[j]=MassJacobian_strong_c(dmom_w_acc_w_t,vel_trial[eN_k_j]) + 
		AdvectionJacobian_strong_c(&q_velocity_last[eN_k_nSpace],&vel_grad_trial[eN_k_j_nSpace]);
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

extern "C" void calculateVelocityAverage_RANS2PV2(int nExteriorElementBoundaries_global,
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
  using namespace RANS2PV2;
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

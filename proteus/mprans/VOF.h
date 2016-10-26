#ifndef VOF_H
#define VOF_H
#include <cmath>
#include <iostream>
#include "CompKernel.h"
#include "ModelFactory.h"

#define EDGE_VISCOSITY 1
#define LUMPED_MASS_MATRIX 1 
//ENTROPY FUNCTIONS and SOME FLAGS (MQL)//
//#define entropy_power 1. // phiL and phiR are dummy variables
//#define ENTROPY(phi,phiL,phiR) 1./entropy_power*std::pow(phi,entropy_power)
//#define ENTROPY_GRAD(phi,phix,phiL,phiR) std::pow(phi,entropy_power-1.)*phix
// LOG ENTROPY FOR LEVEL SET FROM 0 to 1
#define ENTROPY(phi,phiL,phiR) std::log(std::abs((phi-phiL)*(phiR-phi))+1E-14)
#define ENTROPY_GRAD(phi,phix,phiL,phiR) (phiL+phiR-2*phi)*phix*((phi-phiL)*(phiR-phi)>=0 ? 1 : -1)/(std::abs((phi-phiL)*(phiR-phi))+1E-14) 

namespace proteus
{
  class VOF_base
  {
    //The base class defining the interface
  public:
    virtual ~VOF_base(){}
    virtual void calculateResidual(//element
				   double* mesh_trial_ref,
				   double* mesh_grad_trial_ref,
				   double* mesh_dof,
				   double* meshVelocity_dof,
				   double MOVING_DOMAIN,
				   int* mesh_l2g,
				   double* dV_ref,
				   double* u_trial_ref,
				   double* u_grad_trial_ref,
				   double* u_test_ref,
				   double* u_grad_test_ref,
				   //element boundary
				   double* mesh_trial_trace_ref,
				   double* mesh_grad_trial_trace_ref,
				   double* dS_ref,
				   double* u_trial_trace_ref,
				   double* u_grad_trial_trace_ref,
				   double* u_test_trace_ref,
				   double* u_grad_test_trace_ref,
				   double* normal_ref,
				   double* boundaryJac_ref,
				   //physics
				   int nElements_global,
			           double useMetrics, 
				   double alphaBDF,
				   int lag_shockCapturing,
				   double shockCapturingDiffusion,
			           double sc_uref, double sc_alpha,
				   //VRANS
				   const double* q_porosity,
				   //
				   int* u_l2g, 
				   double* elementDiameter,
				   double* u_dof,double* u_dof_old,
				   double* velx_tn_dof, 
				   double* vely_tn_dof, // HACKED TO 2D FOR NOW (MQL)
				   double* velocity,
				   double* q_m,
				   double* q_u,
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
				   //VRANS
				   const double* ebqe_porosity_ext,
				   //
				   int* isDOFBoundary_u,
				   double* ebqe_bc_u_ext,
				   int* isFluxBoundary_u,
				   double* ebqe_bc_flux_u_ext,
				   double* ebqe_phi,double epsFact,
				   double* ebqe_u,
				   double* ebqe_flux,
				   // PARAMETERS FOR ENTROPY_VISCOSITY 
				   double cE,
				   double cMax, 
				   double cK,
				   int ENTROPY_VISCOSITY,
				   int IMPLICIT, 
				   int SUPG, 
				   // PARAMETERS FOR LOG BASED ENTROPY FUNCTION 
				   double uL, 
				   double uR, 
				   // PARAMETERS FOR EDGE VISCOSITY 
				   int numDOFs,
				   int* csrRowIndeces,
				   int* csrColumnOffsets,
				   double* Cx, 
				   double* Cy,
				   double* CTx,
				   double* CTy)=0;
    virtual void calculateJacobian(//element
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
				   //element boundary
				   double* mesh_trial_trace_ref,
				   double* mesh_grad_trial_trace_ref,
				   double* dS_ref,
				   double* u_trial_trace_ref,
				   double* u_grad_trial_trace_ref,
				   double* u_test_trace_ref,
				   double* u_grad_test_trace_ref,
				   double* normal_ref,
				   double* boundaryJac_ref,
				   //physics
				   int nElements_global,
			           double useMetrics, 
				   double alphaBDF,
				   int lag_shockCapturing,/*mwf not used yet*/
				   double shockCapturingDiffusion,
				   //VRANS
				   const double* q_porosity,
				   //
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
				   //VRANS
				   const double* ebqe_porosity_ext,
				   //
				   int* isDOFBoundary_u,
				   double* ebqe_bc_u_ext,
				   int* isFluxBoundary_u,
				   double* ebqe_bc_flux_u_ext,
				   int* csrColumnOffsets_eb_u_u,
				   int IMPLICIT, 
				   int SUPG)=0;
  };

  template<class CompKernelType,
	   int nSpace,
	   int nQuadraturePoints_element,
	   int nDOF_mesh_trial_element,
	   int nDOF_trial_element,
	   int nDOF_test_element,
	   int nQuadraturePoints_elementBoundary>
  class VOF : public VOF_base
  {
  public:
    const int nDOF_test_X_trial_element;
    CompKernelType ck;
    VOF():
      nDOF_test_X_trial_element(nDOF_test_element*nDOF_trial_element),
      ck()
    {}
    inline
    void evaluateCoefficients(const double v[nSpace],
			      const double& u,
			      const double& porosity, //VRANS specific
			      double& m,
			      double& dm,
			      double f[nSpace],
			      double df[nSpace])
    {
    m = porosity*u;
    dm= porosity;
    for (int I=0; I < nSpace; I++)
      {
	f[I] = v[I]*porosity*u;
	df[I] = v[I]*porosity;
      }
    }

    inline
    void calculateCFL(const double& elementDiameter,
		      const double df[nSpace],
		      double& cfl)
    {
      double h,nrm_v;
      h = elementDiameter;
      nrm_v=0.0;
      for(int I=0;I<nSpace;I++)
	nrm_v+=df[I]*df[I];
      nrm_v = sqrt(nrm_v);
      cfl = nrm_v/h;
    }

    inline
    void calculateSubgridError_tau(const double& elementDiameter,
				   const double& dmt,
				   const double dH[nSpace],
				   double& cfl,
				   double& tau)
    {
      double h,nrm_v,oneByAbsdt;
      h = elementDiameter;
      nrm_v=0.0;
      for(int I=0;I<nSpace;I++)
	nrm_v+=dH[I]*dH[I];
      nrm_v = sqrt(nrm_v);
      cfl = nrm_v/h;
      oneByAbsdt =  fabs(dmt);
      tau = 1.0/(2.0*nrm_v/h + oneByAbsdt + 1.0e-8);
    }

 
    inline
    void calculateSubgridError_tau(     const double&  Ct_sge,
                                        const double   G[nSpace*nSpace],
					const double&  A0,
					const double   Ai[nSpace],
					double& tau_v,
					double& cfl)	
    {
      double v_d_Gv=0.0; 
      for(int I=0;I<nSpace;I++) 
         for (int J=0;J<nSpace;J++) 
           v_d_Gv += Ai[I]*G[I*nSpace+J]*Ai[J];     
    
      tau_v = 1.0/sqrt(Ct_sge*A0*A0 + v_d_Gv + 1.0e-8);    
    } 
 
 

    inline 
    void calculateNumericalDiffusion(const double& shockCapturingDiffusion,
				     const double& elementDiameter,
				     const double& strong_residual,
				     const double grad_u[nSpace],
				     double& numDiff)
    {
      double h,
	num,
	den,
	n_grad_u;
      h = elementDiameter;
      n_grad_u = 0.0;
      for (int I=0;I<nSpace;I++)
	n_grad_u += grad_u[I]*grad_u[I];
      num = shockCapturingDiffusion*0.5*h*fabs(strong_residual);
      den = sqrt(n_grad_u) + 1.0e-8;
      numDiff = num/den;
    }

    inline
    void exteriorNumericalAdvectiveFlux(const int& isDOFBoundary_u,
					const int& isFluxBoundary_u,
					const double n[nSpace],
					const double& bc_u,
					const double& bc_flux_u,
					const double& u,
					const double velocity[nSpace],
					double& flux)
    {

      double flow=0.0;
      for (int I=0; I < nSpace; I++)
	flow += n[I]*velocity[I];
      //std::cout<<" isDOFBoundary_u= "<<isDOFBoundary_u<<" flow= "<<flow<<std::endl;
      if (isDOFBoundary_u == 1)
	{
	  //std::cout<<"Dirichlet boundary u and bc_u "<<u<<'\t'<<bc_u<<std::endl;
	  if (flow >= 0.0)
	    {
	      flux = u*flow;
	      //flux = flow;
	    }
	  else
	    {
	      flux = bc_u*flow;
	      //flux = flow;
	    }
	}
      else if (isFluxBoundary_u == 1)
	{
	  flux = bc_flux_u;
	  //std::cout<<"Flux boundary flux and flow"<<flux<<'\t'<<flow<<std::endl;
	}
      else
	{
	  //std::cout<<"No BC boundary flux and flow"<<flux<<'\t'<<flow<<std::endl;
	  if (flow >= 0.0)
	    {
	      flux = u*flow;
	    }
	  else
	    {
	      std::cout<<"warning: VOF open boundary with no external trace, setting to zero for inflow"<<std::endl;
	      flux = 0.0;
	    }

	}
      //flux = flow;
      //std::cout<<"flux error "<<flux-flow<<std::endl;
      //std::cout<<"flux in computationa"<<flux<<std::endl;
    }

    inline
    void exteriorNumericalAdvectiveFluxDerivative(const int& isDOFBoundary_u,
						  const int& isFluxBoundary_u,
						  const double n[nSpace],
						  const double velocity[nSpace],
						  double& dflux)
    {
      double flow=0.0;
      for (int I=0; I < nSpace; I++)
	flow += n[I]*velocity[I];
      //double flow=n[0]*velocity[0]+n[1]*velocity[1]+n[2]*velocity[2];
      dflux=0.0;//default to no flux
      if (isDOFBoundary_u == 1)
	{
	  if (flow >= 0.0)
	    {
	      dflux = flow;
	    }
	  else
	    {
	      dflux = 0.0;
	    }
	}
      else if (isFluxBoundary_u == 1)
	{
	  dflux = 0.0;
	}
      else
	{
	  if (flow >= 0.0)
	    {
	      dflux = flow;
	    }
	}
    }

    void calculateResidual(//element
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
			   //element boundary
			   double* mesh_trial_trace_ref,
			   double* mesh_grad_trial_trace_ref,
			   double* dS_ref,
			   double* u_trial_trace_ref,
			   double* u_grad_trial_trace_ref,
			   double* u_test_trace_ref,
			   double* u_grad_test_trace_ref,
			   double* normal_ref,
			   double* boundaryJac_ref,
			   //physics
			   int nElements_global,
			   double useMetrics, 
			   double alphaBDF,
			   int lag_shockCapturing, /*mwf not used yet*/
			   double shockCapturingDiffusion,
			   double sc_uref, double sc_alpha,
			   //VRANS
			   const double* q_porosity,
			   //
			   int* u_l2g, 
			   double* elementDiameter,
			   double* u_dof,double* u_dof_old,
			   double* velx_tn_dof, 
			   double* vely_tn_dof, // HACKED TO 2D FOR NOW (MQL)
			   double* velocity,
			   double* q_m,
			   double* q_u,
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
			   //VRANS
			   const double* ebqe_porosity_ext,
			   //
			   int* isDOFBoundary_u,
			   double* ebqe_bc_u_ext,
			   int* isFluxBoundary_u,
			   double* ebqe_bc_flux_u_ext,
			   double* ebqe_phi,double epsFact,
			   double* ebqe_u,
			   double* ebqe_flux,
			   double cE,
			   double cMax, 
			   double cK,
			   int ENTROPY_VISCOSITY,
			   int IMPLICIT, 
			   int SUPG,
			   // PARAMETERS FOR LOG BASED ENTROPY FUNCTION 
			   double uL, 
			   double uR,
			   // PARAMETERS FOR EDGE VISCOSITY 
			   int numDOFs,
			   int* csrRowIndeces,
			   int* csrColumnOffsets,
			   double* Cx, 
			   double* Cy, 
			   double* CTx,
			   double* CTy)
    {
      double dt = 1./alphaBDF; // HACKED to work just for BDF1
      if (EDGE_VISCOSITY==1)
	{
	  // ** LOOP IN DOFs FOR EDGE BASED TERMS ** //
	  int ij=0;
	  for (int i=0; i<numDOFs; i++)
	    {
	      double vxi = velx_tn_dof[i];
	      double vyi = vely_tn_dof[i]; // velocity at time tn for the ith DOF
	      double solni = u_dof_old[i]; // solution at time tn for the ith DOF
	      
	      double ith_flux_term = 0;
	      double ith_dissipative_term = 0;
	      
	      for (int offset=csrRowIndeces[i]; offset<csrRowIndeces[i+1]; offset++)
		{
		  int j = csrColumnOffsets[offset];
		  double vxj = velx_tn_dof[j];
		  double vyj = vely_tn_dof[j]; // velocity at time tn for the jth DOF
		  double solnj = u_dof_old[j]; // solution at time tn for the jth DOF
		  
		  ith_flux_term += solnj*(vxj*Cx[ij] + vyj*Cy[ij]);
		  if (i != j) //NOTE: if no matrices are computed then there is no need to check for i!=j (see formula for ith_dissipative_term)
		    {
		      double dLij = -std::max(std::abs(vxi*Cx[ij] + vyi*Cy[ij]),std::abs(vxj*CTx[ij] + vyj*CTy[ij]));
		      //double dLii -= dLij;
		      ith_dissipative_term += dLij*(solnj-solni);
		    }
		  //update ij
		  ij+=1;
		}
	      // update residual 
	      globalResidual[i] = dt*(ith_flux_term + ith_dissipative_term);
	    }
	  // ** LOOP IN CELLS FOR CELL BASED TERMS ** //
	  for(int eN=0;eN<nElements_global;eN++)
	    {
	      //declare local storage for element residual and initialize
	      register double elementResidual_u[nDOF_test_element];
	      for (int i=0;i<nDOF_test_element;i++)
		elementResidual_u[i]=0.0;
	      //loop over quadrature points and compute integrands
	      for  (int k=0;k<nQuadraturePoints_element;k++)
		{
		  //compute indeces and declare local storage
		  register int eN_k = eN*nQuadraturePoints_element+k,
		    eN_k_nSpace = eN_k*nSpace,
		    eN_nDOF_trial_element = eN*nDOF_trial_element;
		  register double u=0.0, m=0.0, dm=0.0, f[nSpace], df[nSpace], m_t=0.0,dm_t=0.0,
		    jac[nSpace*nSpace], jacDet, jacInv[nSpace*nSpace],
		    u_test_dV[nDOF_trial_element], 
		    dV,x,y,z,
		    //VRANS
		    porosity;
		  //get the physical integration weight
		  ck.calculateMapping_element(eN,
					      k,
					      mesh_dof,
					      mesh_l2g,
					      mesh_trial_ref,
					      mesh_grad_trial_ref,
					      jac,
					      jacDet,
					      jacInv,
					      x,y,z);
		  dV = fabs(jacDet)*dV_ref[k];
		  //get the solution
		  ck.valFromDOF(u_dof,&u_l2g[eN_nDOF_trial_element],&u_trial_ref[k*nDOF_trial_element],u);
		  //precalculate test function products with integration weights
		  for (int j=0;j<nDOF_trial_element;j++)
		    u_test_dV[j] = u_test_ref[k*nDOF_trial_element+j]*dV;
		  //evaluate coefficients to compute time derivative
		  porosity = q_porosity[eN_k];
		  evaluateCoefficients(&velocity[eN_k_nSpace],
				       u,
				       //VRANS
				       porosity,
				       //
				       m,
				       dm,
				       f,
				       df);
		  //calculate time derivative at quadrature points
		  if (q_dV_last[eN_k] <= -100)
		  q_dV_last[eN_k] = dV;
		  q_dV[eN_k] = dV;
		  ck.bdf(alphaBDF,
			 q_m_betaBDF[eN_k]*q_dV_last[eN_k]/dV,//ensure prior mass integral is correct for  m_t with BDF1
			 m,
			 dm,
			 m_t,
			 dm_t);
		  // CALCULATE CFL //
		  calculateCFL(elementDiameter[eN],df,cfl[eN_k]); // TODO: ADJUST SPEED IF MESH IS MOVING
		  for(int i=0;i<nDOF_test_element;i++) 
		    { 
		      //register int eN_k_i=eN_k*nDOF_test_element+i,
		      //eN_k_i_nSpace = eN_k_i*nSpace,
		      register int i_nSpace=i*nSpace;
		      if (LUMPED_MASS_MATRIX==1)
			elementResidual_u[i] += u_test_dV[i]; // LUMPING
		      else 
			elementResidual_u[i] += dt*ck.Mass_weak(m_t,u_test_dV[i]);

		    }//i
		  //save solution for other models 
		  q_u[eN_k] = u;
		  q_m[eN_k] = m;
		}
	      //load cell based element into global residual
	      for(int i=0;i<nDOF_test_element;i++) 
		{ 
		  register int eN_i=eN*nDOF_test_element+i;
		  register int gi = offset_u+stride_u*u_l2g[eN_i];
		  if (LUMPED_MASS_MATRIX==1)
		    globalResidual[gi] += elementResidual_u[i]*(u_dof[gi]-u_dof_old[gi]); //LUMPING
		  else
		    globalResidual[gi] += elementResidual_u[i];

		}//i
	    }//elements
	  ////////////////////////////////
	}
      else // CELL BASED VISCOSITY/METHODS
	{
	  // ** COMPUTE QUANTITIES PER CELL (MQL) ** //
	  double entropy_max=-1.E10, entropy_min=1.E10, cell_entropy_mean, cell_volume, volume=0, entropy_mean=0;
	  double cell_vel_max, cell_entropy_residual;
	  double dt = 1./alphaBDF; // HACKED to work just for BDF1
	  double entropy_residual[nElements_global], vel_max[nElements_global];
	  double entropy_normalization_factor=1.0;	  
	  if (ENTROPY_VISCOSITY==1)
	    {
	      for(int eN=0;eN<nElements_global;eN++)
		{
		  cell_volume = 0;
		  cell_entropy_mean = 0;
		  cell_vel_max = 0;
		  cell_entropy_residual = 0;
		  //loop over quadrature points and compute integrands
		  for  (int k=0;k<nQuadraturePoints_element;k++)
		    {
		      //compute indeces and declare local storage
		      register int eN_k = eN*nQuadraturePoints_element+k,
			eN_k_nSpace = eN_k*nSpace,
			eN_nDOF_trial_element = eN*nDOF_trial_element;
		      register double un=0.0,unm1=0, grad_un[nSpace], vn[nSpace],
			jac[nSpace*nSpace],jacDet,jacInv[nSpace*nSpace],
			u_grad_trial[nDOF_trial_element*nSpace],
			dV,x,y,z,
			porosity=q_porosity[eN_k];
		      ck.calculateMapping_element(eN,k,mesh_dof,mesh_l2g,mesh_trial_ref,mesh_grad_trial_ref,jac,jacDet,jacInv,x,y,z);
		      //get the physical integration weight
		      dV = fabs(jacDet)*dV_ref[k];
		      //get the trial function gradients
		      ck.gradTrialFromRef(&u_grad_trial_ref[k*nDOF_trial_element*nSpace],jacInv,u_grad_trial);
		      //get the solution at quad point at tn and tnm1
		      ck.valFromDOF(u_dof,&u_l2g[eN_nDOF_trial_element],&u_trial_ref[k*nDOF_trial_element],un);
		      ck.valFromDOF(u_dof_old,&u_l2g[eN_nDOF_trial_element],&u_trial_ref[k*nDOF_trial_element],unm1);
		      //get the solution gradients at tn
		      ck.gradFromDOF(u_dof,&u_l2g[eN_nDOF_trial_element],u_grad_trial,grad_un);
		      //velocity at tn
		      vn[0] = velocity[eN_k_nSpace];
		      vn[1] = velocity[eN_k_nSpace+1];
		      // compute entropy min and max
		      entropy_max = std::max(entropy_max,ENTROPY(un,uL,uR));
		      entropy_min = std::min(entropy_min,ENTROPY(un,uL,uR));
		      cell_entropy_mean += ENTROPY(un,uL,uR)*dV;
		      cell_volume += dV;
		      cell_vel_max = std::max(cell_vel_max,std::max(std::abs(vn[0]),std::abs(vn[1])));
		      cell_entropy_residual 
			= std::max(std::abs((ENTROPY(un,uL,uR) - ENTROPY(unm1,uL,uR))/dt
					    + vn[0]*ENTROPY_GRAD(un,grad_un[0],uL,uR)+vn[1]*ENTROPY_GRAD(un,grad_un[1],uL,uR) 
					    + ENTROPY(un,uL,uR)*(vn[0]+vn[1])),cell_entropy_residual);
		    }
		  volume += cell_volume;
		  entropy_mean += cell_entropy_mean;
		  vel_max[eN]=cell_vel_max;
		  entropy_residual[eN] = cell_entropy_residual;
		}//elements
	      entropy_mean /= volume;
	      // ** END OF CELL COMPUTATIONS (MQL) ** //
	      entropy_normalization_factor = std::max(std::abs(entropy_max-entropy_mean),
						      std::abs(entropy_min-entropy_mean));
	      //abort();
	    } //ENTROPY_VISCOSITY==1
	  //////////////////////////////////////////
	  //std::cout<<"numDiff address "<<q_numDiff_u<<std::endl
	  //       <<"ndlast  address "<<q_numDiff_u_last<<std::endl;
	  
	  //cek should this be read in?
	  double Ct_sge = 4.0;
	  
	  //loop over elements to compute volume integrals and load them into element and global residual
	  //
	  //eN is the element index
	  //eN_k is the quadrature point index for a scalar
	  //eN_k_nSpace is the quadrature point index for a vector
	  //eN_i is the element test function index
	  //eN_j is the element trial function index
	  //eN_k_j is the quadrature point index for a trial function
	  //eN_k_i is the quadrature point index for a trial function
	  for(int eN=0;eN<nElements_global;eN++)
	    {
	      //declare local storage for element residual and initialize
	      register double elementResidual_u[nDOF_test_element];
	      for (int i=0;i<nDOF_test_element;i++)
		{
		  elementResidual_u[i]=0.0;
		}//i
	      //loop over quadrature points and compute integrands
	      for  (int k=0;k<nQuadraturePoints_element;k++)
		{
		  //compute indeces and declare local storage
		  register int eN_k = eN*nQuadraturePoints_element+k,
		    eN_k_nSpace = eN_k*nSpace,
		    eN_nDOF_trial_element = eN*nDOF_trial_element;
		  register double u=0.0, u_old=0.0, u_star=0.0, grad_u[nSpace], grad_u_old[nSpace], grad_u_star[nSpace],
		    m_star=0.0, dm_star=0.0, m=0.0, dm=0.0, 
		    f_star[nSpace], df_star[nSpace], f[nSpace], df[nSpace],
		    m_t=0.0,dm_t=0.0,
		    pdeResidual_u_star=0.0,
		    Lstar_u[nDOF_test_element],
		    subgridError_u=0.0,
		    tau=0.0,tau0=0.0,tau1=0.0,
		    numDiff0=0.0,numDiff1=0.0,
		    jac[nSpace*nSpace],
		    jacDet,
		    jacInv[nSpace*nSpace],
		    u_grad_trial[nDOF_trial_element*nSpace],
		    u_test_dV[nDOF_trial_element],
		    u_grad_test_dV[nDOF_test_element*nSpace],
		    dV,x,y,z,xt,yt,zt,
		    //VRANS
		    porosity,
		    //
		    G[nSpace*nSpace],G_dd_G,tr_G;//norm_Rv;
		  
		  ck.calculateMapping_element(eN,
					      k,
					      mesh_dof,
					      mesh_l2g,
					      mesh_trial_ref,
					      mesh_grad_trial_ref,
					      jac,
					      jacDet,
					      jacInv,
					      x,y,z);
		  ck.calculateMappingVelocity_element(eN,
						      k,
						      mesh_velocity_dof,
						      mesh_l2g,
						      mesh_trial_ref,
						      xt,yt,zt);
		  //get the physical integration weight
		  dV = fabs(jacDet)*dV_ref[k];
		  ck.calculateG(jacInv,G,G_dd_G,tr_G);
		  //get the trial function gradients
		  ck.gradTrialFromRef(&u_grad_trial_ref[k*nDOF_trial_element*nSpace],jacInv,u_grad_trial);
		  //get the solution
		  ck.valFromDOF(u_dof,&u_l2g[eN_nDOF_trial_element],&u_trial_ref[k*nDOF_trial_element],u);
		  ck.valFromDOF(u_dof_old,&u_l2g[eN_nDOF_trial_element],&u_trial_ref[k*nDOF_trial_element],u_old);
		  //get the solution gradients
		  ck.gradFromDOF(u_dof,&u_l2g[eN_nDOF_trial_element],u_grad_trial,grad_u);
		  ck.gradFromDOF(u_dof_old,&u_l2g[eN_nDOF_trial_element],u_grad_trial,grad_u_old);
		  //precalculate test function products with integration weights
		  for (int j=0;j<nDOF_trial_element;j++)
		    {
		      u_test_dV[j] = u_test_ref[k*nDOF_trial_element+j]*dV;
		      for (int I=0;I<nSpace;I++)
			{
			  u_grad_test_dV[j*nSpace+I] = u_grad_trial[j*nSpace+I]*dV;//cek warning won't work for Petrov-Galerkin
			}
		    }
		  //VRANS
		  porosity = q_porosity[eN_k];
		  // COMPUTE u and u_grad star to allow easy change between BACKWARD OR FORWARD EULER (for transport)
		  u_star = IMPLICIT*u+(1-IMPLICIT)*u_old;
		  for (int I=0; I<nSpace; I++)
		    grad_u_star[I] = IMPLICIT*grad_u[I]+(1-IMPLICIT)*grad_u_old[I];
		  //
		  //calculate pde coefficients at quadrature points
		  //
		  evaluateCoefficients(&velocity[eN_k_nSpace],
				       u_star,
				       //VRANS
				       porosity,
				       //
				       m_star,
				       dm_star,
				       f_star,
				       df_star);
		  evaluateCoefficients(&velocity[eN_k_nSpace],
				       u,
				       //VRANS
				       porosity,
				       //
				       m,
				       dm,
				       f,
				       df);
		  //
		  //moving mesh
		  //
		  double mesh_velocity[3];
		  mesh_velocity[0] = xt;
		  mesh_velocity[1] = yt;
		  mesh_velocity[2] = zt;
		  //std::cout<<"q mesh_velocity"<<std::endl;
		  for (int I=0;I<nSpace;I++)
		    {
		      //std::cout<<mesh_velocity[I]<<std::endl;
		      f_star[I] -= MOVING_DOMAIN*m_star*mesh_velocity[I];
		      df_star[I] -= MOVING_DOMAIN*dm_star*mesh_velocity[I];
		    }
		  //
		  //calculate time derivative at quadrature points
		  //
		  if (q_dV_last[eN_k] <= -100)
		    q_dV_last[eN_k] = dV;
		  q_dV[eN_k] = dV;
		  ck.bdf(alphaBDF,
			 q_m_betaBDF[eN_k]*q_dV_last[eN_k]/dV,//ensure prior mass integral is correct for  m_t with BDF1
			 m,
			 dm,
			 m_t,
			 dm_t);
		  if (ENTROPY_VISCOSITY==0)
		    {
		      //
		      //calculate subgrid error (strong residual and adjoint)
		      //
		      //calculate strong residual
		      pdeResidual_u_star = ck.Mass_strong(m_t) + ck.Advection_strong(df_star,grad_u_star);
		      //calculate adjoint
		      for (int i=0;i<nDOF_test_element;i++)
			{
			  // register int eN_k_i_nSpace = (eN_k*nDOF_trial_element+i)*nSpace;
			  // Lstar_u[i]  = ck.Advection_adjoint(df,&u_grad_test_dV[eN_k_i_nSpace]);
			  register int i_nSpace = i*nSpace;
			  Lstar_u[i]  = ck.Advection_adjoint(df_star,&u_grad_test_dV[i_nSpace]);
			}
		      //calculate tau and tau*Res
		      calculateSubgridError_tau(elementDiameter[eN],dm_t,df_star,cfl[eN_k],tau0);
		      calculateSubgridError_tau(Ct_sge,
						G,
						dm_t,
						df_star,
						tau1,
						cfl[eN_k]);					
		      tau = useMetrics*tau1+(1.0-useMetrics)*tau0;
		      subgridError_u = -tau*pdeResidual_u_star;
		      //
		      //calculate shock capturing diffusion
		      //
		      ck.calculateNumericalDiffusion(shockCapturingDiffusion,elementDiameter[eN],pdeResidual_u_star,grad_u_star,numDiff0);
		      ck.calculateNumericalDiffusion(shockCapturingDiffusion,sc_uref, sc_alpha,G,G_dd_G,pdeResidual_u_star,grad_u_star,numDiff1);
		      q_numDiff_u[eN_k] = useMetrics*numDiff1+(1.0-useMetrics)*numDiff0;
		    } //ENTROPY_VISCOSITY=0
		  else
		    {
		      // CALCULATE CFL //
		      calculateCFL(elementDiameter[eN],df_star,cfl[eN_k]); // TODO: ADJUST SPEED IF MESH IS MOVING
		      // ** LINEAR DIFFUSION (MQL) ** //
		      // calculate linear viscosity 
		      double h=elementDiameter[eN];
		      double vMax = std::max(std::abs(df_star[0]),std::abs(df_star[1]));
		      double linear_viscosity = cMax*h*vel_max[eN]; // Cell based
		      
		      // ** ENTROPY VISCOSITY (MQL) ** //
		      double entropy_viscosity = cE*h*h*entropy_residual[eN]/entropy_normalization_factor;
		      q_numDiff_u[eN_k] = std::min(linear_viscosity,entropy_viscosity);
		      
		      // ** ARTIFICIAL COMPRESSION (MQL) ** //
		      double n_grad_u=0.0; 
		      for (int I=0; I<nSpace; I++)
			n_grad_u += grad_u[I]*grad_u[I];
		      n_grad_u = sqrt(n_grad_u);
		      double compression_factor = fmax(1-cK*fmax(u*(1.0-u),0.)/(h*n_grad_u+1.0e-8),0.);
		      //double compression_factor = 1-cK*fmax(u*(1.0-u),0.)/(h*n_grad_u+1.0e-8);
		      q_numDiff_u[eN_k] *= compression_factor;
		      ////////////////////////////////////////
		    }
		  
		  for(int i=0;i<nDOF_test_element;i++) 
		    { 
		      //register int eN_k_i=eN_k*nDOF_test_element+i,
		      //eN_k_i_nSpace = eN_k_i*nSpace,
		      register int i_nSpace=i*nSpace;
		      elementResidual_u[i] += 
			dt*ck.Mass_weak(m_t,u_test_dV[i]) + 
			dt*ck.Advection_weak(f_star,&u_grad_test_dV[i_nSpace]) + 
			dt*SUPG*ck.SubgridError(subgridError_u,Lstar_u[i]) + 		   
			dt*ck.NumericalDiffusion(q_numDiff_u_last[eN_k],grad_u_star,&u_grad_test_dV[i_nSpace]);
		    }//i
		  //
		  //cek/ido todo, get rid of m, since u=m
		  //save momentum for time history and velocity for subgrid error
		  //save solution for other models 
		  //
		  q_u[eN_k] = u;
		  q_m[eN_k] = m;
		}
	      //
	      //load element into global residual and save element residual
	      //
	      for(int i=0;i<nDOF_test_element;i++) 
		{ 
		  register int eN_i=eN*nDOF_test_element+i;
		  globalResidual[offset_u+stride_u*u_l2g[eN_i]] += elementResidual_u[i];
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
		ebN_local = elementBoundaryLocalElementBoundariesArray[ebN*2+0],
		eN_nDOF_trial_element = eN*nDOF_trial_element;
	      register double elementResidual_u[nDOF_test_element];
	      for (int i=0;i<nDOF_test_element;i++)
		{
		  elementResidual_u[i]=0.0;
		}
	      for  (int kb=0;kb<nQuadraturePoints_elementBoundary;kb++) 
		{ 
		  register int ebNE_kb = ebNE*nQuadraturePoints_elementBoundary+kb,
		    ebNE_kb_nSpace = ebNE_kb*nSpace,
		    ebN_local_kb = ebN_local*nQuadraturePoints_elementBoundary+kb,
		    ebN_local_kb_nSpace = ebN_local_kb*nSpace;
		  register double u_ext=0.0, u_old_ext=0.0, 
		    grad_u_ext[nSpace], grad_u_old_ext[nSpace], 
		    m_ext=0.0,
		    dm_ext=0.0,
		    f_ext[nSpace],
		    df_ext[nSpace],
		    flux_ext=0.0,
		    bc_u_ext=0.0,
		    //bc_grad_u_ext[nSpace],
		    bc_m_ext=0.0,
		    bc_dm_ext=0.0,
		    bc_f_ext[nSpace],
		    bc_df_ext[nSpace],
		    jac_ext[nSpace*nSpace],
		    jacDet_ext,
		    jacInv_ext[nSpace*nSpace],
		    boundaryJac[nSpace*(nSpace-1)],
		    metricTensor[(nSpace-1)*(nSpace-1)],
		    metricTensorDetSqrt,
		    dS,
		    u_test_dS[nDOF_test_element],
		    u_grad_trial_trace[nDOF_trial_element*nSpace],
		    normal[nSpace],x_ext,y_ext,z_ext,xt_ext,yt_ext,zt_ext,integralScaling,
		    //VRANS
		    porosity_ext,
		    //
		    G[nSpace*nSpace],G_dd_G,tr_G;
		  // 
		  //calculate the solution and gradients at quadrature points 
		  // 
		  //compute information about mapping from reference element to physical element
		  ck.calculateMapping_elementBoundary(eN,
						      ebN_local,
						      kb,
						      ebN_local_kb,
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
		  ck.calculateMappingVelocity_elementBoundary(eN,
							      ebN_local,
							      kb,
							      ebN_local_kb,
							      mesh_velocity_dof,
							      mesh_l2g,
							      mesh_trial_trace_ref,
							      xt_ext,yt_ext,zt_ext,
							      normal,
							      boundaryJac,
							      metricTensor,
							      integralScaling);
		  //std::cout<<"metricTensorDetSqrt "<<metricTensorDetSqrt<<" integralScaling "<<integralScaling<<std::endl;
		  dS = ((1.0-MOVING_DOMAIN)*metricTensorDetSqrt + MOVING_DOMAIN*integralScaling)*dS_ref[kb];
		  //get the metric tensor
		  //cek todo use symmetry
		  ck.calculateG(jacInv_ext,G,G_dd_G,tr_G);
		  //compute shape and solution information
		  //shape
		  ck.gradTrialFromRef(&u_grad_trial_trace_ref[ebN_local_kb_nSpace*nDOF_trial_element],jacInv_ext,u_grad_trial_trace);
		  //precalculate test function products with integration weights
		  for (int j=0;j<nDOF_trial_element;j++)
		    {
		      u_test_dS[j] = u_test_trace_ref[ebN_local_kb*nDOF_test_element+j]*dS;
		    }
		  //solution and gradients	
		  ck.valFromDOF(u_dof,&u_l2g[eN_nDOF_trial_element],&u_trial_trace_ref[ebN_local_kb*nDOF_test_element],u_ext);
		  ck.gradFromDOF(u_dof,&u_l2g[eN_nDOF_trial_element],u_grad_trial_trace,grad_u_ext);
		  ck.valFromDOF(u_dof_old,&u_l2g[eN_nDOF_trial_element],&u_trial_trace_ref[ebN_local_kb*nDOF_test_element],u_old_ext);
		  ck.gradFromDOF(u_dof_old,&u_l2g[eN_nDOF_trial_element],u_grad_trial_trace,grad_u_old_ext);
		  //load the boundary values
		  //
		  bc_u_ext = isDOFBoundary_u[ebNE_kb]*ebqe_bc_u_ext[ebNE_kb]+(1-isDOFBoundary_u[ebNE_kb])*u_ext;
		  //VRANS
		  porosity_ext = ebqe_porosity_ext[ebNE_kb];
		  //
		  // 
		  //calculate the pde coefficients using the solution and the boundary values for the solution 
		  // 
		  evaluateCoefficients(&ebqe_velocity_ext[ebNE_kb_nSpace],
				       u_ext,
				       //VRANS
				       porosity_ext,
				       //
				       m_ext,
				       dm_ext,
				       f_ext,
				       df_ext);
		  evaluateCoefficients(&ebqe_velocity_ext[ebNE_kb_nSpace],
				       bc_u_ext,
				       //VRANS
				       porosity_ext,
				       //
				       bc_m_ext,
				       bc_dm_ext,
				       bc_f_ext,
				       bc_df_ext);    
		  //
		  //moving mesh
		  //
		  double mesh_velocity[3];
		  mesh_velocity[0] = xt_ext;
		  mesh_velocity[1] = yt_ext;
		  mesh_velocity[2] = zt_ext;
		  //std::cout<<"mesh_velocity ext"<<std::endl;
		  for (int I=0;I<nSpace;I++)
		    {
		      //std::cout<<mesh_velocity[I]<<std::endl;
		      f_ext[I] -= MOVING_DOMAIN*m_ext*mesh_velocity[I];
		      df_ext[I] -= MOVING_DOMAIN*dm_ext*mesh_velocity[I];
		      bc_f_ext[I] -= MOVING_DOMAIN*bc_m_ext*mesh_velocity[I];
		      bc_df_ext[I] -= MOVING_DOMAIN*bc_dm_ext*mesh_velocity[I];
		    }
		  // 
		  //calculate the numerical fluxes 
		  // 
		  exteriorNumericalAdvectiveFlux(isDOFBoundary_u[ebNE_kb],
						 isFluxBoundary_u[ebNE_kb],
						 normal,
						 bc_u_ext,
						 ebqe_bc_flux_u_ext[ebNE_kb],
						 u_ext,//smoothedHeaviside(eps,ebqe_phi[ebNE_kb]),//cek hack
						 df_ext,//VRANS includes porosity
						 flux_ext);
		  ebqe_flux[ebNE_kb] = flux_ext;
		  //save for other models? cek need to be consistent with numerical flux
		  if(flux_ext >=0.0)
		    ebqe_u[ebNE_kb] = u_ext;
		  else
		    ebqe_u[ebNE_kb] = bc_u_ext;
		  //
		  //update residuals
		  //
		  for (int i=0;i<nDOF_test_element;i++)
		    {
		      //int ebNE_kb_i = ebNE_kb*nDOF_test_element+i;
		      elementResidual_u[i] += ck.ExteriorElementBoundaryFlux(flux_ext,u_test_dS[i]);
		    }//i
		}//kb
	      //
	      //update the element and global residual storage
	      //
	      for (int i=0;i<nDOF_test_element;i++)
		{
		  int eN_i = eN*nDOF_test_element+i;
		  
		  globalResidual[offset_u+stride_u*u_l2g[eN_i]] += elementResidual_u[i];
		}//i
	    }//ebNE
	}
    }
    
    void calculateJacobian(//element
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
			   //element boundary
			   double* mesh_trial_trace_ref,
			   double* mesh_grad_trial_trace_ref,
			   double* dS_ref,
			   double* u_trial_trace_ref,
			   double* u_grad_trial_trace_ref,
			   double* u_test_trace_ref,
			   double* u_grad_test_trace_ref,
			   double* normal_ref,
			   double* boundaryJac_ref,
			   //physics
			   int nElements_global,
			   double useMetrics, 
			   double alphaBDF,
			   int lag_shockCapturing,/*mwf not used yet*/
			   double shockCapturingDiffusion,
			   //VRANS
			   const double* q_porosity,
			   //
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
			   //VRANS
			   const double* ebqe_porosity_ext,
			   //
			   int* isDOFBoundary_u,
			   double* ebqe_bc_u_ext,
			   int* isFluxBoundary_u,
			   double* ebqe_bc_flux_u_ext,
			   int* csrColumnOffsets_eb_u_u,
			   int IMPLICIT, 
			   int SUPG)
    {
      double dt = 1./alphaBDF; // valid just for forward/backward euler
      //std::cout<<"ndjaco  address "<<q_numDiff_u_last<<std::endl;
      double Ct_sge = 4.0;
      //
      //loop over elements to compute volume integrals and load them into the element Jacobians and global Jacobian
      //
      for(int eN=0;eN<nElements_global;eN++)
	{
	  register double  elementJacobian_u_u[nDOF_test_element][nDOF_trial_element];
	  for (int i=0;i<nDOF_test_element;i++)
	    for (int j=0;j<nDOF_trial_element;j++)
	      {
		elementJacobian_u_u[i][j]=0.0;
	      }
	  for  (int k=0;k<nQuadraturePoints_element;k++)
	    {
	      int eN_k = eN*nQuadraturePoints_element+k, //index to a scalar at a quadrature point
		eN_k_nSpace = eN_k*nSpace,
		eN_nDOF_trial_element = eN*nDOF_trial_element; //index to a vector at a quadrature point

	      //declare local storage
	      register double u=0.0,
		grad_u[nSpace],
		m=0.0,dm=0.0,
		f[nSpace],df[nSpace],
		m_t=0.0,dm_t=0.0,
		dpdeResidual_u_u[nDOF_trial_element],
		Lstar_u[nDOF_test_element],
		dsubgridError_u_u[nDOF_trial_element],
		tau=0.0,tau0=0.0,tau1=0.0,
		jac[nSpace*nSpace],
		jacDet,
		jacInv[nSpace*nSpace],
		u_grad_trial[nDOF_trial_element*nSpace],
		dV,
		u_test_dV[nDOF_test_element],
		u_grad_test_dV[nDOF_test_element*nSpace],
		x,y,z,xt,yt,zt,
		//VRANS
		porosity,
		//
		G[nSpace*nSpace],G_dd_G,tr_G;
	      //
	      //calculate solution and gradients at quadrature points
	      //
	      // u=0.0;
	      // for (int I=0;I<nSpace;I++)
	      //   {
	      //     grad_u[I]=0.0;
	      //   }
	      // for (int j=0;j<nDOF_trial_element;j++)
	      //   {
	      //     int eN_j=eN*nDOF_trial_element+j;
	      //     int eN_k_j=eN_k*nDOF_trial_element+j;
	      //     int eN_k_j_nSpace = eN_k_j*nSpace;
              
	      //     u += valFromDOF_c(u_dof[u_l2g[eN_j]],u_trial[eN_k_j]);
	      //     for (int I=0;I<nSpace;I++)
	      //       {
	      //         grad_u[I] += gradFromDOF_c(u_dof[u_l2g[eN_j]],u_grad_trial[eN_k_j_nSpace+I]);
	      //       }
	      //   }
	      //get jacobian, etc for mapping reference element
	      ck.calculateMapping_element(eN,
					  k,
					  mesh_dof,
					  mesh_l2g,
					  mesh_trial_ref,
					  mesh_grad_trial_ref,
					  jac,
					  jacDet,
					  jacInv,
					  x,y,z);
	      ck.calculateMappingVelocity_element(eN,
						  k,
						  mesh_velocity_dof,
						  mesh_l2g,
						  mesh_trial_ref,
						  xt,yt,zt);
	      //get the physical integration weight
	      dV = fabs(jacDet)*dV_ref[k];
	      ck.calculateG(jacInv,G,G_dd_G,tr_G);
	      //get the trial function gradients
	      ck.gradTrialFromRef(&u_grad_trial_ref[k*nDOF_trial_element*nSpace],jacInv,u_grad_trial);
	      //get the solution 	
	      ck.valFromDOF(u_dof,&u_l2g[eN_nDOF_trial_element],&u_trial_ref[k*nDOF_trial_element],u);
	      //get the solution gradients
	      ck.gradFromDOF(u_dof,&u_l2g[eN_nDOF_trial_element],u_grad_trial,grad_u);
	      //precalculate test function products with integration weights
	      for (int j=0;j<nDOF_trial_element;j++)
		{
		  u_test_dV[j] = u_test_ref[k*nDOF_trial_element+j]*dV;
		  for (int I=0;I<nSpace;I++)
		    {
		      u_grad_test_dV[j*nSpace+I]   = u_grad_trial[j*nSpace+I]*dV;//cek warning won't work for Petrov-Galerkin
		    }
		}
	      //VRANS
	      porosity = q_porosity[eN_k];
	      //
	      //
	      //calculate pde coefficients and derivatives at quadrature points
	      //
	      evaluateCoefficients(&velocity[eN_k_nSpace],
				   u,
				   //VRANS
				   porosity,
				   //
				   m,
				   dm,
				   f,
				   df);
	      //
	      //moving mesh
	      //
	      double mesh_velocity[3];
	      mesh_velocity[0] = xt;
	      mesh_velocity[1] = yt;
	      mesh_velocity[2] = zt;
	      //std::cout<<"qj mesh_velocity"<<std::endl;
	      for(int I=0;I<nSpace;I++)
		{
		  //std::cout<<mesh_velocity[I]<<std::endl;
		  f[I] -= MOVING_DOMAIN*m*mesh_velocity[I];
		  df[I] -= MOVING_DOMAIN*dm*mesh_velocity[I];
		}
	      //
	      //calculate time derivatives
	      //
	      ck.bdf(alphaBDF,
		     q_m_betaBDF[eN_k],//since m_t isn't used, we don't have to correct mass
		     m,
		     dm,
		     m_t,
		     dm_t);
	      //
	      //calculate subgrid error contribution to the Jacobian (strong residual, adjoint, jacobian of strong residual)
	      //
	      //calculate the adjoint times the test functions
	      for (int i=0;i<nDOF_test_element;i++)
		{
		  // int eN_k_i_nSpace = (eN_k*nDOF_trial_element+i)*nSpace;
		  // Lstar_u[i]=ck.Advection_adjoint(df,&u_grad_test_dV[eN_k_i_nSpace]);	      
		  register int i_nSpace = i*nSpace;
		  Lstar_u[i]=ck.Advection_adjoint(df,&u_grad_test_dV[i_nSpace]);	      
		}
	      //calculate the Jacobian of strong residual
	      for (int j=0;j<nDOF_trial_element;j++)
		{
		  //int eN_k_j=eN_k*nDOF_trial_element+j;
		  //int eN_k_j_nSpace = eN_k_j*nSpace;
		  int j_nSpace = j*nSpace;
		  dpdeResidual_u_u[j]= ck.MassJacobian_strong(dm_t,u_trial_ref[k*nDOF_trial_element+j]) +
		    ck.AdvectionJacobian_strong(df,&u_grad_trial[j_nSpace]);
		}
	      //tau and tau*Res
	      calculateSubgridError_tau(elementDiameter[eN],
					dm_t,
					df,
					cfl[eN_k],
					tau0);
  
              calculateSubgridError_tau(Ct_sge,
                                        G,
					dm_t,
					df,
					tau1,
				        cfl[eN_k]);
              tau = useMetrics*tau1+(1.0-useMetrics)*tau0;

	      for(int j=0;j<nDOF_trial_element;j++)
		dsubgridError_u_u[j] = -tau*dpdeResidual_u_u[j];
	      double h=elementDiameter[eN];
	      for(int i=0;i<nDOF_test_element;i++)
		{
		  //int eN_k_i=eN_k*nDOF_test_element+i;
		  //int eN_k_i_nSpace=eN_k_i*nSpace;
		  for(int j=0;j<nDOF_trial_element;j++) 
		    {
		      if (LUMPED_MASS_MATRIX==1)
			{
			  if (i==j)
			    elementJacobian_u_u[i][j] += u_test_dV[i];
			}
		      else
			{
			  //int eN_k_j=eN_k*nDOF_trial_element+j;
			  //int eN_k_j_nSpace = eN_k_j*nSpace;
			  int j_nSpace = j*nSpace;
			  int i_nSpace = i*nSpace;
			  //std::cout<<"jac "<<'\t'<<q_numDiff_u_last[eN_k]<<'\t'<<dm_t<<'\t'<<df[0]<<df[1]<<'\t'<<dsubgridError_u_u[j]<<std::endl;
			  elementJacobian_u_u[i][j] += 
			    dt*ck.MassJacobian_weak(dm_t,u_trial_ref[k*nDOF_trial_element+j],u_test_dV[i]) + 
			    dt*IMPLICIT*ck.AdvectionJacobian_weak(df,u_trial_ref[k*nDOF_trial_element+j],&u_grad_test_dV[i_nSpace]) +
			    dt*SUPG*IMPLICIT*ck.SubgridErrorJacobian(dsubgridError_u_u[j],Lstar_u[i]) +
			    dt*IMPLICIT*ck.NumericalDiffusionJacobian(q_numDiff_u_last[eN_k],&u_grad_trial[j_nSpace],&u_grad_test_dV[i_nSpace]); //implicit
			}
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
		  globalJacobian[csrRowIndeces_u_u[eN_i] + csrColumnOffsets_u_u[eN_i_j]] += elementJacobian_u_u[i][j];
		}//j
	    }//i
	}//elements
      //
      //loop over exterior element boundaries to compute the surface integrals and load them into the global Jacobian
      //
      for (int ebNE = 0; ebNE < nExteriorElementBoundaries_global; ebNE++) 
	{ 
	  register int ebN = exteriorElementBoundariesArray[ebNE]; 
	  register int eN  = elementBoundaryElementsArray[ebN*2+0],
            ebN_local = elementBoundaryLocalElementBoundariesArray[ebN*2+0],
            eN_nDOF_trial_element = eN*nDOF_trial_element;
	  for  (int kb=0;kb<nQuadraturePoints_elementBoundary;kb++) 
	    { 
	      register int ebNE_kb = ebNE*nQuadraturePoints_elementBoundary+kb,
		ebNE_kb_nSpace = ebNE_kb*nSpace,
		ebN_local_kb = ebN_local*nQuadraturePoints_elementBoundary+kb,
		ebN_local_kb_nSpace = ebN_local_kb*nSpace;

	      register double u_ext=0.0,
		grad_u_ext[nSpace],
		m_ext=0.0,
		dm_ext=0.0,
		f_ext[nSpace],
		df_ext[nSpace],
		dflux_u_u_ext=0.0,
		bc_u_ext=0.0,
		//bc_grad_u_ext[nSpace],
		bc_m_ext=0.0,
		bc_dm_ext=0.0,
		bc_f_ext[nSpace],
		bc_df_ext[nSpace],
		fluxJacobian_u_u[nDOF_trial_element],
		jac_ext[nSpace*nSpace],
		jacDet_ext,
		jacInv_ext[nSpace*nSpace],
		boundaryJac[nSpace*(nSpace-1)],
		metricTensor[(nSpace-1)*(nSpace-1)],
		metricTensorDetSqrt,
		dS,
		u_test_dS[nDOF_test_element],
		u_grad_trial_trace[nDOF_trial_element*nSpace],
		normal[nSpace],x_ext,y_ext,z_ext,xt_ext,yt_ext,zt_ext,integralScaling,
		//VRANS
		porosity_ext,
		//
		G[nSpace*nSpace],G_dd_G,tr_G;
	      // 
	      //calculate the solution and gradients at quadrature points 
	      // 
	      // u_ext=0.0;
	      // for (int I=0;I<nSpace;I++)
	      //   {
	      //     grad_u_ext[I] = 0.0;
	      //     bc_grad_u_ext[I] = 0.0;
	      //   }
	      // for (int j=0;j<nDOF_trial_element;j++) 
	      //   { 
	      //     register int eN_j = eN*nDOF_trial_element+j,
	      //       ebNE_kb_j = ebNE_kb*nDOF_trial_element+j,
	      //       ebNE_kb_j_nSpace= ebNE_kb_j*nSpace;
	      //     u_ext += valFromDOF_c(u_dof[u_l2g[eN_j]],u_trial_ext[ebNE_kb_j]); 
	                     
	      //     for (int I=0;I<nSpace;I++)
	      //       {
	      //         grad_u_ext[I] += gradFromDOF_c(u_dof[u_l2g[eN_j]],u_grad_trial_ext[ebNE_kb_j_nSpace+I]); 
	      //       } 
	      //   }
	      ck.calculateMapping_elementBoundary(eN,
						  ebN_local,
						  kb,
						  ebN_local_kb,
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
	      ck.calculateMappingVelocity_elementBoundary(eN,
							  ebN_local,
							  kb,
							  ebN_local_kb,
							  mesh_velocity_dof,
							  mesh_l2g,
							  mesh_trial_trace_ref,
							  xt_ext,yt_ext,zt_ext,
							  normal,
							  boundaryJac,
							  metricTensor,
							  integralScaling);
	      //std::cout<<"J mtsqrdet "<<metricTensorDetSqrt<<" integralScaling "<<integralScaling<<std::endl;
	      dS = ((1.0-MOVING_DOMAIN)*metricTensorDetSqrt + MOVING_DOMAIN*integralScaling)*dS_ref[kb];
	      //dS = metricTensorDetSqrt*dS_ref[kb];
	      ck.calculateG(jacInv_ext,G,G_dd_G,tr_G);
	      //compute shape and solution information
	      //shape
	      ck.gradTrialFromRef(&u_grad_trial_trace_ref[ebN_local_kb_nSpace*nDOF_trial_element],jacInv_ext,u_grad_trial_trace);
	      //solution and gradients	
	      ck.valFromDOF(u_dof,&u_l2g[eN_nDOF_trial_element],&u_trial_trace_ref[ebN_local_kb*nDOF_test_element],u_ext);
	      ck.gradFromDOF(u_dof,&u_l2g[eN_nDOF_trial_element],u_grad_trial_trace,grad_u_ext);
	      //precalculate test function products with integration weights
	      for (int j=0;j<nDOF_trial_element;j++)
		{
		  u_test_dS[j] = u_test_trace_ref[ebN_local_kb*nDOF_test_element+j]*dS;
		}
	      //
	      //load the boundary values
	      //
	      bc_u_ext = isDOFBoundary_u[ebNE_kb]*ebqe_bc_u_ext[ebNE_kb]+(1-isDOFBoundary_u[ebNE_kb])*u_ext;
	      //VRANS
	      porosity_ext = ebqe_porosity_ext[ebNE_kb];
	      //
	      // 
	      //calculate the internal and external trace of the pde coefficients 
	      // 
	      evaluateCoefficients(&ebqe_velocity_ext[ebNE_kb_nSpace],
				   u_ext,
				   //VRANS
				   porosity_ext,
				   //
				   m_ext,
				   dm_ext,
				   f_ext,
				   df_ext);
	      evaluateCoefficients(&ebqe_velocity_ext[ebNE_kb_nSpace],
				   bc_u_ext,
				   //VRANS
				   porosity_ext,
				   //
				   bc_m_ext,
				   bc_dm_ext,
				   bc_f_ext,
				   bc_df_ext);
	      //
	      //moving domain
	      //
	      double mesh_velocity[3];
	      mesh_velocity[0] = xt_ext;
	      mesh_velocity[1] = yt_ext;
	      mesh_velocity[2] = zt_ext;
	      //std::cout<<"ext J mesh_velocity"<<std::endl;
	      for (int I=0;I<nSpace;I++)
		{
		  //std::cout<<mesh_velocity[I]<<std::endl;
		  f_ext[I] -= MOVING_DOMAIN*m_ext*mesh_velocity[I];
		  df_ext[I] -= MOVING_DOMAIN*dm_ext*mesh_velocity[I];
		  bc_f_ext[I] -= MOVING_DOMAIN*bc_m_ext*mesh_velocity[I];
		  bc_df_ext[I] -= MOVING_DOMAIN*bc_dm_ext*mesh_velocity[I];
		}
	      // 
	      //calculate the numerical fluxes 
	      // 
	      exteriorNumericalAdvectiveFluxDerivative(isDOFBoundary_u[ebNE_kb],
						       isFluxBoundary_u[ebNE_kb],
						       normal,
						       df_ext,//VRANS holds porosity
						       dflux_u_u_ext);
	      //
	      //calculate the flux jacobian
	      //
	      for (int j=0;j<nDOF_trial_element;j++)
		{
		  //register int ebNE_kb_j = ebNE_kb*nDOF_trial_element+j;
		  register int ebN_local_kb_j=ebN_local_kb*nDOF_trial_element+j;
	      
		  fluxJacobian_u_u[j]=ck.ExteriorNumericalAdvectiveFluxJacobian(dflux_u_u_ext,u_trial_trace_ref[ebN_local_kb_j]);
		}//j
	      //
	      //update the global Jacobian from the flux Jacobian
	      //
	      for (int i=0;i<nDOF_test_element;i++)
		{
		  register int eN_i = eN*nDOF_test_element+i;
		  //register int ebNE_kb_i = ebNE_kb*nDOF_test_element+i;
		  for (int j=0;j<nDOF_trial_element;j++)
		    {
		      register int ebN_i_j = ebN*4*nDOF_test_X_trial_element + i*nDOF_trial_element + j;

		      globalJacobian[csrRowIndeces_u_u[eN_i] + csrColumnOffsets_eb_u_u[ebN_i_j]] += fluxJacobian_u_u[j]*u_test_dS[i];
		    }//j
		}//i
	    }//kb
	}//ebNE
    }//computeJacobian
  };//VOF

  inline VOF_base* newVOF(int nSpaceIn,
				int nQuadraturePoints_elementIn,
				int nDOF_mesh_trial_elementIn,
				int nDOF_trial_elementIn,
				int nDOF_test_elementIn,
				int nQuadraturePoints_elementBoundaryIn,
				int CompKernelFlag)
  {
    if (nSpaceIn == 2)
      return proteus::chooseAndAllocateDiscretization2D<VOF_base,VOF,CompKernel>(nSpaceIn,
										 nQuadraturePoints_elementIn,
										 nDOF_mesh_trial_elementIn,
										 nDOF_trial_elementIn,
										 nDOF_test_elementIn,
										 nQuadraturePoints_elementBoundaryIn,
										 CompKernelFlag);
    else
      return proteus::chooseAndAllocateDiscretization<VOF_base,VOF,CompKernel>(nSpaceIn,
									       nQuadraturePoints_elementIn,
									       nDOF_mesh_trial_elementIn,
									       nDOF_trial_elementIn,
									       nDOF_test_elementIn,
									       nQuadraturePoints_elementBoundaryIn,
									       CompKernelFlag);
  }
}//proteus
#endif

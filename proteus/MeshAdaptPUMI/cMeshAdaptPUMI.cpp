#include <gmi.h>
#include <gmi_mesh.h>
#include <gmi_null.h>
#include <ma.h>
#include <maShape.h>
#include <apfMDS.h>
#include <PCU.h>
#include <apf.h>
#include<lionPrint.h>

#include <iostream>
#include <fstream>

#include "MeshAdaptPUMI.h"
#include <sam.h>
#include <samSz.h>

#include <apfShape.h>

#include <apfOmega_h.h>
#include <Omega_h_library.hpp>
#include <Omega_h_mesh.hpp>
#include <Omega_h_file.hpp>
#include "Omega_h_adapt.hpp"
#include <Omega_h_metric.hpp>
//#include <Omega_h_build.hpp>
#include "Omega_h_for.hpp"

extern double dt_err;

#ifdef PROTEUS_USE_SIMMETRIX
//PROTEUS_USE_SIMMETRIX is a compiler macro that indicates whether Simmetrix libraries are used
//This is defined in proteus/config/default.py and is contingent on the existence of a SIM_INCLUDE_DIR path
  #include <gmi_sim.h>
  #include <SimUtil.h>
  #include <SimModel.h>
  #include <MeshSim.h>
  #include <SimMeshTools.h>
  #define FACE 2
  pAManager SModel_attManager(pModel model);
#endif

/** 
 * \file cMeshAdaptPUMI.cpp
 * \ingroup MeshAdaptPUMI 
 @{ 
*/

MeshAdaptPUMIDrvr::MeshAdaptPUMIDrvr(double Hmax, double Hmin, double HPhi,int AdaptMesh, int NumIter, int NumAdaptSteps,const char* sfConfig, const char* maType,const char* logType, double targetError, double targetElementCount,int reconstructedFlag,double maxAspectRatio, double gradingFact)
/**
 * MeshAdaptPUMIDrvr is the highest level class that handles the interface between Proteus and the PUMI libraries
 * See MeshAdaptPUMI.h for the list of class variables/functions/objects
 * This is the constructor for the class
*/
{
  m = 0;
  PCU_Comm_Init();
  PCU_Protect();

#ifdef PROTEUS_USE_SIMMETRIX
  Sim_readLicenseFile(0);
  SimModel_start();
  gmi_register_sim();
#endif
  hmin=Hmin; hmax=Hmax; hPhi=HPhi;
  numIter=NumIter;
  adaptMesh = AdaptMesh;
  nAdapt=0;
  nTriggers=0;
  numAdaptSteps = NumAdaptSteps;
  nEstimate=0;
  if(PCU_Comm_Self()==0)
     printf("MeshAdapt: Setting hmax=%lf, hmin=%lf, numIters(meshadapt)=%d\n",
       hmax, hmin, numIter);
  global[0] = global[1] = global[2] = global[3] = 0;
  local[0] = local[1] = local[2] = local[3] = 0;
  size_iso = 0;
  size_scale = 0;
  size_frame = 0;
  err_reg = 0;
  vmsErrH1 = 0;
  errRho_reg = 0;
  errRel_reg = 0;
  error_reference = 0;
  gmi_register_mesh();
  gmi_register_null();
  approximation_order = 2;
  integration_order = 3;
  total_error = 0.0;
  errRho_max = 0.0;
  rel_err_total = 0.0;
  exteriorGlobaltoLocalElementBoundariesArray = NULL;
  size_field_config = sfConfig;
  modelFileName = NULL; 
  adapt_type_config = maType;
  logging_config = logType;
  has_gBC = false;
  target_error = targetError;
  target_element_count = targetElementCount;
  domainVolume = 0.0;
  THRESHOLD = 0.0;
  isReconstructed = reconstructedFlag;
  initialReconstructed = 0;
  maxAspect = maxAspectRatio;
  gradingFactor = gradingFact;
}

MeshAdaptPUMIDrvr::~MeshAdaptPUMIDrvr()
/**
 * Destructor for MeshAdaptPUMIDrvr
 */
{
  freeField(err_reg);
  freeField(vmsErrH1);
  freeField(errRho_reg);
  freeField(errRel_reg);
  freeField(error_reference);
  freeField(size_iso);
  freeField(size_scale);
  freeField(size_frame);
  if(isReconstructed){
    free(modelVertexMaterial);
    free(modelBoundaryMaterial);
    free(modelRegionMaterial);
    //m->destroyNative();
    //gmi_destroy(m->getModel());
    //apf::destroyMesh(m);
  }
  PCU_Comm_Free();
#ifdef PROTEUS_USE_SIMMETRIX
  SimModel_stop();
  Sim_unregisterAllKeys();
#endif
}


static bool ends_with(std::string const& str, std::string const& ext)
{
  return str.size() >= ext.size() &&
         str.compare(str.size() - ext.size(), ext.size(), ext) == 0;
}
/**
 * @brief Load the mesh and model for SCOREC libraries
 *
 * The default filetypes are .dmg (model) and .smb (mesh), but can support GMSH meshes (.msh) and Simmetrix models (.smd) and meshes (.sms)
 * GMSH models are not used and .null filenames and passed instead.
 * Each of the the GMSH and Simmetrix filetypes can be converted into a SCOREC filetype via tools in scorec
 * Diffusive flux boundary conditions are supported with Simmetrix models and can be passed into the error estimator 
 */

int MeshAdaptPUMIDrvr::loadModelAndMesh(const char* modelFile, const char* meshFile)
{
  comm_size = PCU_Comm_Peers();
  comm_rank = PCU_Comm_Self();
  if (ends_with(meshFile, ".msh")){
    m = apf::loadMdsFromGmsh(gmi_load(modelFile), meshFile);
    std::cout<<"Boundary Condition functionality has not been built in for gmsh yet.\n";
  }
  else if (ends_with(modelFile,".smd")){
    m = apf::loadMdsMesh(modelFile, meshFile);
    modelFileName=(char *) malloc(sizeof(char) * strlen(modelFile));
    strcpy(modelFileName,modelFile);
    getSimmetrixBC();
  }
  else{
    m = apf::loadMdsMesh(modelFile, meshFile);
  }

  m->verify();
  return 0;
}


int MeshAdaptPUMIDrvr::getSimmetrixBC()
/**
 * @brief Function used to read in diffusive flux BC from Simmetrix Model.
 *
 * Simmetrix BCs set via the GUI are read-in through the Simmetrix API.
 * The values are stored as apf tags on the mesh.
 * BCs are not currently supported with any other type of model.
 */
{

#ifdef PROTEUS_USE_SIMMETRIX
  pGModel model = 0;
  model=GM_load(modelFileName,NULL,NULL);

  pAManager attmngr = SModel_attManager(model);
  pACase acase = AMAN_findCaseByType(attmngr, "problem definition");
  if (acase){
     if(comm_rank==0)std::cout<<"Found case, setting the model"<<std::endl;
     AttCase_setModel(acase,model);
     has_gBC=true;
  AttCase_associate(acase,NULL);

  pGFace gFace;
  GFIter gfIter = GM_faceIter(model);
  pAttribute Att[GM_numFaces(model)];
  int attMap[GM_numFaces(model)];
  int nF=0;
  
  char strAtt[2][25] = {"traction vector","comp3"};
  int modelEntTag;

  while(gFace = GFIter_next(gfIter))
  {
    if(GEN_attrib((pGEntity)gFace,strAtt[0]))
    { 
      modelEntTag=GEN_tag((pGEntity)gFace);
      Att[nF]=GEN_attrib((pGEntity)gFace,strAtt[0]);
      attMap[nF] = modelEntTag;
      nF++;
    }
  }
  GFIter_delete(gfIter);
  
  apf::MeshIterator* fIter = m->begin(FACE);
  apf::MeshEntity* fEnt;
  apf::Vector3 evalPt;
  int numqpt=0;
  const int nsd = 3;
  const int bcFlag[nsd+1] = {0,1,1,1};

  //assign a label to the BC type tag
  char label[9],labelflux[4][9],type_flag;
  
  sprintf(label,"BCtype");
  BCtag = m->createIntTag(label,4);
  for(int idx=0;idx<4;idx++)
  {
    if(idx == 0) sprintf(&type_flag,"p");
    else if(idx == 1) sprintf(&type_flag,"u");
    else if(idx == 2) sprintf(&type_flag,"v");
    else if(idx == 3) sprintf(&type_flag,"w");
    if(idx>0) sprintf(labelflux[idx],"%c_flux",type_flag);
  }

  while(fEnt = m->iterate(fIter))
  {
    apf::ModelEntity* me=m->toModel(fEnt);
    modelEntTag = m->getModelTag(me);
    apf::ModelEntity* boundary_face = m->findModelEntity(FACE,modelEntTag);
    if(numqpt==0)
    {
      apf::MeshElement* testElem = apf::createMeshElement(m,fEnt);
      numqpt = apf::countIntPoints(testElem,integration_order);
      for(int idx=1;idx<nsd+1;idx++)
        fluxtag[idx]= m->createDoubleTag(labelflux[idx],numqpt);
      apf::destroyMeshElement(testElem);
    }
    if(me==boundary_face)
    {
      for(int i=0;i<nF;i++)
      {
        if(attMap[i]==modelEntTag)
        {
          apf::MeshElement* testElem = apf::createMeshElement(m,fEnt);
          double data[nsd+1][numqpt];
          for(int k=0; k<numqpt;k++)
          {
            apf::getIntPoint(testElem,integration_order,k,evalPt);
            apf::Vector3 evalPtGlobal;
            apf::mapLocalToGlobal(testElem,evalPt,evalPtGlobal);
            double evalPtSim[nsd];
            evalPtGlobal.toArray(evalPtSim);
            for(int j=0;j<nsd;j++)
              data[j+1][k]=AttributeTensor1_evalDS((pAttributeTensor1)Att[i], j,evalPtSim);
          }
          m->setIntTag(fEnt,BCtag,&(bcFlag[0]));
          for(int idx=1;idx<nsd+1;idx++)
          {
            m->setDoubleTag(fEnt,fluxtag[idx],data[idx]); //set the quadrature points
          }
          apf::destroyMeshElement(testElem);
          break;
        } //end if on model
        else
        {
          int dummy[4] = {0,0,0,0};
          m->setIntTag(fEnt,BCtag,&(dummy[0]));
        }
      }//end loop over attributes
      if(nF==0)
      {
          int dummy[4] = {0,0,0,0};
          m->setIntTag(fEnt,BCtag,&(dummy[0]));
      }
    } 
  }//end while
  m->end(fIter);
  AMAN_release( attmngr );
  } else {
      if(comm_rank==0)
        std::cout<<"Case not found, no BCs?\n"<<std::endl;
      //exit(1);
  }

  if(comm_rank==0)std::cout<<"Finished reading and storing diffusive flux BCs\n"; 
#endif
  return 0;
} 

static int countTotal(apf::Mesh* m, int dim)
{
  int total = apf::countOwned(m, dim);
  PCU_Add_Ints(&total, 1);
  return total;
}

#include "PyEmbeddedFunctions.h"
int MeshAdaptPUMIDrvr::willErrorAdapt() 
/**
 * @brief Looks at the estimated error and determines if mesh adaptation is necessary.
 *
 * Function used to define whether a mesh needs to be adapted based on the error estimator
 * The return value is a flag indicating whether the mesh will not (0) or will be (1) adapted 
 * The THRESHOLD will be set to the error estimate after the wind-up step, but is currently 0
 * Assertion is set to ensure that all ranks in a parallel execution will enter the adapt stage
 */
{
  int adaptFlag=0;
  int assertFlag;

  //get current size field
  apf::Field* currentField;

/*
  if(!size_iso) //if no previous size field
  {
    currentField = samSz::isoSize(m);
  }
  else //otherwise use previous size field, remember to reflect this in interfaceAdapt or collapse to a single function
  {
    currentField  = apf::createFieldOn(m, "currentField", apf::SCALAR);
    apf::copyData(currentField,size_iso);
  }
*/

  //currentField  = apf::createFieldOn(m, "currentField", apf::SCALAR);
  //apf::copyData(currentField,size_iso);
  currentField = samSz::isoSize(m);

  //get error-based size field
  getERMSizeField(total_error);
  apf::Field* errorField = sizeFieldList.front();
  sizeFieldList.pop(); //remove this size field from the queue

  apf::Field *errorTriggered = apf::createLagrangeField(m, "errorTriggered", apf::SCALAR, 1);
  //determine if desired mesh is contained in current mesh
  apf::MeshEntity* ent;
  apf::MeshIterator* it = m->begin(0);
  while( (ent = m->iterate(it)) )
  {
    double h_current = apf::getScalar(currentField,ent,0);
    double h_needed = apf::getScalar(errorField,ent,0);
    if(h_current>h_needed*1.5){
      adaptFlag+=1;        
      apf::setScalar(errorTriggered,ent,0,h_current/h_needed*1.0);
      //std::cout<<"What is the ent? "<<localNumber(ent)<<std::endl;
      //std::exit(1);
      //break;
    }  
    else
      apf::setScalar(errorTriggered,ent,0,-1);
  }//end while
  m->end(it);

  //modify error field to be the ratio
  it = m->begin(0);
  while( (ent = m->iterate(it)) )
  {
    double h_current = apf::getScalar(currentField,ent,0);
    double h_needed = apf::getScalar(errorField,ent,0);
    apf::setScalar(errorField,ent,0,h_current/h_needed*1.0);
  }//end while
  m->end(it);


  assertFlag = adaptFlag;
  PCU_Add_Ints(&assertFlag,1);
  //assert(assertFlag ==0 || assertFlag == PCU_Proc_Peers());

  if(assertFlag>0)
  {
    double totalNodes = countTotal(m,0);
    double triggeredPercentage = assertFlag*100.0/totalNodes;
    char buffer[50];
    sprintf(buffer,"Need to error adapt %f%%",triggeredPercentage);
    logEvent(buffer,3);

/*
    if(nTriggers%10==0)
    {
        char namebuffer[50];
        sprintf(namebuffer,"needErrorAdapt_%i",nTriggers);
        apf::writeVtkFiles(namebuffer, m);
    }
*/
    nTriggers++;
  }
  apf::destroyField(currentField);
  //apf::destroyField(errorField);
  apf::destroyField(errorTriggered);

  //return assertFlag;
  return 0;
}


int MeshAdaptPUMIDrvr::willErrorAdapt_reference() 
{
  int adaptFlag=0;
  int assertFlag;

  getERMSizeField(total_error);
  sizeFieldList.pop(); //remove this size field from the queue

  //if(m->findField("errorTriggered"))
  //  apf::destroyField(m->findField("errorTriggered"));
  apf::Field* errorTriggered;
  if(m->findField("errorTriggered"))
    errorTriggered = m->findField("errorTriggered");
  else
    errorTriggered = apf::createField(m,"errorTriggered",apf::SCALAR,apf::getVoronoiShape(m->getDimension(),1));
 
  apf::Field* error_current = m->findField("VMSH1");
  apf::Field* error_reference=NULL;


  if(m->findField("errorRate"))
    apf::destroyField(m->findField("errorRate"));
  apf::Field* errorRateField = apf::createField(m,"errorRate",apf::SCALAR,apf::getVoronoiShape(m->getDimension(),1));

  //need to set the error trigger field to 0
  apf::MeshEntity* ent;
  apf::MeshIterator* it;

  if(nTriggers == 0)
  {
    it = m->begin(m->getDimension());
    while( (ent = m->iterate(it) ) )
    {
        apf::setScalar(errorTriggered,ent,0,-1.0);
        apf::setScalar(errorRateField,ent,0,0.0);
    }
    m->end(it);
    logEvent("SET ERROR TRIGGERED!",4);
  }


  //need to have size field here just to define it

  //set the error reference field to be the current error field for next time otherwise, retrieve the error_reference field
  if(!m->findField("error_reference"))
  {  
    T_reference = T_current;
    error_reference = apf::createField(m,"error_reference",apf::SCALAR,apf::getVoronoiShape(nsd,1));
    apf::copyData(error_reference,error_current);
    logEvent("SUCCESSFULLY COPIED!",4);

    return 0;
  }
  else
  {
    error_reference = m->findField("error_reference");
  }

  double dt_step = dt_err; //global variable imported from VMS

  if(m->findField("sizeRatio"))
    apf::destroyField(m->findField("sizeRatio"));
  apf::Field* sizeRatioField = apf::createField(m,"sizeRatio",apf::SCALAR,apf::getVoronoiShape(m->getDimension(),1));

  it = m->begin(m->getDimension());

  while( (ent = m->iterate(it) ) )
  {   
    double err_local_current = apf::getScalar(error_current,ent,0);
    double err_local_ref = apf::getScalar(error_reference,ent,0);
    double errorRate = (err_local_current-err_local_ref)/(T_current-T_reference);
    apf::setScalar(errorRateField,ent,0,errorRate);
    double err_predict = errorRate*delta_T_next + err_local_current;

    //double sizeRatio_local = apf::getScalar(m->findField("sizeRatio"),ent,0);
    //if(errorRate > 0 && (err_predict > target_error) && sizeRatio_local>2.0 && nTriggers>=5)
    //double sizeRatio_local_predict = pow(err_predict/target_error,2.0/(2*1+2));

    double h_old;
    double h_new;
    apf::MeshElement* element = apf::createMeshElement(m, ent);

    if (m->getDimension() == 2)
      h_old = apf::computeLargestHeightInTri(m,ent);
    else
      h_old = apf::computeShortestHeightInTet(m,ent);
    h_new = h_old * pow((target_error / err_predict),2.0/(2.0*(1.0)+nsd));
    //clamp h_new and then compare against h_old
    if(h_new > hmax)
        h_new = hmax;
    if(h_new < hmin)
        h_new = hmin;
    double sizeRatio_local_predict = h_old/h_new;
    apf::setScalar(sizeRatioField,ent,0,sizeRatio_local_predict);
    apf::destroyMeshElement(element);

    //apf::setScalar(sizeRatioField,ent,0,size_ratio_local_predict);
    if(errorRate > 0 && sizeRatio_local_predict>2.0 && nTriggers>= (0.1*numAdaptSteps))
    {
        if(apf::getScalar(errorTriggered,ent,0)==1)
        {
            adaptFlag = 1;
            apf::setScalar(errorTriggered,ent,0,2.0);   
        }
        else
            apf::setScalar(errorTriggered,ent,0,1.0);
    }
    else
        apf::setScalar(errorTriggered,ent,0,-1.0);
  }
  m->end(it);

  //upper bound on adapt steps
  if(nTriggers>=2*numAdaptSteps)
  {
    logEvent("Adapting because upper limit was reached.",4);
    adaptFlag = 1;
  }

  assertFlag = adaptFlag;
  PCU_Add_Ints(&assertFlag,1);
  nTriggers++;

  //if adapt, modify the error field to be predictive
  //otherwise, store the current error field to be the next reference field
  if(assertFlag > 0)
  {
    logEvent("Need to error based adapt!!!",4);
/*
    it = m->begin(m->getDimension());
    while( (ent = m->iterate(it) ) )
    {   
        double err_local_current = apf::getScalar(error_current,ent,0);
        double err_local_ref = apf::getScalar(error_reference,ent,0);
        //double errorRate = (err_local_current-err_local_ref)/(T_current-T_reference);
        double errorRate = apf::getScalar(errorRateField,ent,0);
    
        //double err_predict = errorRate*dt_step*numAdaptSteps + err_local_current;
        double err_predict = errorRate*dt_step*numAdaptSteps/10.0 + err_local_current;
        if(err_predict > err_local_current)
        {
            apf::setScalar(error_current,ent,0,err_predict);
        }
    }
    m->end(it);
*/
  }
  else
  {
    it = m->begin(m->getDimension());
    while( (ent = m->iterate(it) ) )
    {   
        double err_local_current = apf::getScalar(error_current,ent,0);
        //set the reference field for the next step
        apf::setScalar(error_reference,ent,0,err_local_current);
    }

  }
  
  T_reference = T_current;

  return assertFlag;
}


int MeshAdaptPUMIDrvr::willAdapt() 
//Master function that calls other adapt-trigger functions
{
  int adaptFlag = 0;
  if(size_field_config == "combined" or size_field_config == "isotropic")
    adaptFlag += willInterfaceAdapt(); 
  if(size_field_config == "combined" or size_field_config == "VMS")
  {
    //willErrorAdapt_reference();
    adaptFlag += willErrorAdapt_reference();
    //willErrorAdapt_reference();
    //willErrorAdapt();
  }

  if(adaptFlag > 0)
    adaptFlag = 1;

  if(adaptFlag == 0)
  {
    //allocated in transfer fields... this is not a good way of doing things, but don't know how to pass a numpy array without having to allocate memory just yet
    free(rho);
    free(nu);
  }

  return adaptFlag;
}


int MeshAdaptPUMIDrvr::willInterfaceAdapt() 
//Does banded adapt need to happen for an isotropic mesh?
//I need to loop over all mesh edges and determine if the edge intersects the blending region.
//If so, need to check the size values on the edge-adjacent vertices.
//If either size value is greater than h_interface*1.5, then we know we need to adapt
{
  int adaptFlag=0;
  int assertFlag;

  //get current size field
  apf::Field* currentField;
/*
  if(!size_iso) //if no previous size field
  {
    currentField = samSz::isoSize(m);
  }
  else //otherwise use previous size field, remember to reflect this in interfaceAdapt or collapse to a single function
  {
    currentField  = apf::createFieldOn(m, "currentField", apf::SCALAR);
    apf::copyData(currentField,size_iso);
  }
*/

  currentField = samSz::isoSize(m);
  double edgeRatio = 1.5; //need to be taken from MeshAdapt library

  //get banded size field
  double L_band = (N_interface_band)*hPhi;
  calculateSizeField(L_band);
  apf::Field* interfaceField = sizeFieldList.front();
  sizeFieldList.pop(); //destroy this size field


  //determine if desired mesh is contained in current mesh
  apf::MeshEntity* ent;
  apf::MeshIterator* it = m->begin(0);
  while( (ent = m->iterate(it)) )
  {
    double h_current = apf::getScalar(currentField,ent,0);
    double h_needed = apf::getScalar(interfaceField,ent,0);
    if(h_current/h_needed > edgeRatio){
      adaptFlag=1;        
      break;
    }  
/*
    else if(h_current > edgeRatio*2.0*h_needed) //if the mesh is too fine
    {
      adaptFlag=1;
      break;
    }
*/
  }//end while

  assertFlag = adaptFlag;
  PCU_Add_Ints(&assertFlag,1);
  //assert(assertFlag ==0 || assertFlag == PCU_Proc_Peers());

  apf::destroyField(currentField);
  apf::destroyField(interfaceField);

  return assertFlag;
}

void getEdgeLengthStatistics(apf::Mesh* m )
{
    //need to get min, max, avg edge lengths in the mesh
    //need to be able to handle parallel
    apf::MeshIterator* it=m->begin(1);
    apf::MeshEntity* ent;

    double minLength = 1000;
    double maxLength = -1;
    double sumLength = 0;
    int numEdges=0;
    while( (ent = m->iterate(it)) )
    {
        if(m->isOwned(ent))
        {
            double length = apf::measure(m,ent);
            sumLength+= length;
            if(length<minLength)
                minLength = length;
            else if(length > maxLength)
                maxLength = length;
            numEdges++;
        }
    }

/*
    if(PCU_Comm_Self()==1)
        std::cout<<"This is numEdges 1 "<<numEdges<<" min "<< minLength<<" max "<<maxLength<<" sum "<< sumLength<<std::endl;
    PCU_Barrier();
    if(PCU_Comm_Self()==0)
        std::cout<<"This is numEdges 0 "<<numEdges<<" min "<< minLength<<" max "<<maxLength<<" sum "<< sumLength<<std::endl;
*/    

    PCU_Add_Doubles(&sumLength,1);
    PCU_Add_Ints(&numEdges,1);
    PCU_Max_Doubles(&maxLength,1);
    PCU_Min_Doubles(&minLength,1);

    double averageLength = sumLength/numEdges;
    if(PCU_Comm_Self()==0)    
    {
        //write mesh statistics out
   
        std::ofstream myfile;
        myfile.open("meshEdgeLengths.txt", std::ios::app );
        myfile <<minLength<<","<<maxLength<<","<<averageLength<<","<<numEdges<<std::endl;
        myfile.close();
    }
    return;
}

int MeshAdaptPUMIDrvr::adaptPUMIMesh(const char* inputString)
/**
 * @brief Function used to trigger adaptation
 *
 * Inputs are the type of size-field that is desired:
 * "interface" refers to explicitly adapting to the current interface position based on the level-set field
 * "ERM" refers to the error-residual method and using error estimates to determine how a mesh should be adapted
 *  Within ERM, there is an isotropic and anisotropic configuration. The anisotropic configuration requires more development.
 *  "Isotropic" refers to a uniform refinement based on a global hmin size and is primarily used for testing
 *  Predictive load balancers Zoltan and ParMA are used in combination for before, during, and after adapt with a preset tolerance of imbalance
 *  The nAdapt counter is iterated to track how many times a mesh is adapted
 */
{
  if (size_field_config == "interface")
      calculateAnisoSizeField();
  else if (size_field_config == "ERM"){
      assert(err_reg);
      removeBCData();
      double t1 = PCU_Time();
      getERMSizeField(total_error);
      double t2 = PCU_Time();
    if(comm_rank==0 && logging_config == "on"){
      std::ofstream myfile;
      myfile.open("error_estimator_timing.txt", std::ios::app );
      myfile << t2-t1<<std::endl;
      myfile.close();
    }
  } 
  else if(size_field_config == "VMS"){
    assert(vmsErrH1);
    getERMSizeField(total_error);
  }
  else if (size_field_config == "meshQuality"){
    size_iso = samSz::isoSize(m);
  }
  else if (size_field_config == "isotropic" || std::string(inputString)=="interface")
  {
    double L_band = (N_interface_band+1)*hPhi;
    calculateSizeField(L_band);
    if(nAdapt>1)
        predictiveInterfacePropagation();
  }
  else if (size_field_config == "isotropicProteus")
    size_iso = m->findField("proteus_size");
  else if (size_field_config == "anisotropicProteus"){
      size_frame = m->findField("proteus_sizeFrame");
      size_scale = m->findField("proteus_sizeScale");
      adapt_type_config = "anisotropic";
  }
  else if (size_field_config == "test"){
    testIsotropicSizeField();
  }
  else if(size_field_config == "uniform"){
      //special situation where I only care about err_reg
      freeField(errRho_reg); 
      freeField(errRel_reg); 
  }
  else if(size_field_config == "combined" && std::string(inputString)==""){
    assert(vmsErrH1);
    //double L_band = (numAdaptSteps+N_interface_band)*hPhi;
    //calculateSizeField(L_band);
    double L_band = (N_interface_band+1)*hPhi;
    calculateSizeField(L_band);
    if(nAdapt>2)
        predictiveInterfacePropagation();
    getERMSizeField(total_error);
  }
  else {
    std::cerr << "unknown size field config " << size_field_config << '\n';
    abort();
  }

  isotropicIntersect();

  if(logging_config=="on"){
    char namebuffer[50];
    sprintf(namebuffer,"pumi_preadapt_%i",nAdapt);
    apf::writeVtkFiles(namebuffer, m);
    //sprintf(namebuffer,"beforeAnisotropicAdapt%i_.smb",nAdapt);
    //m->writeNative(namebuffer);
  }

  if(size_field_config=="ERM"){
      //MeshAdapt error will be thrown if region fields are not freed
      freeField(err_reg); 
      freeField(errRho_reg); 
      freeField(errRel_reg); 
  }
  if(size_field_config=="VMS" || size_field_config=="combined"){
    freeField(vmsErrH1);
    if(PCU_Comm_Self()==0) std::cout<<"cleared VMS field\n";
  }

  //This needs to be done for mass and momentum transfer

 
  std::cout<<"Flag0.0\n";
  if(m->findField("invert_size"))
    apf::destroyField(m->findField("invert_size"));

  apf::Field* size_iso_invert = apf::createLagrangeField(m, "invert_size", apf::SCALAR, 1);
  if(m->findField("density"))
    apf::destroyField(m->findField("density"));
  apf::Field* density = apf::createLagrangeField(m,"density",apf::SCALAR,1);
  apf::MeshIterator* it_invert = m->begin(0);
  apf::MeshEntity* ent_invert;
  while( (ent_invert = m->iterate(it_invert) ) )
  {
    apf::setScalar(size_iso_invert,ent_invert,0,1.0/apf::getScalar(size_iso,ent_invert,0));
    double vofVal = apf::getScalar(m->findField("vof"),ent_invert,0);
    apf::setScalar(density,ent_invert,0,getMPvalue(vofVal,998.2,1.205));
    //hack size field
/*
    apf::Vector3 pt; 
    m->getPoint(ent_invert,0,pt);
    if(pt[0]<1.6 && pt[1]<0.65)
    {
        apf::setScalar(size_iso,ent_invert,0,hmin);
    }
*/
  }
  //gradeMesh();

  m->end(it_invert);

  std::cout<<"Flag1.0\n";
  if(m->findField("density_element"))
    apf::destroyField(m->findField("density_element"));
  apf::Field* density_element = apf::createField(m,"density_element",apf::SCALAR,apf::getVoronoiShape(nsd,1));

  it_invert = m->begin(m->getDimension());
  int idx = 0;
  while( (ent_invert = m->iterate(it_invert) ) )
  {
    apf::setScalar(density_element,ent_invert,0,rho[localNumber(ent_invert)]);
    //std::cout<<"localNumber(ent_invert) "<<rho[localNumber(ent_invert)]<<std::endl;
    idx++;
  }
  m->end(it_invert);

  if(m->findField("velocity_reference"))
    apf::destroyField(m->findField("velocity_reference"));
  apf::Field* velRef = apf::createLagrangeField(m,"velocity_reference",apf::VECTOR,1);
  apf::copyData(velRef, m->findField("velocity"));

  // These are relics from an attempt to pass BCs from proteus into the error estimator.
  // They maybe useful in the future.
  //m->destroyTag(fluxtag[1]); m->destroyTag(fluxtag[2]); m->destroyTag(fluxtag[3]);
  delete [] exteriorGlobaltoLocalElementBoundariesArray;
  exteriorGlobaltoLocalElementBoundariesArray = NULL;


  apf::MeshEntity* edgeForSwap_target;
  if(nAdapt == 5)
  {
    apf::MeshIterator* it_edge = m->begin(1);
    apf::MeshEntity* edgeForSwap;
    while( (edgeForSwap = m->iterate(it_edge)) )
    {   
        apf::Adjacent edge2Face;
        m->getAdjacent(edgeForSwap,2,edge2Face);
        if(edge2Face.getSize()==2)
        {
            //if(localNumber(edge2Face[0])==235 && localNumber(edge2Face[1])==1331 )
            if(localNumber(edge2Face[0])==1331 && localNumber(edge2Face[1])==235 )
            //if(localNumber(edge2Face[0])==490 && localNumber(edge2Face[1])==800 )
            //if(localNumber(edge2Face[0])==3041 && localNumber(edge2Face[1])==642 )
            {
                std::cout<<"centroid is at "<<apf::getLinearCentroid(m,edgeForSwap)<<std::endl;
                std::cout<<"edgeFace "<<localNumber(edge2Face[0])<<" "<<localNumber(edge2Face[1])<<std::endl;
                edgeForSwap_target = edgeForSwap;
                logEvent("Got the target edge!",4);
            }   
        }
    }
    m->end(it_edge);
  }

  for (int d = 0; d <= m->getDimension(); ++d)
    freeNumbering(local[d]);

  apf::Field* adaptSize;
  apf::Field* adaptFrame;

  if(m->findField("adaptSizeMetric"))
    apf::destroyField(m->findField("adaptSizeMetric"));

  if(m->findField("adaptSizeFrame"))
    apf::destroyField(m->findField("adaptSizeFrame"));

  apf::Field* adaptSizeMetric = apf::createLagrangeField(m,"adaptSizeMetric",apf::VECTOR,1);
  apf::Field* adaptSizeFrame= apf::createLagrangeField(m,"adaptSizeFrame",apf::MATRIX,1);

  std::cout<<"Flag2.0\n";
  /// Adapt the mesh
  ma::Input* in;
  if(size_field_config == "uniform"){
    in = ma::configureUniformRefine(m);
    in->shouldFixShape=false;
  }
  else{
    assert(size_iso || (size_scale && size_frame));
    if(adapt_type_config=="anisotropic" || size_field_config== "interface"){
     //in = ma::configure(m, size_scale, size_frame);
      adaptSize  = apf::createFieldOn(m, "adapt_size", apf::VECTOR);
      adaptFrame = apf::createFieldOn(m, "adapt_frame", apf::MATRIX);
      apf::copyData(adaptSize, size_scale);
      apf::copyData(adaptFrame, size_frame);
      in = ma::configure(m, adaptSize, adaptFrame);
    }
    else{
      if(m->findField("adapt_size"))
        apf::destroyField(m->findField("adapt_size"));
        
      adaptSize  = apf::createFieldOn(m, "adapt_size", apf::SCALAR);
      apf::copyData(adaptSize, size_iso);
      in = ma::configure(m, adaptSize);

      
      apf::MeshIterator* it = m->begin(0);
      apf::MeshEntity* ent;
      while( (ent = m->iterate(it) ) )
      {
        double sizeVal = apf::getScalar(adaptSize,ent,0);
        apf::Vector3 sizeVec(sizeVal,sizeVal,sizeVal);
        apf::Matrix3x3 sizeFrames(1.0,0.0,0.0,0.0,1.0,0.0,0.0,0.0,1.0);
        sizeFrames = sizeFrames*sizeVal;
        apf::setVector(adaptSizeMetric,ent,0,sizeVec);
        apf::setMatrix(adaptSizeFrame,ent,0,sizeFrames);
      }
      m->end(0);
    }
  }

/*
  if((nAdapt-1) == 3)
  {

          /// Adapt the mesh
          apf::MeshIterator* it = m->begin(0);
          apf::MeshEntity* ent;
          while( (ent = m->iterate(it) ) )
          { 
            apf::Vector3 pt;
            m->getPoint(ent,0,pt);
            if((pt[0]>1.33 && pt[0]<1.74) && (pt[1]>1.12 && pt[1]<1.51))
            {
                double sizeVal = apf::getScalar(adaptSize,ent,0);
                apf::setScalar(adaptSize,ent,0,sizeVal/2.0);
            }
          }
          m->end(0);

  }
*/

  lion_set_verbosity(1);
  ma::validateInput(in);
  in->shouldRunPreZoltan = true;
  in->shouldRunMidZoltan = true;
  in->shouldRunPostZoltan = true;
  //in->shouldRunMidParma = true;
  //in->shouldRunPostParma = true;
  in->maximumImbalance = 1.05;
  in->maximumIterations = numIter;
  in->shouldSnap = false;
  in->debugFolder = "./debug_testAdapt";
  //in->goodQuality = 0.064;//0.027;
  //double mass_before = getTotalMass();
  
  //m->writeNative("beforeMesh.smb");
  double t1 = PCU_Time();
  //ma::adapt(in);
  std::cout<<"Begin adapt!\n";

/*
  if(nAdapt == 5)
  {
    //std::abort();
    ma::adaptSwap(in,edgeForSwap_target);
  }
  else 
*/
    ma::adaptVerbose(in,false);


  std::cout<<"End adapt!\n";
  double t2 = PCU_Time();

  m->verify();
  //double mass_after = getTotalMass();
  //PCU_Add_Doubles(&mass_before,1);
  //PCU_Add_Doubles(&mass_after,1);

/*
  std::cout<<"I am the alpha\n";
  apf::writeVtkFiles("PreAdapt_PUMI",m);
  apf::destroyField(m->findField("ma_sizes"));
  apf::destroyField(m->findField("ma_frame"));
  {

    auto lib = Omega_h::Library(NULL, NULL);
    Omega_h::Mesh om(&lib);
    apf::to_omega_h(&om, m);
    om.set_parting(OMEGA_H_ELEM_BASED);
    om.balance();
    Omega_h::binary::write("OmegaMesh.osh", &om);

    std::cout<<"I am the alpha 2.0\n";

  auto target_metrics = Omega_h::Write<Omega_h::Real>(om.nverts() * Omega_h::symm_ncomps(om.dim()));

  auto sizeField = (&om)->get_array<Omega_h::Real>(Omega_h::VERT, "proteus_size");
  auto f = OMEGA_H_LAMBDA(Omega_h::LO vert) {
    //get value
    auto sizeVal = Omega_h::get_vector<1>(sizeField,vert);
    
    auto target_metric = Omega_h::compose_metric(
      Omega_h::identity_matrix<2, 2>(), Omega_h::vector_2(sizeVal[0], sizeVal[0] ) );
    Omega_h::set_vector(target_metrics, vert, Omega_h::symm2vector(target_metric));
  };

  Omega_h::parallel_for(om.nverts(), f);
  om.add_tag(Omega_h::VERT, "initial_metric", Omega_h::symm_ncomps(om.dim()), Omega_h::Reals(target_metrics) );

    //set up a metric field
    Omega_h::MetricInput input;
    input.sources.push_back(Omega_h::MetricSource{OMEGA_H_GIVEN, 1.0, "initial_metric",OMEGA_H_ISO_SIZE,OMEGA_H_ABSOLUTE});
    input.should_limit_gradation = true;
    input.verbose=true;

    Omega_h::add_implied_isos_tag(&om);
    Omega_h::generate_target_metric_tag(&om, input);

    char namebuffer[50];
    sprintf(namebuffer,"PreAdapt_ConvertToOmega_%i.vtu",nAdapt);
    Omega_h::vtk::write_vtu(namebuffer, &om);
    auto opts = Omega_h::AdaptOpts(&om);

    opts.xfer_opts.type_map["density_element"] = OMEGA_H_CONSERVE;
    opts.xfer_opts.integral_map["density_element"] = "mass";
    opts.xfer_opts.integral_diffuse_map["mass"] = Omega_h::VarCompareOpts::none(); //Omega_h::VarCompareOpts{Omega_h::VarCompareOpts::RELATIVE, 0.01, 0.0};
    //opts.xfer_opts.integral_diffuse_map["mass"] = Omega_h::VarCompareOpts{Omega_h::VarCompareOpts::RELATIVE, 1e-3, 0.0};

    opts.xfer_opts.type_map["velocity"] = OMEGA_H_MOMENTUM_VELOCITY;
    opts.xfer_opts.velocity_density_map["velocity"] = "density_element";
    opts.xfer_opts.velocity_momentum_map["velocity"] = "momentum";
    opts.xfer_opts.integral_diffuse_map["momentum"] =Omega_h::VarCompareOpts::none(); //Omega_h::VarCompareOpts{Omega_h::VarCompareOpts::RELATIVE, 0.01, 1e-6};
    //opts.xfer_opts.integral_diffuse_map["momentum"] = Omega_h::VarCompareOpts{Omega_h::VarCompareOpts::RELATIVE, 1e-2, 0.0};
    //opts.xfer_opts.integral_diffuse_map["momentum"] =Omega_h::VarCompareOpts::defaults();

    //opts.xfer_opts.type_map["velocity"] = OMEGA_H_LINEAR_INTERP;
    opts.xfer_opts.type_map["p"] = OMEGA_H_LINEAR_INTERP;

    opts.xfer_opts.type_map["velocity_reference"] = OMEGA_H_LINEAR_INTERP;
    opts.xfer_opts.type_map["velocity_old"] = OMEGA_H_LINEAR_INTERP;
    opts.xfer_opts.type_map["velocity_old_old"] = OMEGA_H_LINEAR_INTERP;
    opts.xfer_opts.type_map["p_old"] = OMEGA_H_LINEAR_INTERP;
    opts.xfer_opts.type_map["p_old_old"] = OMEGA_H_LINEAR_INTERP;

    opts.xfer_opts.type_map["vof"] = OMEGA_H_LINEAR_INTERP;
    opts.xfer_opts.type_map["vof_old"] = OMEGA_H_LINEAR_INTERP;
    opts.xfer_opts.type_map["vof_old_old"] = OMEGA_H_LINEAR_INTERP;
    opts.xfer_opts.type_map["phi"] = OMEGA_H_LINEAR_INTERP;
    opts.xfer_opts.type_map["phi_old"] = OMEGA_H_LINEAR_INTERP;
    opts.xfer_opts.type_map["phi_old_old"] = OMEGA_H_LINEAR_INTERP;
    opts.xfer_opts.type_map["phid"] = OMEGA_H_LINEAR_INTERP;
    opts.xfer_opts.type_map["phid_old"] = OMEGA_H_LINEAR_INTERP;
    opts.xfer_opts.type_map["phid_old_old"] = OMEGA_H_LINEAR_INTERP;
    opts.xfer_opts.type_map["phiCorr"] = OMEGA_H_LINEAR_INTERP;
    opts.xfer_opts.type_map["phiCorr_old"] = OMEGA_H_LINEAR_INTERP;
    opts.xfer_opts.type_map["phiCorr_old_old"] = OMEGA_H_LINEAR_INTERP;

    //opts.xfer_opts.type_map["velocity_old_old"] = OMEGA_H_MOMENTUM_VELOCITY;

    //Omega_h::adapt(&om, opts);
    //Omega_h::approach_metric(&om,opts);

    opts.max_length_allowed = 1.5;
    while (Omega_h::approach_metric(&om, opts)) Omega_h::adapt(&om, opts);
    char namebuffer2[50];
    sprintf(namebuffer2,"PostAdapt_ConvertToOmega_%i.vtu",nAdapt);
    Omega_h::vtk::write_vtu(namebuffer2, &om);

    std::cout<<"I am the alpha 3.0\n";
    apf::Mesh2* m2 = apf::makeEmptyMdsMesh(m->getModel(), 2, false);
    apf::from_omega_h(m2, &om);
    std::cout<<"I am the alpha 4.0\n";
    m2->writeNative("PostAdapt_ConvertToAPF.smb");
    m=m2;
    
  }
  apf::writeVtkFiles("PostAdapt_PUMI",m);
  std::cout<<"I am the omega\n";
*/

  if(size_field_config=="ERM"){
    if (has_gBC)
      getSimmetrixBC();
  }
  if(logging_config=="on"){
    char namebuffer[50];
    sprintf(namebuffer,"pumi_postadapt_%i",nAdapt);
    apf::writeVtkFiles(namebuffer, m);
    //sprintf(namebuffer,"afterAnisotropicAdapt%i_.smb",nAdapt);
    //m->writeNative(namebuffer);
  }
  //isReconstructed = 0; //this is needed to maintain consistency with the post-adapt conversion back to Proteus
  apf::destroyField(adaptSize);
  if(adapt_type_config=="anisotropic")
    apf::destroyField(adaptFrame);
  nAdapt++; //counter for number of adapt steps

  if(logging_config=="debugRestart")
    m->writeNative("DEBUG_restart.smb");

  //freeField(error_reference);
  apf::destroyField(m->findField("error_reference"));
  apf::destroyField(m->findField("errorTriggered"));
  nTriggers=0;


  //compute correction factor to solution fields
/*
  numberLocally();
  if(nAdapt == 5)
  {
    apf::MeshIterator* it_correct = m->begin(0);
    apf::MeshEntity* ent;
    while( (ent = m->iterate(it_correct)))
    {
        apf::Vector3 velocity_modify(0.0,0.0,0.0);
        apf::Vector3 velocity_current;
        apf::getVector(m->findField("velocity"),ent,0,velocity_current);
   
        if(localNumber(ent) == 371) 
        {
           velocity_modify[0] = 1.25583197348640e-05;
           velocity_modify[1] =-0.00423121346013942;
        }
        else if(localNumber(ent) == 1809)
        {
           velocity_modify[0]  = 3.88168026510991e-06;
           velocity_modify[1] = -0.00203268246503486;
        }
        else if(localNumber(ent) == 2211)
        {
           velocity_modify[0]  = 9.23831973491868e-06;
           velocity_modify[1] =-0.00203229947645753 ;
        }
        else if(localNumber(ent) == 2483)
        {
           velocity_modify[0]  = 4.39501359840435e-06;
           velocity_modify[1] =-0.00203275286503485 ;
        }

        velocity_current = velocity_current + velocity_modify;
        apf::setVector(m->findField("velocity"),ent,0,velocity_current);

    }

  }  
*/


//momentum correction via solution transfer
/*
  numberLocally();
  if(nAdapt == 5)
  {
    apf::MeshIterator* it_correct = m->begin(0);
    apf::MeshEntity* ent;
    while( (ent = m->iterate(it_correct)))
    {
        apf::Vector3 velocity_modify(0.0,0.0,0.0);
        apf::Vector3 velocity_current;
        apf::getVector(m->findField("velocity"),ent,0,velocity_current);
   
        if(localNumber(ent) == 223) 
        {
           velocity_modify[0] = 0.0;
           velocity_modify[1] = -0.0027673;
        }
        else if(localNumber(ent) == 1520)
        {
           velocity_modify[0]  = 0.0;
           velocity_modify[1] = 0.0069181;
        }
        else if(localNumber(ent) == 1902)
        {
           velocity_modify[0]  = 0.0;
           velocity_modify[1] =-0.0027673;
        }
        else if(localNumber(ent) == 2148)
        {
           velocity_modify[0]  = 0.0;
           velocity_modify[1] = -0.012453;
        }

        velocity_current = velocity_current + velocity_modify;
        apf::setVector(m->findField("velocity"),ent,0,velocity_current);

    }

  }  
*/

/*
  if(nAdapt==6)
  {
    numberLocally();
    getSolutionCorrection(m);
    //std::abort();
  }
*/
/*
  if(nAdapt == 5)
  {
    numberLocally();
    apf::MeshIterator * patchIter = m->begin(0);
    apf::MeshEntity* ent;
    while( (ent = m->iterate(patchIter) ) )
    {
        if(localNumber(ent) == 1520)
        {
            getPatchRelaxation(ent);
        }
    }
    m->end(patchIter);
  }
*/
/*
  if((nAdapt-1) == 4)
  {

    char namebuffer3[50];
    sprintf(namebuffer3,"HACK_before_%i",nAdapt);
    apf::writeVtkFiles(namebuffer3, m);


          /// Adapt the mesh
          ma::Input* in;
          if(m->findField("adapt_size"))
            apf::destroyField(m->findField("adapt_size"));
                
          adaptSize  = apf::createFieldOn(m, "adapt_size", apf::SCALAR);
          apf::copyData(adaptSize, size_iso);

          apf::MeshIterator* it = m->begin(0);
          apf::MeshEntity* ent;
          while( (ent = m->iterate(it) ) )
          { 
            apf::Vector3 pt;
            m->getPoint(ent,0,pt);
            if((pt[0]>1.33 && pt[0]<1.74) && (pt[1]>1.12 && pt[1]<1.51))
            {
                double sizeVal = apf::getScalar(adaptSize,ent,0);
                apf::setScalar(adaptSize,ent,0,sizeVal/2.0);
            }
          }
          m->end(0);
          in = ma::configure(m, adaptSize);
      
          lion_set_verbosity(1);
          ma::validateInput(in);
          in->shouldRunPreZoltan = true;
          in->shouldRunMidZoltan = true;
          in->shouldRunPostZoltan = true;
          in->maximumImbalance = 1.05;
          in->maximumIterations = numIter;
          in->shouldSnap = false;
          in->debugFolder = "./debug_testAdapt";
          double t1 = PCU_Time();
          std::cout<<"Begin adapt!\n";
          ma::adaptVerbose(in,false);
          std::cout<<"End adapt!\n";
          double t2 = PCU_Time();
          m->verify();

    sprintf(namebuffer3,"HACK_after_%i",nAdapt);
    apf::writeVtkFiles(namebuffer3, m);

  }
*/
  free(rho);
  free(nu);

  getEdgeLengthStatistics(m);
    
  return 0;
}

#include <petscksp.h>
#include <apfDynamicMatrix.h>
void MeshAdaptPUMIDrvr::getSolutionCorrection(apf::Mesh* m)
{
//try to correct the pressure
//serial only for now
//assume material properties are the same for each mesh for now....
//actually will need the material properties at quadrature points

    //Start computing element quantities
    int numqpt; //number of quadrature points
    const int nshl= 3; //number of local shape functions
    int elem_type; //what type of topology
    double weight; //value container for the weight at each qpt
    double Jdet;
    int int_order = 2; //because integrand is 2nd order polynomial
  
    apf::FieldShape* correction_shape = apf::getLagrange(1);
    apf::EntityShape* elem_shape;
    apf::Vector3 qpt; //container for quadrature points
    apf::MeshElement* element;
    apf::Element* velo_elem;
    apf::Matrix3x3 J; //actual Jacobian matrix
    apf::Matrix3x3 invJ; //inverse of Jacobian
    apf::NewArray <double> shpval; //array to store shape function values at quadrature points
    apf::NewArray <apf::Vector3> shgval; //array to store shape function values at quadrature points
    apf::NewArray <apf::DynamicVector> shgval_copy;

    apf::DynamicMatrix invJ_copy;
    apf::NewArray <apf::DynamicVector> shdrv;
  
    apf::Field* velf = m->findField("velocity");
    //apf::Field* velf = m->findField("velocity_old");

    int nsd = m->getDimension();
    apf::MeshIterator* iter = m->begin(nsd); //loop over elements
    apf::MeshEntity* ent;

    //LHS Matrix Initialization
    int ndofs = m->count(0);
    Mat K; //matrix size depends on nshl, which may vary from element to element
    MatCreate(PETSC_COMM_SELF,&K);
    MatSetSizes(K,ndofs,ndofs,ndofs,ndofs);
    MatSetFromOptions(K);
    MatSetUp(K);

    //RHS Vector Initialization
    Vec F;
    VecCreate(PETSC_COMM_SELF,&F);
    VecSetSizes(F,ndofs,ndofs);
    VecSetUp(F);
 
    //create element matrix for LHS construction and zero out
    apf::Matrix<nshl,nshl> elementMatLHS;
    apf::Vector<nshl> elementRHS;
 
    while(ent = m->iterate(iter))
    //loop through all elements
    { 
    
        //reset values
        for(int i=0;i<nshl;i++)
        {
            for(int j=0;j<nshl;j++)
                elementMatLHS[i][j]=0.0;
            elementRHS[i] = 0.0;
        }


        elem_type = m->getType(ent);
        if(!(elem_type != 4 || elem_type != 2)){ //2|TRI, 4|TET
            std::cout<<"Not a Tri or Tet present"<<std::endl;
            exit(0); 
        }
        element = apf::createMeshElement(m,ent);
        velo_elem = apf::createElement(velf,element);
  
        numqpt=apf::countIntPoints(element,int_order); //generally p*p maximum for shape functions
        shpval.allocate(nshl);   
        shdrv.allocate(nshl);
        shgval.allocate(nshl);
        shgval_copy.allocate(nshl);

        //loop through all qpts
        for(int k=0;k<numqpt;k++)
        {
            apf::getIntPoint(element,int_order,k,qpt); //get a quadrature point and store in qpt
            apf::getJacobian(element,qpt,J); //evaluate the Jacobian at the quadrature point
            J = apf::transpose(J); //Is PUMI still defined in this way?
            if(nsd==2)
                J[2][2] = 1.0; //this is necessary to avoid singular matrix
            invJ = invert(J);
            Jdet=fabs(apf::getJacobianDeterminant(J,nsd)); 
            weight = apf::getIntWeight(element,int_order,k);
            invJ_copy = apf::fromMatrix(invJ);

            //first get the shape function values
            elem_shape = correction_shape->getEntityShape(elem_type);
            elem_shape->getValues(NULL,NULL,qpt,shpval);
            elem_shape->getLocalGradients(NULL,NULL,qpt,shgval); 

            for(int i =0;i<nshl;i++){ //get the true derivative and copy only the edge modes for use
                shgval_copy[i] = apf::fromVector(shgval[i]);
                apf::multiply(shgval_copy[i],invJ_copy,shdrv[i]); 
            }
/* check for spatial derivatives; makes sense
      std::cout<<"local element number "<<localNumber(ent)<<std::endl;
      std::cout<<"local gradients "<<shgval[0]<<" "<<shgval[1]<<" "<<shgval[2]<<std::endl;
      std::cout<<"Jacobian "<<J<<std::endl;
      std::cout<<"inverse Jacobian "<<invJ_copy<<std::endl;
      std::cout<<"final derivatives "<<shdrv[0]<<" "<<shdrv[1]<<" "<<shdrv[2]<<std::endl;
      std::exit(1);
*/

            for(int i=0;i<nshl;i++)
            {
                for(int j=0;j<nshl;j++)
                {
                    double temp = 0.0;
                    for(int space_dim = 0; space_dim<nsd;space_dim++ )
                        temp += shdrv[i][space_dim]*shdrv[j][space_dim];
                    elementMatLHS[i][j] += temp*weight*Jdet;
                }
            }

            //obtain needed values for RHS

            apf::Matrix3x3 vel_grad;
            apf::getVectorGrad(velo_elem,qpt,vel_grad);
            double div_vel = 0.0;
            for(int i=0;i<nsd;i++)
            {
                div_vel += vel_grad[i][i];
            }
            for(int i=0;i<nshl;i++)
            {
                elementRHS[i] += div_vel*shpval[i]*weight*Jdet;
                //elementRHS[i] += -2.0*shpval[i]*weight*Jdet;
            }
/*
      std::cout<<"local Number "<<localNumber(ent)<<std::endl;
      std::cout<<"velocity gradient "<<vel_grad<<std::endl;
      std::cout<<"divergence "<<div_vel<<std::endl;
      std::cout<<"weight "<<weight<<std::endl;
      std::cout<<"Jdet "<<Jdet<<std::endl;
      std::cout<<"shpval 0"<<shpval[0]<<std::endl;
      std::cout<<"elementRHS "<<elementRHS[0]<<" "<<elementRHS[1]<<" "<<elementRHS[2]<<std::endl;
*/      


    
        } // end quadrature loop

        //transfer element matrix information to global LHS matrix
        apf::Adjacent adj_verts;
        m->getAdjacent(ent,0,adj_verts);

        PetscScalar LHS[nshl][nshl];
        int idx[nshl]; //indices for PETSc Mat insertion
        for(int i=0; i<nshl;i++)
        {
            idx[i] = localNumber(adj_verts[i]);
            std::cout<<"shape functions local number "<<idx[i]<<std::endl;
            for(int j=0;j<nshl;j++)
            {
                LHS[i][j] = elementMatLHS[i][j];
            }
        }
        PetscScalar RHS[nshl];
        for (int i=0; i<nshl; i++)
        {
/*check assembly; looks right
        if(idx[i]==2273)
            RHS[i] = 1;
        else
*/
            RHS[i] = elementRHS[i];
        }

        //this seems wrong but I don't know how else to specify the idx array as the rows and column indices do refer to the same degrees of freedom
        MatSetValues(K,nshl,idx,nshl,idx,LHS[0],ADD_VALUES);
        VecSetValues(F,nshl,idx,&(RHS[0]),ADD_VALUES);
    
        apf::destroyElement(velo_elem);
        apf::destroyMeshElement(element);

    } //end while loop for elements
    m->end(iter);

    MatAssemblyBegin(K,MAT_FINAL_ASSEMBLY);
    MatAssemblyEnd(K,MAT_FINAL_ASSEMBLY);
 
    VecAssemblyBegin(F);
    VecAssemblyEnd(F); 
   
//

    //get boundary dofs and zero out
    PetscInt nrows = 0;
    PetscInt* rows;
    iter = m->begin(0);
    while(ent = m->iterate(iter))
    {
        apf::ModelEntity* geomEnt = m->toModel(ent);
        if(m->getModelType(geomEnt) < m->getDimension())
        {
            apf::Adjacent vert_adjFace;
            m->getAdjacent(ent,m->getDimension()-1,vert_adjFace);
            apf::MeshEntity* face;
            for(int i =0; i<vert_adjFace.getSize();i++)
            {
                face=vert_adjFace[i];
                geomEnt = m->toModel(face);

                //IF mesh face is classified on boundary
                if(m->getModelType(geomEnt) == m->getDimension()-1)
                {
                    if(m->getModelTag(geomEnt) == 4) 
                    {       
                        //get local number store somewhere  
                        nrows++;
                        break;
                    }
/*
                    else if(m->getModelTag(geomEnt) == 3)
                    {
                        nrows++;
                        break;
                    }
*/
                }
            }
        }

    }
    m->end(iter);
    PetscMalloc1(nrows,&rows);

    PetscScalar zeroVals[nrows];

    iter = m->begin(0);
    int rowidx = 0;
/*
    while(ent = m->iterate(iter))
    {
        //if classification of vertex is not a region
        if(m->getModelType(m->toModel(ent)) < m->getDimension()) 
        {
            //get local number store somewhere  
            rows[rowidx] = localNumber(ent);    
            zeroVals[rowidx]=0.0;
            rowidx++;
        }
    }
    m->end(iter);
*/
    Vec solVals;
    VecCreate(PETSC_COMM_SELF,&solVals);
    VecSetSizes(solVals,nrows,nrows);
    VecSetUp(solVals);
    PetscScalar solVal = 0.0;
    PetscInt oneVal = 1;
  
    while(ent = m->iterate(iter))
    {
        apf::ModelEntity* geomEnt = m->toModel(ent);
        if(m->getModelType(geomEnt) < m->getDimension())
        {
            apf::Adjacent vert_adjFace;
            m->getAdjacent(ent,m->getDimension()-1,vert_adjFace);
            apf::MeshEntity* face;
            for(int i =0; i<vert_adjFace.getSize();i++)
            {
                face=vert_adjFace[i];
                geomEnt = m->toModel(face);

                //IF mesh face is classified on boundary
                if(m->getModelType(geomEnt) == m->getDimension()-1)
                {
                    if(m->getModelTag(geomEnt) == 4) 
                    {       
                        //get local number store somewhere  
                        std::cout<<"localnumber "<<localNumber(ent)<<std::endl;
                        rows[rowidx] = localNumber(ent);    
                        zeroVals[rowidx]=0.0;
/*
                        zeroVals[rowidx]=3.24;
                        solVal=3.24;
                        VecSetValues(solVals,oneVal,&(rows[rowidx]),&solVal,INSERT_VALUES);
*/
                        rowidx++;
                        break;
                    }
/*
                    else if(m->getModelTag(geomEnt) == 3)
                    {
                        rows[rowidx] = localNumber(ent);    
                        zeroVals[rowidx]=0.0;
                        solVal = 0.0;
                        //VecSetValues(solVals,oneVal,&rowidx,&solVal,INSERT_VALUES);
                        VecSetValues(solVals,oneVal,&(rows[rowidx]),&solVal,INSERT_VALUES);
                        rowidx++;
                        break;
                    }
*/

                }
            }
        }

    }

    MatZeroRowsColumns(K, nrows,rows,1.0,NULL,NULL);
    //MatZeroRowsColumns(K, nrows,rows,1.0,solVals,F);
    VecSetValues(F,nrows,rows,zeroVals,INSERT_VALUES);
    VecAssemblyBegin(F);
    VecAssemblyEnd(F);
    
    //std::cout<<"row value "<<rows[5]<<std::endl;
    std::cout<<"Look at LHS "<<std::endl;
    MatView(K,PETSC_VIEWER_STDOUT_SELF);
    std::cout<<"Look at RHS "<<std::endl;
    VecView(F,PETSC_VIEWER_STDOUT_SELF);

    PetscBool flg;
    MatIsSymmetric(K,0.0,&flg);
    std::cout<<"Is matrix symmetric "<<flg<<std::endl;
        
    PetscFree(rows);

//

    Vec coef;
    VecCreate(PETSC_COMM_SELF,&coef);
    VecSetSizes(coef,ndofs,ndofs);
    VecSetUp(coef);
    
/* checks out
    std::cout<<"look for 2273, should be 5\n";
    //VecView(F,PETSC_VIEWER_STDOUT_SELF);
    PetscScalar finalValue[1];
    PetscInt numberOfElements =1;
    PetscInt finalIndex = 2273;
    VecGetValues(F,numberOfElements,&finalIndex,&(finalValue[0]));
    std::cout<<"What is final value? "<<finalValue[0]<<std::endl;
*/

    KSP ksp; //initialize solver context
    KSPCreate(PETSC_COMM_SELF,&ksp);
    KSPSetOperators(ksp,K,K);
    KSPSetType(ksp,KSPPREONLY);
    PC pc;
    KSPGetPC(ksp,&pc);
    PCSetType(pc,PCLU);
    KSPSetFromOptions(ksp);

    KSPSolve(ksp,F,coef);
    
    KSPDestroy(&ksp); //destroy ksp


    VecView(coef,PETSC_VIEWER_STDOUT_SELF);
    PetscScalar *array; 
    VecGetArray(coef,&array);
    apf::Field* correctionField = apf::createLagrangeField(m, "velocity_correction", apf::SCALAR, 1);

    iter = m->begin(0);
    while(ent = m->iterate(iter))
    {
        int dofIdx = localNumber(ent);
        apf::setScalar(correctionField,ent,0,array[dofIdx]);
    }
    m->end(iter);

    VecRestoreArray(coef,&array);
    MatDestroy(&K); //destroy the matrix
    VecDestroy(&F); //destroy vector
    VecDestroy(&coef); //destroy vector

    apf::writeVtkFiles("Debug_beforeCorrection",m);

/*
    apf::Field* gradientLambda = apf::recoverGradientByVolume(correctionField);
    iter = m->begin(0);
    while(ent = m->iterate(iter))
    {
        apf::Vector3 correction;
        apf::Vector3 velocity_current; 
        apf::getVector(gradientLambda,ent,0,correction);
        apf::getVector(velf,ent,0,velocity_current);
        apf::setVector(velf,ent,0,velocity_current+correction);
    }
    m->end(iter);
*/
/* Correction on a single element is insufficient
    iter = m->begin(2);
    while(ent = m->iterate(iter))
    {
        //apf::MeshElement* correctElem = apf::createMeshElement(correctionField,ent);
        if(localNumber(ent) == 2807)
        {
            apf::MeshElement* meshElem = apf::createMeshElement(m,ent);
            //apf::Element* correctElem = apf::createElement(correctionField,ent);
            apf::Element* correctElem = apf::createElement(correctionField,meshElem);
            apf::Vector3 correction;
            apf::getGrad(correctElem,apf::Vector3(1./3,1./3,1./3),correction);
            std::cout<<"What is the correction factor? "<<correction<<std::endl;

            apf::getGrad(correctElem,apf::Vector3(1./3,1./3,0.0),correction);
            std::cout<<"What is the correction factor? "<<correction<<std::endl;


            apf::Adjacent elementAdjVert;
            m->getAdjacent(ent,0,elementAdjVert);
            for(int i=0; i<elementAdjVert.getSize();i++)
            {
                apf::Vector3 velocity_current; 
                //apf::getVector(gradientLambda,ent,0,correction);
                apf::getVector(velf,elementAdjVert[i],0,velocity_current);
                apf::setVector(velf,elementAdjVert[i],0,velocity_current+correction);
            }
            apf::destroyElement(correctElem); 
            apf::destroyMeshElement(meshElem);
        }
    }
    m->end(iter);
*/
   
    //nodal reconstruction through basis-weighted averaging
    apf::Field* gradientLambdaInitial = apf::recoverGradientByVolume(correctionField);
    apf::Field* gradientLambda = apf::createLagrangeField(m, "gradLambda_basis", apf::VECTOR, 1);
    iter = m->begin(0);
    while(ent = m->iterate(iter))
    {
        //integration quanitites
        double total_shp_integral=0.0;
        apf::Vector3 theVelocityCorrection(0.0,0.0,0.0);

        apf::Adjacent vertAdjElem;
        m->getAdjacent(ent,m->getDimension(),vertAdjElem);

        for(int i=0;i<vertAdjElem.getSize(); i++)
        {
            apf::MeshElement* meshElem = apf::createMeshElement(m,vertAdjElem[i]);
            apf::Element* correctElem = apf::createElement(correctionField,meshElem);

            //get gradient
            apf::Vector3 correction;
            apf::getGrad(correctElem,apf::Vector3(1./3,1./3,1./3),correction);

            //get shape function and integrate

            //find which shape function corresponds to initial vertex
            apf::Adjacent element2vertex;
            m->getAdjacent(vertAdjElem[i],0,element2vertex);
            int shpval_idx;
            for(int j=0;j<element2vertex.getSize();j++)
            {
                if(localNumber(element2vertex[j]) == localNumber(ent))
                {
                    shpval_idx = j;
                    break;
                }
            }
            
            double shp_integral = 0.0;
            //loop through all qpts
            for(int k=0;k<numqpt;k++)
            {
                apf::getIntPoint(meshElem,int_order,k,qpt); //get a quadrature point and store in qpt
                apf::getJacobian(meshElem,qpt,J); //evaluate the Jacobian at the quadrature point
                J = apf::transpose(J); //Is PUMI still defined in this way?
                if(nsd==2)
                    J[2][2] = 1.0; //this is necessary to avoid singular matrix
                invJ = invert(J);
                Jdet=fabs(apf::getJacobianDeterminant(J,nsd)); 
                weight = apf::getIntWeight(meshElem,int_order,k);
                invJ_copy = apf::fromMatrix(invJ);

                //first get the shape function values
                elem_shape = correction_shape->getEntityShape(elem_type);
                elem_shape->getValues(NULL,NULL,qpt,shpval);
                
                shp_integral+=shpval[shpval_idx]*weight*Jdet;
            }
            theVelocityCorrection += correction*shp_integral;
            total_shp_integral+= shp_integral;
            apf::destroyElement(correctElem); 
            apf::destroyMeshElement(meshElem);
        }
        theVelocityCorrection = theVelocityCorrection/total_shp_integral;

        apf::Vector3 velocity_current; 
        apf::setVector(gradientLambda,ent,0,theVelocityCorrection);
        apf::getVector(velf,ent,0,velocity_current);
        apf::setVector(velf,ent,0,velocity_current+theVelocityCorrection);
    }
    m->end(iter);

    apf::writeVtkFiles("Debug_afterCorrection",m);
}

void MeshAdaptPUMIDrvr::getPatchRelaxation(apf::MeshEntity* vert)
{
//This code is to test how we might be able to correct the velocities or orthogonlize the error to the new mesh/function space
//We try to accomplish this by solving a BVP over the patch of elements around a vertex where all other vertices other than the vertex of interest are DBCs. 
//The problem is based on the NSE which is linearized by the current solution.

    //Start computing element quantities
    int numqpt; //number of quadrature points
    const int nshl= 3; //number of local shape functions
    int elem_type; //what type of topology
    double weight; //value container for the weight at each qpt
    double Jdet;
    int int_order = 2; //because integrand is 2nd order polynomial
  
    apf::FieldShape* update_shape = apf::getLagrange(1);
    apf::EntityShape* elem_shape;
    apf::Vector3 qpt; //container for quadrature points
    apf::MeshElement* element;
    apf::Element* velo_elem;
    apf::Element* velo_old_elem;
    apf::Element* pres_elem;
    apf::Matrix3x3 J; //actual Jacobian matrix
    apf::Matrix3x3 invJ; //inverse of Jacobian
    apf::NewArray <double> shpval; //array to store shape function values at quadrature points
    apf::NewArray <apf::Vector3> shgval; //array to store shape function values at quadrature points
    apf::NewArray <apf::DynamicVector> shgval_copy;

    apf::DynamicMatrix invJ_copy;
    apf::NewArray <apf::DynamicVector> shdrv;
  
    apf::Field* velf = m->findField("velocity");
    apf::Field* velf_old = m->findField("velocity_old");
    apf::Field* pref = m->findField("p");


    int nsd = m->getDimension();

    //Get Patch of Elements
    apf::Adjacent vertexPatch;
    apf::Adjacent vertex2vertex;
    m->getAdjacent(vert,nsd,vertexPatch);
    apf::getBridgeAdjacent (m, vert, 1, 0, vertex2vertex);
    
    //LHS Matrix Initialization
    int ndofs = vertex2vertex.getSize()+1;
    Mat K; //matrix size depends on nshl, which may vary from element to element
    MatCreate(PETSC_COMM_SELF,&K);
    MatSetSizes(K,ndofs,ndofs,ndofs,ndofs);
    MatSetFromOptions(K);
    MatSetUp(K);

    //RHS Vector Initialization
    Vec F;
    VecCreate(PETSC_COMM_SELF,&F);
    VecSetSizes(F,ndofs,ndofs);
    VecSetUp(F);


    //get Vertex numbering; assume vert is always labelled first as 0
    std::map<int,int> vertexNumbering;
    vertexNumbering.insert( std::pair<int,int>(localNumber(vert),0));
    int idx = 1;
    for(int i=0; i<vertex2vertex.getSize();i++)
    {
        vertexNumbering.insert( std::pair<int,int>(localNumber(vertex2vertex[i]),idx));
        idx++;
    }

    //get IEN array;     
    std::map<int,int> IEN;
    idx =0;
    for(int i=0; i<vertexPatch.getSize();i++)
    {
        IEN.insert( std::pair<int,int>(localNumber(vertexPatch[i]),idx));
        idx++;
    }
 
    std::cout<<"size of vertex numbering "<<vertexNumbering.size()<<std::endl;;
    std::cout<<"size of IEN "<<IEN.size()<<std::endl;
    
    std::cout<<"check vertex numbering "<<vertexNumbering[223]<<" "<<vertexNumbering[1520]<<" "<<vertexNumbering[1902]<<std::endl;
    std::cout<<"check IEN value "<<IEN[2783]<<" "<<IEN[6554]<<" "<<IEN[3376]<<std::endl;

    //create element matrix for LHS construction and zero out
    apf::Matrix<nshl,nshl> elementMatLHS;
    apf::Vector<nshl> elementRHS;
 
    apf::MeshEntity* ent;
    for(int z = 0; z<vertexPatch.getSize();z++)
    //loop through all elements
    { 
        ent = vertexPatch[z];
        //reset values
        for(int i=0;i<nshl;i++)
        {
            for(int j=0;j<nshl;j++)
                elementMatLHS[i][j]=0.0;
            elementRHS[i] = 0.0;
        }


        elem_type = m->getType(ent);
        if(!(elem_type != 4 || elem_type != 2)){ //2|TRI, 4|TET
            std::cout<<"Not a Tri or Tet present"<<std::endl;
            exit(0); 
        }
        element = apf::createMeshElement(m,ent);
        velo_elem = apf::createElement(velf,element);
        velo_old_elem = apf::createElement(velf_old,element);
        pres_elem = apf::createElement(pref,element);
  
        numqpt=apf::countIntPoints(element,int_order); //generally p*p maximum for shape functions
        shpval.allocate(nshl);   
        shdrv.allocate(nshl);
        shgval.allocate(nshl);
        shgval_copy.allocate(nshl);

        //loop through all qpts
        for(int k=0;k<numqpt;k++)
        {
            apf::getIntPoint(element,int_order,k,qpt); //get a quadrature point and store in qpt
            apf::getJacobian(element,qpt,J); //evaluate the Jacobian at the quadrature point
            J = apf::transpose(J); //Is PUMI still defined in this way?
            if(nsd==2)
                J[2][2] = 1.0; //this is necessary to avoid singular matrix
            invJ = invert(J);
            Jdet=fabs(apf::getJacobianDeterminant(J,nsd)); 
            weight = apf::getIntWeight(element,int_order,k);
            invJ_copy = apf::fromMatrix(invJ);

            //first get the shape function values
            elem_shape = update_shape->getEntityShape(elem_type);
            elem_shape->getValues(NULL,NULL,qpt,shpval);
            elem_shape->getLocalGradients(NULL,NULL,qpt,shgval); 

            for(int i =0;i<nshl;i++){ //get the true derivative and copy only the edge modes for use
                shgval_copy[i] = apf::fromVector(shgval[i]);
                apf::multiply(shgval_copy[i],invJ_copy,shdrv[i]); 
            }


            apf::Vector3 velocity;
            apf::getVector(velo_elem,qpt,velocity);

            apf::Vector3 velocity_old;
            apf::getVector(velo_old_elem,qpt,velocity_old);

            //LHS computations
            for(int i=0;i<nshl;i++)
            {
                
                for(int j=0;j<nshl;j++)
                {
                    //time derivative contribution
            
                    double time_contribution = shpval[i]*shpval[j]/delta_T; 
                    
                    //convective term
                    double temp_conv = 0.0;
                    for(int space_dim = 0; space_dim<nsd;space_dim++)
                    {
                        temp_conv += velocity[space_dim]*shdrv[j][space_dim];
                    }
                    double convective_contribution = temp_conv*shpval[i];
                
                    //diffusive term
                    //needs viscosity term
                    double viscosity= 1.500e-5; //hardcoded for now
                    double temp = 0.0;
                    for(int space_dim = 0; space_dim<nsd;space_dim++ )
                        temp += shdrv[i][space_dim]*shdrv[j][space_dim]*viscosity;
                    elementMatLHS[i][j] += (time_contribution+convective_contribution+temp ) *weight*Jdet;
                }
            }

            //obtain needed values for RHS

            apf::Matrix3x3 vel_grad;
            apf::getVectorGrad(velo_elem,qpt,vel_grad);
            double div_vel = 0.0;
            for(int i=0;i<nsd;i++)
            {
                div_vel += vel_grad[i][i];
            }
            //double pressure = apf::getScalar(pres_elem,qpt);
            apf::Vector3 grad_pres;
            apf::getGrad(pres_elem,qpt,grad_pres);
            double density = 1.205;
            

            for(int i=0;i<nshl;i++)
            {
                elementRHS[i] += div_vel*shpval[i]*weight*Jdet;
            }
    
        } // end quadrature loop

        //transfer element matrix information to global LHS matrix
        apf::Adjacent adj_verts;
        m->getAdjacent(ent,0,adj_verts);

        PetscScalar LHS[nshl][nshl];
        int idx[nshl]; //indices for PETSc Mat insertion
        for(int i=0; i<nshl;i++)
        {
            idx[i] = vertexNumbering[localNumber(adj_verts[i])];
            std::cout<<"shape functions local number "<<idx[i]<<std::endl;
            for(int j=0;j<nshl;j++)
            {
                LHS[i][j] = elementMatLHS[i][j];
            }
        }
        PetscScalar RHS[nshl];
        for (int i=0; i<nshl; i++)
        {
            RHS[i] = elementRHS[i];
        }

        //this seems wrong but I don't know how else to specify the idx array as the rows and column indices do refer to the same degrees of freedom
        MatSetValues(K,nshl,idx,nshl,idx,LHS[0],ADD_VALUES);
        VecSetValues(F,nshl,idx,&(RHS[0]),ADD_VALUES);
    
        apf::destroyElement(pres_elem);
        apf::destroyElement(velo_elem);
        apf::destroyElement(velo_old_elem);
        apf::destroyMeshElement(element);

    } //end loop around patch

    MatAssemblyBegin(K,MAT_FINAL_ASSEMBLY);
    MatAssemblyEnd(K,MAT_FINAL_ASSEMBLY);
 
    std::cout<<"Look at LHS "<<std::endl;
    MatView(K,PETSC_VIEWER_STDOUT_SELF);


    VecAssemblyBegin(F);
    VecAssemblyEnd(F); 
   
//
/*

    //get boundary dofs and zero out
    PetscInt nrows = 0;
    PetscInt* rows;
    iter = m->begin(0);
    while(ent = m->iterate(iter))
    {
        apf::ModelEntity* geomEnt = m->toModel(ent);
        if(m->getModelType(geomEnt) < m->getDimension())
        {
            apf::Adjacent vert_adjFace;
            m->getAdjacent(ent,m->getDimension()-1,vert_adjFace);
            apf::MeshEntity* face;
            for(int i =0; i<vert_adjFace.getSize();i++)
            {
                face=vert_adjFace[i];
                geomEnt = m->toModel(face);

                //IF mesh face is classified on boundary
                if(m->getModelType(geomEnt) == m->getDimension()-1)
                {
                    if(m->getModelTag(geomEnt) == 4) 
                    {       
                        //get local number store somewhere  
                        nrows++;
                        break;
                    }
                }
            }
        }

    }
    m->end(iter);
    PetscMalloc1(nrows,&rows);

    PetscScalar zeroVals[nrows];

    iter = m->begin(0);
    int rowidx = 0;
    Vec solVals;
    VecCreate(PETSC_COMM_SELF,&solVals);
    VecSetSizes(solVals,nrows,nrows);
    VecSetUp(solVals);
    PetscScalar solVal = 0.0;
    PetscInt oneVal = 1;
  
    while(ent = m->iterate(iter))
    {
        apf::ModelEntity* geomEnt = m->toModel(ent);
        if(m->getModelType(geomEnt) < m->getDimension())
        {
            apf::Adjacent vert_adjFace;
            m->getAdjacent(ent,m->getDimension()-1,vert_adjFace);
            apf::MeshEntity* face;
            for(int i =0; i<vert_adjFace.getSize();i++)
            {
                face=vert_adjFace[i];
                geomEnt = m->toModel(face);

                //IF mesh face is classified on boundary
                if(m->getModelType(geomEnt) == m->getDimension()-1)
                {
                    if(m->getModelTag(geomEnt) == 4) 
                    {       
                        //get local number store somewhere  
                        std::cout<<"localnumber "<<localNumber(ent)<<std::endl;
                        rows[rowidx] = localNumber(ent);    
                        zeroVals[rowidx]=0.0;
                        rowidx++;
                        break;
                    }

                }
            }
        }

    }

    MatZeroRowsColumns(K, nrows,rows,1.0,NULL,NULL);
    VecSetValues(F,nrows,rows,zeroVals,INSERT_VALUES);
    VecAssemblyBegin(F);
    VecAssemblyEnd(F);
    
    std::cout<<"Look at LHS "<<std::endl;
    MatView(K,PETSC_VIEWER_STDOUT_SELF);
    std::cout<<"Look at RHS "<<std::endl;
    VecView(F,PETSC_VIEWER_STDOUT_SELF);

    PetscBool flg;
    MatIsSymmetric(K,0.0,&flg);
    std::cout<<"Is matrix symmetric "<<flg<<std::endl;
        
    PetscFree(rows);

    Vec coef;
    VecCreate(PETSC_COMM_SELF,&coef);
    VecSetSizes(coef,ndofs,ndofs);
    VecSetUp(coef);
    
    KSP ksp; //initialize solver context
    KSPCreate(PETSC_COMM_SELF,&ksp);
    KSPSetOperators(ksp,K,K);
    KSPSetType(ksp,KSPPREONLY);
    PC pc;
    KSPGetPC(ksp,&pc);
    PCSetType(pc,PCLU);
    KSPSetFromOptions(ksp);

    KSPSolve(ksp,F,coef);
    
    KSPDestroy(&ksp); //destroy ksp


    VecView(coef,PETSC_VIEWER_STDOUT_SELF);
    PetscScalar *array; 
    VecGetArray(coef,&array);
    apf::Field* correctionField = apf::createLagrangeField(m, "velocity_correction", apf::SCALAR, 1);

    iter = m->begin(0);
    while(ent = m->iterate(iter))
    {
        int dofIdx = localNumber(ent);
        apf::setScalar(correctionField,ent,0,array[dofIdx]);
    }
    m->end(iter);
    VecRestoreArray(coef,&array);
*/

    MatDestroy(&K); //destroy the matrix
    VecDestroy(&F); //destroy vector
    //VecDestroy(&coef); //destroy vector


}




double MeshAdaptPUMIDrvr::getMinimumQuality()
/**
 * @brief Function used to get the worst element quality in the mesh.
 *
 * Measures the quality via SCOREC library; returns the minimum quality
 * Meant to be used to trigger adaptation, but has not been implemented yet
 */
{
  ma::SizeField* isf = new ma::IdentitySizeField(m);
  apf::MeshIterator* it = m->begin(m->getDimension());
  apf::MeshEntity* e;
  double minq = 1;
  while ((e = m->iterate(it)))
    minq = std::min(minq, ma::measureElementQuality(m, isf, e));
  m->end(it);
  delete isf;
  return PCU_Min_Double(minq);
}

double MeshAdaptPUMIDrvr::getTotalMass()
/**
 * @brief Function to track total mass of the domain.
 *
 * Returns total mass across all ranks
 */
{
  apf::Field* voff = m->findField("vof");
  assert(voff);
  apf::MeshEntity* e;
  apf::MeshIterator* it = m->begin(m->getDimension());
  double mass = 0.0;
  while ((e = m->iterate(it))) {
    apf::MeshElement* elem = apf::createMeshElement(m,e);
    apf::Element* voff_elem = apf::createElement(voff, elem);
    int int_order = 4;
    for(int l = 0; l < apf::countIntPoints(elem, int_order); ++l) {
      apf::Vector3 qpt;
      apf::getIntPoint(elem,int_order,l,qpt);
      double vof_val = apf::getScalar(voff_elem,qpt);
      double rho_val = getMPvalue(vof_val,rho[0],rho[1]);
      double weight = apf::getIntWeight(elem,int_order,l);
      apf::Matrix3x3 J;
      apf::getJacobian(elem,qpt,J); //evaluate the Jacobian at the quadrature point
      double Jdet = apf::getJacobianDeterminant(J,m->getDimension());
      mass += rho_val*weight*Jdet;
    }
    apf::destroyElement(voff_elem);
    apf::destroyMeshElement(elem);
  }
  m->end(it);
  PCU_Add_Doubles(&mass,1);
  return mass;
}
/** @} */

//Save mesh with solution

void MeshAdaptPUMIDrvr::writeMesh(const char* meshFile)
{
  //m->writeNative(meshFile);
  apf::writeVtkFiles(meshFile,m);
}

//Clean mesh of all fields and tags

void MeshAdaptPUMIDrvr::cleanMesh()
{
  //destroy all fields...
    
  for(int i =0;i<m->countFields();i++)
  {
    apf::Field* sample = m->getField(i);
    freeField(sample);
  }
  //std::cout<<"find field " <<m->getField(m->countFields())<<" how many fields? "<<m->countFields()<<std::endl;
  //std::cout<<"is velocity_old here? "<<m->findField("velocity_old")<<std::endl;
  apf::Field* sample = m->findField("velocity_old");
  freeField(sample);

  sample = m->findField("vof_old");
  freeField(sample);
  sample = m->findField("ls_old");
  freeField(sample);
  sample = m->findField("phi");
  freeField(sample);
  sample = m->findField("phi_old");
  freeField(sample);
  sample = m->findField("phi_old_old");
  freeField(sample);
  sample = m->findField("phid_old");
  freeField(sample);
  sample = m->findField("phiCorr");
  freeField(sample);
  sample = m->findField("phiCorr_old");
  freeField(sample);
  sample = m->findField("phiCorr_old_old");
  freeField(sample);
  sample = m->findField("p_old");
  freeField(sample);
  sample = m->findField("p");
  freeField(sample);
  sample = m->findField("p_old_old");
  freeField(sample);

  sample = m->findField("VMSH1");
  freeField(sample);
  sample = m->findField("VMSL2");
  freeField(sample);

  //destroy all tags
  apf::DynamicArray<apf::MeshTag*> listTags;
  m->getTags(listTags);
  int nTags = listTags.getSize();
  int numDim = m->getDimension();
  for(int i=0; i < nTags; i++)
  {
    std::string ignoreString ("proteus_number");
    std::string tagName (m->getTagName(listTags[i]));
    if(tagName.find(ignoreString) != std::string::npos)
    {
      //do nothing
    }
    else{
      for(int j=0;j<(numDim+1);j++)
      {
        apf::MeshIterator* it = m->begin(j);
        apf::MeshEntity* ent;
        while( (ent = m->iterate(it)) )
        {
          if(m->hasTag(ent,listTags[i]))
            m->removeTag(ent,listTags[i]);
        }
        m->end(it);
      }
      m->destroyTag(listTags[i]);
    }
  }
}

void MeshAdaptPUMIDrvr::set_nAdapt(int numberAdapt)
{
  nAdapt = numberAdapt;
  return;
}

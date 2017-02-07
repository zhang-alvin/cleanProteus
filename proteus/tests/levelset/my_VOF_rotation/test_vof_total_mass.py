#!/usr/bin/env python
"""
Test module for VOF rotation with EV
"""
import os,sys,inspect

cmd_folder = os.path.realpath(os.path.abspath(os.path.split(inspect.getfile( inspect.currentframe() ))[0]))
if cmd_folder not in sys.path:
    sys.path.insert(0,cmd_folder)

cmd_subfolder = os.path.realpath(os.path.abspath(os.path.join(os.path.split(inspect.getfile( inspect.currentframe() )) [0],"import_modules")))
if cmd_subfolder not in sys.path:
    sys.path.insert(0,cmd_subfolder)

from proteus.iproteus import *
from proteus import Comm
comm = Comm.get()
Profiling.logLevel=2
Profiling.verbose=True
import numpy as np
import vof_rotation_2d_test_template as vf

class TestVOFrotationEV():

    @classmethod
    def setup_class(cls):
        pass

    @classmethod
    def teardown_class(cls):
        pass

    def setup_method(self,method):
        """Initialize the test problem. """
        reload(vf)
        self.sim_names = []
        self.aux_names = []


    def teardown_method(self,method):
        filenames = []
        for sim_name in self.sim_names:
            filenames.extend([sim_name+'.'+post for post in ['xmf','h5']])
        for aux_name in self.aux_names:
            filenames.extend(aux_name)

        for f in filenames:
            if os.path.exists(f):
                try:
                    os.remove(f)
                except OSError, e:
                    print ("Error: %s - %s." %(e.filename, e.strerror ))
            else:
                pass
    def test_vof_total_mass_T1m1_FE(self):
        """
        Test total mass for Forward Euler Integration running for final time T=0.1

        These are the flags used for VOF.h in this benchmark for now
        #define POWER_SMOOTHNESS_INDICATOR 2
        #define LUMPED_MASS_MATRIX 0
        #define KUZMINS_METHOD 1
        #define INTEGRATE_BY_PARTS 1
        #define QUANTITIES_OF_INTEREST 0
        #define FIX_BOUNDARY_KUZMINS 1
        """
        run_dir = os.path.dirname(os.path.abspath(__file__))
        
        #set the time step
        vf.p.T = 0.1
        vf.n.nDTout = 10
        vf.n.DT = vf.p.T/float(vf.n.nDTout)
        vf.so.DT = vf.n.DT
        vf.so.tnList = [i*vf.n.DT for i  in range(vf.n.nDTout+1)]

        #force F2orwardEuler
        vf.timeIntegration_vof="FE"
        vf.n.timeOrder = 1
        vf.n.nStagesTime = 1
        vf.rot2D.soname=vf.rot2D.soname.replace("SSP33","FE")
        vf.p.name = vf.p.name.replace("SSP33","FE")
        vf.so.name = vf.rot2D.soname
        
        ns = proteus.NumericalSolution.NS_base(vf.so,[vf.p],[vf.n],vf.so.sList,opts)
        sim_name = ns.modelList[0].name
        aux = ns.auxiliaryVariables[ns.modelList[0].name][0]
        self.sim_names.append(sim_name)
        self.aux_names.append(aux.ofile.name)

        ns.calculateSolution('test_vof_total_mass_T1m1')
        aux.ofile.close() #have to close manually for now, would be good to have a hook for this
        ref_total_mass = np.loadtxt(os.path.join(run_dir,'comparison_files','total_mass_comp_0_T1m1_'+sim_name+'.txt'))
        sim_total_mass = np.loadtxt('total_mass_comp_0_'+sim_name+'.txt')

        failed = np.allclose(ref_total_mass, sim_total_mass,
                             rtol=1e-05, atol=1e-07, equal_nan=True) 
        assert(failed) 

    def no_test_vof_total_mass_T1m1_SSP33(self):
        """
        Test total mass for SSP33 Integration running for final time T=0.1

        These are the flags used for VOF.h in this benchmark for now
        #define EDGE_VISCOSITY 1
        #define USE_EDGE_BASED_EV 1 // if not then dissipative matrix is based purely on smoothness indicator of the solution
        #define POWER_SMOOTHNESS_INDICATOR 2
        #define LUMPED_MASS_MATRIX 0

        """
        run_dir = os.path.dirname(os.path.abspath(__file__))
        
        #set the time step
        vf.p.T = 0.1
        vf.n.nDTout = 10
        vf.n.DT = vf.p.T/float(vf.n.nDTout)
        vf.so.DT = vf.n.DT
        vf.so.tnList = [i*vf.n.DT for i  in range(vf.n.nDTout+1)]

        #force SSP33
        vf.timeIntegration_vof="SSP33"
        vf.n.timeOrder = 3
        vf.n.nStagesTime = 3
        vf.rot2D.soname=vf.rot2D.soname.replace("FE","SSP33")
        vf.p.name = vf.p.name.replace("FE","SSP33")
        vf.so.name = vf.rot2D.soname
        
        ns = proteus.NumericalSolution.NS_base(vf.so,[vf.p],[vf.n],vf.so.sList,opts)
        sim_name = ns.modelList[0].name
        aux = ns.auxiliaryVariables[ns.modelList[0].name][0]
        self.sim_names.append(sim_name)
        self.aux_names.append(aux.ofile.name)

        ns.calculateSolution('test_vof_total_mass_T1m1')
        aux.ofile.close() #have to close manually for now, would be good to have a hook for this
        ref_total_mass = np.loadtxt(os.path.join(run_dir,'comparison_files','total_mass_comp_0_T1m1_'+sim_name+'.txt'))
        sim_total_mass = np.loadtxt('total_mass_comp_0_'+sim_name+'.txt')

        failed = np.allclose(ref_total_mass, sim_total_mass,
                             rtol=1e-05, atol=1e-07, equal_nan=True)
        assert(failed) 

    def notest_vof_total_mass_T1_FE(self):

        run_dir = os.path.dirname(os.path.abspath(__file__))
        
        #set the time step
        vf.p.T = 1.0
        vf.n.nDTout = 10
        vf.n.DT = vf.p.T/float(vf.n.nDTout)
        vf.so.DT = vf.n.DT
        vf.so.tnList = [i*vf.n.DT for i  in range(vf.n.nDTout+1)]

        #force ForwardEuler
        vf.timeIntegration_vof="FE"
        vf.n.timeOrder = 1
        vf.n.nStagesTime = 1
        vf.rot2D.soname=vf.rot2D.soname.replace("SSP33","FE")
        vf.p.name = vf.p.name.replace("SSP33","FE")
        vf.so.name = vf.rot2D.soname
        
        ns = proteus.NumericalSolution.NS_base(vf.so,[vf.p],[vf.n],vf.so.sList,opts)
        sim_name = ns.modelList[0].name
        aux = ns.auxiliaryVariables[ns.modelList[0].name][0]
        self.sim_names.append(sim_name)
        self.aux_names.append(aux.ofile.name)

        ns.calculateSolution('test_vof_total_mass_T1')
        aux.ofile.close() #have to close manually for now, would be good to have a hook for this
        ref_total_mass = np.loadtxt(os.path.join(run_dir,'comparison_files','total_mass_comp_0_T1_'+sim_name+'.txt'))
        sim_total_mass = np.loadtxt('total_mass_comp_0_'+sim_name+'.txt')

        failed = np.allclose(ref_total_mass, sim_total_mass,
                             rtol=1e-05, atol=1e-07, equal_nan=True)
        assert(failed) 

    def notest_vof_total_mass_T1_SSP33(self):

        run_dir = os.path.dirname(os.path.abspath(__file__))
        
        #set the time step
        vf.p.T = 1.0
        vf.n.nDTout = 10
        vf.n.DT = vf.p.T/float(vf.n.nDTout)
        vf.so.DT = vf.n.DT
        vf.so.tnList = [i*vf.n.DT for i  in range(vf.n.nDTout+1)]

        #force ForwardEuler
        vf.timeIntegration_vof="SSP33"
        vf.n.timeOrder = 3
        vf.n.nStagesTime = 3
        vf.rot2D.soname=vf.rot2D.soname.replace("FE","SSP33")
        vf.p.name = vf.p.name.replace("FE","SSP33")
        vf.so.name = vf.rot2D.soname
        
        ns = proteus.NumericalSolution.NS_base(vf.so,[vf.p],[vf.n],vf.so.sList,opts)
        sim_name = ns.modelList[0].name
        aux = ns.auxiliaryVariables[ns.modelList[0].name][0]
        self.sim_names.append(sim_name)
        self.aux_names.append(aux.ofile.name)

        ns.calculateSolution('test_vof_total_mass_T1')
        aux.ofile.close() #have to close manually for now, would be good to have a hook for this
        ref_total_mass = np.loadtxt(os.path.join(run_dir,'comparison_files','total_mass_comp_0_T1_'+sim_name+'.txt'))
        sim_total_mass = np.loadtxt('total_mass_comp_0_'+sim_name+'.txt')

        failed = np.allclose(ref_total_mass, sim_total_mass,
                             rtol=1e-05, atol=1e-07, equal_nan=True)
        assert(failed) 

if __name__ == '__main__':
    pass

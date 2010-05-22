from pyadh import *
from pyadh.default_p import *
from rans_step_2d import *
import step2d

"""
k-epsilon turbulence model for backward facing step
epsilon equation
"""


class ConstantIC:
    def __init__(self,cval=0.0):
        self.cval=cval
    def uOfXT(self,x,t):
        return self.cval

coefficients = kEpsilon_epsilon(flowModelID=0,
                                kModelID=1,
                                nd=nd,
                                g=g,
                                nu=nu,
                                rho=rho)

kInflow = 0.003*inflow*inflow
epsilonInflow = coefficients.c_mu*kInflow**(1.5)/(0.03*upstream_height)
#mwf hack
#coefficients.c_mu = 0.0#turn off nonlinearities

initialConditions = {0:ConstantIC(cval=epsilonInflow*0.001)}

analyticalSolution = None


def getDBC_epsilon(x,flag):
    if flag == boundaryTags['upstream']:
        return lambda x,t:epsilonInflow
    
dirichletConditions = {0:getDBC_epsilon}


fluxBoundaryConditions = {0:'outFlow'}

diffusiveFluxBoundaryConditions = {0:{}}

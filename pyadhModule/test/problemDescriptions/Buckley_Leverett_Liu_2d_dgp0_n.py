from pyadh import *
from pyadh.default_n import *
from Buckley_Leverett_Liu_2d_p import *

timeOrder =1
nStagesTime = timeOrder

runCFL = 0.4 #0.1


timeIntegration = SSPRKPIintegration
stepController=Min_dt_RKcontroller
DT=None
nDTout = 10

#femSpaces = {0:DG_Constants}
femSpaces = {0:DG_AffineP0_OnSimplexWithMonomialBasis}#

elementQuadrature = SimplexGaussQuadrature(nd,2)
elementBoundaryQuadrature = SimplexGaussQuadrature(nd-1,2)

nn = 1
nLevels = 3

subgridError = None

massLumping = False

numericalFluxType =  RusanovNumericalFlux_Diagonal#

shockCapturing = None

multilevelNonlinearSolver  = Newton#NLNI

usingSSPRKNewton=True
levelNonlinearSolver = SSPRKNewton#

nonlinearSmoother = NLGaussSeidel

fullNewtonFlag = True

tolFac = 0.01

nl_atol_res = 1.0e-8

matrix = SparseMatrix

multilevelLinearSolver = LU

levelLinearSolver = LU

linearSmoother = GaussSeidel

linTolFac = 0.001

conservativeFlux = None


archiveFlag = ArchiveFlags.EVERY_USER_STEP

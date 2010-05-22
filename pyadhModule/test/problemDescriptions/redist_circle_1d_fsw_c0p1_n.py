from pyadh import *
from pyadh.default_n import *
from redist_circle_1d_p import *

timeIntegration = NoIntegration

femSpaces = {0:C0_AffineLinearOnSimplexWithNodalBasis}


elementQuadrature = SimplexGaussQuadrature(nd,3)
#
elementBoundaryQuadrature = SimplexGaussQuadrature(nd-1,3)

nn=3
nLevels = 5

DT = None
nDTout = 1

multilevelNonlinearSolver  = MultilevelEikonalSolver
levelNonlinearSolver = UnstructuredFMMandFSWsolvers.FSWEikonalSolver

nl_atol_res = 1.0e-12
tolFac = 1.0e-12
maxNonlinearIts = 100

matrix = SparseMatrix


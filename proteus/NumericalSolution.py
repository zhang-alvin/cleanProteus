"""
A hierarchy of classes for managing complete numerical solution implementations

.. inheritance-diagram:: proteus.NumericalSolution
   :parts: 1
"""
from __future__ import print_function
from __future__ import absolute_import
from __future__ import division

from builtins import zip
from builtins import str
from builtins import input
from builtins import range
from builtins import object
from past.utils import old_div
import os
import numpy
from subprocess import check_call, check_output

from . import LinearSolvers
from . import NonlinearSolvers
from . import MeshTools
from . import Profiling
from . import Transport
from . import SimTools
from . import Archiver
from . import Viewers
from .Archiver import ArchiveFlags
from . import Domain
from .MeshAdaptPUMI import Checkpoint
from .Profiling import logEvent

# Global to control whether the kernel starting is active.
embed_ok = True

class NS_base(object):  # (HasTraits):
    r"""
    The base class for managing the numerical solution of  PDE's.

    The constructor must build all the objects required by a numerical
    method to approximate the solution over a sequence of time intervals.

    calculateSolution(runName) carries out the numerical solution.

    .. graphviz::

       digraph NumericalSolutionHasA {
       node [shape=record, fontname=Helvetica, fontsize=12];
       NS   [label="NumericalSolution" URL="\ref NumericalSolution", style="filled", fillcolor="gray"];
       mList [label="MultilevelTranportModel [n]" URL="\ref proteus::Transport::MultilevelTransport"];
       nsList [label="NonLinearSolver [n] " URL="\ref proteus::NonLinearSolver"];
       lsList [label="LinearSolver [n] " URL="\ref proteus::LinearSolver"];
       pList [label="Problem Specification [n]" URL="\ref proteus::default_p"];
       nList [label="Numerics Specifiation [n]" URL="\ref proteus::default_n"];
       sList [label="Output Specification [n]" URL="\ref proteus::SimTools"];
       so [label="Coupling Specification " URL="\ref proteus::SO_base"];
       ar [label="Archiver" URL="\ref proteus::AR_base"];
       NS -> pList [arrowhead="normal", style="dashed", color="purple"];
       NS -> nList [arrowhead="normal", style="dashed", color="purple"];
       NS -> so [arrowhead="normal", style="dashed", color="purple"];
       NS -> sList [arrowhead="normal", style="dashed", color="purple"];
       NS -> mList [arrowhead="normal", style="dashed", color="purple"];
       NS -> nsList [arrowhead="normal", style="dashed", color="purple"];
       NS -> lsList [arrowhead="normal", style="dashed", color="purple"];
       NS -> ar [arrowhead="normal", style="dashed", color="purple"];
       }
    """

    def __init__(self,so,pList,nList,sList,opts,simFlagsList=None,TwoPhaseFlow=False):
        from . import Comm
        comm=Comm.get()
        self.comm=comm
        message = "Initializing NumericalSolution for "+so.name+"\n System includes: \n"
        for p in pList:
            message += p.name+"\n"
        logEvent(message)
        #: SplitOperator initialize file
        self.so=so
        #: List of physics initialize files
        self.pList=pList
        #: List of numerics initialize files
        self.nList=nList
        #: Dictionary of command line arguments
        self.opts=opts
        self.simFlagsList=simFlagsList
        self.TwoPhaseFlow=TwoPhaseFlow
        self.timeValues={}
        Profiling.memory("Memory used before initializing"+so.name)
        memBase = Profiling.memLast #save current memory usage for later
        if not so.useOneMesh:
            so.useOneArchive=False
        logEvent("Setting Archiver(s)")

        if hasattr(self.so,"fastArchive"):
            self.fastArchive = self.so.fastArchive
        else:
            self.fastArchive = False

        if so.useOneArchive:
            self.femSpaceWritten={}
            tmp  = Archiver.XdmfArchive(opts.dataDir,so.name,useTextArchive=opts.useTextArchive,
                                        gatherAtClose=opts.gatherArchive,hotStart=opts.hotStart,
                                        useGlobalXMF=(not opts.subdomainArchives),
                                        global_sync=opts.global_sync)
            if self.fastArchive==True:
                self.ar = dict([(0,tmp)])
            else:
                self.ar = dict([(i,tmp) for i in range(len(self.pList))])
        elif len(self.pList) == 1:
            self.ar = {0:Archiver.XdmfArchive(opts.dataDir,so.name,useTextArchive=opts.useTextArchive,
                                              gatherAtClose=opts.gatherArchive,hotStart=opts.hotStart)} #reuse so.name if possible
        else:
            self.ar = dict([(i,Archiver.XdmfArchive(opts.dataDir,p.name,useTextArchive=opts.useTextArchive,
                                                    gatherAtClose=opts.gatherArchive,hotStart=opts.hotStart)) for i,p in enumerate(self.pList)])
        #by default do not save quadrature point info
        self.archive_q                 = dict([(i,False) for i in range(len(self.pList))]);
        self.archive_ebq_global        = dict([(i,False) for i in range(len(self.pList))]);
        self.archive_ebqe              = dict([(i,False) for i in range(len(self.pList))]);
        self.archive_pod_residuals = dict([(i,False) for i in range(len(self.pList))]);
        if simFlagsList is not None:
            assert len(simFlagsList) == len(self.pList), "len(simFlagsList) = %s should be %s " % (len(simFlagsList),len(self.pList))
            for index in range(len(self.pList)):
                if 'storeQuantities' in simFlagsList[index]:
                    for quant in [a for a in simFlagsList[index]['storeQuantities'] if a is not None]:
                        recType = quant.split(':')
                        if len(recType) > 1 and recType[0] == 'q':
                            self.archive_q[index] = True
                        elif len(recType) > 1 and recType[0] == 'ebq_global':
                            self.archive_ebq_global[index] = True
                        elif len(recType) > 1 and recType[0] == 'ebqe':
                            self.archive_ebqe[index] = True
                        #
                        elif recType[0] == 'pod_residuals':
                            self.archive_pod_residuals[index]=True
                        else:
                            logEvent("Warning Numerical Solution storeQuantity = %s not recognized won't archive" % quant)
                    #
                #
            #
        #
        logEvent("Setting up MultilevelMesh")
        mlMesh_nList = []
        if so.useOneMesh:
            logEvent("Building one multilevel mesh for all models")
            nListForMeshGeneration=[nList[0]]
            pListForMeshGeneration=[pList[0]]
        else:
            logEvent("Building seperate meshes for each model")
            nListForMeshGeneration=nList
            pListForMeshGeneration=pList
        for p,n in zip(pListForMeshGeneration,nListForMeshGeneration):
            if opts.hotStart:
                p.genMesh = False
                logEvent("Hotstarting, using existing mesh "+p.name)
            else:
                logEvent("Generating mesh for "+p.name)
            #support for old-style domain input
            if p.domain is None:
                if p.nd == 1:
                    p.domain = Domain.RectangularDomain(L=p.L[:1],
                                                        x=p.x0[:1],
                                                        name=p.name)
                elif p.nd == 2:
                    if p.polyfile is not None:
                        p.domain = Domain.PlanarStraightLineGraphDomain(fileprefix=p.polyfile,name=p.polyfile)
                    elif p.meshfile != None:
                        p.domain = Domain.Mesh2DMDomain(p.meshfile)
                    else:
                        p.domain = Domain.RectangularDomain(L=p.L[:2],
                                                            x=p.x0[:2],
                                                            name=p.name)
                elif p.nd == 3:
                    if p.polyfile is not None:
                        p.domain = Domain.PiecewiseLinearComplexDomain(fileprefix=p.polyfile,name=p.polyfile)
                    elif p.meshfile is not None:
                        p.domain = Domain.Mesh3DMDomain(p.meshfile)
                    else:
                        p.domain = Domain.RectangularDomain(L=p.L[:3],
                                                            x=p.x0[:3],
                                                            name=p.name)
                else:
                    raise RuntimeError("No support for domains in more than three dimensions")
            #now generate meshes, could move to Domain and use polymorphism or MeshTools
            if isinstance(p.domain,Domain.RectangularDomain):
                if p.domain.nd == 1:
                    mlMesh = MeshTools.MultilevelEdgeMesh(n.nn, 1, 1,
                                                          p.domain.x[0], 0.0, 0.0,
                                                          p.domain.L[0], 1.0, 1.0,
                                                          refinementLevels=n.nLevels,
                                                          nLayersOfOverlap=n.nLayersOfOverlapForParallel,
                                                          parallelPartitioningType=n.parallelPartitioningType)
                elif p.domain.nd == 2:
                    if (n.nnx == n.nny is None):
                        nnx = nny = n.nn
                    else:
                        nnx = n.nnx
                        nny = n.nny
                    logEvent("Building %i x %i rectangular mesh for %s" % (nnx,nny,p.name))

                    if not hasattr(n,'quad'):
                        n.quad = False

                    if (n.quad):
                        mlMesh = MeshTools.MultilevelQuadrilateralMesh(nnx,nny,1,
                                                                       p.domain.x[0], p.domain.x[1], 0.0,
                                                                       p.domain.L[0],p.domain.L[1],1,
                                                                       refinementLevels=n.nLevels,
                                                                       nLayersOfOverlap=n.nLayersOfOverlapForParallel,
                                                                       parallelPartitioningType=n.parallelPartitioningType)
                    else:
                        if hasattr(n,'triangleFlag')==True:
                            triangleFlag=n.triangleFlag
                        else:
                            triangleFlag=0
                        mlMesh = MeshTools.MultilevelTriangularMesh(nnx,nny,1,
                                                                    p.domain.x[0], p.domain.x[1], 0.0,
                                                                    p.domain.L[0],p.domain.L[1],1,
                                                                    refinementLevels=n.nLevels,
                                                                    nLayersOfOverlap=n.nLayersOfOverlapForParallel,
                                                                    parallelPartitioningType=n.parallelPartitioningType,
                                                                    triangleFlag=triangleFlag)

                elif p.domain.nd == 3:
                    if (n.nnx == n.nny == n.nnz  is None):
                        nnx = nny = nnz = n.nn
                    else:
                        nnx = n.nnx
                        nny = n.nny
                        nnz = n.nnz
                    logEvent("Building %i x %i x %i rectangular mesh for %s" % (nnx,nny,nnz,p.name))

                    if not hasattr(n,'hex'):
                        n.hex = False

                    if not hasattr(n,'NURBS'):
                        n.NURBS = False

                    if (n.NURBS):
                        mlMesh = MeshTools.MultilevelNURBSMesh(nnx,nny,nnz,
                                                               n.px,n.py,n.pz,
                                                               p.domain.x[0], p.domain.x[1], p.domain.x[2],
                                                               p.domain.L[0], p.domain.L[1], p.domain.L[2],
                                                               refinementLevels=n.nLevels,
                                                               nLayersOfOverlap=n.nLayersOfOverlapForParallel,
                                                               parallelPartitioningType=n.parallelPartitioningType)
                    elif (n.hex):
                        if not hasattr(n,'px'):
                            n.px=0
                            n.py=0
                            n.pz=0
                        mlMesh = MeshTools.MultilevelHexahedralMesh(nnx, nny, nnz,
                                                                    n.px,n.py,n.pz,
                                                                    p.domain.x[0], p.domain.x[1], p.domain.x[2],
                                                                    p.domain.L[0], p.domain.L[1], p.domain.L[2],
                                                                    refinementLevels=n.nLevels,
                                                                    nLayersOfOverlap=n.nLayersOfOverlapForParallel,
                                                                    parallelPartitioningType=n.parallelPartitioningType)
                    else :
                        mlMesh = MeshTools.MultilevelTetrahedralMesh(nnx, nny, nnz,
                                                                     p.domain.x[0], p.domain.x[1], p.domain.x[2],
                                                                     p.L[0], p.L[1], p.L[2],
                                                                     refinementLevels=n.nLevels,
                                                                     nLayersOfOverlap=n.nLayersOfOverlapForParallel,
                                                                     parallelPartitioningType=n.parallelPartitioningType)

            elif isinstance(p.domain,Domain.PlanarStraightLineGraphDomain):
                fileprefix = None
                # run mesher
                if p.domain.use_gmsh is True:
                    fileprefix = p.domain.geofile
                    if comm.isMaster() and (p.genMesh or not (os.path.exists(fileprefix+".ele") and
                                                              os.path.exists(fileprefix+".node") and
                                                              os.path.exists(fileprefix+".edge"))):
                        if p.genMesh or not os.path.exists(fileprefix+".msh"):
                            logEvent("Running gmsh to generate 2D mesh for "+p.name,level=1)
                            gmsh_cmd = "time gmsh {0:s} -v 10 -2 -o {1:s} -format msh2".format(fileprefix+".geo", fileprefix+".msh")
                            logEvent("Calling gmsh on rank 0 with command %s" % (gmsh_cmd,))
                            check_call(gmsh_cmd, shell=True)
                            logEvent("Done running gmsh; converting to triangle")
                        else:
                            logEvent("Using "+fileprefix+".msh to convert to triangle")
                        # convert gmsh to triangle format
                        MeshTools.msh2simplex(fileprefix=fileprefix, nd=2)
                else:
                    fileprefix = p.domain.polyfile
                    if comm.isMaster() and p.genMesh:
                        logEvent("Calling Triangle to generate 2D mesh for "+p.name)
                        tricmd = "triangle -{0} -e {1}.poly".format(n.triangleOptions, fileprefix)
                        logEvent("Calling triangle on rank 0 with command %s" % (tricmd,))
                        output=check_output(tricmd, shell=True)
                        logEvent(str(output,'utf-8'))
                        logEvent("Done running triangle")
                        check_call("mv {0:s}.1.ele {0:s}.ele".format(fileprefix), shell=True)
                        check_call("mv {0:s}.1.node {0:s}.node".format(fileprefix), shell=True)
                        check_call("mv {0:s}.1.edge {0:s}.edge".format(fileprefix), shell=True)
                comm.barrier()
                assert fileprefix is not None, 'did not find mesh file name'
                # convert mesh to proteus format
                mesh = MeshTools.TriangularMesh()
                mesh.generateFromTriangleFiles(filebase=fileprefix,
                                               base=1)
                mlMesh = MeshTools.MultilevelTriangularMesh(0,0,0,skipInit=True,
                                                            nLayersOfOverlap=n.nLayersOfOverlapForParallel,
                                                            parallelPartitioningType=n.parallelPartitioningType)
                logEvent("Generating %i-level mesh from coarse Triangle mesh" % (n.nLevels,))
                mlMesh.generateFromExistingCoarseMesh(mesh,n.nLevels,
                                                      nLayersOfOverlap=n.nLayersOfOverlapForParallel,
                                                      parallelPartitioningType=n.parallelPartitioningType)

            elif isinstance(p.domain,Domain.PiecewiseLinearComplexDomain):
                from subprocess import call
                import sys
                if p.domain.use_gmsh is True:
                    fileprefix = p.domain.geofile
                else:
                    fileprefix = p.domain.polyfile
                if comm.rank() == 0 and (p.genMesh or not (os.path.exists(fileprefix+".ele") and
                                                           os.path.exists(fileprefix+".node") and
                                                           os.path.exists(fileprefix+".face"))):
                    if p.domain.use_gmsh is True:
                        if p.genMesh or not os.path.exists(fileprefix+".msh"):
                            logEvent("Running gmsh to generate 3D mesh for "+p.name,level=1)
                            gmsh_cmd = "time gmsh {0:s} -v 10 -3 -o {1:s} -format msh2".format(fileprefix+'.geo', p.domain.geofile+'.msh')
                            logEvent("Calling gmsh on rank 0 with command %s" % (gmsh_cmd,))
                            check_call(gmsh_cmd, shell=True)
                            logEvent("Done running gmsh; converting to tetgen")
                        else:
                            logEvent("Using "+p.domain.geofile+".msh to convert to tetgen")
                        MeshTools.msh2simplex(fileprefix=fileprefix, nd=3)
                        check_call("tetgen -Vfeen {0:s}.ele".format(fileprefix), shell=True)
                    else:
                        logEvent("Running tetgen to generate 3D mesh for "+p.name, level=1)
                        tetcmd = "tetgen -{0} {1}.poly".format(n.triangleOptions, fileprefix)
                        logEvent("Calling tetgen on rank 0 with command %s" % (tetcmd,))
                        check_call(tetcmd, shell=True)
                        logEvent("Done running tetgen")
                    check_call("mv {0:s}.1.ele {0:s}.ele".format(fileprefix), shell=True)
                    check_call("mv {0:s}.1.node {0:s}.node".format(fileprefix), shell=True)
                    check_call("mv {0:s}.1.face {0:s}.face".format(fileprefix), shell=True)
                    try:
                        check_call("mv {0:s}.1.neigh {0:s}.neigh".format(fileprefix), shell=True)
                    except:
                        logEvent("Warning: couldn't move {0:s}.1.neigh".format(fileprefix))
                        pass
                    try:
                        check_call("mv {0:s}.1.edge {0:s}.edge".format(fileprefix), shell=True)
                    except:
                        logEvent("Warning: couldn't move {0:s}.1.edge".format(fileprefix))
                        pass
                comm.barrier()
                logEvent("Initializing mesh and MultilevelMesh")
                nbase = 1
                mesh=MeshTools.TetrahedralMesh()
                mlMesh = MeshTools.MultilevelTetrahedralMesh(0,0,0,skipInit=True,
                                                             nLayersOfOverlap=n.nLayersOfOverlapForParallel,
                                                             parallelPartitioningType=n.parallelPartitioningType)
                if opts.generatePartitionedMeshFromFiles:
                    logEvent("Generating partitioned mesh from Tetgen files")
                    if("f" not in n.triangleOptions or "ee" not in n.triangleOptions):
                        sys.exit("ERROR: Remake the mesh with the `f` flag and `ee` flags in triangleOptions.")
                    mlMesh.generatePartitionedMeshFromTetgenFiles(fileprefix,nbase,mesh,n.nLevels,
                                                                  nLayersOfOverlap=n.nLayersOfOverlapForParallel,
                                                                  parallelPartitioningType=n.parallelPartitioningType)
                else:
                    logEvent("Generating coarse global mesh from Tetgen files")
                    mesh.generateFromTetgenFiles(fileprefix,nbase,parallel = comm.size() > 1)
                    logEvent("Generating partitioned %i-level mesh from coarse global Tetgen mesh" % (n.nLevels,))
                    mlMesh.generateFromExistingCoarseMesh(mesh,n.nLevels,
                                                          nLayersOfOverlap=n.nLayersOfOverlapForParallel,
                                                          parallelPartitioningType=n.parallelPartitioningType)
            elif isinstance(p.domain,Domain.PUMIDomain):
                import sys
                if(comm.size()>1 and p.domain.MeshOptions.parallelPartitioningType!=MeshTools.MeshParallelPartitioningTypes.element):
                  sys.exit("The mesh must be partitioned by elements and NOT nodes for adaptivity functionality. Do this with: `domain.MeshOptions.setParallelPartitioningType('element')'.")
                if comm.size() > 1 and n.conservativeFlux != None:
                    sys.exit("ERROR: Element based partitions don't have a functioning conservative flux calculation. Set conservativeFlux to None in twp_navier_stokes")
                #attach the checkpointer
                self.PUMIcheckpointer = Checkpoint.Checkpointer(self,p.domain.checkpointFrequency) 
                #ibaned: PUMI conversion #1
                if p.domain.nd == 3:
                  mesh = MeshTools.TetrahedralMesh()
                else:
                  mesh = MeshTools.TriangularMesh()
                logEvent("Converting PUMI mesh to Proteus")
                mesh.convertFromPUMI(p.domain,p.domain.PUMIMesh, p.domain.faceList,
                    p.domain.regList,
                    parallel = comm.size() > 1, dim = p.domain.nd)
                if p.domain.nd == 3:
                  mlMesh = MeshTools.MultilevelTetrahedralMesh(
                      0,0,0,skipInit=True,
                      nLayersOfOverlap=n.nLayersOfOverlapForParallel,
                      parallelPartitioningType=n.parallelPartitioningType)
                if p.domain.nd == 2:
                  mlMesh = MeshTools.MultilevelTriangularMesh(
                      0,0,0,skipInit=True,
                      nLayersOfOverlap=n.nLayersOfOverlapForParallel,
                      parallelPartitioningType=n.parallelPartitioningType)
                logEvent("Generating %i-level mesh from PUMI mesh" % (n.nLevels,))
                if comm.size()==1:
                  mlMesh.generateFromExistingCoarseMesh(
                      mesh,n.nLevels,
                      nLayersOfOverlap=n.nLayersOfOverlapForParallel,
                      parallelPartitioningType=n.parallelPartitioningType)
                else:
                  mlMesh.generatePartitionedMeshFromPUMI(
                      mesh,n.nLevels,
                      nLayersOfOverlap=n.nLayersOfOverlapForParallel)
            elif isinstance(p.domain,Domain.MeshTetgenDomain):
                nbase = 1
                mesh=MeshTools.TetrahedralMesh()
                logEvent("Reading coarse mesh from tetgen file")
                mlMesh = MeshTools.MultilevelTetrahedralMesh(0,0,0,skipInit=True,
                                                             nLayersOfOverlap=n.nLayersOfOverlapForParallel,
                                                             parallelPartitioningType=n.parallelPartitioningType)
                if opts.generatePartitionedMeshFromFiles:
                    logEvent("Generating partitioned mesh from Tetgen files")
                    mlMesh.generatePartitionedMeshFromTetgenFiles(p.domain.meshfile,nbase,mesh,n.nLevels,
                                                                  nLayersOfOverlap=n.nLayersOfOverlapForParallel,
                                                                  parallelPartitioningType=n.parallelPartitioningType)
                else:
                    logEvent("Generating coarse global mesh from Tetgen files")
                    mesh.generateFromTetgenFiles(p.domain.polyfile,nbase,parallel = comm.size() > 1)
                    logEvent("Generating partitioned %i-level mesh from coarse global Tetgen mesh" % (n.nLevels,))
                    mlMesh.generateFromExistingCoarseMesh(mesh,n.nLevels,
                                                          nLayersOfOverlap=n.nLayersOfOverlapForParallel,
                                                          parallelPartitioningType=n.parallelPartitioningType)
            elif isinstance(p.domain,Domain.Mesh3DMDomain):
                mesh=MeshTools.TetrahedralMesh()
                logEvent("Reading coarse mesh from 3DM file")
                mesh.generateFrom3DMFile(p.domain.meshfile)
                mlMesh = MeshTools.MultilevelTetrahedralMesh(0,0,0,skipInit=True,
                                                             nLayersOfOverlap=n.nLayersOfOverlapForParallel,
                                                             parallelPartitioningType=n.parallelPartitioningType)
                logEvent("Generating %i-level mesh from coarse 3DM mesh" % (n.nLevels,))
                mlMesh.generateFromExistingCoarseMesh(mesh,n.nLevels,
                                                      nLayersOfOverlap=n.nLayersOfOverlapForParallel,
                                                      parallelPartitioningType=n.parallelPartitioningType)
            elif isinstance(p.domain,Domain.Mesh2DMDomain):
                mesh=MeshTools.TriangularMesh()
                logEvent("Reading coarse mesh from 2DM file")
                mesh.generateFrom2DMFile(p.domain.meshfile)
                mlMesh = MeshTools.MultilevelTriangularMesh(0,0,0,skipInit=True,
                                                             nLayersOfOverlap=n.nLayersOfOverlapForParallel,
                                                             parallelPartitioningType=n.parallelPartitioningType)
                logEvent("Generating %i-level mesh from coarse 2DM mesh" % (n.nLevels,))
                mlMesh.generateFromExistingCoarseMesh(mesh,n.nLevels,
                                                      nLayersOfOverlap=n.nLayersOfOverlapForParallel,
                                                      parallelPartitioningType=n.parallelPartitioningType)
            elif isinstance(p.domain,Domain.MeshHexDomain):
                mesh=MeshTools.HexahedralMesh()
                logEvent("Reading coarse mesh from file")
                mesh.generateFromHexFile(p.domain.meshfile)
                mlMesh = MeshTools.MultilevelHexahedralMesh(0,0,0,skipInit=True,
                                                             nLayersOfOverlap=n.nLayersOfOverlapForParallel,
                                                             parallelPartitioningType=n.parallelPartitioningType)
                logEvent("Generating %i-level mesh from coarse mesh" % (n.nLevels,))
                mlMesh.generateFromExistingCoarseMesh(mesh,n.nLevels,
                                                      nLayersOfOverlap=n.nLayersOfOverlapForParallel,
                                                      parallelPartitioningType=n.parallelPartitioningType)
            elif isinstance(p.domain,Domain.GMSH_3D_Domain):
                from subprocess import call
                import sys
                if comm.rank() == 0 and (p.genMesh or not (os.path.exists(p.domain.polyfile+".ele") and
                                                           os.path.exists(p.domain.polyfile+".node") and
                                                           os.path.exists(p.domain.polyfile+".face"))):
                    logEvent("Running gmsh to generate 3D mesh for "+p.name,level=1)
                    gmsh_cmd = "time gmsh {0:s} -v 10 -3 -o {1:s}  -format mesh  -clmax {2:f}".format(p.domain.geofile, p.domain.name+".mesh", 0.5*p.domain.he)

                    logEvent("Calling gmsh on rank 0 with command %s" % (gmsh_cmd,))

                    check_call(gmsh_cmd, shell=True)

                    logEvent("Done running gmsh; converting to tetgen")

                    gmsh2tetgen_cmd = "gmsh2tetgen {0} {1:f} {2:d} {3:d} {4:d}".format(
                        p.domain.name+".mesh",
                        p.domain.length_scale,
                        p.domain.permute_dims[0]+1,#switch to base 1 index...
                        p.domain.permute_dims[1]+1,
                        p.domain.permute_dims[2]+1)

                    check_call(gmsh2tetgen_cmd, shell=True)
                    check_call("tetgen -Vfeen %s.ele" % ("mesh",), shell=True)
                    check_call("mv %s.1.ele %s.ele" % ("mesh","mesh"), shell=True)
                    check_call("mv %s.1.node %s.node" % ("mesh","mesh"), shell=True)
                    check_call("mv %s.1.face %s.face" % ("mesh","mesh"), shell=True)
                    check_call("mv %s.1.neigh %s.neigh" % ("mesh","mesh"), shell=True)
                    check_call("mv %s.1.edge %s.edge" % ("mesh","mesh"), shell=True)
                    elefile  = "mesh.ele"
                    nodefile = "mesh.node"
                    facefile = "mesh.face"
                    edgefile = "mesh.edge"
                    assert os.path.exists(elefile), "no mesh.ele"
                    tmp = "%s.ele" % p.domain.polyfile
                    os.rename(elefile,tmp)
                    assert os.path.exists(tmp), "no .ele"
                    assert os.path.exists(nodefile), "no mesh.node"
                    tmp = "%s.node" % p.domain.polyfile
                    os.rename(nodefile,tmp)
                    assert os.path.exists(tmp), "no .node"
                    if os.path.exists(facefile):
                        tmp = "%s.face" % p.domain.polyfile
                        os.rename(facefile,tmp)
                        assert os.path.exists(tmp), "no .face"
                    if os.path.exists(edgefile):
                        tmp = "%s.edge" % p.domain.polyfile
                        os.rename(edgefile,tmp)
                        assert os.path.exists(tmp), "no .edge"
                comm.barrier()
                logEvent("Initializing mesh and MultilevelMesh")
                nbase = 1
                mesh=MeshTools.TetrahedralMesh()
                mlMesh = MeshTools.MultilevelTetrahedralMesh(0,0,0,skipInit=True,
                                                             nLayersOfOverlap=n.nLayersOfOverlapForParallel,
                                                             parallelPartitioningType=n.parallelPartitioningType)
                if opts.generatePartitionedMeshFromFiles:
                    logEvent("Generating partitioned mesh from Tetgen files")
                    mlMesh.generatePartitionedMeshFromTetgenFiles(p.domain.polyfile,nbase,mesh,n.nLevels,
                                                                  nLayersOfOverlap=n.nLayersOfOverlapForParallel,
                                                                  parallelPartitioningType=n.parallelPartitioningType)
                else:
                    logEvent("Generating coarse global mesh from Tetgen files")
                    mesh.generateFromTetgenFiles(p.domain.polyfile,nbase,parallel = comm.size() > 1)
                    logEvent("Generating partitioned %i-level mesh from coarse global Tetgen mesh" % (n.nLevels,))
                    mlMesh.generateFromExistingCoarseMesh(mesh,n.nLevels,
                                                          nLayersOfOverlap=n.nLayersOfOverlapForParallel,
                                                          parallelPartitioningType=n.parallelPartitioningType)

            mlMesh_nList.append(mlMesh)
            if opts.viewMesh:
                logEvent("Attempting to visualize mesh")
                try:
                    from proteusGraphical import vtkViewers
                    vtkViewers.ViewMesh(mlMesh.meshList[0],viewMaterialTypes=True)
                    vtkViewers.ViewBoundaryMesh(mlMesh.meshList[0],viewBoundaryMaterialTypes=True)
                except:
                    logEvent("NumericalSolution ViewMesh failed for coarse mesh")
            for l in range(n.nLevels):
                try:
                    logEvent(mlMesh.meshList[l].meshInfo())
                except:
                    logEvent("meshInfo() method not implemented for this mesh type")
                if opts.viewMesh and opts.viewLevels and l > 0:
                    logEvent("Attempting to visualize mesh")
                    try:
                        from proteusGraphical import vtkViewers
                        vtkViewers.ViewMesh(mlMesh.meshList[l],title="mesh level %s " % l,
                                            viewMaterialTypes=True)
                        vtkViewers.ViewBoundaryMesh(mlMesh.meshList[l],title="boundary mesh level %s " % l,
                                                    viewBoundaryMaterialTypes=True)
                    except:
                        logEvent("NumericalSolution ViewMesh failed for mesh level %s" % l)

        theMesh = mlMesh.meshList[0].subdomainMesh
        pCT = self.pList[0]#self.pList[0].ct
        nCT = self.nList[0]#self.nList[0].ct
        theDomain = pCT.domain

        if hasattr(theDomain,"PUMIMesh") and not isinstance(theDomain,Domain.PUMIDomain) :
          logEvent("Reconstruct based on Proteus, convert PUMI mesh to Proteus")

          from scipy import spatial
          meshVertexTree = spatial.cKDTree(theMesh.nodeArray)
          meshVertex2Model= [0]*theMesh.nNodes_owned

          assert theDomain.vertices, "model vertices (domain.vertices) were not specified"
          assert theDomain.vertexFlags, "model classification (domain.vertexFlags) needs to be specified"

          for idx,vertex in enumerate(theDomain.vertices):
            if(pCT.nd==2 and len(vertex) == 2): #there might be a smarter way to do this
              vertex.append(0.0) #need to make a 3D coordinate
            closestVertex = meshVertexTree.query(vertex)
            meshVertex2Model[closestVertex[1]] = 1

          isModelVert = numpy.asarray(meshVertex2Model).astype("i")

          meshBoundaryConnectivity = numpy.zeros((theMesh.nExteriorElementBoundaries_global,2+pCT.nd),dtype=numpy.int32)
          for elementBdyIdx in range(len(theMesh.exteriorElementBoundariesArray)):
            exteriorIdx = theMesh.exteriorElementBoundariesArray[elementBdyIdx]
            meshBoundaryConnectivity[elementBdyIdx][0] =  theMesh.elementBoundaryMaterialTypes[exteriorIdx]
            meshBoundaryConnectivity[elementBdyIdx][1] = theMesh.elementBoundaryElementsArray[exteriorIdx][0]
            meshBoundaryConnectivity[elementBdyIdx][2] = theMesh.elementBoundaryNodesArray[exteriorIdx][0]
            meshBoundaryConnectivity[elementBdyIdx][3] = theMesh.elementBoundaryNodesArray[exteriorIdx][1]
            if(pCT.nd==3):
              meshBoundaryConnectivity[elementBdyIdx][4] = theMesh.elementBoundaryNodesArray[exteriorIdx][2]

          pCT.domain.PUMIMesh.reconstructFromProteus2(theMesh.cmesh,isModelVert,meshBoundaryConnectivity)

        if so.useOneMesh:
            for p in pList[1:]: mlMesh_nList.append(mlMesh)
            try:
                if (nList[0].MeshAdaptMesh.size_field_config() == 'isotropicProteus'):
                    mlMesh.meshList[0].subdomainMesh.size_field = numpy.ones((mlMesh.meshList[0].subdomainMesh.nNodes_global,1),'d')*1.0e-1
                if (nList[0].MeshAdaptMesh.size_field_config() == 'anisotropicProteus'):
                    mlMesh.meshList[0].subdomainMesh.size_scale = numpy.ones((mlMesh.meshList[0].subdomainMesh.nNodes_global,3),'d')
                    mlMesh.meshList[0].subdomainMesh.size_frame = numpy.ones((mlMesh.meshList[0].subdomainMesh.nNodes_global,9),'d')
            except:
                pass
        Profiling.memory("Mesh")
        from collections import OrderedDict
        self.modelSpinUp = OrderedDict()
        for p in pList:
            p.coefficients.opts = self.opts
            if p.coefficients.sdInfo == {}:
                for ci,ckDict in p.coefficients.diffusion.items():
                    for ck in list(ckDict.keys()):
                        if (ci,ck) not in p.coefficients.sdInfo:
                            p.coefficients.sdInfo[(ci,ck)] = (numpy.arange(start=0,stop=p.nd**2+1,step=p.nd,dtype='i'),
                                                              numpy.array([list(range(p.nd)) for row in range(p.nd)],dtype='i').flatten())
                            logEvent("Numerical Solution Sparse diffusion information key "+repr((ci,ck))+' = '+repr(p.coefficients.sdInfo[(ci,ck)]))
        self.sList = sList
        self.mlMesh_nList = mlMesh_nList
        self.allocateModels()
        #collect models to be used for spin up
        for index in so.modelSpinUpList:
            self.modelSpinUp[index] = self.modelList[index]
        logEvent("Finished setting up models and solvers")
        if self.opts.save_dof:
            for m in self.modelList:
                for lm in m.levelModelList:
                    for ci in range(lm.coefficients.nc):
                        lm.u[ci].dof_last = lm.u[ci].dof.copy()
        self.archiveFlag= so.archiveFlag
        logEvent("Setting up SimTools for "+p.name)
        self.simOutputList = []
        self.auxiliaryVariables = {}
        if self.simFlagsList is not None:
            for p,n,simFlags,model,index in zip(pList,nList,simFlagsList,self.modelList,list(range(len(pList)))):
                self.simOutputList.append(SimTools.SimulationProcessor(flags=simFlags,nLevels=n.nLevels,
                                                                       pFile=p,nFile=n,
                                                                       analyticalSolution=p.analyticalSolution))
                model.simTools = self.simOutputList[-1]
                self.auxiliaryVariables[model.name]= [av.attachModel(model,self.ar[index]) for av in n.auxiliaryVariables]
        else:
            for p,n,s,model,index in zip(pList,nList,sList,self.modelList,list(range(len(pList)))):
                self.simOutputList.append(SimTools.SimulationProcessor(pFile=p,nFile=n))
                model.simTools = self.simOutputList[-1]
                model.viewer = Viewers.V_base(p,n,s)
                self.auxiliaryVariables[model.name]= [av.attachModel(model,self.ar[index]) for av in n.auxiliaryVariables]
        for avList in list(self.auxiliaryVariables.values()):
            for av in avList:
                av.attachAuxiliaryVariables(self.auxiliaryVariables)
        logEvent(Profiling.memory("NumericalSolution memory",className='NumericalSolution',memSaved=memBase))
        if so.tnList is None:
            logEvent("Building tnList from model = "+pList[0].name+" nDTout = "+repr(nList[0].nDTout))
            self.tnList=[float(n)*nList[0].T/float(nList[0].nDTout)
                         for n in range(nList[0].nDTout+1)]
        else:
            logEvent("Using tnList from so = "+so.name)
            self.tnList = so.tnList
        logEvent("Time sequence"+repr(self.tnList))
        logEvent("NAHeader Num Time Steps "+repr(len(self.tnList)-1))
        logEvent("Setting "+so.name+" systemStepController to object of type "+str(so.systemStepControllerType))
        self.systemStepController = so.systemStepControllerType(self.modelList,stepExact=so.systemStepExact)
        self.systemStepController.setFromOptions(so)
        logEvent("Finished NumericalSolution initialization")

    def allocateModels(self):
        self.modelList=[]
        self.lsList=[]
        self.nlsList=[]

        for p,n,s,mlMesh,index \
            in zip(self.pList,self.nList,self.sList,self.mlMesh_nList,list(range(len(self.pList)))):

            if self.so.needEBQ_GLOBAL:
                n.needEBQ_GLOBAL = True

            if self.so.needEBQ:
                n.needEBQ = True
            ## \todo clean up tolerances: use rtol_u,atol_u and rtol_res, atol_res; allow scaling by mesh diameter
            ## \todo pass in options = (p,n) instead of using monster ctor signature
            tolList=[]
            linTolList=[]
            for l in range(n.nLevels):
                #if mlMesh.meshList[l].hasGeometricInfo != True:
                #    mlMesh.meshList[l].computeGeometricInfo()

                #fac = (mlMesh.meshList[l].h/mlMesh.meshList[0].h)**2
                fac = 1.0
                tolList.append(n.tolFac*fac)
                linTolList.append(n.linTolFac*fac)

            logEvent("Setting up MultilevelTransport for "+p.name)

            model \
                = Transport.MultilevelTransport(p,
                                                n,
                                                mlMesh,
                                                OneLevelTransportType=p.LevelModelType)
            self.modelList.append(model)

            model.name = p.name
            logEvent("Setting "+model.name+" stepController to "+str(n.stepController))
            model.stepController = n.stepController(model,n)

            Profiling.memory("MultilevelTransport for "+p.name)
            logEvent("Setting up MultilevelLinearSolver for"+p.name)

            #allow options database to set model specific parameters?
            linear_solver_options_prefix = None
            if 'linear_solver_options_prefix' in dir(n):
                linear_solver_options_prefix = n.linear_solver_options_prefix

            (multilevelLinearSolver,directSolverFlag) = LinearSolvers.multilevelLinearSolverChooser(
                linearOperatorList = model.jacobianList,
                par_linearOperatorList = model.par_jacobianList,
                multilevelLinearSolverType = n.multilevelLinearSolver,
                computeSolverRates=n.computeLinearSolverRates,
                printSolverInfo=n.printLinearSolverInfo,
                levelLinearSolverType = n.levelLinearSolver,
                computeLevelSolverRates=n.computeLevelLinearSolverRates,
                printLevelSolverInfo=n.printLevelLinearSolverInfo,
                smootherType = n.linearSmoother,
                computeSmootherRates=n.computeLinearSmootherRates,
                printSmootherInfo=n.printLinearSmootherInfo,
                prolongList = model.meshTransfers.prolongList,
                restrictList = model.meshTransfers.restrictList,
                connectivityListList = [model.levelModelList[l].sparsityInfo for l in range(n.nLevels)],
                relativeToleranceList = linTolList,
                absoluteTolerance = n.l_atol_res,
                solverMaxIts = n.linearSolverMaxIts,
                solverConvergenceTest=n.linearSolverConvergenceTest,
                cycles=n.linearWCycles,
                preSmooths=n.linearPreSmooths,
                postSmooths=n.linearPostSmooths,
                ##\todo logic needs to handle element boundary partition too
                parallelUsesFullOverlap=(n.nLayersOfOverlapForParallel > 0 or n.parallelPartitioningType == MeshTools.MeshParallelPartitioningTypes.node),
                par_duList=model.par_duList,
                solver_options_prefix=linear_solver_options_prefix,
                computeEigenvalues = n.computeEigenvalues,
                linearSmootherOptions = n.linearSmootherOptions)
            self.lsList.append(multilevelLinearSolver)
            Profiling.memory("MultilevelLinearSolver for "+p.name)
            logEvent("Setting up MultilevelNonLinearSolver for "+p.name)
            self.nlsList.append(NonlinearSolvers.multilevelNonlinearSolverChooser(
                model.levelModelList,
                model.jacobianList,
                model.par_jacobianList,
                duList=model.duList,
                par_duList=model.par_duList,
                multilevelNonlinearSolverType = n.multilevelNonlinearSolver,
                computeSolverRates=n.computeNonlinearSolverRates,
                solverConvergenceTest=n.nonlinearSolverConvergenceTest,
                levelSolverConvergenceTest=n.levelNonlinearSolverConvergenceTest,
                printSolverInfo=n.printNonlinearSolverInfo,
                relativeToleranceList = tolList,
                absoluteTolerance = n.nl_atol_res,
                levelNonlinearSolverType=n.levelNonlinearSolver,
                computeLevelSolverRates=n.computeNonlinearLevelSolverRates,
                printLevelSolverInfo=n.printNonlinearLevelSolverInfo,
                smootherType = n.nonlinearSmoother,
                computeSmootherRates=n.computeNonlinearSmootherRates,
                printSmootherInfo=n.printNonlinearSmootherInfo,
                preSmooths=n.nonlinearPreSmooths,
                postSmooths=n.nonlinearPostSmooths,
                cycles=n.nonlinearWCycles,
                maxSolverIts=n.maxNonlinearIts,
                prolong_bcList = model.meshTransfers.prolong_bcListDict,
                restrict_bcList = model.meshTransfers.restrict_bcListDict,
                restrict_bcSumList = model.meshTransfers.restrict_bcSumListDict,
                prolongList = model.meshTransfers.prolongList,
                restrictList = model.meshTransfers.restrictList,
                restrictionRowSumList = model.meshTransfers.restrictSumList,
                connectionListList=[model.levelModelList[l].sparsityInfo for l in range(n.nLevels)],
                linearSolverList=multilevelLinearSolver.solverList,
                linearDirectSolverFlag=directSolverFlag,
                solverFullNewtonFlag=n.fullNewtonFlag,
                levelSolverFullNewtonFlag=n.fullNewtonFlag,
                smootherFullNewtonFlag=n.fullNewtonFlag,
                EWtol=n.useEisenstatWalker,
                maxLSits=n.maxLineSearches,
                #\todo need to add logic in multilevel NL solver chooser to account for numerical method's stencil as well
                parallelUsesFullOverlap=(n.nLayersOfOverlapForParallel > 0 or n.parallelPartitioningType == MeshTools.MeshParallelPartitioningTypes.node),
                nonlinearSolverNorm = n.nonlinearSolverNorm))
            model.solver=self.nlsList[-1]
            model.viewer = Viewers.V_base(p,n,s)
            Profiling.memory("MultilevelNonlinearSolver for"+p.name)

    def PUMI_recomputeStructures(self,modelListOld):

        ##This section is to correct any differences in the quadrature point field from the old model

        #Shock capturing lagging needs to be matched

        import copy
        #This sections gets beta bdf right
        #self.modelList[1].levelModelList[0].u_store = copy.deepcopy(self.modelList[1].levelModelList[0].u)
        #self.modelList[1].levelModelList[0].u[0].dof[:] = self.modelList[1].levelModelList[0].u[0].dof_last
        #self.modelList[1].levelModelList[0].calculateElementResidual()
        #self.modelList[1].levelModelList[0].q[('m_last',0)][:] = self.modelList[1].levelModelList[0].q[('m_tmp',0)]

        ##this section gets numDiff right
        #self.modelList[1].levelModelList[0].u[0].dof[:] = self.modelList[1].levelModelList[0].u_store[0].dof
        #self.modelList[1].levelModelList[0].u[0].dof_last[:] = self.modelList[1].levelModelList[0].u_store[0].dof_last

        #self.modelList[1].levelModelList[0].calculateElementResidual()
        #self.modelList[1].levelModelList[0].q[('m_last',0)][:] = self.modelList[1].levelModelList[0].q[('m_tmp',0)]

        #if(modelListOld[1].levelModelList[0].shockCapturing.nStepsToDelay is not None and modelListOld[1].levelModelList[0].shockCapturing.nSteps > modelListOld[1].levelModelList[0].shockCapturing.nStepsToDelay):
        #    self.modelList[1].levelModelList[0].shockCapturing.nSteps=self.modelList[1].levelModelList[0].shockCapturing.nStepsToDelay
        #    self.modelList[1].levelModelList[0].shockCapturing.updateShockCapturingHistory()

        ###Details for solution transfer
        #To get shock capturing lagging correct, the numDiff array needs to be computed correctly with the u^{n} solution.
        #numDiff depends on the PDE residual and can depend on the subgrid error (SGE)
        #the PDE residual depends on the alpha and beta_bdf terms which depend on m_tmp from u^{n-1} as well as VOF or LS fields.
        #getResidual() is used to populate m_tmp, numDiff.
        #The goal is therefore to populate the nodal fields with the old solution, get m_tmp properly and lagged sge properly.
        #Mimic the solver stagger with a new loop to repopulate the nodal fields with u^{n} solution. This is necessary because NS relies on the u^{n-1} field for VOF/LS

        ###This loop stores the current solution (u^n) and loads in the previous timestep solution (u^{n-1}

        for m,mOld in zip(self.modelList, modelListOld):
            for lm, lu, lr, lmOld in zip(m.levelModelList, m.uList, m.rList, mOld.levelModelList):
                #lm.coefficients.postAdaptStep() #MCorr needs this at the moment
                lm.u_store = lm.u.copy()
                for ci in range(0,lm.coefficients.nc):
                    lm.u_store[ci] = lm.u[ci].copy()
                lm.dt_store = copy.deepcopy(lm.timeIntegration.dt)
                for ci in range(0,lm.coefficients.nc):
                    lm.u[ci].dof[:] = lm.u[ci].dof_last
                lm.setFreeDOF(lu)

        #All solution fields are now in state u^{n-1} and used to get m_tmp and u_sge
        for m,mOld in zip(self.modelList, modelListOld):
            for lm, lu, lr, lmOld in zip(m.levelModelList, m.uList, m.rList, mOld.levelModelList):
                lm.getResidual(lu,lr)

                #This gets the subgrid error history correct
                if(modelListOld[0].levelModelList[0].stabilization.lag and ((modelListOld[0].levelModelList[0].stabilization.nSteps - 1) > modelListOld[0].levelModelList[0].stabilization.nStepsToDelay) ):
                    self.modelList[0].levelModelList[0].stabilization.nSteps = self.modelList[0].levelModelList[0].stabilization.nStepsToDelay
                    self.modelList[0].levelModelList[0].stabilization.updateSubgridErrorHistory()

                #update the eddy-viscosity history
                lm.calculateAuxiliaryQuantitiesAfterStep()


        #shock capturing depends on m_tmp or m_last (if lagged). m_tmp is modified by mass-correction and is pushed into m_last during updateTimeHistory().
        #This leads to a situation where m_last comes from the mass-corrected solutions so post-step is needed to get this behavior.
        #If adapt is called after the first time-step, then skip the post-step for the old solution
        if( (abs(self.systemStepController.t_system_last - self.tnList[1])> 1e-12 and  abs(self.systemStepController.t_system_last - self.tnList[0])> 1e-12 ) 
          or self.opts.hotStart):

            for idx in [3,4]:
                model = self.modelList[idx]
                self.preStep(model)
                self.setWeakDirichletConditions(model)
                model.stepController.setInitialGuess(model.uList,model.rList)
                solverFailed = model.solver.solveMultilevel(uList=model.uList,
                                                    rList=model.rList,
                                                    par_uList=model.par_uList,
                                                    par_rList=model.par_rList)
                self.postStep(model)

        for m,mOld in zip(self.modelList, modelListOld):
            for lm, lu, lr, lmOld in zip(m.levelModelList, m.uList, m.rList, mOld.levelModelList):
                lm.timeIntegration.postAdaptUpdate(lmOld.timeIntegration)

                if(hasattr(lm.timeIntegration,"dtLast") and lm.timeIntegration.dtLast is not None):
                    lm.timeIntegration.dt = lm.timeIntegration.dtLast

        ###This loop reloads the current solution and the previous solution into proper places
        for m,mOld in zip(self.modelList, modelListOld):
            for lm, lu, lr, lmOld in zip(m.levelModelList, m.uList, m.rList, mOld.levelModelList):
                for ci in range(0,lm.coefficients.nc):
                    lm.u[ci].dof[:] = lm.u_store[ci].dof
                    lm.u[ci].dof_last[:] = lm.u_store[ci].dof_last

                lm.setFreeDOF(lu)
                lm.getResidual(lu,lr)

                #This gets the subgrid error history correct
                if(modelListOld[0].levelModelList[0].stabilization.lag and modelListOld[0].levelModelList[0].stabilization.nSteps > modelListOld[0].levelModelList[0].stabilization.nStepsToDelay):
                    self.modelList[0].levelModelList[0].stabilization.nSteps = self.modelList[0].levelModelList[0].stabilization.nStepsToDelay
                    self.modelList[0].levelModelList[0].stabilization.updateSubgridErrorHistory()
        ###

        ###need to re-distance and mass correct
        if( (abs(self.systemStepController.t_system_last - self.tnList[0])> 1e-12) or self.opts.hotStart  ):
            for idx in [3,4]:
                model = self.modelList[idx]
                self.preStep(model)
                self.setWeakDirichletConditions(model)
                model.stepController.setInitialGuess(model.uList,model.rList)
                solverFailed = model.solver.solveMultilevel(uList=model.uList,
                                                            rList=model.rList,
                                                            par_uList=model.par_uList,
                                                            par_rList=model.par_rList)
                self.postStep(model)

        for m,mOld in zip(self.modelList, modelListOld):
            for lm, lu, lr, lmOld in zip(m.levelModelList, m.uList, m.rList, mOld.levelModelList):

              lm.timeIntegration.postAdaptUpdate(lmOld.timeIntegration)
              lm.timeIntegration.dt = lm.dt_store

        ###Shock capturing update happens with the time history update
              if(lmOld.shockCapturing and lmOld.shockCapturing.nStepsToDelay is not None and lmOld.shockCapturing.nSteps > lmOld.shockCapturing.nStepsToDelay):
                    lm.shockCapturing.nSteps=lm.shockCapturing.nStepsToDelay
                    lm.shockCapturing.updateShockCapturingHistory()

              #update the eddy-viscosity history
              lm.calculateAuxiliaryQuantitiesAfterStep()

    def PUMI_reallocate(self,mesh):
        p0 = self.pList[0]
        n0 = self.nList[0]
        if self.TwoPhaseFlow:
            nLevels = p0.myTpFlowProblem.general['nLevels']
            nLayersOfOverlapForParallel = p0.myTpFlowProblem.general['nLayersOfOverlapForParallel']
            parallelPartitioningType = MeshTools.MeshParallelPartitioningTypes.element
            domain = p0.myTpFlowProblem.domain
            domain.MeshOptions.setParallelPartitioningType('element')
        else:
            nLevels = n0.nLevels
            nLayersOfOverlapForParallel = n0.nLayersOfOverlapForParallel
            parallelPartitioningType = n0.parallelPartitioningType
            domain = p0.domain

        logEvent("Generating %i-level mesh from PUMI mesh" % (nLevels,))
        if domain.nd == 3:
          mlMesh = MeshTools.MultilevelTetrahedralMesh(
              0,0,0,skipInit=True,
              nLayersOfOverlap=nLayersOfOverlapForParallel,
              parallelPartitioningType=parallelPartitioningType)
        if domain.nd == 2:
          mlMesh = MeshTools.MultilevelTriangularMesh(
              0,0,0,skipInit=True,
              nLayersOfOverlap=nLayersOfOverlapForParallel,
              parallelPartitioningType=parallelPartitioningType)
        if self.comm.size()==1:
            mlMesh.generateFromExistingCoarseMesh(
                mesh,nLevels,
                nLayersOfOverlap=nLayersOfOverlapForParallel,
                parallelPartitioningType=parallelPartitioningType)
        else:
            mlMesh.generatePartitionedMeshFromPUMI(
                mesh,nLevels,
                nLayersOfOverlap=nLayersOfOverlapForParallel)
        self.mlMesh_nList=[]
        for p in self.pList:
            self.mlMesh_nList.append(mlMesh)
        if (domain.PUMIMesh.size_field_config() == "isotropicProteus"):
            mlMesh.meshList[0].subdomainMesh.size_field = numpy.ones((mlMesh.meshList[0].subdomainMesh.nNodes_global,1),'d')*1.0e-1
        if (domain.PUMIMesh.size_field_config() == 'anisotropicProteus'):
            mlMesh.meshList[0].subdomainMesh.size_scale = numpy.ones((mlMesh.meshList[0].subdomainMesh.nNodes_global,3),'d')
            mlMesh.meshList[0].subdomainMesh.size_frame = numpy.ones((mlMesh.meshList[0].subdomainMesh.nNodes_global,9),'d')

        #may want to trigger garbage collection here
        self.modelListOld = self.modelList
        logEvent("Allocating models on new mesh")
        self.allocateModels()
        #logEvent("Attach auxiliary variables to new models")


    def PUMI2Proteus(self,domain):
        #p0 = self.pList[0] #This can probably be cleaned up somehow
        #n0 = self.nList[0]
        p0 = self.pList[0]
        n0 = self.nList[0]

        modelListOld = self.modelListOld
        logEvent("Attach auxiliary variables to new models")
        #(cut and pasted from init, need to cleanup)
        self.simOutputList = []
        self.auxiliaryVariables = {}
        self.newAuxiliaryVariables = {}
        if self.simFlagsList is not None:
            for p, n, simFlags, model, index in zip(
                    self.pList,
                    self.nList,
                    self.simFlagsList,
                    self.modelList,
                    list(range(len(self.pList)))):
                self.simOutputList.append(
                    SimTools.SimulationProcessor(
                        flags=simFlags,
                        nLevels=n.nLevels,
                        pFile=p,
                        nFile=n,
                        analyticalSolution=p.analyticalSolution))
                model.simTools = self.simOutputList[-1]

                #Code to refresh attached gauges. The goal is to first purge
                #existing point gauge node associations as that may have changed
                #If there is a line gauge, then all the points must be deleted
                #and remade.
                from collections import OrderedDict
                for av in n.auxiliaryVariables:
                  if hasattr(av,'adapted'):
                    av.adapted=True
                    for point, l_d in av.points.items():
                      if 'nearest_node' in l_d:
                        l_d.pop('nearest_node')
                    if(av.isLineGauge or av.isLineIntegralGauge): #if line gauges, need to remove all points
                      av.points = OrderedDict()
                    if(av.isGaugeOwner):
                      if(self.comm.rank()==0 and not av.file.closed):
                        av.file.close()
                      for item in av.pointGaugeVecs:
                        item.destroy()
                      for item in av.pointGaugeMats:
                        item.destroy()
                      for item in av.dofsVecs:
                        item.destroy()

                      av.pointGaugeVecs = []
                      av.pointGaugeMats = []
                      av.dofsVecs = []
                      av.field_ids=[]
                      av.isGaugeOwner=False
                ##reinitialize auxiliaryVariables
                self.auxiliaryVariables[model.name]= [av.attachModel(model,self.ar[index]) for av in n.auxiliaryVariables]
        else:
            for p,n,s,model,index in zip(
                    self.pList,
                    self.nList,
                    self.sList,
                    self.modelList,
                    list(range(len(self.pList)))):
                self.simOutputList.append(SimTools.SimulationProcessor(pFile=p,nFile=n))
                model.simTools = self.simOutputList[-1]
                model.viewer = Viewers.V_base(p,n,s)
                self.auxiliaryVariables[model.name]= [av.attachModel(model,self.ar[index]) for av in n.auxiliaryVariables]
        for avList in list(self.auxiliaryVariables.values()):
            for av in avList:
                av.attachAuxiliaryVariables(self.auxiliaryVariables)

        logEvent("Transfering fields from PUMI to Proteus")
        for m in self.modelList:
          for lm in m.levelModelList:
            coef = lm.coefficients
            if coef.vectorComponents is not None:
              vector=numpy.zeros((lm.mesh.nNodes_global,3),'d')
              domain.PUMIMesh.transferFieldToProteus(
                     coef.vectorName, vector)
              for vci in range(len(coef.vectorComponents)):
                lm.u[coef.vectorComponents[vci]].dof[:] = vector[:,vci]
              domain.PUMIMesh.transferFieldToProteus(
                     coef.vectorName+"_old", vector)
              for vci in range(len(coef.vectorComponents)):
                lm.u[coef.vectorComponents[vci]].dof_last[:] = vector[:,vci]
              domain.PUMIMesh.transferFieldToProteus(
                     coef.vectorName+"_old_old", vector)
              for vci in range(len(coef.vectorComponents)):
                lm.u[coef.vectorComponents[vci]].dof_last_last[:] = vector[:,vci]

              del vector
            for ci in range(coef.nc):
              if coef.vectorComponents is None or \
                 ci not in coef.vectorComponents:
                scalar=numpy.zeros((lm.mesh.nNodes_global,1),'d')
                domain.PUMIMesh.transferFieldToProteus(
                    coef.variableNames[ci], scalar)
                lm.u[ci].dof[:] = scalar[:,0]
                domain.PUMIMesh.transferFieldToProteus(
                    coef.variableNames[ci]+"_old", scalar)
                lm.u[ci].dof_last[:] = scalar[:,0]
                domain.PUMIMesh.transferFieldToProteus(
                    coef.variableNames[ci]+"_old_old", scalar)
                lm.u[ci].dof_last_last[:] = scalar[:,0]

                del scalar

        logEvent("Attaching models on new mesh to each other")
        for m,ptmp,mOld in zip(self.modelList, self.pList, modelListOld):
            for lm, lu, lr, lmOld in zip(m.levelModelList, m.uList, m.rList,mOld.levelModelList):
                #save_dof=[]
                #for ci in range(lm.coefficients.nc):
                #    save_dof.append( lm.u[ci].dof.copy())
                #    lm.u[ci].dof_last = lm.u[ci].dof.copy()
                lm.setFreeDOF(lu)
                #for ci in range(lm.coefficients.nc):
                #    assert((save_dof[ci] == lm.u[ci].dof).all())
                lm.calculateSolutionAtQuadrature()
                lm.timeIntegration.tLast = lmOld.timeIntegration.tLast
                lm.timeIntegration.t = lmOld.timeIntegration.t
                lm.timeIntegration.dt = lmOld.timeIntegration.dt
                assert(lmOld.timeIntegration.tLast == lm.timeIntegration.tLast)
                assert(lmOld.timeIntegration.t == lm.timeIntegration.t)
                assert(lmOld.timeIntegration.dt == lm.timeIntegration.dt)
            m.stepController.dt_model = mOld.stepController.dt_model
            m.stepController.t_model = mOld.stepController.t_model
            m.stepController.t_model_last = mOld.stepController.t_model_last
            m.stepController.substeps = mOld.stepController.substeps

        #if first time-step / initial adapt & not hotstarted
        if(abs(self.systemStepController.t_system_last - self.tnList[0])< 1e-12 and not self.opts.hotStart):
            for index,p,n,m,simOutput in zip(range(len(self.modelList)),self.pList,self.nList,self.modelList,self.simOutputList):
                if p.initialConditions is not None:
                    logEvent("Setting initial conditions for "+p.name)
                    m.setInitialConditions(p.initialConditions,self.tnList[0])
 

        #Attach models and do sample residual calculation. The results are usually irrelevant.
        #What's important right now is to re-establish the relationships between data structures.
        #The necessary values will be written in later.
        for m,ptmp,mOld in zip(self.modelList, self.pList, modelListOld):
            logEvent("Attaching models to model "+ptmp.name)
            m.attachModels(self.modelList)
        logEvent("Evaluating residuals and time integration")

        for m,ptmp,mOld in zip(self.modelList, self.pList, modelListOld):
            for lm, lu, lr, lmOld in zip(m.levelModelList, m.uList, m.rList, mOld.levelModelList):
                lm.timeTerm=True
                lm.getResidual(lu,lr)
                lm.timeIntegration.initializeTimeHistory(resetFromDOF=True)
                lm.initializeTimeHistory()
                lm.timeIntegration.initializeSpaceHistory()
                lm.getResidual(lu,lr)
                #lm.estimate_mt() #function is empty in all models
            assert(m.stepController.dt_model == mOld.stepController.dt_model)
            assert(m.stepController.t_model == mOld.stepController.t_model)
            assert(m.stepController.t_model_last == mOld.stepController.t_model_last)
            logEvent("Initializing time history for model step controller")
            if(not self.opts.hotStart):
              m.stepController.initializeTimeHistory()
        #p0.domain.initFlag=True #For next step to take initial conditions from solution, only used on restarts

            #m.stepController.initializeTimeHistory()
        #domain.initFlag=True #For next step to take initial conditions from solution, only used on restarts

        self.systemStepController.modelList = self.modelList
        self.systemStepController.exitModelStep = {}
        self.systemStepController.controllerList = []
        for model in self.modelList:
            self.systemStepController.exitModelStep[model] = False
            if model.levelModelList[-1].timeIntegration.isAdaptive:
                self.systemStepController.controllerList.append(model)
                self.systemStepController.maxFailures = model.stepController.maxSolverFailures

        #this sets the timeIntegration time, which might be unnecessary for restart 
        if(self.opts.hotStart):
          self.systemStepController.stepSequence=[(self.systemStepController.t_system,m) for m in self.systemStepController.modelList]
        else:
          self.systemStepController.choose_dt_system()

        #Don't do anything if this is the initial adapt
        if(abs(self.systemStepController.t_system_last - self.tnList[0])> 1e-12  or
          (abs(self.systemStepController.t_system_last - self.tnList[0]) < 1e-12 and self.opts.hotStart)):
            self.PUMI_recomputeStructures(modelListOld)

            #something different is needed for initial conditions
            #do nothing if archive sequence step because there will be an archive
            #if self.archiveFlag != ArchiveFlags.EVERY_SEQUENCE_STEP:
            #  self.tCount+=1
            #  for index,model in enumerate(self.modelList):
            #    #import pdb; pdb.set_trace()
            #    self.archiveSolution(
            #      model,
            #      index,
            #      #self.systemStepController.t_system_last+1.0e-6)
            #      self.systemStepController.t_system)
  
            #This logic won't account for if final step doesn't match frequency or if adapt isn't being called
            if((self.PUMIcheckpointer.frequency>0) and ( (domain.PUMIMesh.nAdapt()!=0) and (domain.PUMIMesh.nAdapt() % self.PUMIcheckpointer.frequency==0 ) or self.systemStepController.t_system_last==self.tnList[-1])):

              self.PUMIcheckpointer.checkpoint(self.systemStepController.t_system_last)

        #del modelListOld to free up memory
        del modelListOld
        import gc;
        gc.disable()
        gc.collect()
        self.comm.barrier()

    def PUMI_transferFields(self):
        p0 = self.pList[0]
        n0 = self.nList[0]

        if self.TwoPhaseFlow:
            domain = p0.myTpFlowProblem.domain
            rho_0 = p0.myTpFlowProblem.physical_parameters['densityA']
            nu_0 = p0.myTpFlowProblem.physical_parameters['kinematicViscosityA']
            rho_1 = p0.myTpFlowProblem.physical_parameters['densityB']
            nu_1 = p0.myTpFlowProblem.physical_parameters['kinematicViscosityB']
            g = p0.myTpFlowProblem.physical_parameters['gravity']
            epsFact_density = p0.myTpFlowProblem.clsvof_parameters['epsFactHeaviside']
        else:
            domain = p0.domain
            rho_0  = p0.rho_0
            nu_0   = p0.nu_0
            rho_1  = p0.rho_1
            nu_1   = p0.nu_1
            g      = p0.g
            epsFact_density = p0.epsFact_density
        logEvent("Copying coordinates to PUMI")
        domain.PUMIMesh.transferFieldToPUMI("coordinates",
            self.modelList[0].levelModelList[0].mesh.nodeArray)

        #put the solution field as uList
        #VOF and LS needs to reset the u.dof array for proper transfer
        #but needs to be returned to the original form if not actually adapting....be careful with the following statements, unsure if this doesn't break something else 
        import copy
        for m in self.modelList:
            for lm in m.levelModelList:
                lm.u_store = lm.u.copy()
                for ci in range(0,lm.coefficients.nc):
                    lm.u_store[ci] = lm.u[ci].copy()

        self.modelList[1].levelModelList[0].setUnknowns(self.modelList[1].uList[0])
        self.modelList[2].levelModelList[0].setUnknowns(self.modelList[2].uList[0])

        logEvent("Copying DOF and parameters to PUMI")
        for m in self.modelList:
          for lm in m.levelModelList:
            coef = lm.coefficients
            if coef.vectorComponents is not None:
              vector=numpy.zeros((lm.mesh.nNodes_global,3),'d')
              for vci in range(len(coef.vectorComponents)):
                vector[:,vci] = lm.u[coef.vectorComponents[vci]].dof[:]

              domain.PUMIMesh.transferFieldToPUMI(
                  coef.vectorName, vector)
              #Transfer dof_last
              for vci in range(len(coef.vectorComponents)):
                vector[:,vci] = lm.u[coef.vectorComponents[vci]].dof_last[:]
              domain.PUMIMesh.transferFieldToPUMI(
                     coef.vectorName+"_old", vector)
              #Transfer dof_last_last
              for vci in range(len(coef.vectorComponents)):
                vector[:,vci] = lm.u[coef.vectorComponents[vci]].dof_last_last[:]
              p0.domain.PUMIMesh.transferFieldToPUMI(
                     coef.vectorName+"_old_old", vector)

              del vector
            for ci in range(coef.nc):
              if coef.vectorComponents is None or \
                 ci not in coef.vectorComponents:
                scalar=numpy.zeros((lm.mesh.nNodes_global,1),'d')
                scalar[:,0] = lm.u[ci].dof[:]
                domain.PUMIMesh.transferFieldToPUMI(
                    coef.variableNames[ci], scalar)

                #Transfer dof_last
                scalar[:,0] = lm.u[ci].dof_last[:]
                domain.PUMIMesh.transferFieldToPUMI(
                     coef.variableNames[ci]+"_old", scalar)
                #Transfer dof_last_last
                scalar[:,0] = lm.u[ci].dof_last_last[:]
                p0.domain.PUMIMesh.transferFieldToPUMI(
                     coef.variableNames[ci]+"_old_old", scalar)

                del scalar

        scalar=numpy.zeros((lm.mesh.nNodes_global,1),'d')

        del scalar
        #Get Physical Parameters
        #Can we do this in a problem-independent  way?
        
        rho = numpy.array([rho_0,
                           rho_1])
        nu = numpy.array([nu_0,
                          nu_1])
        g = numpy.asarray(g)
        
        #This condition is to account for adapting before the simulation started
        if(hasattr(self,"tn")):
            #deltaT = self.tn-self.tn_last
            #is actually the time step for next step, self.tn and self.tn_last refer to entries in tnList
            deltaT = self.systemStepController.dt_system 
        else:
            deltaT = 0

        epsFact = epsFact_density
        domain.PUMIMesh.transferPropertiesToPUMI(rho,nu,g,deltaT,epsFact)

        del rho, nu, g, epsFact


    def PUMI_estimateError(self):
        """
        Estimate the error using the classical element residual method by
        Ainsworth and Oden and generates a corresponding error field.
        """

        p0 = self.pList[0]
        n0 = self.nList[0]
        #p0 = self.pList[0].ct
        #n0 = self.nList[0].ct

        adaptMeshNow = False
        #will need to move this to earlier when the mesh is created
        #from proteus.MeshAdaptPUMI import MeshAdaptPUMI
        #if not hasattr(p0.domain,'PUMIMesh') and not isinstance(p0.domain,Domain.PUMIDomain) and p0.domain.PUMIMesh.adaptMesh():
        #    import sys
        #    if(self.comm.size()>1 and p0.domain.MeshOptions.parallelPartitioningType!=MeshTools.MeshParallelPartitioningTypes.element):
        #        sys.exit("The mesh must be partitioned by elements and NOT nodes for adaptivity functionality. Do this with: `domain.MeshOptions.setParallelPartitioningType('element')'.")
        #    p0.domain.PUMIMesh=n0.MeshAdaptMesh
            #p0.domain.hasModel = n0.useModel
            #numModelEntities = numpy.array([len(p0.domain.vertices),len(p0.domain.segments),len(p0.domain.facets),len(p0.domain.regions)]).astype("i")
            ##force initialization of arrays to enable passage through to C++ code
            #mesh2Model_v= numpy.asarray([[0,0]]).astype("i")
            #mesh2Model_e=numpy.asarray([[0,0]]).astype("i")
            #mesh2Model_b=numpy.asarray([[0,0]]).astype("i")

            #segmentList = numpy.asarray([[0,0]]).astype("i")
            #newFacetList = numpy.asarray([[0,0]]).astype("i")
            ##only appropriate for 2D use at the moment
            #if p0.domain.vertices and p0.domain.hasModel and p0.domain.nd==2:
            #  p0.domain.getMesh2ModelClassification(self.modelList[0].levelModelList[0].mesh)
            #  segmentList = numpy.asarray(p0.domain.segments).astype("i")
            #  #force initialize the unused arrays for proper cythonization
            #  import copy
            #  newFacetList = []
            #  if(not p0.domain.facets):
            #    p0.domain.facets = [(-1,-1)]
            #    newFacetList = copy.deepcopy(p0.domain.facets)
            #  else:
            #    facetList = []
            #    maxFacetLength = 0
            #    numHoles = len(p0.domain.holes)
            #    if(numHoles): #if there are holes, there can be multiple lists of facets
            #      for i in range(numHoles,len(p0.domain.facets)):
            #        for j in range(len(p0.domain.facets[i])):
            #          maxFacetLength = max(maxFacetLength,len(p0.domain.facets[i][j]))
            #      for i in range(numHoles,len(p0.domain.facets)):
            #        facetList.append(list(p0.domain.facets[i][0]))
            #        if(len(p0.domain.facets[i][0])<maxFacetLength):
            #          initLength = len(p0.domain.facets[i][0])
            #          lenDiff = maxFacetLength-initLength
            #          for j in range(lenDiff):
            #            facetList[i-numHoles].append(-1)
            #    else:
            #      for i in range(len(p0.domain.facets)):
            #        maxFacetLength = max(maxFacetLength,len(p0.domain.facets[i]))
            #      for i in range(len(p0.domain.facets)):
            #        facetList.append(list(p0.domain.facets[i]))
            #        if(len(p0.domain.facets[i])<maxFacetLength):
            #          initLength = len(p0.domain.facets[i])
            #          lenDiff = maxFacetLength-initLength
            #          for j in range(lenDiff):
            #            facetList[i-numHoles].append(-1)

            #    #substitute the vertex IDs with segment IDs
            #    newFacetList = copy.deepcopy(facetList)
            #    for i in range(len(facetList)):
            #      for j in range(maxFacetLength):
            #        if(j==maxFacetLength-1 or facetList[i][j+1]==-1):
            #          testSegment = [facetList[i][j],facetList[i][0]]
            #        else:
            #          testSegment = [facetList[i][j],facetList[i][j+1]]
            #        try:
            #          edgIdx = p0.domain.segments.index(testSegment)
            #        except ValueError:
            #          edgIdx = p0.domain.segments.index(list(reversed(testSegment)))
            #        newFacetList[i][j] = edgIdx
            #        if(j==maxFacetLength-1 or facetList[i][j+1]==-1):
            #          break
            #  newFacetList = numpy.asarray(newFacetList).astype("i")
            #  mesh2Model_v = numpy.asarray(p0.domain.meshVertex2Model).astype("i")
            #  mesh2Model_e = numpy.asarray(p0.domain.meshEdge2Model).astype("i")
            #  mesh2Model_b = numpy.asarray(p0.domain.meshBoundary2Model).astype("i")

            #p0.domain.PUMIMesh.transferModelInfo(numModelEntities,segmentList,newFacetList,mesh2Model_v,mesh2Model_e,mesh2Model_b)
            #p0.domain.PUMIMesh.reconstructFromProteus(self.modelList[0].levelModelList[0].mesh.cmesh,self.modelList[0].levelModelList[0].mesh.globalMesh.cmesh,p0.domain.hasModel)

        if self.TwoPhaseFlow:
            domain = p0.myTpFlowProblem.domain
        else:
            domain = p0.domain

        if (hasattr(domain, 'PUMIMesh') and
            domain.PUMIMesh.adaptMesh() and
            self.so.useOneMesh): #and
            #self.nSolveSteps%domain.PUMIMesh.numAdaptSteps()==0):
            if (domain.PUMIMesh.size_field_config() == "isotropicProteus"):
                domain.PUMIMesh.transferFieldToPUMI("proteus_size",
                                                       self.modelList[0].levelModelList[0].mesh.size_field)
            if (domain.PUMIMesh.size_field_config() == 'anisotropicProteus'):
                #Insert a function to define the size_scale/size_frame fields here.
                #For a given vertex, the i-th size_scale is roughly the desired edge length along the i-th direction specified by the size_frame
                for i in range(len(self.modelList[0].levelModelList[0].mesh.size_scale)):
                  self.modelList[0].levelModelList[0].mesh.size_scale[i,0] =  1e-1
                  self.modelList[0].levelModelList[0].mesh.size_scale[i,1] =  (old_div(self.modelList[0].levelModelList[0].mesh.nodeArray[i,1],0.584))*1e-1
                  for j in range(3):
                    for k in range(3):
                      if(j==k):
                        self.modelList[0].levelModelList[0].mesh.size_frame[i,3*j+k] = 1.0
                      else:
                        self.modelList[0].levelModelList[0].mesh.size_frame[i,3*j+k] = 0.0
                self.modelList[0].levelModelList[0].mesh.size_scale
                domain.PUMIMesh.transferFieldToPUMI("proteus_sizeScale", self.modelList[0].levelModelList[0].mesh.size_scale)
                domain.PUMIMesh.transferFieldToPUMI("proteus_sizeFrame", self.modelList[0].levelModelList[0].mesh.size_frame)

            self.PUMI_transferFields()


            logEvent("Estimate Error")
            sfConfig = domain.PUMIMesh.size_field_config()
            if(sfConfig=="ERM"):
              errorTotal= domain.PUMIMesh.get_local_error()
              if(domain.PUMIMesh.willAdapt()):
                adaptMeshNow=True
                logEvent("Need to Adapt")
            elif(sfConfig=="VMS" or sfConfig=="combined"):
              errorTotal = p0.domain.PUMIMesh.get_VMS_error()
              if(p0.domain.PUMIMesh.willAdapt()):
                adaptMeshNow=True
                logEvent("Need to Adapt")
            elif(sfConfig=='interface' ):
              adaptMeshNow=True
              logEvent("Need to Adapt")
            elif(sfConfig=='isotropic'):
              if(p0.domain.PUMIMesh.willInterfaceAdapt()):
                  adaptMeshNow=True
                  logEvent("Need to Adapt")
                  logEvent('numSolveSteps %f ' % self.nSolveSteps)
            elif(sfConfig=='meshQuality'):
              minQual = domain.PUMIMesh.getMinimumQuality()
              logEvent('The quality is %f ' % (minQual**(1./3.)))
              #adaptMeshNow=True
              if(minQual**(1./3.)<0.25):
                adaptMeshNow=True
                logEvent("Need to Adapt")
              
              if (self.auxiliaryVariables['rans2p'][0].subcomponents[0].__class__.__name__== 'ProtChBody'):
                sphereCoords = numpy.asarray(self.auxiliaryVariables['rans2p'][0].subcomponents[0].position)
                domain.PUMIMesh.updateSphereCoordinates(sphereCoords)
                logEvent("Updated the sphere coordinates %f %f %f" % (sphereCoords[0],sphereCoords[1],sphereCoords[2]))
              else:
                sys.exit("Haven't been implemented code yet to cover this behavior.")
            else:
              adaptMeshNow=True
              logEvent("Need to Adapt")
            #if not adapting need to return data structures to original form which was modified by PUMI_transferFields()
            if(adaptMeshNow == False):
                for m in self.modelList:
                    for lm in m.levelModelList:
                        lm.u[0].dof[:]=lm.u_store[0].dof
                
        return adaptMeshNow


    def PUMI_adaptMesh(self,inputString=""):
        """
        Uses a computed error field to construct a size field and adapts
        the mesh using SCOREC tools (a.k.a. MeshAdapt)
        """
        ##
        ## zhang-alvin's BC communication for N-S error estimation
        ##
        #  #for idx in range (0, self.modelList[0].levelModelList[0].coefficients.nc):
        #    #if idx>0:
        #    #    diff_flux = self.modelList[0].levelModelList[0].ebqe[('diffusiveFlux_bc',idx,idx)]
        #    #else:
        #    #    diff_flux = numpy.empty([2,2]) #dummy diff flux
        #    #p.domain.PUMIMesh.transferBCtagsToProteus(
        #    #    self.modelList[0].levelModelList[0].numericalFlux.isDOFBoundary[idx],
        #    #    idx,
        #    #    self.modelList[0].levelModelList[0].numericalFlux.mesh.exteriorElementBoundariesArray,
        #    #    self.modelList[0].levelModelList[0].numericalFlux.mesh.elementBoundaryElementsArray,
        #    #    diff_flux)
        #    #p.domain.PUMIMesh.transferBCtagsToProteus(
        #    #    self.modelList[0].levelModelList[0].numericalFlux.isDiffusiveFluxBoundary[idx],
        #    #    idx,
        #    #    self.modelList[0].levelModelList[0].numericalFlux.mesh.exteriorElementBoundariesArray,
        #    #    self.modelList[0].levelModelList[0].numericalFlux.mesh.elementBoundaryElementsArray,
        #    #    diff_flux)

        p0 = self.pList[0]#.ct
        n0 = self.nList[0]#.ct

        if self.TwoPhaseFlow:
            domain = p0.myTpFlowProblem.domain
        else:
            domain = p0.domain

        sfConfig = domain.PUMIMesh.size_field_config()
        if(hasattr(self,"nSolveSteps")):
          logEvent("h-adapt mesh by calling AdaptPUMIMesh at step %s" % self.nSolveSteps)        
        if(sfConfig=="pseudo"):
            logEvent("Testing solution transfer and restart feature of adaptation. No actual mesh adaptation!")
        else:
            domain.PUMIMesh.adaptPUMIMesh(inputString)

        #code to suggest adapting until error is reduced;
        #not fully baked and can lead to infinite loops of adaptation
        #if(sfConfig=="ERM"):
        #  domain.PUMIMesh.get_local_error()
        #  while(domain.PUMIMesh.willAdapt()):
        #    domain.PUMIMesh.adaptPUMIMesh()
        #    domain.PUMIMesh.get_local_error()

        logEvent("Converting PUMI mesh to Proteus")
        #ibaned: PUMI conversion #2
        #TODO: this code is nearly identical to
        #PUMI conversion #1, they should be merged
        #into a function
        if domain.nd == 3:
          mesh = MeshTools.TetrahedralMesh()
        else:
          mesh = MeshTools.TriangularMesh()

        mesh.convertFromPUMI(domain,
                             domain.PUMIMesh,
                             domain.faceList,
                             domain.regList,
                             parallel = self.comm.size() > 1,
                             dim = domain.nd)
  
        self.PUMI_reallocate(mesh)
        self.PUMI2Proteus(domain)
      ##chitak end Adapt

    ## compute the solution

    def hotstartWithPUMI(self):
      #Call restart functions
      logEvent("Converting PUMI mesh to Proteus")
      if self.pList[0].domain.nd == 3:
        mesh = MeshTools.TetrahedralMesh()
      else:
        mesh = MeshTools.TriangularMesh()

      mesh.convertFromPUMI(self.pList[0].domain.PUMIMesh,
                             self.pList[0].domain.faceList,
                             self.pList[0].domain.regList,
                             parallel = self.comm.size() > 1,
                             dim = self.pList[0].domain.nd)

      if(self.pList[0].domain.checkpointInfo==None):
        sys.exit("Need to specify checkpointInfo file in inputs")
      else:
        self.PUMIcheckpointer.DecodeModel(self.pList[0].domain.checkpointInfo)
      
      self.PUMI_reallocate(mesh) #need to double check if this call is necessaryor if it can be simplified to a shorter call
      self.PUMI2Proteus(self.pList[0].domain)

    def calculateSolution(self,runName):
        """ Cacluate the PDEs numerical solution.

        Parameters
        ----------
        runName : str
            A name for the calculated solution.
        """

        #Get mesh entities for reconstruction
        #theMesh = self.modelList[0].levelModelList[0].mesh
        #from scipy import spatial
        #meshVertexTree = spatial.cKDTree(theMesh.nodeArray)
        #meshVertex2Model= [0]*theMesh.nNodes_owned
        #file0 = open('modelNodeArray.csv','w')
        #file0.write('%i\n' % len(self.pList[0].domain.vertices))
        #for idx,vertex in enumerate(self.pList[0].domain.vertices):
        #  #if(self.nd==2 and len(vertex) == 2): #there might be a smarter way to do this
        #  #  vertex.append(0.0) #need to make a 3D coordinate
        #  closestVertex = meshVertexTree.query(vertex)
        #  #file0.write('%i, %i\n' % (closestVertex[1],theMesh.nodeMaterialTypes[closestVertex[1]]))
        #  file0.write('%i, %i\n' % (closestVertex[1],idx))
        #file0.close()

        #file1 = open('meshNodeArray.csv','w')
        #file1.write('%i\n' % theMesh.nNodes_owned)
        #for nodeIdx in range(len(theMesh.nodeArray)):
        #  file1.write('%i, %.15f, %.15f, %.15f\n' % (nodeIdx,
        #     theMesh.nodeArray[nodeIdx][0],
        #     theMesh.nodeArray[nodeIdx][1],
        #     theMesh.nodeArray[nodeIdx][2]))
        #file1.close()
        #file2 = open('meshConnectivity.csv','w')
        #file2.write('%i\n' % theMesh.nElements_owned)
        #for elementIdx in range(len(theMesh.elementNodesArray)):
        #  file2.write('%i, %i, %i, %i, %i\n' % (elementIdx, theMesh.elementNodesArray[elementIdx][0],
        #     theMesh.elementNodesArray[elementIdx][1], theMesh.elementNodesArray[elementIdx][2],
        #     theMesh.elementNodesArray[elementIdx][3]))
        #file2.close()
        #file3 = open('meshBoundaryConnectivity.csv','w')
        #file3.write('%i\n' % theMesh.nExteriorElementBoundaries_global)
        #for elementBdyIdx in range(len(theMesh.exteriorElementBoundariesArray)):
        #  exteriorIdx = theMesh.exteriorElementBoundariesArray[elementBdyIdx]
        #  file3.write('%i, %i, %i, %i, %i, %i\n' % (exteriorIdx,
        #     theMesh.elementBoundaryMaterialTypes[exteriorIdx],
        #     theMesh.elementBoundaryElementsArray[exteriorIdx][0], #should be adjacent to only one boundary
        #     theMesh.elementBoundaryNodesArray[exteriorIdx][0],
        #     theMesh.elementBoundaryNodesArray[exteriorIdx][1],
        #     theMesh.elementBoundaryNodesArray[exteriorIdx][2],
        #      ))
        #file3.close()
        #exit()

        logEvent("Setting initial conditions",level=0)
        for index,p,n,m,simOutput in zip(list(range(len(self.modelList))),self.pList,self.nList,self.modelList,self.simOutputList):
            if self.opts.hotStart:
                logEvent("Setting initial conditions from hot start file for "+p.name)
                tCount = int(self.ar[index].tree.getroot()[-1][-1][-1][0].attrib['Name'])
                offset=0
                while tCount > 0:
                    time = float(self.ar[index].tree.getroot()[-1][-1][-1-offset][0].attrib['Value'])
                    if time <= self.opts.hotStartTime:
                        break
                    else:
                        tCount -=1
                        offset +=1
                self.ar[index].n_datasets = tCount + 1
                if len(self.ar[index].tree.getroot()[-1][-1]) - offset - 1 > 0:
                    dt = time - float(self.ar[index].tree.getroot()[-1][-1][-1-offset-1][0].attrib['Value'])
                else:
                    logEvent("Not enough steps in hot start file set set dt, setting dt to 1.0")
                    dt = 1.0
                logEvent("Hot starting from time step t = "+repr(time))
                #the number of nodes in an adapted mesh is not necessarily going to be the same as that of the solution field when archived...but it's not important because things should be bookkept correctly later on
                #if not isinstance(p.domain,Domain.PUMIDomain):

                for lm,lu,lr in zip(m.levelModelList,m.uList,m.rList):
                    for cj in range(lm.coefficients.nc): 

                        if not isinstance(self.pList[0].domain,Domain.PUMIDomain):
                          lm.u[cj].femSpace.readFunctionXdmf(self.ar[index],lm.u[cj],tCount)
                        lm.setFreeDOF(lu)
                        lm.timeIntegration.tLast = time
                        lm.timeIntegration.t = time
                        lm.timeIntegration.dt = dt
                self.tCount = tCount
            elif p.initialConditions is not None:
                logEvent("Setting initial conditions for "+p.name)
                m.setInitialConditions(p.initialConditions,self.tnList[0])
                #It's only safe to calculate the solution and solution
                #gradients because the models aren't attached yet
                for lm in m.levelModelList:
                    lm.calculateSolutionAtQuadrature()
            else:
                logEvent("No initial conditions provided for model "+p.name)
        if self.opts.hotStart:
            if time >= self.tnList[-1] - 1.0e-5:
                logEvent("Modifying time interval to be tnList[-1] + tnList since tnList hasn't been modified already")
                ndtout = len(self.tnList)
                self.tnList = [time + i for i in self.tnList]
                self.tnList.insert(1, 0.9*self.tnList[0]+0.1*self.tnList[1])

            else:
                tnListNew=[time]
                for n,t in enumerate(self.tnList):
                    if time < t-1.0e-8:
                        tnListNew.append(t)
                self.tnList=tnListNew
                logEvent("Hotstarting, new tnList is"+repr(self.tnList))
        else:
            self.tCount=0#time step counter


        logEvent("Attaching models and running spin-up step if requested")
        self.firstStep = True ##\todo get rid of firstStep flag in NumericalSolution if possible?
        spinup = []
        if (not self.opts.hotStart) or (not self.so.skipSpinupOnHotstart):
            for index,m in self.modelSpinUp.items():
                spinup.append((self.pList[index],self.nList[index],m,self.simOutputList[index]))
        for index,m in enumerate(self.modelList):
            logEvent("Attaching models to model "+m.name)
            m.attachModels(self.modelList)
            if index not in self.modelSpinUp:
                spinup.append((self.pList[index],self.nList[index],m,self.simOutputList[index]))
        for m in self.modelList:
            for lm,lu,lr in zip(m.levelModelList,
                                m.uList,
                                m.rList):
                #calculate the coefficients, any explicit-in-time
                #terms will be wrong
                lm.getResidual(lu,lr)
        for p,n,m,simOutput in spinup:
            logEvent("Attaching models to model "+p.name)
            m.attachModels(self.modelList)
            if m in list(self.modelSpinUp.values()):
                logEvent("Spin-Up Estimating initial time derivative and initializing time history for model "+p.name)
                #now the models are attached so we can calculate the coefficients
                for lm,lu,lr in zip(m.levelModelList,
                                    m.uList,
                                    m.rList):
                    #calculate the coefficients, any explicit-in-time
                    #terms will be wrong
                    lm.getResidual(lu,lr)
                    #post-process velocity
                    #lm.calculateAuxiliaryQuantitiesAfterStep()
                    #load in the initial conditions into time
                    #integration history to get explict terms right
                    lm.initializeTimeHistory()
                    lm.timeIntegration.initializeSpaceHistory()
                    #recalculate coefficients
                    lm.getResidual(lu,lr)
                    #calculate consistent time derivative
                    lm.estimate_mt()
                    #post-process velocity
                    lm.calculateAuxiliaryQuantitiesAfterStep()
                logEvent("Spin-Up Choosing initial time step for model "+p.name)
                m.stepController.initialize_dt_model(self.tnList[0],self.tnList[1])
                #mwf what if user wants spin-up to be over (t_0,t_1)?

                if m.stepController.stepExact and m.stepController.t_model_last != self.tnList[1]:
                    logEvent("Spin-up step exact called for model %s" % (m.name,),level=3)
                    m.stepController.stepExact_model(self.tnList[1])
                logEvent("Spin-Up Initializing time history for model step controller")

                m.stepController.initializeTimeHistory()
                m.stepController.setInitialGuess(m.uList,m.rList)

                solverFailed = m.solver.solveMultilevel(uList=m.uList,
                                                        rList=m.rList,
                                                        par_uList=m.par_uList,
                                                        par_rList=m.par_rList)
                Profiling.memory("solver.solveMultilevel")
                if solverFailed:
                    logEvent("Spin-Up Step Failed t=%12.5e, dt=%12.5e for model %s, CONTINUING ANYWAY!" %  (m.stepController.t_model,
                                                                                                     m.stepController.dt_model,
                                                                                                     m.name))

                else:
                    if n.restrictFineSolutionToAllMeshes:
                        logEvent("Using interpolant of fine mesh an all meshes")
                        self.restrictFromFineMesh(m)
                    self.postStep(m)
                    self.systemStepController.modelStepTaken(m,self.tnList[0])
                    logEvent("Spin-Up Step Taken, Model step t=%12.5e, dt=%12.5e for model %s" % (m.stepController.t_model,
                                                                                             m.stepController.dt_model,
                                                                                             m.name))

        for p,n,m,simOutput,index in zip(self.pList,self.nList,self.modelList,self.simOutputList,list(range(len(self.pList)))):
            if not self.opts.hotStart:
                logEvent("Archiving initial conditions")
                self.archiveInitialSolution(m,index)
            else:
                self.ar[index].domain = self.ar[index].tree.find("Domain")
            #if(not hasattr(self.pList[0].domain,'PUMIMesh') and not self.opts.hotStart):
            self.initializeViewSolution(m)
            logEvent("Estimating initial time derivative and initializing time history for model "+p.name)
            #now the models are attached so we can calculate the coefficients
            for lm,lu,lr in zip(m.levelModelList,
                                m.uList,
                                m.rList):
                if self.opts.save_dof:
                    import copy
                    lm.u_store = lm.u.copy()
                    for ci in range(0,lm.coefficients.nc):
                        lm.u_store[ci] = lm.u[ci].copy()

                    lm.setUnknowns(m.uList[0])
                    for ci in range(lm.coefficients.nc):
                        lm.u[ci].dof_last_last[:] = lm.u[ci].dof_last
                        lm.u[ci].dof_last[:] = lm.u[ci].dof
                        lm.u[ci].dof[:] = lm.u_store[ci].dof

                #calculate the coefficients, any explicit terms will be wrong
                lm.timeTerm=False
                lm.getResidual(lu,lr)
                #post-process velocity
                #lm.calculateAuxiliaryQuantitiesAfterStep()
                #load in the initial conditions into time integration history to get explict terms right
                lm.initializeTimeHistory()
                lm.timeIntegration.initializeSpaceHistory()
                #recalculate  coefficients with the explicit terms correct
                lm.getResidual(lu,lr)
                #post-process velocity
                #lm.calculateAuxiliaryQuantitiesAfterStep()
                lm.timeTerm=True
                #calculate consistent
                lm.estimate_mt()
                #
            logEvent("Choosing initial time step for model "+p.name)
            m.stepController.initialize_dt_model(self.tnList[0],self.tnList[1])
            #recalculate  with all terms ready
            for lm,lu,lr in zip(m.levelModelList,
                                m.uList,
                                m.rList):
                lm.getResidual(lu,lr)
            logEvent("Initializing time history for model step controller")
            m.stepController.initializeTimeHistory()
        self.systemStepController.initialize_dt_system(self.tnList[0],self.tnList[1]) #may reset other dt's
        for m in self.modelList:
            logEvent("Auxiliary variable calculations for model %s" % (m.name,))
            for av in self.auxiliaryVariables[m.name]:
                av.calculate_init()
        logEvent("Starting time stepping",level=0)
        systemStepFailed=False
        stepFailed=False


        #### Perform an initial adapt after applying initial conditions ####
        # The initial adapt is based on interface, but will eventually be generalized to any sort of initialization
        # Needs to be placed here at this time because of the post-adapt routine requirements

        if (hasattr(self.pList[0].domain, 'PUMIMesh') and
            self.pList[0].domain.PUMIMesh.adaptMesh() and
            (self.pList[0].domain.PUMIMesh.size_field_config() == "combined" or self.pList[0].domain.PUMIMesh.size_field_config() == "pseudo" or self.pList[0].domain.PUMIMesh.size_field_config() == "isotropic") and
            self.so.useOneMesh and not self.opts.hotStart):

            self.PUMI_transferFields()
            logEvent("Initial Adapt before Solve")
            self.PUMI_adaptMesh("interface")
 
            self.PUMI_transferFields()
            logEvent("Initial Adapt 2 before Solve")
            self.PUMI_adaptMesh("interface")

        #NS_base has a fairly complicated time stepping loop structure
        #to accommodate fairly general split operator approaches. The
        #outer loop is over user defined time intervals for the entire
        #system of models. The next loop is over potentially adaptive
        #steps for the entire system. The next loop is for iterations
        #over the entire system such as for interactive split
        #operator. The next loop is for a sequence of model steps such
        #as for alternating split operator or fractional step
        #schemes. The next loop is for each model to step, potentially
        #adaptively, to the time in the stepSequence. Lastly there is
        #a loop for substeps(stages).

       # for p,n,m,simOutput,index in zip(self.pList,self.nList,self.modelList,self.simOutputList,range(len(self.pList))):
       #   for lm,lu,lr in zip(m.levelModelList,
       #                         m.uList,
       #                         m.rList):
       #     lm.getResidual(lu,lr)
       #     print "Initial Field \n %s" % lu
       #     print "Initial Residual \n %s" % lr
       #     print "Min / Max residual %s / %s" %(lr.min(),lr.max())

        self.nSequenceSteps = 0
        nSequenceStepsLast=self.nSequenceSteps # prevent archiving the same solution twice
        self.nSolveSteps=0


        self.opts.save_dof = True
        if self.opts.save_dof:
            import copy
            for m in self.modelList:
                for lm in m.levelModelList:
                    lm.u_store = lm.u.copy()
                    for ci in range(0,lm.coefficients.nc):
                        lm.u_store[ci] = lm.u[ci].copy()

                    lm.setUnknowns(m.uList[0])
                    for ci in range(lm.coefficients.nc):
                        lm.u[ci].dof_last_last[:] = lm.u[ci].dof_last
                        lm.u[ci].dof_last[:] = lm.u[ci].dof
                        lm.u[ci].dof[:] = lm.u_store[ci].dof

        #### If PUMI and hotstarting then decode info and proceed with restart #### 
        #### This has to be done after the dof histories are saved because DOF histories are already present on the mesh ####

        if (hasattr(self.pList[0].domain, 'PUMIMesh') and self.opts.hotStart):
          f = open(self.pList[0].domain.checkpointInfo, 'r')
          import json
          previousInfo = json.load(f)
          f.close()
          if(previousInfo["checkpoint_status"]=="endsystem"):
            self.hotstartWithPUMI()
            self.opts.hotStart = False 
            #Need to clean mesh for output again      
            self.pList[0].domain.PUMIMesh.cleanMesh()
        ####

        import time
        if hasattr(self.so,'measureSpeedOfCode'):
            measureSpeed = self.so.measureSpeedOfCode
        elif hasattr(n,'measureSpeedOfCode'):
            measureSpeed = n.measureSpeedOfCode
        else:
            measureSpeed = False
        #
        append=False
        startToMeasureSpeed = False
        numTimeSteps=0
        start=0

        for (self.tn_last,self.tn) in zip(self.tnList[:-1],self.tnList[1:]):
            logEvent("==============================================================",level=0)
            logEvent("Solving over interval [%12.5e,%12.5e]" % (self.tn_last,self.tn),level=0)
            logEvent("==============================================================",level=0)
            if measureSpeed and startToMeasureSpeed and numTimeSteps==0 and self.comm.isMaster():
                start = time.time()
                logEvent("**********... start measuring speed of the code",level=1)
            #
#            logEvent("NumericalAnalytics Time Step " + `self.tn`, level=0)

            if self.systemStepController.stepExact and self.systemStepController.t_system_last != self.tn:
                self.systemStepController.stepExact_system(self.tn)
            while self.systemStepController.t_system_last < self.tn:
                logEvent("System time step t=%12.5e, dt=%12.5e" % (self.systemStepController.t_system,
                                                              self.systemStepController.dt_system),level=3)

                while (not self.systemStepController.converged() and
                       not systemStepFailed):
      

                    if (hasattr(self.pList[0].domain, 'PUMIMesh') and self.opts.hotStart):
                      self.hotstartWithPUMI()
                      self.opts.hotStart = False 
                      #Need to clean mesh for output again      
                      self.pList[0].domain.PUMIMesh.cleanMesh()

                    #This should be the only place dofs are saved otherwise there might be a double-shift for last_last
                    self.opts.save_dof = True
                    if self.opts.save_dof:
                        import copy
                        for m in self.modelList:
                            for lm in m.levelModelList:
                                lm.u_store = lm.u.copy()
                                for ci in range(lm.coefficients.nc):
                                    lm.u_store[ci] = lm.u[ci].copy()
                                lm.setUnknowns(m.uList[0])
                                for ci in range(lm.coefficients.nc):
                                    lm.u[ci].dof_last_last[:] = lm.u[ci].dof_last
                                    lm.u[ci].dof_last[:] = lm.u[ci].dof
                                for ci in range(lm.coefficients.nc):
                                    lm.u[ci].dof[:] = lm.u_store[ci].dof
                                #lm.setFreeDOF(m.uList[0])
                        logEvent("saving previous velocity dofs %s" % self.nSolveSteps)


                    logEvent("Split operator iteration %i" % (self.systemStepController.its,),level=3)
                    self.nSequenceSteps += 1
                    for (self.t_stepSequence,model) in self.systemStepController.stepSequence:

                        logEvent("NumericalAnalytics Model %s " % (model.name), level=5)
                        logEvent("Model: %s" % (model.name),level=1)
                        logEvent("NumericalAnalytics Time Step " + repr(self.t_stepSequence), level=7)
                        logEvent("Fractional step %12.5e for model %s" % (self.t_stepSequence,model.name),level=3)
                        for m in model.levelModelList:
                            if m.movingDomain and m.tLast_mesh != self.systemStepController.t_system_last:
                                m.t_mesh = self.systemStepController.t_system_last
                                m.updateAfterMeshMotion()
                                m.tLast_mesh = m.t_mesh

                        self.preStep(model)
                        self.setWeakDirichletConditions(model)

                        stepFailed = False
                        if model.stepController.stepExact and model.stepController.t_model_last != self.t_stepSequence:
                            logEvent("Step exact called for model %s" % (model.name,),level=3)
                            model.stepController.stepExact_model(self.t_stepSequence)
                        while (model.stepController.t_model_last < self.t_stepSequence and
                               not stepFailed and
                               not self.systemStepController.exitModelStep[model]):
                            logEvent("Model step t=%12.5e, dt=%12.5e for model %s" % (model.stepController.t_model,
                                                                                 model.stepController.dt_model,
                                                                                 model.name),level=3)
                            for self.tSubstep in model.stepController.substeps:

                                logEvent("Model substep t=%12.5e for model %s" % (self.tSubstep,model.name),level=3)
                                #TODO: model.stepController.substeps doesn't seem to be updated after a solver failure unless model.stepController.stepExact is true
                                logEvent("Model substep t=%12.5e for model %s model.timeIntegration.t= %12.5e" % (self.tSubstep,model.name,model.levelModelList[-1].timeIntegration.t),level=3)
                


                                model.stepController.setInitialGuess(model.uList,model.rList)
                                solverFailed = model.solver.solveMultilevel(uList=model.uList,
                                                                            rList=model.rList,
                                                                            par_uList=model.par_uList,
                                                                            par_rList=model.par_rList)

                                Profiling.memory("solver.solveMultilevel")
                                if self.opts.wait:
                                    input("Hit any key to continue")
                                if solverFailed:
                                    break
                                else:
                                    if n.restrictFineSolutionToAllMeshes:
                                        logEvent("Using interpolant of fine mesh an all meshes")
                                        self.restrictFromFineMesh(model)
                                    model.stepController.updateSubstep()
                            #end model substeps
                            if solverFailed:
                                logEvent("Step failed due to solver failure")
                                stepFailed = not self.systemStepController.retryModelStep_solverFailure(model)
                            elif model.stepController.errorFailure():
                                logEvent("Step failed due to error failure")
                                stepFailed = not self.systemStepController.retryModelStep_errorFailure(model)
                            else:
                                #set up next step
                                self.systemStepController.modelStepTaken(model,self.t_stepSequence)
                                logEvent("Step Taken, t_stepSequence= %s Model step t=%12.5e, dt=%12.5e for model %s" % (self.t_stepSequence,
                                                                                                                         model.stepController.t_model,
                                                                                                                         model.stepController.dt_model,
                                                                                                                         model.name),level=3)

                        #end model step
                        if stepFailed:
                            logEvent("Sequence step failed")
                            if not self.systemStepController.ignoreSequenceStepFailure(model):
                                break
                            else:
                                logEvent("IGNORING STEP FAILURE")
                                self.postStep(model)
                                self.systemStepController.sequenceStepTaken(model)
                        else:
                            self.postStep(model)
                            self.systemStepController.sequenceStepTaken(model)
                    #end model split operator step
                    if stepFailed:
                        systemStepFailed = not self.systemStepController.retrySequence_modelStepFailure()
                        if not systemStepFailed:
                            stepFailed=False
                            logEvent("Retrying sequence")
                        else:
                            logEvent("Sequence failed")
                    else:
                        self.firstStep=False
                        systemStepFailed=False
                        logEvent("Step Taken, Model step t=%12.5e, dt=%12.5e for model %s" % (model.stepController.t_model,
                                                                                              model.stepController.dt_model,
                                                                                              model.name))
                        self.systemStepController.sequenceTaken()
                        for index,model in enumerate(self.modelList):
                            self.viewSolution(model,index)
                        if self.archiveFlag == ArchiveFlags.EVERY_MODEL_STEP:
                            self.tCount+=1
                            for index,model in enumerate(self.modelList):
                                self.archiveSolution(model,index,self.systemStepController.t_system)
                #end system split operator sequence
                if systemStepFailed:
                    logEvent("System Step Failed")
                    #go ahead and update as if the time step had succeeded
                    self.postStep(model)
                    self.systemStepController.modelStepTaken(model,self.t_stepSequence)
                    self.systemStepController.sequenceTaken()
                    self.systemStepController.updateTimeHistory()
                    #you're dead if retrySequence didn't work
                    logEvent("Step Failed, Model step t=%12.5e, dt=%12.5e for model %s" % (model.stepController.t_model,
                                                                                           model.stepController.dt_model,
                                                                                           model.name))
                    break
                else:
                    self.systemStepController.updateTimeHistory()
                    logEvent("Step Taken, System time step t=%12.5e, dt=%12.5e" % (self.systemStepController.t_system,
                                                                                   self.systemStepController.dt_system))
                    self.systemStepController.choose_dt_system()
                    logEvent("Potential System time step t=%12.5e, dt=%12.5e for next step" % (self.systemStepController.t_system,
                                                                                               self.systemStepController.dt_system))
                    if self.systemStepController.stepExact and self.systemStepController.t_system_last != self.tn:
                        self.systemStepController.stepExact_system(self.tn)
                #
                for model in self.modelList:
                    for av in self.auxiliaryVariables[model.name]:
                        av.calculate()
                if self.archiveFlag == ArchiveFlags.EVERY_SEQUENCE_STEP:
                    self.tCount+=1
                    for index,model in enumerate(self.modelList):
                        self.archiveSolution(model,index,self.systemStepController.t_system_last)
                  
                #can only handle PUMIDomain's for now
                #if(self.tn < 0.05):
                #  self.nSolveSteps=0#self.nList[0].adaptMesh_nSteps-2
                self.nSolveSteps += 1
                import gc; gc.collect()
                if(self.PUMI_estimateError()):
                    self.PUMI_adaptMesh()
                #
                if measureSpeed and startToMeasureSpeed and self.comm.isMaster():
                    numTimeSteps += 1
                    logEvent("**********... end of time step. Number of time steps (to measure speed of the code): " + str(numTimeSteps),level=1)
                if measureSpeed and numTimeSteps==100 and self.comm.isMaster():
                    end = time.time()
                    Nproc = self.comm.size()
                    NDOFs=0
                    for i,mod in enumerate(self.modelList):
                        if (i in self.so.modelSpinUpList) == False: #To remove spin up models
                            NDOFs += mod.par_uList[0].size if mod.par_uList[0] is not None else len(mod.uList[0])
                    #
                    if append==False:
                        mode="w"
                    else:
                        mode="a"
                    with open ("speed_measurement.txt",mode) as file:
                        append=True
                        # write file and log this event
                        multiple_line_string = """ ******************** Measurements of speed ********************
                        Num of time steps: {nts:d}
                        Total time: {t:f}
                        Num of processors: {Nproc:d}
                        Total num of DOFs: {NDOFs:d}
                        Num of DOFs per processor: {aux1:d}
                        Time per time step, per DOF, per processor: {aux2:.4E} \n""".format(nts=numTimeSteps,
                                                                                            t=(end-start),
                                                                                            Nproc=Nproc,
                                                                                            NDOFs=NDOFs,
                                                                                            aux1=int(NDOFs/Nproc),
                                                                                            aux2=(end-start)/numTimeSteps*Nproc/NDOFs)
                        file.write(multiple_line_string)
                        logEvent(multiple_line_string,level=4)
                    #
                    measureSpeed = False
                #
            if measureSpeed and startToMeasureSpeed and self.comm.isMaster():
                end = time.time()
                Nproc = self.comm.size()
                NDOFs=0
                for i,mod in enumerate(self.modelList):
                    if (i in self.so.modelSpinUpList) == False:
                        NDOFs += mod.par_uList[0].size if mod.par_uList[0] is not None else len(mod.uList[0])
                #
                if append==False:
                    mode="w"
                else:
                    mode="a"
                with open ("speed_measurement.txt",mode) as file:
                    append=True
                    # write file and log this event
                    multiple_line_string = """ ******************** Measurements of speed ********************
                    Num of time steps: {nts:d}
                    Total time: {t:f}
                    Num of processors: {Nproc:d}
                    Total num of DOFs: {NDOFs:d}
                    Num of DOFs per processor: {aux1:d}
                    Time per time step, per DOF, per processor: {aux2:.4E} \n""".format(nts=numTimeSteps,
                               t=(end-start),
                               Nproc=Nproc,
                               NDOFs=NDOFs,
                               aux1=int(NDOFs/Nproc),
                               aux2=(end-start)/numTimeSteps*Nproc/NDOFs)
                    file.write(multiple_line_string)
                    logEvent(multiple_line_string,level=4)
                #
                measureSpeed = False
            #
            #end system step iterations
            if self.archiveFlag == ArchiveFlags.EVERY_USER_STEP and self.nSequenceSteps > nSequenceStepsLast:
                nSequenceStepsLast = self.nSequenceSteps
                self.tCount+=1
                for index,model in enumerate(self.modelList):
                    self.archiveSolution(model,index,self.systemStepController.t_system_last)

            if systemStepFailed:
                break
            #
            #h-adapt mesh, cekees modified from chitak
            #
            #assuming same for all physics and numerics  for now

            #can only handle PUMIDomain's for now
            #self.nSolveSteps += 1
            #if(self.PUMI_estimateError()):
            #  self.PUMI_adaptMesh()
            if measureSpeed and self.comm.isMaster():
                startToMeasureSpeed = True
            if measureSpeed==False and append==True:
                measureSpeed=True
                numTimeSteps=0
            #
        logEvent("Finished calculating solution",level=3)
        # compute auxiliary quantities at last time step
        for index,model in enumerate(self.modelList):
            if hasattr(model.levelModelList[-1],'runAtEOS'):
                model.levelModelList[-1].runAtEOS()

        if(hasattr(self.pList[0].domain,"PUMIMesh")):
        #Transfer solution to PUMI mesh for output
          self.pList[0].domain.PUMIMesh.transferFieldToPUMI("coordinates",
            self.modelList[0].levelModelList[0].mesh.nodeArray)

          for m in self.modelList:
            for lm in m.levelModelList:
              coef = lm.coefficients
              if coef.vectorComponents is not None:
                vector=numpy.zeros((lm.mesh.nNodes_global,3),'d')
                for vci in range(len(coef.vectorComponents)):
                  vector[:,vci] = lm.u[coef.vectorComponents[vci]].dof[:]
                self.pList[0].domain.PUMIMesh.transferFieldToPUMI(
                   coef.vectorName, vector)
                #Transfer dof_last
                for vci in range(len(coef.vectorComponents)):
                  vector[:,vci] = lm.u[coef.vectorComponents[vci]].dof_last[:]
                self.pList[0].domain.PUMIMesh.transferFieldToPUMI(
                     coef.vectorName+"_old", vector)
                #Transfer dof_last_last
                for vci in range(len(coef.vectorComponents)):
                  vector[:,vci] = lm.u[coef.vectorComponents[vci]].dof_last_last[:]
                self.pList[0].domain.PUMIMesh.transferFieldToPUMI(
                     coef.vectorName+"_old_old", vector)
                del vector

              for ci in range(coef.nc):
                if coef.vectorComponents is None or \
                  ci not in coef.vectorComponents:
                  scalar=numpy.zeros((lm.mesh.nNodes_global,1),'d')
                  scalar[:,0] = lm.u[ci].dof[:]
                  self.pList[0].domain.PUMIMesh.transferFieldToPUMI(
                      coef.variableNames[ci], scalar)
                  #Transfer dof_last
                  scalar[:,0] = lm.u[ci].dof_last[:]
                  self.pList[0].domain.PUMIMesh.transferFieldToPUMI(
                     coef.variableNames[ci]+"_old", scalar)
                  #Transfer dof_last_last
                  scalar[:,0] = lm.u[ci].dof_last_last[:]
                  self.pList[0].domain.PUMIMesh.transferFieldToPUMI(
                     coef.variableNames[ci]+"_old_old", scalar)
                  del scalar

          self.pList[0].domain.PUMIMesh.writeMesh("finalMesh.smb")
          if((self.PUMIcheckpointer.frequency>0) ):
            self.modelListOld = self.modelList
            self.PUMIcheckpointer.checkpoint(self.systemStepController.t_system_last)

        for index,model in enumerate(self.modelList):
            self.finalizeViewSolution(model)
            self.closeArchive(model,index)

        return systemStepFailed
    #
    #try to make preStep and postStep just manipulate "current values" and let the step controllers manage the history setting
    ##intermodel transfer before a solution step
    def preStep(self,model):
        for level,levelModel in enumerate(model.levelModelList):
            preCopy = levelModel.coefficients.preStep(model.stepController.t_model,firstStep=self.firstStep)
            if (preCopy is not None and ('copy_uList') in preCopy and preCopy['copy_uList'] == True):
                for u_ci_lhs,u_ci_rhs in zip(list(levelModel.u.values()),list(self.modelList[preCopy['uList_model']].levelModelList[level].u.values())):
                    u_ci_lhs.dof[:] = u_ci_rhs.dof
                levelModel.setFreeDOF(model.uList[level])
            if preCopy is not None and ('clear_uList') in preCopy and preCopy['clear_uList'] == True:
                for u_ci_lhs in list(levelModel.u.values()):
                    u_ci_lhs.dof[:] = 0.0
                levelModel.setFreeDOF(model.uList[level])
            if preCopy is not None and ('reset_uList') in preCopy and preCopy['reset_uList'] == True:
                levelModel.setFreeDOF(model.uList[level])
                levelModel.getResidual(model.uList[level],model.rList[level])

    ##intermodel transfer after a step
    def postStep(self,model):
        for level,levelModel in enumerate(model.levelModelList):
            postCopy = levelModel.coefficients.postStep(model.stepController.t_model,firstStep=self.firstStep)
            if postCopy is not None and ('copy_uList') in postCopy and postCopy['copy_uList'] == True:
                for u_ci_lhs,u_ci_rhs in zip(list(self.modelList[postCopy['uList_model']].levelModelList[level].u.values()),list(model.levelModelList[level].u.values())):
                    u_ci_lhs.dof[:] = u_ci_rhs.dof
                self.modelList[postCopy['uList_model']].levelModelList[level].setFreeDOF(self.modelList[postCopy['uList_model']].uList[level])

    def setWeakDirichletConditions(self,model):
        if model.weakDirichletConditions is not None:
            for levelModel in model.levelModelList:
                levelModel.dirichletNodeSetList={}
                levelModel.dirichletGlobalNodeSet={}
                levelModel.dirichletValues={}
            for ci in model.weakDirichletConditions:
                for levelModel in model.levelModelList:
                    model.weakDirichletConditions[ci](levelModel)

    def restrictFromFineMesh(self,model):
        for level in range(len(model.levelModelList)-1,0,-1):
            for cj in range(model.levelModelList[-1].coefficients.nc):
                model.meshTransfers.interp_bcListDict[cj][level].matvec(model.levelModelList[level].u[cj].dof,
                                                                                 model.levelModelList[level-1].u[cj].dof)
            model.levelModelList[level-1].setFreeDOF(model.uList[level-1])
            model.levelModelList[level-1].calculateCoefficients()

    ##save model's initial solution values to archive
    def archiveInitialSolution(self,model,index):
        if True if self.fastArchive == False else 'clsvof' in model.name:
            import xml.etree.ElementTree as ElementTree
            if self.archiveFlag == ArchiveFlags.UNDEFINED:
                return
            logEvent("Writing initial mesh for  model = "+model.name,level=3)
            logEvent("Writing initial conditions for  model = "+model.name,level=3)
            if not self.so.useOneArchive or index==0:
                self.ar[index].domain = ElementTree.SubElement(self.ar[index].tree.getroot(),"Domain")
            if self.so.useOneArchive:
                model.levelModelList[-1].archiveFiniteElementSolutions(self.ar[index],self.tnList[0],self.tCount,initialPhase=True,
                                                                       writeVectors=True,meshChanged=True,femSpaceWritten=self.femSpaceWritten,
                                                                       writeVelocityPostProcessor=self.opts.writeVPP)
            else:
                model.levelModelList[-1].archiveFiniteElementSolutions(self.ar[index],self.tnList[0],self.tCount,initialPhase=True,
                                                                       writeVectors=True,meshChanged=True,femSpaceWritten={},
                                                                       writeVelocityPostProcessor=self.opts.writeVPP)
            model.levelModelList[-1].archiveAnalyticalSolutions(self.ar[index],self.pList[index].analyticalSolution,
                                                                self.tnList[0],
                                                                self.tCount)
            #could just pull the code and flags out from SimTools rathter than asking it to parse them
            #uses values in simFlags['storeQuantities']
            #q dictionary
            if self.archive_q[index] == True:
                scalarKeys = model.simTools.getScalarElementStorageKeys(model,self.tnList[0])
                vectorKeys = model.simTools.getVectorElementStorageKeys(model,self.tnList[0])
                tensorKeys = model.simTools.getTensorElementStorageKeys(model,self.tnList[0])
                model.levelModelList[-1].archiveElementQuadratureValues(self.ar[index],self.tnList[0],self.tCount,
                                                                        scalarKeys=scalarKeys,vectorKeys=vectorKeys,tensorKeys=tensorKeys,
                                                                        initialPhase=True,meshChanged=True)
            if self.archive_ebq_global[index] == True:
                #ebq_global dictionary
                scalarKeys = model.simTools.getScalarElementBoundaryStorageKeys(model,self.tnList[0])
                vectorKeys = model.simTools.getVectorElementBoundaryStorageKeys(model,self.tnList[0])
                tensorKeys = model.simTools.getTensorElementBoundaryStorageKeys(model,self.tnList[0])
                model.levelModelList[-1].archiveElementBoundaryQuadratureValues(self.ar[index],self.tnList[0],self.tCount,
                                                                                scalarKeys=scalarKeys,vectorKeys=vectorKeys,tensorKeys=tensorKeys,
                                                                                initialPhase=True,meshChanged=True)
            if self.archive_ebqe[index] == True:
                #ebqe dictionary
                scalarKeys = model.simTools.getScalarExteriorElementBoundaryStorageKeys(model,self.tnList[0])
                vectorKeys = model.simTools.getVectorExteriorElementBoundaryStorageKeys(model,self.tnList[0])
                tensorKeys = model.simTools.getTensorExteriorElementBoundaryStorageKeys(model,self.tnList[0])
                model.levelModelList[-1].archiveExteriorElementBoundaryQuadratureValues(self.ar[index],self.tnList[0],self.tCount,
                                                                                        scalarKeys=scalarKeys,vectorKeys=vectorKeys,tensorKeys=tensorKeys,
                                                                                        initialPhase=True,meshChanged=True)
            try:
                phi_s = {}
                phi_s[0] = model.levelModelList[-1].coefficients.phi_s
                model.levelModelList[-1].archiveFiniteElementResiduals(self.ar[index],
                                                                       self.tnList[0],
                                                                       self.tCount,
                                                                       phi_s,
                                                                       res_name_base='phi_s')
                logEvent("Writing initial phi_s at DOFs for = "+model.name+" at time t="+str(t),level=3)
            except:
                pass
            try:
                phi_sp = {}
                phi_sp[0] = model.levelModelList[-1].coefficients.phi_sp
                model.levelModelList[-1].archiveFiniteElementResiduals(self.ar[index],
                                                                       self.tnList[0],
                                                                       self.tCount,
                                                                       phi_sp,
                                                                       res_name_base='phi_sp')
                logEvent("Writing initial phi_sp at DOFs for = "+model.name+" at time t="+str(self.tnList[0]),level=3)
            except:
                pass
            if 'clsvof' in model.name:
                vofDOFs = {}
                vofDOFs[0] = model.levelModelList[-1].vofDOFs
                model.levelModelList[-1].archiveFiniteElementResiduals(self.ar[index],
                                                                       self.tnList[0],
                                                                       self.tCount,
                                                                       vofDOFs,
                                                                       res_name_base='vof')
                logEvent("Writing initial vof from clsvof at time t="+str(0),level=3)
            #For aux quantity of interest (MQL)
            try:
                if model.levelModelList[-1].coefficients.outputQuantDOFs==True:
                    quantDOFs = {}
                    quantDOFs[0] = model.levelModelList[-1].quantDOFs
                    model.levelModelList[-1].archiveFiniteElementResiduals(self.ar[index],
                                                                           self.tnList[0],
                                                                           self.tCount,
                                                                           quantDOFs,
                                                                           res_name_base='quantDOFs_for_'+model.name)
                    logEvent("Writing initial quantity of interest at DOFs for = "+model.name+" at time t="+str(0),level=3)
            except:
                pass
            #Write bathymetry for Shallow water equations (MQL)
            try:
                bathymetry = {}
                bathymetry[0] = model.levelModelList[-1].coefficients.b.dof
                model.levelModelList[-1].archiveFiniteElementResiduals(self.ar[index],
                                                                       self.tnList[0],
                                                                       self.tCount,
                                                                       bathymetry,
                                                                       res_name_base='bathymetry')
                logEvent("Writing bathymetry for = "+model.name,level=3)
            except:
                pass
            #write eta=h+bathymetry for SWEs (MQL)
            try:
                eta = {}
                eta[0] = model.levelModelList[-1].coefficients.b.dof+model.levelModelList[-1].u[0].dof
                model.levelModelList[-1].archiveFiniteElementResiduals(self.ar[index],
                                                                       self.tnList[0],
                                                                       self.tCount,
                                                                       eta,
                                                                       res_name_base='eta')
                logEvent("Writing bathymetry for = "+model.name,level=3)
            except:
                pass
            #for nonlinear POD
            if self.archive_pod_residuals[index] == True:
                res_space = {}; res_mass = {}
                for ci in range(model.levelModelList[-1].coefficients.nc):
                    res_space[ci] = numpy.zeros(model.levelModelList[-1].u[ci].dof.shape,'d')
                    model.levelModelList[-1].getSpatialResidual(model.levelModelList[-1].u[ci].dof,res_space[ci])
                    res_mass[ci] = numpy.zeros(model.levelModelList[-1].u[ci].dof.shape,'d')
                    model.levelModelList[-1].getMassResidual(model.levelModelList[-1].u[ci].dof,res_mass[ci])
                model.levelModelList[-1].archiveFiniteElementResiduals(self.ar[index],self.tnList[0],self.tCount,res_space,res_name_base='spatial_residual')
                model.levelModelList[-1].archiveFiniteElementResiduals(self.ar[index],self.tnList[0],self.tCount,res_mass,res_name_base='mass_residual')
            if not self.opts.cacheArchive:
                if not self.so.useOneArchive:
                    self.ar[index].sync()
                else:
                    if index == len(self.ar) - 1:
                        self.ar[index].sync()

    ##save model's solution values to archive
    def archiveSolution(self,model,index,t=None):
        if True if self.fastArchive == False else 'clsvof' in model.name:
            if self.archiveFlag == ArchiveFlags.UNDEFINED:
                return
            if t is None:
                t = self.systemStepController.t_system
            logEvent("Writing mesh header for  model = "+model.name+" at time t="+str(t),level=3)
            logEvent("Writing solution for  model = "+model.name,level=3)
            if self.so.useOneArchive:
                if index==0:
                    self.femSpaceWritten={}
                model.levelModelList[-1].archiveFiniteElementSolutions(self.ar[index],t,self.tCount,
                                                                       initialPhase=False,
                                                                       writeVectors=True,meshChanged=True,femSpaceWritten=self.femSpaceWritten,
                                                                       writeVelocityPostProcessor=self.opts.writeVPP)
            else:
                model.levelModelList[-1].archiveFiniteElementSolutions(self.ar[index],t,self.tCount,
                                                                       initialPhase=False,
                                                                       writeVectors=True,meshChanged=True,femSpaceWritten={},
                                                                       writeVelocityPostProcessor=self.opts.writeVPP)
            model.levelModelList[-1].archiveAnalyticalSolutions(self.ar[index],self.pList[index].analyticalSolution,
                                                                t,
                                                                self.tCount)
            #uses values in simFlags['storeQuantities']
            #q dictionary
            if self.archive_q[index] == True and self.fastArchive==False:
                scalarKeys = model.simTools.getScalarElementStorageKeys(model,t)
                vectorKeys = model.simTools.getVectorElementStorageKeys(model,t)
                tensorKeys = model.simTools.getTensorElementStorageKeys(model,t)
                model.levelModelList[-1].archiveElementQuadratureValues(self.ar[index],t,self.tCount,
                                                                        scalarKeys=scalarKeys,vectorKeys=vectorKeys,tensorKeys=tensorKeys,
                                                                        initialPhase=False,meshChanged=True)
            #ebq_global dictionary
            if self.archive_ebq_global[index] == True and self.fastArchive==False:
                scalarKeys = model.simTools.getScalarElementBoundaryStorageKeys(model,t)
                vectorKeys = model.simTools.getVectorElementBoundaryStorageKeys(model,t)
                tensorKeys = model.simTools.getTensorElementBoundaryStorageKeys(model,t)
                model.levelModelList[-1].archiveElementBoundaryQuadratureValues(self.ar[index],t,self.tCount,
                                                                                scalarKeys=scalarKeys,vectorKeys=vectorKeys,tensorKeys=tensorKeys,
                                                                                initialPhase=False,meshChanged=True)
            if self.archive_ebqe[index] == True and self.fastArchive==False:
                #ebqe dictionary
                scalarKeys = model.simTools.getScalarExteriorElementBoundaryStorageKeys(model,t)
                vectorKeys = model.simTools.getVectorExteriorElementBoundaryStorageKeys(model,t)
                tensorKeys = model.simTools.getTensorExteriorElementBoundaryStorageKeys(model,t)
                model.levelModelList[-1].archiveExteriorElementBoundaryQuadratureValues(self.ar[index],t,self.tCount,
                                                                                        scalarKeys=scalarKeys,vectorKeys=vectorKeys,tensorKeys=tensorKeys,
                                                                                        initialPhase=False,meshChanged=True)
            if self.fastArchive==False:
                try:
                    phi_s = {}
                    phi_s[0] = model.levelModelList[-1].coefficients.phi_s
                    model.levelModelList[-1].archiveFiniteElementResiduals(self.ar[index],
                                                                           self.tnList[0],
                                                                           self.tCount,
                                                                           phi_s,
                                                                           res_name_base='phi_s')
                    logEvent("Writing phi_s at DOFs for = "+model.name+" at time t="+str(t),level=3)
                except:
                    pass
                try:
                    phi_sp = {}
                    phi_sp[0] = model.levelModelList[-1].coefficients.phi_sp
                    model.levelModelList[-1].archiveFiniteElementResiduals(self.ar[index],
                                                                           t,
                                                                           self.tCount,
                                                                           phi_sp,
                                                                           res_name_base='phi_sp')
                    logEvent("Writing phi_sp at DOFs for = "+model.name+" at time t="+str(t),level=3)
                except:
                    pass
            if 'clsvof' in model.name and self.fastArchive==False:
                vofDOFs = {}
                vofDOFs[0] = model.levelModelList[-1].vofDOFs
                model.levelModelList[-1].archiveFiniteElementResiduals(self.ar[index],
                                                                       self.tnList[0],
                                                                       self.tCount,
                                                                       vofDOFs,
                                                                       res_name_base='vof')
                logEvent("Writing initial vof from clsvof at time t="+str(t),level=3)
            if self.fastArchive==False:
                try:
                    if model.levelModelList[-1].coefficients.outputQuantDOFs==True:
                        quantDOFs = {}
                        quantDOFs[0] = model.levelModelList[-1].quantDOFs
                        model.levelModelList[-1].archiveFiniteElementResiduals(self.ar[index],
                                                                               self.tnList[0],
                                                                               self.tCount,
                                                                               quantDOFs,
                                                                               res_name_base='quantDOFs_for_'+model.name)
                        logEvent("Writing quantity of interest at DOFs for = "+model.name+" at time t="+str(t),level=3)
                except:
                    pass
            #Write bathymetry for Shallow water equations (MQL)
            if self.fastArchive==False:
                try:
                    bathymetry = {}
                    bathymetry[0] = model.levelModelList[-1].coefficients.b.dof
                    model.levelModelList[-1].archiveFiniteElementResiduals(self.ar[index],
                                                                           self.tnList[0],
                                                                           self.tCount,
                                                                           bathymetry,
                                                                           res_name_base='bathymetry')
                    logEvent("Writing bathymetry for = "+model.name,level=3)
                except:
                    pass
                #write eta=h+bathymetry for SWEs (MQL)
                try:
                    eta = {}
                    eta[0] = model.levelModelList[-1].coefficients.b.dof+model.levelModelList[-1].u[0].dof
                    model.levelModelList[-1].archiveFiniteElementResiduals(self.ar[index],
                                                                           self.tnList[0],
                                                                           self.tCount,
                                                                           eta,
                                                                           res_name_base='eta')
                    logEvent("Writing bathymetry for = "+model.name,level=3)
                except:
                    pass

            #for nonlinear POD
            if self.archive_pod_residuals[index] == True and self.fastArchive==False:
                res_space = {}; res_mass = {}
                for ci in range(model.levelModelList[-1].coefficients.nc):
                    res_space[ci] = numpy.zeros(model.levelModelList[-1].u[ci].dof.shape,'d')
                    model.levelModelList[-1].getSpatialResidual(model.levelModelList[-1].u[ci].dof,res_space[ci])
                    res_mass[ci] = numpy.zeros(model.levelModelList[-1].u[ci].dof.shape,'d')
                    model.levelModelList[-1].getMassResidual(model.levelModelList[-1].u[ci].dof,res_mass[ci])

                model.levelModelList[-1].archiveFiniteElementResiduals(self.ar[index],t,self.tCount,res_space,res_name_base='spatial_residual')
                model.levelModelList[-1].archiveFiniteElementResiduals(self.ar[index],t,self.tCount,res_mass,res_name_base='mass_residual')
            if not self.opts.cacheArchive:
                if not self.so.useOneArchive:
                    self.ar[index].sync()
                else:
                    if index == len(self.ar) - 1:
                        self.ar[index].sync()

    ## clean up archive
    def closeArchive(self,model,index):
        if self.archiveFlag is None:
            return
        if self.so.useOneArchive:
            if index==0:
                logEvent("Closing solution archive for "+self.so.name)
                self.ar[index].close()
        else:
            logEvent("Closing solution archive for "+model.name)
            self.ar[index].close()

    def initializeViewSolution(self,model):
        """
        """
        model.viewer.preprocess(model,model.stepController.t_model_last)
        model.simTools.preprocess(model,model.stepController.t_model_last)

    ## run time visualization for modela
    def viewSolution(self,model,initialCondition=False):
        """

        """
        #mwf looking at last solns
        if (model.viewer.viewerType != 'matlab' or model.stepController.t_model_last <= self.tnList[0] or
            model.stepController.t_model_last >= self.tnList[-1]):
            model.viewer.processTimeLevel(model,model.stepController.t_model_last)
            model.simTools.processTimeLevel(model,model.stepController.t_model_last)


    ## clean up runtime visualization
    def finalizeViewSolution(self,model):
        model.viewer.postprocess(model,model.stepController.t_model_last)
        model.simTools.postprocess(model,model.stepController.t_model_last)

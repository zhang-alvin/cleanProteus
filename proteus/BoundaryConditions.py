import cython
"""
Module for creating boundary conditions. Imported in SpatialTools.py

.. inheritance-diagram:: proteus.BoundaryConditions
   :parts: 1
"""


class BC_Base():
    """
    Generic class regrouping boundary conditions
    """
    def __init__(self, shape=None, name=None, b_or=None, b_i=0):
        self.Shape = shape
        self.name = name
        self.BC_type = 'None'
        self._b_or = b_or  # array of orientation of all boundaries of shape
        self._b_i = b_i  # indice for this boundary in list of boundaries

    @classmethod
    def newGlobalBC(cls, name, default_value):
        """
        Makes a new BoundaryCondition class instance attached to BC_Base.
        This creates a new class attributed that will be added to all BC_Base
        class instances.

        Parameters
        ----------
        name: string
        """
        setattr(cls, name, default_value)

    @classmethod
    def getContext(cls, context=None):
        """
        Gets context from proteus.Context or

        Parameters
        ----------
        context: class, optional
             if set to None, the context will be created from proteus.Context
        """
        if context:
            from proteus import Context
            cls.ct = Context.get()
        else:
            cls.ct = context


class BoundaryCondition():
    """
    Boundary condition class

    Attributes
    ----------
    uOfXT: func or None
        boundary condition function of x (array_like) and t (float) or None for
        no boundary condition
    """
    def __init__(self):
        self.uOfXT = None

    def init_cython(self):
        return self.uOfXT

    def resetBC(self):
        self.uOfXT = None

    def setConstantBC(self, value):
        """
        function returning constant BC

        Parameters
        ----------
        value :

        """
        self.uOfXT = lambda x, t: value


    def setLinearBC(self, a0, a1, i):
        self.uOfXT = lambda x, t: a0+a1*x[i]

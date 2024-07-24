"""
.. autofunction:: nested_dissection
.. autofunction:: part_graph
.. autofunction:: part_mesh
.. autofunction:: verify_nd

.. autoclass:: Options
.. autoclass:: MeshPartition
.. autoclass:: Status
.. autoclass:: OPType
.. autoclass:: OptionKey
.. autoclass:: PType
.. autoclass:: GType
.. autoclass:: CType
.. autoclass:: IPType
.. autoclass:: RType
.. autoclass:: DebugLevel
.. autoclass:: ObjType
"""

__copyright__ = "Copyright (C) 2009-2013 Andreas Kloeckner"

__license__ = """
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
"""

from pymetis.version import version, version_tuple  # noqa
from pymetis._internal import Options as OptionsBase

from pymetis._internal import Status


Status.__doc__ = """A wrapper for METIS return codes.

.. attribute:: OK

   Returned normally

.. attribute:: ERROR_INPUT

   Returned due to erroneous inputs and/or options

.. attribute:: ERROR_MEMORY

   Returned due to insufficient memory

.. attribute:: ERROR

   Some other errors
"""

from pymetis._internal import OPType


OPType.__doc__ = """A wrapper for METIS operation type
codes.

.. attribute:: PMETIS
.. attribute:: KMETIS
.. attribute:: OMETIS
"""

from pymetis._internal import OptionKey


OptionKey.__doc__ = """A wrapper for METIS option codes.

.. attribute:: PTYPE
.. attribute:: OBJTYPE
.. attribute:: CTYPE
.. attribute:: IPTYPE
.. attribute:: RTYPE
.. attribute:: DBGLVL
.. attribute:: NITER
.. attribute:: NCUTS
.. attribute:: SEED
.. attribute:: NO2HOP
.. attribute:: MINCONN
.. attribute:: CONTIG
.. attribute:: COMPRESS
.. attribute:: CCORDER
.. attribute:: PFACTOR
.. attribute:: NSEPS
.. attribute:: UFACTOR
.. attribute:: NUMBERING
.. attribute:: HELP
.. attribute:: TPWGTS
.. attribute:: NCOMMON
.. attribute:: NOOUTPUT
.. attribute:: BALANCE
.. attribute:: GTYPE
.. attribute:: UBVEC
"""

from pymetis._internal import PType


PType.__doc__ = """A wrapper for METIS partitioning scheme
codes.

.. attribute:: RB
.. attribute:: KWAY
"""

from pymetis._internal import GType


GType.__doc__ = """A wrapper for METIS graph type codes.

.. attribute:: NODAL
.. attribute:: DUAL
"""

from pymetis._internal import CType


CType.__doc__ = """A wrapper for METIS coarsening scheme
codes.

.. attribute:: RM
.. attribute:: SHEM
"""

from pymetis._internal import IPType


IPType.__doc__ = """A wrapper for METIS initial
partitioning scheme codes.

.. attribute:: GROW
.. attribute:: RANDOM
.. attribute:: EDGE
.. attribute:: NODE
.. attribute:: METISRB
"""

from pymetis._internal import RType


RType.__doc__ = """A wrapper for METIS refinement scheme
codes.

.. attribute:: FM
.. attribute:: GREEDY
.. attribute:: SEP2SIDED
.. attribute:: SEP1SIDED
"""

from pymetis._internal import DebugLevel


DebugLevel.__doc__ = """A wrapper for METIS debug level
codes.

.. attribute:: INFO

   Shows various diagnostic message
.. attribute:: TIME

   Perform timing analysis
.. attribute:: COARSEN

   Show the coarsening progress
.. attribute:: REFINE

   Show the refinement progress
.. attribute:: IPART

   Show info on initial partitioning
.. attribute:: MOVEINFO

   Show info on vertex moves during refinement
.. attribute:: SEPINFO

   Show info in vertex moves during se refinement
.. attribute:: CONNINFO

   Show info on minimization of subdomain connectivity
.. attribute:: CONTIGINFO

   Show info on elimination of connected components
.. attribute:: MEMORY

   Show info related to wspace allocation
"""

from pymetis._internal import ObjType


ObjType.__doc__ = """A wrapper for METIS objective codes.

.. attribute:: CUT
.. attribute:: VOL
.. attribute:: NODE
"""

# Create Named Tuple for Mesh Partition
from collections import namedtuple


MeshPartition = namedtuple("MeshPartition",
    ["edge_cuts", "element_part", "vertex_part"])
MeshPartition.__doc__ = """A named tuple for describing the partitioning of a
mesh.

.. attribute:: edge_cuts

   Number of edges which needed cutting to form partitions
.. attribute:: element_part

   List with element partition indices
.. attribute:: vertex_part

   List with vertex partition indices
"""


# {{{ Options handling

def _options_get_index(name):
    if not name.islower():
        raise AttributeError(name)
    from pymetis._internal import options_indices
    return getattr(options_indices, name.upper())


class Options(OptionsBase):
    """See the `METIS manual
    <http://glaros.dtc.umn.edu/gkhome/fetch/sw/metis/manual.pdf>`__
    for context.

    .. attribute:: ncuts
    .. attribute:: nseps
    .. attribute:: numbering
    .. attribute:: niter
    .. attribute:: minconn
    .. attribute:: no2hop
    .. attribute:: seed
    .. attribute:: contig
    .. attribute:: compress
    .. attribute:: ccorder
    .. attribute:: pfactor
    .. attribute:: ufactor
    """

    def __init__(self, **kwargs):
        super().__init__()
        for name, val in kwargs.items():
            setattr(self, name, val)

    def __getattr__(self, name):
        return self._get(_options_get_index(name))

    def __setattr__(self, name, value):
        if not isinstance(value, int):
            raise TypeError("METIS options accept only integer values.")
        self._set(_options_get_index(name), value)

# }}}


def verify_nd(perm, iperm):
    from pymetis._internal import verify_nd
    return verify_nd(perm, iperm)


def _prepare_graph(adjacency, xadj, adjncy):
    if adjacency is not None:
        assert xadj is None
        assert adjncy is None

        xadj = [0]
        adjncy = []

        for i in range(len(adjacency)):
            adj = adjacency[i]
            if adj is not None and len(adj):
                assert max(adj) < len(adjacency)
            adjncy += list(map(int, adj))
            xadj.append(len(adjncy))
    else:
        assert xadj is not None
        assert adjncy is not None

    return xadj, adjncy


def nested_dissection(adjacency=None, xadj=None, adjncy=None, options=None):
    """This function computes fill reducing orderings of sparse matrices using
    the multilevel nested dissection algorithm.

    The input graph is given as either a Pythonic way as the *adjacency* parameter
    or in the direct C-like way that Metis likes as *xadj* and *adjncy*. It
    is an error to specify both graph inputs.
    """
    xadj, adjncy = _prepare_graph(adjacency, xadj, adjncy)

    if options is None:
        options = Options()

    if options.numbering not in [-1, 0]:
        raise ValueError("METIS numbering option must be set to 0 or the default")

    from pymetis._internal import edge_nd
    return edge_nd(xadj, adjncy, options)


def part_graph(nparts, adjacency=None, xadj=None, adjncy=None,
        vweights=None, eweights=None, recursive=None, contiguous=None, options=None):
    """Return a partition (cutcount, part_vert) into nparts for an input graph.

    The input graph is given in either a Pythonic way as the *adjacency* parameter
    or in the direct C-like way that Metis likes as *xadj* and *adjncy*. It
    is an error to specify both graph inputs.

    The Pythonic graph specifier *adjacency* is required to have the following
    properties:

    - len(adjacency) needs to return the number of vertices
    - ``adjacency[i]`` needs to be an iterable of vertices adjacent to vertex i.
      Both directions of an undirected graph edge are required to be stored.

    If you would like to use *eweights* (edge weights), you need to use the
    xadj/adjncy way of specifying graph connectivity. This works as follows:

        The adjacency structure of the graph is stored as follows: The
        adjacency list of vertex *i* is stored in array *adjncy* starting at
        index ``xadj[i]`` and ending at (but not including) index ``xadj[i +
        1]``. That is, for each vertex i, its adjacency list is stored in
        consecutive locations in the array *adjncy*, and the array *xadj* is
        used to point to where it begins and where it ends.

        The weights of the edges (if any) are stored in an additional array
        called *eweights*. This array contains *2m* elements (where *m* is the
        number of edges, taking into account the undirected nature of the
        graph), and the weight of edge ``adjncy[j]`` is stored at location
        ``eweights[j]``. The edge-weights must be integers greater than zero. If
        all the edges of the graph have the same weight (i.e., the graph is
        unweighted), then the eweight can be set to ``None``.

    METIS runtime options can be specified by supplying an :class:`Options` object in
    the input.

    (quoted with slight adaptations from the Metis docs)
    """
    xadj, adjncy = _prepare_graph(adjacency, xadj, adjncy)

    if recursive is None:
        recursive = nparts <= 8

    if options is None:
        options = Options()

    if contiguous is True:
        # Check that the contiguous flag isn't set twice
        if options.contig != -1:
            raise RuntimeError(
                "Contiguous setting should be specified either through "
                "`options` OR through the `contiguous` flag.")

        from warnings import warn
        warn("Passing the 'contiguous' flag is deprecated. Pass the equivalent "
                "flag in Options instead. This will go stop working in 2022.",
                DeprecationWarning, stacklevel=2)

        options.contig = True

    if options.numbering not in [-1, 0]:
        raise ValueError("METIS numbering option must be set to 0 or the default")

    if nparts == 1:
        # metis has a bug in this case--it disregards the index base
        return 0, [0] * (len(xadj) - 1)

    from pymetis._internal import part_graph
    return part_graph(nparts, xadj, adjncy, vweights,
                      eweights, options, recursive)


def part_mesh(n_parts, connectivity, options=None, tpwgts=None, gtype=None,
              ncommon=1):
    """This function is used to partition a mesh into *n_parts* parts based on a
    graph partitioning where each vertex is a node in the graph. A mesh is a
    collection of non-overlapping elements which are identified by their vertices.
    An element can have a different number of vertices based on its topology,
    ie 3 -> triangular, 6 -> prism, 8 -> hexahedron.

    The mesh *connectivity* is specified as a list of elements where each element
    is specified by a list of its vertex indices,
    eg ``[ [0, 1, 5, 4], [1, 2, 6, 5], ... ]``. The spatial points, which make up the
    element vertices, are needed for defining the mesh, but are not needed for
    partitioning.

    - ``len(connectivity)`` should be the number of elements in mesh

    METIS expects a connectivity which is flattened with a corresponding offset
    vector that points to the beginning and end of each element declaration. This
    connectivity format allows a mesh to be comprised of multiple element types, eg
    triangles and quads. The ``part_mesh`` method will deduce these vectors based on
    the *connectivity* supplied.

    METIS runtime options can be specified by supplying an :class:`Options`
    object in the input.

    ``tpwgts`` is a list of size ``n_parts`` that specifies the desired weight
    for each partition.

    ``gtype`` specifies the partitioning is based on a nodal/dual graph of the mesh.
    It has to be one of :attr:`GType.NODAL` or :attr:`GType.DUAL`.

    ``ncommon`` is needed when ``gtype = GType.DUAL``. It Specifies the number of
    common nodes that two elements must have in order to put an edge between them
    in the dual graph. For example, for tetrahedron meshes, ncommon should be 3,
    which creates an edge between two tets when they share a triangular face
    (i.e., 3 nodes).

    Returns a namedtuple of ``(edge_cuts, element_part, vertex_part)``, where
    ``edge_cuts`` is the number of cuts to the connectivity graph, ``element_part``
    is an array of length n_elements, with entries identifying the element's
    partition index, and ``vertex_part`` is an array of length n_vertices with
    entries identifying the vertex's partition index.
    """

    # Generate flattened connectivity with offsets array, suitable for Metis
    from itertools import accumulate
    conn = [it for cell in connectivity for it in cell]
    conn_offset = [0, *accumulate([len(cell) for cell in connectivity])]

    n_elements = len(connectivity)
    n_vertex = len(set(conn))

    # Handle option validation
    if options is None:
        options = Options()

    if options.numbering not in [-1, 0]:
        raise ValueError("METIS numbering option must be set to 0 or the default")

    if tpwgts is None:
        tpwgts = []
    if len(tpwgts) > 0:
        if len(tpwgts) != n_parts:
            raise RuntimeError("The length of tpwgts mismatches `n_part`")

        if any(w < 0.0 for w in tpwgts):
            raise ValueError("The values of tpwgts should be non-negative")

        # rescale tpwgts to ensure sum(tpwgts) == 1
        total_weights = sum(tpwgts)
        tpwgts = [w / total_weights for w in tpwgts]

    if gtype is None:
        from pymetis._internal import GType
        gtype = GType.NODAL

    # Trivial partitioning
    if n_parts < 2:
        return MeshPartition(0, [0] * n_elements, [0] * n_vertex)

    from pymetis._internal import part_mesh
    return MeshPartition(*part_mesh(n_parts, conn_offset, conn,
        tpwgts, gtype, n_elements, n_vertex, ncommon, options))

# vim: foldmethod=marker

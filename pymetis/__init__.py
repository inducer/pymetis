"""
.. autofunction:: nested_dissection
.. autofunction:: part_graph
.. autofunction:: verify_nd

.. autoclass:: Options
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

from six.moves import map, range

from pymetis.version import version, version_tuple  # noqa
from pymetis._internal import Options as OptionsBase


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
        ``adjwgt[j]``. The edge-weights must be integers greater than zero. If
        all the edges of the graph have the same weight (i.e., the graph is
        unweighted), then the adjwgt can be set to ``None``.

    METIS runtime options can be specified by supplying an :class:`Options` object in
    the input.

    (quoted with slight adaptations from the Metis docs)
    """
    xadj, adjncy = _prepare_graph(adjacency, xadj, adjncy)

    if recursive is None:
        if nparts > 8:
            recursive = False
        else:
            recursive = True

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
        return 0, [0] * (len(xadj)-1)

    from pymetis._internal import part_graph
    return part_graph(nparts, xadj, adjncy, vweights,
                      eweights, options, recursive)

# vim: foldmethod=marker

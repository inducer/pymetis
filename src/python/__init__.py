version = "0.90"




def part_graph(nparts, adjacency=None, xadj=None, adjncy=None, 
        vweights=None, eweights=None, recursive=None):
    """Return a partition (cutcount, part_vert) into nparts for an input graph.

    The input graph is given as either a Pythonic way as the `adjacency' parameter
    or in the direct C-like way that Metis likes as `xadj' and `adjncy'. It
    is an error to specify both graph inputs.

    The Pythonic graph specifier `adjacency' is required to have the following
    properties:

    - len(adjacency) needs to return the number of vertices
    - adjacency[i] needs to return an iterable of vertices adjacent to vertex i.
      Both directions of an undirected graph edge are required to be stored.

    For details on how `xadj' and `adjncy' are specified, see the Metis 
    documentation.
    """

    if recursive is None:
        if nparts > 8:
            recursive = False
        else:
            recursive = True

    from pymetis._internal import part_graph

    if adjacency is not None:
        assert xadj is None
        assert adjncy is None

        xadj = [0]
        adjncy = []

        for i in range(len(adjacency)):
            adj = adjacency[i]
            assert max(adj) < len(adjacency)
            adjncy += adj
            xadj.append(len(adjncy))
    else:
        assert xadj is not None
        assert adjncy is not None

    return part_graph(nparts, xadj, adjncy, vweights, eweights, recursive)

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

from warnings import catch_warnings

import numpy as np
import pytest

import pymetis


def test_tet_mesh(visualize=False):
    pytest.importorskip("meshpy")

    from math import cos, pi, sin

    from meshpy.geometry import (
        EXT_CLOSED_IN_RZ,
        GeometryBuilder,
        generate_surface_of_revolution,
    )
    from meshpy.tet import MeshInfo, build

    pytest.importorskip("meshpy")

    big_r = 3
    little_r = 1.5

    points = 50
    dphi = 2 * pi / points

    rz = np.array([[big_r + little_r * cos(i * dphi), little_r * sin(i * dphi)]
            for i in range(points)])

    geo = GeometryBuilder()
    geo.add_geometry(
            *generate_surface_of_revolution(rz,
                closure=EXT_CLOSED_IN_RZ, radial_subdiv=20))

    mesh_info = MeshInfo()
    geo.set(mesh_info)

    mesh = build(mesh_info)

    def tet_face_vertices(vertices):
        return [(vertices[0], vertices[1], vertices[2]),
                (vertices[0], vertices[1], vertices[3]),
                (vertices[0], vertices[2], vertices[3]),
                (vertices[1], vertices[2], vertices[3]),
                ]

    face_map = {}
    for el_id, el in enumerate(mesh.elements):
        for fid, face_vertices in enumerate(tet_face_vertices(el)):
            face_map.setdefault(frozenset(face_vertices), []).append((el_id, fid))

    adjacency: dict[int, list[int]] = {}
    for _face_vertices, els_faces in face_map.items():
        if len(els_faces) == 2:
            (e1, _f1), (e2, _f2) = els_faces
            adjacency.setdefault(e1, []).append(e2)
            adjacency.setdefault(e2, []).append(e1)

    _cuts, part_vert = pymetis.part_graph(17, adjacency)

    if visualize:
        import pyvtk

        vtkelements = pyvtk.VtkData(
            pyvtk.UnstructuredGrid(mesh.points, tetra=mesh.elements),
            "Mesh",
            pyvtk.CellData(pyvtk.Scalars(part_vert, name="partition")))
        vtkelements.tofile("split.vtk")


def test_cliques():
    adjacency_list = [
        np.array([1, 2]),
        np.array([0, 2]),
        np.array([0, 1])
    ]

    num_clusters = 2
    pymetis.part_graph(num_clusters, adjacency=adjacency_list)


def test_unconnected():
    adjacency_list = [
        np.array([2]),
        np.array([]),
        np.array([0])
    ]

    num_clusters = 2
    pymetis.part_graph(num_clusters, adjacency=adjacency_list,
            options=pymetis.Options(contig=False))


def test_part_graph_with_weights():
    def grid_adjacency(nx, ny):
        def idx(i, j):
            return i + j * (nx + 1)

        adj = []
        for j in range(ny + 1):
            for i in range(nx + 1):
                nbr = []
                if i > 0:
                    nbr.append(idx(i - 1, j))
                if i < nx:
                    nbr.append(idx(i + 1, j))
                if j > 0:
                    nbr.append(idx(i, j - 1))
                if j < ny:
                    nbr.append(idx(i, j + 1))
                adj.append(np.array(nbr))
        return adj

    adj = grid_adjacency(9, 9)
    tpwgts = [0.8, 0.2]
    _cuts, parts = pymetis.part_graph(2, adjacency=adj, tpwgts=tpwgts)
    counts = [parts.count(it) for it in range(2)]
    nvert = len(adj)
    assert counts == [int(nvert * tpwgts[0]), int(nvert * tpwgts[1])]


def test_zero_copy():
    tp = pymetis.zero_copy_dtype()

    # make sure it does warn about copies
    with pytest.warns(BytesWarning):
        pymetis.part_graph(2, pymetis.CSRAdjacency(
                    adj_starts=[0, 2, 4, 6, 6],
                    adjacent=np.array([1, 2, 0, 2, 1, 3], tp)),
                    warn_on_copies=True)

    # make sure it does warn about copies
    with pytest.warns(BytesWarning):
        pymetis.part_graph(2, pymetis.CSRAdjacency(
                    adj_starts=np.array([0, 2, 4, 6, 6], np.int16),
                    adjacent=np.array([1, 2, 0, 2, 1, 3], tp)),
                    warn_on_copies=True)

    # make sure array code does not copy
    with catch_warnings(record=True) as wlist:
        pymetis.part_graph(2, pymetis.CSRAdjacency(
                    adj_starts=np.array([0, 2, 4, 6, 6], tp),
                    adjacent=np.array([1, 2, 0, 2, 1, 3], tp)),
                    warn_on_copies=True)

        assert not wlist


def test_nested_dissection():
    pytest.importorskip("scipy")

    import scipy.sparse

    fmat = scipy.sparse.rand(100, 100, density=0.005)
    mmat = fmat.transpose() * fmat
    adjacency_list = [mmat.getrow(i).indices for i in range(mmat.shape[0])]
    node_nd = pymetis.nested_dissection(adjacency=adjacency_list)
    perm, iperm = np.array(node_nd[0]), np.array(node_nd[1])

    assert np.all(perm[iperm] == np.array(range(perm.size)))


def test_options():
    opt = pymetis.Options()
    assert opt.numbering == -1  # apparently the default
    opt.numbering = 0
    assert opt.numbering == 0
    opt.contig = 1
    assert opt.contig == 1
    opt.seed = 123456
    assert opt.seed == 123456
    opt.ncuts = 5
    assert opt.ncuts == 5
    opt.nseps = 5
    assert opt.nseps == 5
    opt.niter = 100
    assert opt.niter == 100
    opt.no2hop = 1
    assert opt.no2hop == 1
    opt.compress = 1
    assert opt.compress == 1
    opt.pfactor = 100
    assert opt.pfactor == 100
    opt.ufactor = 100
    assert opt.ufactor == 100

    with pytest.raises(AttributeError):
        opt.yoink = 100
    with pytest.raises(AttributeError):
        opt.yoink  # noqa: B018

    # Test a small example case with the options set
    adjacency_list = [
        np.array([1, 2]),
        np.array([0, 2]),
        np.array([0, 1])
    ]

    num_clusters = 2
    _n_cuts, _parts = pymetis.part_graph(
        num_clusters,
        adjacency=adjacency_list,
        options=opt
    )


def test_enum():
    from pymetis._internal import (
        CType,
        DebugLevel,
        GType,
        IPType,
        ObjType,
        OptionKey,
        OPType,
        PType,
        RType,
        Status,
    )

    assert isinstance(Status.OK, int)
    assert isinstance(OPType.PMETIS, int)
    assert isinstance(OptionKey.PTYPE, int)
    assert isinstance(PType.KWAY, int)
    assert isinstance(GType.DUAL, int)
    assert isinstance(CType.RM, int)
    assert isinstance(IPType.GROW, int)
    assert isinstance(RType.FM, int)
    assert isinstance(DebugLevel.INFO, int)
    assert isinstance(ObjType.CUT, int)


if __name__ == "__main__":
    import sys
    if len(sys.argv) > 1:
        exec(sys.argv[1])
    else:
        from pytest import main
        main([__file__])

# vim: fdm=marker

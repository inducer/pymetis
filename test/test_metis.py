from __future__ import division, absolute_import

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

from six.moves import range

import numpy as np
import pymetis
import pytest


def test_tet_mesh(visualize=False):
    from math import pi, cos, sin
    from meshpy.tet import MeshInfo, build
    from meshpy.geometry import \
            GeometryBuilder, generate_surface_of_revolution, EXT_CLOSED_IN_RZ

    pytest.importorskip("meshpy")

    big_r = 3
    little_r = 1.5

    points = 50
    dphi = 2*pi/points

    rz = np.array([[big_r+little_r*cos(i*dphi), little_r*sin(i*dphi)]
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

    adjacency = {}
    for face_vertices, els_faces in face_map.items():
        if len(els_faces) == 2:
            (e1, f1), (e2, f2) = els_faces
            adjacency.setdefault(e1, []).append(e2)
            adjacency.setdefault(e2, []).append(e1)

    cuts, part_vert = pymetis.part_graph(17, adjacency)

    if visualize:
        import pyvtk

        vtkelements = pyvtk.VtkData(
            pyvtk.UnstructuredGrid(mesh.points, tetra=mesh.elements),
            "Mesh",
            pyvtk.CellData(pyvtk.Scalars(part_vert, name="partition")))
        vtkelements.tofile('split.vtk')


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
    pymetis.part_graph(num_clusters, adjacency=adjacency_list)


def test_nested_dissection():
    scipy = pytest.importorskip("scipy")
    import scipy.sparse

    F = scipy.sparse.rand(100, 100, density=0.005)
    M = F.transpose() * F
    adjacency_list = [M.getrow(i).indices for i in range(M.shape[0])]
    node_nd = pymetis.nested_dissection(adjacency=adjacency_list)
    perm, iperm = np.array(node_nd[0]), np.array(node_nd[1])

    assert np.all(perm[iperm] == np.array(range(perm.size)))


if __name__ == "__main__":
    import sys
    if len(sys.argv) > 1:
        exec(sys.argv[1])
    else:
        from pytest import main
        main([__file__])

# vim: fdm=marker

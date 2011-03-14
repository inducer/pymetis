def main():
    import numpy as np
    from math import pi, cos, sin
    from meshpy.tet import MeshInfo, build
    from meshpy.geometry import \
            GeometryBuilder, generate_surface_of_revolution, EXT_CLOSED_IN_RZ

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
        return [(vertices[0],vertices[1],vertices[2]), 
                (vertices[0],vertices[1],vertices[3]),
                (vertices[0],vertices[2],vertices[3]),
                (vertices[1],vertices[2],vertices[3]),
                ]

    face_map = {}
    for el_id, el in enumerate(mesh.elements):
        for fid, face_vertices in enumerate(tet_face_vertices(el)):
            face_map.setdefault(frozenset(face_vertices), []).append((el_id, fid))

    adjacency = {}
    for face_vertices, els_faces in face_map.iteritems():
        if len(els_faces) == 2:
            (e1, f1), (e2, f2) = els_faces
            adjacency.setdefault(e1, []).append(e2)
            adjacency.setdefault(e2, []).append(e1)

    from pymetis import part_graph

    cuts, part_vert = part_graph(17, adjacency)

    import pyvtk
    vtkelements = pyvtk.VtkData(
        pyvtk.UnstructuredGrid(mesh.points, tetra=mesh.elements),
        "Mesh",
        pyvtk.CellData(pyvtk.Scalars(part_vert, name="partition")))
    vtkelements.tofile('split.vtk')











if __name__ == "__main__":
    main()


from numpy import linspace
import pymetis
import pytest


def test_2d_quad_mesh(vis=False):
    n_cells_x = 70
    n_cells_y = 50
    points, connectivity = generate_mesh_2d(n_cells_x, n_cells_y)

    n_part = 4
    n_cuts, elem_part, vert_part = pymetis.part_mesh(n_part, connectivity)

    print(n_cuts)
    print([elem_part.count(it) for it in range(n_part)])
    print([vert_part.count(it) for it in range(n_part)])

    if vis:
        import pyvtk
        vtkelements = pyvtk.VtkData(
            pyvtk.UnstructuredGrid(points, quad=connectivity),
            "Mesh",
            pyvtk.CellData(pyvtk.Scalars(elem_part, name="Rank"))
        )
        vtkelements.tofile("quad.vtk")

    # Assertions about partition
    assert min(elem_part) == 0
    assert max(elem_part) == n_part-1
    assert min(vert_part) == 0
    assert max(vert_part) == n_part-1

    assert len(elem_part) == n_cells_x*n_cells_y
    assert len(vert_part) == (n_cells_x+1)*(n_cells_y+1)

    # Test that the partition assigns approx the same number of elements/vertices
    # to each partition
    n_elem = n_cells_x*n_cells_y
    elem_count = [elem_part.count(it) for it in range(n_part)]
    assert elem_count == pytest.approx(
        [float(n_elem)/float(n_part)] * n_part, rel=0.1)

    # Test that the partition assigns approx the same number of elements/vertices
    # to each partition
    n_vert = (n_cells_x+1)*(n_cells_y+1)
    vert_count = [vert_part.count(it) for it in range(n_part)]
    assert vert_count == pytest.approx(
        [float(n_vert)/float(n_part)] * n_part, rel=0.1)


def test_2d_trivial_mesh_part():
    n_cells_x = 70
    n_cells_y = 50
    _, connectivity = generate_mesh_2d(n_cells_x, n_cells_y)

    n_cuts, elem_part, vert_part = pymetis.part_mesh(1, connectivity)
    assert n_cuts == 0
    assert elem_part == [0] * (n_cells_x*n_cells_y)
    assert vert_part == [0] * ((n_cells_x+1)*(n_cells_y+1))


def test_3d_hex_mesh_part(vis=False):
    n_cells_x = 70
    n_cells_y = 50
    n_cells_z = 37
    points, connectivity = generate_mesh_3d(n_cells_x, n_cells_y, n_cells_z)

    n_part = 5
    n_cuts, elem_part, vert_part = pymetis.part_mesh(n_part, connectivity)

    if vis:
        import pyvtk
        vtkelements = pyvtk.VtkData(
            pyvtk.UnstructuredGrid(points, hexahedron=connectivity),
            "Mesh",
            pyvtk.CellData(pyvtk.Scalars(elem_part, name="Rank"))
        )
        vtkelements.tofile("hex.vtk")

    # Assertions about partition
    assert min(elem_part) == 0
    assert max(elem_part) == n_part-1
    assert min(vert_part) == 0
    assert max(vert_part) == n_part-1

    assert len(elem_part) == n_cells_x*n_cells_y*n_cells_z
    assert len(vert_part) == (n_cells_x+1)*(n_cells_y+1)*(n_cells_z+1)

    # Test that the partition assigns approx the same number of elements/vertices
    # to each partition
    n_elem = n_cells_x*n_cells_y*n_cells_z
    elem_count = [elem_part.count(it) for it in range(n_part)]
    assert elem_count == pytest.approx(
        [float(n_elem)/float(n_part)] * n_part, rel=0.1)

    # Test that the partition assigns approx the same number of elements/vertices
    # to each partition
    n_vert = (n_cells_x+1)*(n_cells_y+1)*(n_cells_z+1)
    vert_count = [vert_part.count(it) for it in range(n_part)]
    assert vert_count == pytest.approx(
        [float(n_vert)/float(n_part)] * n_part, rel=0.1)


def test_part_mesh_opts():
    # Check that invalid numbering throws error
    opts = pymetis.Options(numbering=1)
    with pytest.raises(Exception) as e:
        pymetis.part_mesh(1, [], opts)
    assert "METIS numbering" in e.value.args[0]


# ==============================================================================
# Helper Functions
# ==============================================================================
def generate_mesh_1d(nx):
    """
    Generate simple 1D mesh connectivity with linear elements, eg

          0 --- 1 --- 2 --- 3 --- 4

    """
    points = [
        (x, 0.0, 0.0)
        for x in linspace(0, nx, nx+1)
    ]
    connectivity = [
        [it, it+1]
        for it in range(nx)
    ]

    return points, connectivity


def generate_mesh_2d(nx, ny):
    """
    Generate simple 2D mesh connectivity with rectangular elements, eg

          10 -- 11 -- 12 -- 13 -- 14
          |     |     |     |     |
          5 --- 6 --- 7 --- 8 --- 9
          |     |     |     |     |
          0 --- 1 --- 2 --- 3 --- 4

    Notice that the element connectivity is specified in a counter
    clockwise fasion.
    """
    points = [
        (x, y, 0.0)
        for y in linspace(0, ny, ny+1)
        for x in linspace(0, nx, nx+1)
    ]
    connectivity = [
        [it+jt*(nx+1), it+jt*(nx+1)+1, it+(jt+1)*(nx+1)+1, it+(jt+1)*(nx+1)]
        for jt in range(ny)
        for it in range(nx)
    ]

    return points, connectivity


def generate_mesh_3d(nx, ny, nz):
    """
    Generate simple 3D mesh connectivity with hexahedral elements, similar to
    2D numbering
    """
    points = [
        (x, y, z)
        for z in linspace(0, nz, nz+1)
        for y in linspace(0, ny, ny+1)
        for x in linspace(0, nx, nx+1)
    ]
    connectivity = [
        [it+jt*(nx+1)+kt*((nx+1)*(ny+1)),
         it+jt*(nx+1)+kt*((nx+1)*(ny+1))+1,
         it+(jt+1)*(nx+1)+kt*((nx+1)*(ny+1))+1,
         it+(jt+1)*(nx+1)+kt*((nx+1)*(ny+1)),
         it+jt*(nx+1)+(kt+1)*((nx+1)*(ny+1)),
         it+jt*(nx+1)+(kt+1)*((nx+1)*(ny+1))+1,
         it+(jt+1)*(nx+1)+(kt+1)*((nx+1)*(ny+1))+1,
         it+(jt+1)*(nx+1)+(kt+1)*((nx+1)*(ny+1))]
        for kt in range(nz)
        for jt in range(ny)
        for it in range(nx)
    ]

    return points, connectivity

from numpy import linspace
import pymetis
import pytest

def test_2d_quad_mesh(vis=False):
    nCells_x = 70
    nCells_y = 50
    points, connectivity = generate_mesh_2D(nCells_x, nCells_y)

    nPart = 4
    nCuts, elemPart, vertPart = pymetis.part_mesh(nPart, connectivity)

    print(nCuts)
    print([elemPart.count(it) for it in range(nPart)])
    print([vertPart.count(it) for it in range(nPart)])

    if vis:
        import pyvtk
        vtkelements = pyvtk.VtkData(
            pyvtk.UnstructuredGrid(points, quad=connectivity),
            "Mesh",
            pyvtk.CellData(pyvtk.Scalars(elemPart, name="Rank"))
        )
        vtkelements.tofile("quad.vtk")

    # Assertions about partition
    assert min(elemPart) == 0
    assert max(elemPart) == nPart-1
    assert min(vertPart) == 0
    assert max(vertPart) == nPart-1

    assert len(elemPart) == nCells_x*nCells_y
    assert len(vertPart) == (nCells_x+1)*(nCells_y+1)

    # Test that the partition assigns approx the same number of elements/vertices
    # to each partition
    nElem = nCells_x*nCells_y
    elemCount = [elemPart.count(it) for it in range(nPart)]
    assert elemCount == pytest.approx([float(nElem)/float(nPart)] * nPart, rel=0.1)
    
    # Test that the partition assigns approx the same number of elements/vertices
    # to each partition
    nVert = (nCells_x+1)*(nCells_y+1)
    vertCount = [vertPart.count(it) for it in range(nPart)]
    assert vertCount == pytest.approx([float(nVert)/float(nPart)] * nPart, rel=0.1)

def test_2D_trivial_mesh_part():
    nCells_x = 70
    nCells_y = 50
    _, connectivity = generate_mesh_2D(nCells_x, nCells_y)

    nCuts, elemPart, vertPart = pymetis.part_mesh(1, connectivity)
    assert nCuts == 0
    assert elemPart == [0] * (nCells_x*nCells_y)
    assert vertPart == [0] * ((nCells_x+1)*(nCells_y+1))

def test_3D_hex_mesh_part(vis=False):
    nCells_x = 70
    nCells_y = 50
    nCells_z = 37
    points, connectivity = generate_mesh_3D(nCells_x, nCells_y, nCells_z)

    nPart = 5
    nCuts, elemPart, vertPart = pymetis.part_mesh(nPart, connectivity)

    if vis:
        import pyvtk
        vtkelements = pyvtk.VtkData(
            pyvtk.UnstructuredGrid(points, hexahedron=connectivity),
            "Mesh",
            pyvtk.CellData(pyvtk.Scalars(elemPart, name="Rank"))
        )
        vtkelements.tofile("hex.vtk")

    # Assertions about partition
    assert min(elemPart) == 0
    assert max(elemPart) == nPart-1
    assert min(vertPart) == 0
    assert max(vertPart) == nPart-1

    assert len(elemPart) == nCells_x*nCells_y*nCells_z
    assert len(vertPart) == (nCells_x+1)*(nCells_y+1)*(nCells_z+1)

    # Test that the partition assigns approx the same number of elements/vertices
    # to each partition
    nElem = nCells_x*nCells_y*nCells_z
    elemCount = [elemPart.count(it) for it in range(nPart)]
    assert elemCount == pytest.approx([float(nElem)/float(nPart)] * nPart, rel=0.1)
    
    # Test that the partition assigns approx the same number of elements/vertices
    # to each partition
    nVert = (nCells_x+1)*(nCells_y+1)*(nCells_z+1)
    vertCount = [vertPart.count(it) for it in range(nPart)]
    assert vertCount == pytest.approx([float(nVert)/float(nPart)] * nPart, rel=0.1)

def test_part_mesh_opts():
    # Check that invalid numbering throws error
    opts = pymetis.Options(numbering=1)
    with pytest.raises(Exception) as e:
        pymetis.part_mesh(1, [], opts)
    assert "METIS numbering" in e.value.args[0]

#===============================================================================
# Helper Functions
#===============================================================================
def generate_mesh_1D(nX):
    """
    Generate simple 1D mesh connectivity with linear elements, eg

          0 --- 1 --- 2 --- 3 --- 4
    
    """
    points = [
        (x,0.0,0.0)
            for x in linspace(0,nX,nX+1)
        ]
    connectivity = [
        [it, it+1]
            for it in range(nX)
        ]

    return points, connectivity


def generate_mesh_2D(nX, nY):
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
        (x,y,0.0)
            for y in linspace(0,nY,nY+1)
            for x in linspace(0,nX,nX+1)
        ]
    connectivity = [
        [it+jt*(nX+1), it+jt*(nX+1)+1, it+(jt+1)*(nX+1)+1, it+(jt+1)*(nX+1)]
            for jt in range(nY)
            for it in range(nX)
        ]

    return points, connectivity

def generate_mesh_3D(nX, nY, nZ):
    """
    Generate simple 3D mesh connectivity with hexahedral elements, similar to
    2D numbering
    """
    points = [
        (x,y,z)
            for z in linspace(0,nZ,nZ+1)
            for y in linspace(0,nY,nY+1)
            for x in linspace(0,nX,nX+1)
        ]
    connectivity = [
        [it+jt*(nX+1)+kt*((nX+1)*(nY+1)), it+jt*(nX+1)+kt*((nX+1)*(nY+1))+1,
         it+(jt+1)*(nX+1)+kt*((nX+1)*(nY+1))+1, it+(jt+1)*(nX+1)+kt*((nX+1)*(nY+1)),
         it+jt*(nX+1)+(kt+1)*((nX+1)*(nY+1)), it+jt*(nX+1)+(kt+1)*((nX+1)*(nY+1))+1,
         it+(jt+1)*(nX+1)+(kt+1)*((nX+1)*(nY+1))+1, it+(jt+1)*(nX+1)+(kt+1)*((nX+1)*(nY+1))
         ]
            for kt in range(nZ)
            for jt in range(nY)
            for it in range(nX)
        ]

    return points, connectivity
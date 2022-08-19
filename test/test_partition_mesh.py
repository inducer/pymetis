from tkinter import N
from numpy import linspace
import pymetis
import pytest

def test_2d_quad_mesh(vis=False):
    nCells_x = 70
    nCells_y = 50

    # Generate simple 2D mesh connectivity with rectangular elements, eg
    #
    #       10 -- 11 -- 12 -- 13 -- 14
    #       |     |     |     |     |
    #       5 --- 6 --- 7 --- 8 --- 9
    #       |     |     |     |     |
    #       0 --- 1 --- 2 --- 3 --- 4
    #
    # Notice that the element connectivity is specified in a counter
    # clockwise fasion.
    points = [(x,y,0.0) for y in linspace(0,nCells_y,nCells_y+1) for x in linspace(0,nCells_x,nCells_x+1)]
    connectivity = [[it+jt*(nCells_x+1), it+jt*(nCells_x+1)+1, it+(jt+1)*(nCells_x+1)+1, it+(jt+1)*(nCells_x+1)] for jt in range(nCells_y) for it in range(nCells_x)]

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
    assert max(elemPart) == 3
    assert min(vertPart) == 0
    assert max(vertPart) == 3

    assert len(elemPart) == nCells_x*nCells_y
    assert len(vertPart) == (nCells_x+1)*(nCells_y+1)

    nElem = nCells_x*nCells_y
    elemCount = [elemPart.count(it) for it in range(nPart)]
    assert elemCount == pytest.approx([float(nElem)/float(nPart)] * nPart, rel=0.1)
    
    nVert = (nCells_x+1)*(nCells_y+1)
    vertCount = [vertPart.count(it) for it in range(nPart)]
    assert vertCount == pytest.approx([float(nVert)/float(nPart)] * nPart, rel=0.1)

def test_2D_trivial_mesh_part():
    nCells_x = 70
    nCells_y = 50

    connectivity = [[it+jt*(nCells_x+1), it+jt*(nCells_x+1)+1, it+(jt+1)*(nCells_x+1)+1, it+(jt+1)*(nCells_x+1)] for jt in range(nCells_y) for it in range(nCells_x)]

    nCuts, elemPart, vertPart = pymetis.part_mesh(1, connectivity)
    assert nCuts == 0
    assert elemPart == [0] * (nCells_x*nCells_y)
    assert vertPart == [0] * ((nCells_x+1)*(nCells_y+1))
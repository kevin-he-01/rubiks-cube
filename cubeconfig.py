# Generate cube configuration numbers
from typing import Iterable
from enum import Enum, auto

ORIENT = 8
NCORNERS = 8

def uint8_ts_to_uint64_t(uint8: Iterable[int]) -> int:
    return int.from_bytes(bytes(uint8), byteorder='little')

def to_cube_t(corners: Iterable[tuple[int, int]]):
    uint8_array = [ORIENT * orient + pos for pos, orient in corners]
    return uint8_ts_to_uint64_t(uint8_array)

SOLVED_CUBE: list[tuple[int, int]] = [(i, 0) for i in range(NCORNERS)]

cube = SOLVED_CUBE[:]

# Swap fronts
# cube[0] = (1, 1)
# cube[1] = (0, 2)
# print(cube)
# print(to_cube_t(cube))

# Formula (least quarter turns): U U F U R R U R F U' F R F
#                            OR: U2 F U R2 U R F U' F R F

# 3 corner counterclockwise (last step)
cube[0] = (1, 0)
cube[1] = (2, 0)
cube[2] = (0, 0)
print(to_cube_t(cube))
# Formula: F F R U F F U' F R F R' F
# Formula: F2 R U F2 U' F R F R' F

# 3 corner clockwise (last step)
cube[0] = (2, 0)
cube[1] = (0, 0)
cube[2] = (1, 0)
print(to_cube_t(cube))
# Quarter formula: R U R F' R F R R F U F F
# Quarter formula: R U R F' R F R2 F U F2

class Face(Enum):
    U = auto()
    F = auto()
    R = auto()
    D = auto()
    B = auto()
    L = auto()

face_coord: dict[Face, int] = {
    Face.U: 0,
    Face.F: 0,
    Face.R: 0,
    Face.D: 0b100,
    Face.B: 0b010,
    Face.L: 0b001,
}

opposite_faces: dict[Face, Face] = {
    Face.U: Face.D,
    Face.F: Face.B,
    Face.R: Face.L,
    Face.D: Face.U,
    Face.B: Face.F,
    Face.L: Face.R,
}

def get_coord(face: Face):
    return face_coord[face]

def get_opposite(face: Face):
    return opposite_faces[face]

# def get_faces(corner: int):
#     pass

# assuming backmost (7) corner is at the correct position already
# def cube_state_from_faces(faces: dict[Face, list[str]]):
#     # Will auto infer configuration
#     corners: list[list[str]] = [[] for _ in range(NCORNERS)]
#     assert len(faces) == 6
#     for face, colors in faces.items():
#         corner_num = 000

# Auto infer opposing faces, assume corner 7 is in the right position
# All corner description is ABC where A, B, C are colors with
# A pointing either up or down, and ABC are colors shown counterclockwise
def cube_state_from_corners(corners: list[str]):
    color2face: dict[str, Face] = {}
    assert len(corners) == NCORNERS, 'Wrong number of corners'
    colors: set[str] = set()
    for corner in corners:
        assert len(corner) == 3, 'Corner must have length 3'
        colors.update(corner)
    assert len(colors) == 6, 'Must have exactly 6 colors'
    print('Set of colors:', colors)
    lastcorner = corners[-1]
    color2face[lastcorner[0]] = Face.D
    color2face[lastcorner[1]] = Face.L
    color2face[lastcorner[2]] = Face.B
    pairings = list(color2face.items())
    for color, face in pairings:
        oppose_face = get_opposite(face)
        candidate_rcolors = set(colors) # Candidate opposite colors
        for corner in corners:
            if color in corner:
                candidate_rcolors.difference_update(corner)
        assert len(candidate_rcolors) == 1, f'Invalid cube: candidate reverses for {color}: {candidate_rcolors}'
        rcolor = candidate_rcolors.pop()
        print(f'Opposite color of {color}: {rcolor}')
        color2face[rcolor] = oppose_face
    print(f'Inferred opposite relations: {color2face}')
    ocolors = '' # Orientation determining colors
    for color, face in color2face.items():
        if face in [Face.U, Face.D]:
            ocolors += color
    print(f'Orientation determining colors: {ocolors}')
    assert len(ocolors) == 2
    positions = [sum(get_coord(color2face[color]) for color in corner) for corner in corners]
    print('Positions   :', positions)
    assert set(positions) == set(range(NCORNERS))
    assert positions[7] == 7 # assume corner 7 is in the right position
    # orientations = [sum(corner.index(ocolor) for ocolor in ocolors) for corner in corners]
    orientations: list[int] = []
    for corner in corners:
        for i, c in enumerate(corner):
            if c in ocolors:
                orientations.append(i)
                break
        else:
            assert False, f'Bad corner: {corner}'
    print('Orientations:', orientations)
    return to_cube_t(zip(positions, orientations))

# Test
# Front face: Green/blue checkerboard
# F R R U' R R F R U R'
# With YBR on the back
# corners = ['OGW', 'OWB', 'WGR', 'YGO', 'WRB', 'RGY', 'BYO', 'YBR']
# # corners = ['OBY', 'OYG', 'YBR', 'WBO', 'YRG', 'RBW', 'GWO', 'WGR'] # synonymous cube with WGR on the back
# cubestate = cube_state_from_corners(corners)
# print('Cube state: ', cubestate)

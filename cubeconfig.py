# Generate cube configuration numbers
from typing import Iterable
import sys
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

# # 3 corner counterclockwise (last step)
# cube[0] = (1, 0)
# cube[1] = (2, 0)
# cube[2] = (0, 0)
# print(to_cube_t(cube))
# # Best quarter turn formula: F F R U F F U' F R F R' F
# # Best half turn formula   : R F' U F2 U' F R F2 R2

# # 3 corner clockwise (last step)
# cube[0] = (2, 0)
# cube[1] = (0, 0)
# cube[2] = (1, 0)
# print(to_cube_t(cube))
# # Quarter formula: R U R F' R F R R F U F F
# # Best half turn : F' R U' R2 U R' F' R2 F2

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
    # print('Set of colors:', colors)
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
        # print(f'Opposite color of {color}: {rcolor}')
        color2face[rcolor] = oppose_face
    # print(f'Inferred opposite relations: {color2face}')
    ocolors = '' # Orientation determining colors
    for color, face in color2face.items():
        if face in [Face.U, Face.D]:
            ocolors += color
    # print(f'Orientation determining colors: {ocolors}')
    assert len(ocolors) == 2
    positions = [sum(get_coord(color2face[color]) for color in corner) for corner in corners]
    # print('Positions   :', positions)
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
    # print('Orientations:', orientations)
    return to_cube_t(zip(positions, orientations))

CUBE_MESH = '''
     +----+
     |A B |
     |C D |
+----+----+----+----+
|E F |G H |I J |K L |
|M N |O P |Q R |S T |
+----+----+----+----+
     |U V |
     |W X |
     +----+
'''

ansi_bg_colors = {
    'Y': '\x1b[43m',
    'W': '\x1b[47m',
    'B': '\x1b[44m',
    'G': '\x1b[42m',
    'R': '\x1b[41m',
    'O': '\x1b[48;5;202m',
}
ANSI_RESET = '\x1b[0m'

ALL_COLORS = 'YWBGRO'
ALL_TILES = 'ABCDEFGHIJKLMNOPQRSTUVWX'
CMAP = dict[str, str]

CORNER_DEF = [
    'DHI', # 0
    'CFG', # 1
    'BJK', # 2
    'ALE', # 3
    'VQP', # 4
    'UON', # 5
    'XSR', # 6
    'WMT', # 7
]

def assign_colors(tile_range: str, colors: str) -> CMAP:
    assert len(colors) == len(tile_range)
    mp: dict[str, str] = {}
    for tile, color in zip(tile_range, colors):
        assert color in ALL_COLORS, f'Unknown color: {color}'
        mp[tile] = color
    return mp

def display_mesh(cmap: CMAP):
    colored = False
    for char in CUBE_MESH:
        if char in ALL_TILES:
            print(ansi_bg_colors[cmap[char]], end='')
        print(char, end='')
        if colored:
            print(ANSI_RESET, end='')
            colored = False
        if char in ALL_TILES:
            colored = True

def cmap_to_cube_state(cmap: CMAP):
    corners: list[str] = [
        ''.join(cmap[c] for c in CORNER_DEF[i])
        for i in range(NCORNERS)
    ]
    return cube_state_from_corners(corners)

def input_cube_range(tile_range: str):
    print(CUBE_MESH)
    colors = input(f'Input tile colors for range {tile_range}> ')
    colors = colors.upper().replace(' ', '')
    if len(colors) != len(tile_range):
        print(f'Wrong number of entries. Expect {len(tile_range)}, got {len(colors)}', file=sys.stderr)
        return None
    for color in colors:
        if color not in ALL_COLORS:
            print(f'Invalid color: {color}', file=sys.stderr)
            return None
    cmap = assign_colors(tile_range, colors)
    display_mesh(cmap)
    if not input('Is the above cube correct? [Y/n] ').upper().startswith('Y'):
        return None
    return cmap

while True:
    try:
        cmap = input_cube_range(ALL_TILES)
    except EOFError:
        print()
        break
    if cmap is None:
        print('Try again')
        continue
    print('Cube state:', cmap_to_cube_state(cmap))

# Test
# Front face: Green/blue checkerboard
# F R R U' R R F R U R'
# With YBR on the back
# corners = ['OGW', 'OWB', 'WGR', 'YGO', 'WRB', 'RGY', 'BYO', 'YBR']
# # corners = ['OBY', 'OYG', 'YBR', 'WBO', 'YRG', 'RBW', 'GWO', 'WGR'] # synonymous cube with WGR on the back
# cubestate = cube_state_from_corners(corners)
# print('Cube state: ', cubestate)

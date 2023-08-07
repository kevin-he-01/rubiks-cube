# Explores the graph of corner permutations (regardless of orientation)
from sage.all import SymmetricGroup
from typing import Any
from queue import Queue

S8 = SymmetricGroup(8)
P = Any

U = S8((1,2,3,4))
F = S8((1,5,6,2))
R = S8((1,4,8,5))
L = S8((2,6,7,3))
D = S8((5,8,7,6))
B = S8((3,7,8,4))

MOVES: list[tuple[str, P]] = [
    ('U', U),
    ('F', F),
    ('R', R),
    ("U'", U ** -1),
    ("F'", F ** -1),
    ("R'", R ** -1),
    # ('L', L),
    # ('D', D),
    # ('B', B),
    # ("L'", L ** -1),
    # ("D'", D ** -1),
    # ("B'", B ** -1),
]

def bfs(target: P):
    path: dict[P,tuple[str, P]|None] = {}
    start = S8.identity()
    q = Queue()
    path[start] = None
    q.put(start)
    while q.qsize():
        v = q.get()
        if v == target:
            # Rewind to find path
            route = []
            index = path[v]
            while index is not None:
                symbol, prev = index
                route.append(symbol)
                index = path[prev]
            route.reverse()
            return route
        for symbol, move in MOVES:
            w = v * move # Sage applies permutations from left to right
            if w in path:
                continue
            path[w] = (symbol, v)
            q.put(w)
    # Unreachable!

def gods_number():
    path: set[P] = set()
    start = S8.identity()
    q = Queue()
    path.add(start)
    q.put((start, 0))
    dist_dist: list[list[P]] = []
    while q.qsize():
        v, dist = q.get()
        while len(dist_dist) <= dist:
            dist_dist.append([])
        dist_dist[dist].append(v)
        for _, move in MOVES:
            w = v * move # Sage applies permutations from left to right
            if w in path:
                continue
            path.add(w)
            q.put((w, dist + 1))
    print(f"God's #: {len(dist_dist) - 1}")
    print('Stats:')
    for i, val in enumerate(dist_dist):
        print(f'{i}: {len(val)}: Ex. {val[:5]}')

# Proof that any permutation is reachable

def show(target):
    moves = bfs(S8(target))
    print('IMPOSSIBLE' if moves is None else ' '.join(moves))

# show(S8((1,2)))
gods_number()
# show(S8((2,8)))
# show(S8([(1,7),(2,8),(3,5),(4,6)]))

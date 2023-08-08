// Pocket cube enumeration
// Inspired by https://medium.com/@bradenripple/enumerating-all-possible-combinations-of-a-pocket-cube-using-golang-ad80d7af23b
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <stdint.h>
#include <string>
#include <cstring>
#include <vector>
#include <queue>
#include <utility>
#include <algorithm>

// Orientation indicator place
#define ORIENT 8

#define O1 ORIENT
#define O2 (2 * ORIENT)
#define OMOD (3 * ORIENT)

// Number of corners in cube
#define NCORNERS 8

// Number of "basic" (non-inverted) moves
#define NMOVES 3

struct move_t {
    uint8_t shuffle[NCORNERS]; // position shuffle
    uint8_t orientation[NCORNERS]; // post-orientation modification (x ORIENT)
    char face;
    bool inverted;
};

typedef uint64_t cube_t;

// Non-portable representation
constexpr cube_t INITIAL_STATE = 0x0706050403020100ULL;

move_t moves[2*NMOVES] = {
    move_t {
        //              0, 1, 2, 3, 4, 5, 6, 7
        .shuffle =     {2, 0, 3, 1, 4, 5, 6, 7}, // Permutation: (0 1 3 2)
        .orientation = {0, 0, 0, 0, 0, 0, 0, 0},
        .face = 'U',
        .inverted = false,
    },
    move_t {
        //               0, 1, 2, 3, 4, 5, 6, 7
        .shuffle =     { 1, 5, 2, 3, 0, 4, 6, 7}, // Permutation: (0 4 5 1)
        .orientation = {O2,O1, 0, 0,O1,O2, 0, 0},
        .face = 'F',
        .inverted = false,
    },
    move_t {
        //               0, 1, 2, 3, 4, 5, 6, 7
        .shuffle =     { 4, 1, 0, 3, 6, 5, 2, 7}, // Permutation: (0 2 6 4)
        .orientation = {O1, 0,O2, 0,O2, 0,O1, 0},
        .face = 'R',
        .inverted = false,
    },
    // The rest (inverse operations) will be filled in
};

void print_cube(const cube_t &cube) {
    const uint8_t *cube_val = (const uint8_t *)&cube;
    std::cout << "=== CUBE ===\n";
    for (size_t i = 0; i < NCORNERS; i++) {
        int orientation = cube_val[i] / ORIENT;
        int value = cube_val[i] % ORIENT;
        std::cout << "Corner " << i << ": " << (int)(cube_val[i])
            << " (orient:" << orientation << ", cubie:" << value << ")\n";
    }
    std::cout << "\n";
}

// TODO: use SIMD version of this
cube_t move_cube(const cube_t &cube, const move_t &move) {
    // cube_t result;
    uint8_t result[NCORNERS];
    const uint8_t *cube_val = (const uint8_t *)&cube;
    for (size_t i = 0; i < NCORNERS; i++) {
        result[i] = cube_val[move.shuffle[i]] + move.orientation[i];
        if (result[i] >= OMOD) {
            result[i] -= OMOD;
        }
    }
    cube_t result_cube;
    memcpy(&result_cube, &result, sizeof(result)); // Avoid strict aliasing violation
    return result_cube;
}

void invert_move(move_t &dst, const move_t &src) {
    for (size_t i = 0; i < 8; i++) {
        dst.shuffle[src.shuffle[i]] = i;
        // Derive dst.orientation from its pre-orientation, which is inverted (-x mod 3) post-orientation of src
        dst.orientation[i] = OMOD - src.orientation[src.shuffle[i]];
        if (dst.orientation[i] == OMOD) {
            dst.orientation[i] = 0;
        }
    }
    dst.face = src.face;
    dst.inverted = !src.inverted;
}

void init_inverses() {
    for (int i = 0; i < NMOVES; i++) {
        invert_move(moves[NMOVES + i], moves[i]);
    }
}

std::vector<int> stat_cube() {
    // Compute stats on the cube
    std::vector<int> distance_counts; // count of combinations of each distance from origin
    std::unordered_set<cube_t> visited;
    visited.insert(INITIAL_STATE);
    std::queue<std::pair<cube_t, size_t>> bfs_queue;
    bfs_queue.push(std::make_pair(INITIAL_STATE, 0));
    while (!bfs_queue.empty()) {
        auto &pair = bfs_queue.front();
        auto v = pair.first;
        auto dist = pair.second;
        bfs_queue.pop();
        while (distance_counts.size() <= dist) {
            distance_counts.push_back(0);
        }
        distance_counts[dist]++;
        for (const move_t &move : moves) {
            auto w = move_cube(v, move);
            if (visited.count(w))
                continue;
            visited.insert(w);
            bfs_queue.push(std::make_pair(w, dist + 1));
        }
    }
    return distance_counts;
}

typedef const move_t *movespec;
typedef std::unordered_map<cube_t, std::pair<movespec, cube_t>> cubedb;

void init_db(cubedb &db) {
    db.clear();
    db[INITIAL_STATE] = std::pair<movespec, cube_t>(nullptr, INITIAL_STATE); // Dummy
    std::queue<cube_t> bfs_queue;
    bfs_queue.push(INITIAL_STATE);
    while (!bfs_queue.empty()) {
        cube_t v = bfs_queue.front();
        bfs_queue.pop();
        for (const move_t &move : moves) {
            auto w = move_cube(v, move);
            if (db.count(w))
                continue;
            db[w] = std::make_pair(&move, v);
            bfs_queue.push(w);
        }
    }
}

std::vector<movespec> query_db(cubedb &db, cube_t cube) {
    std::vector<movespec> route;
    cube_t current = cube;
    while (db[current].first) {
        route.push_back(db[current].first);
        current = db[current].second;
    }
    std::reverse(route.begin(), route.end());
    return route;
}

void print_route(const std::vector<movespec> &route) {
    for (movespec spec : route) {
        // Assume non-null
        std::cout << spec->face;
        if (spec->inverted) {
            std::cout << '\'';
        }
        std::cout << ' ';
    }
}

void stats_demo() {
    std::vector<int> stats = stat_cube();
    int total = 0;
    std::cout << "God's #: " << (stats.size() - 1) << "\n";
    for (size_t n = 0; n < stats.size(); n++) {
        std::cout << "n = " << n << ", q = " << stats[n] << "\n";
        total += stats[n];
    }
    std::cout << "Total combos: " << total << "\n";
}

void cubedb_demo() {
    cubedb db;
    std::cout << "Initializing DB...\n";
    init_db(db);
    std::cout << "DB initialized!\n";

    cube_t cube = INITIAL_STATE;
    std::cout << "Route: "; print_route(query_db(db, cube)); std::cout << "\n";
    cube = move_cube(cube, moves[0]);
    std::cout << "Route: "; print_route(query_db(db, cube)); std::cout << "\n";
    cube = move_cube(cube, moves[1]);
    std::cout << "Route: "; print_route(query_db(db, cube)); std::cout << "\n";
    cube = move_cube(cube, moves[2]);
    std::cout << "Route: "; print_route(query_db(db, cube)); std::cout << "\n";
    cube = move_cube(cube, moves[2]);
    std::cout << "Route: "; print_route(query_db(db, cube)); std::cout << "\n";
}

int main(int argc, char **argv) {
    init_inverses();
    if (argc != 2) {
        std::cerr << "usage: ./cube [command]\n";
        return 2;
    }
    std::string command = argv[1];
    if (command == "stats") {
        stats_demo();
    } else if (command == "cubedb") {
        cubedb_demo();
    } else {
        std::cerr << "unknown command: " << command << "\n";
        return 2;
    }
    return 0;
}

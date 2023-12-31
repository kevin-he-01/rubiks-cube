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
#include <chrono>

// Orientation indicator place
#define ORIENT 8

#define O1 ORIENT
#define O2 (2 * ORIENT)
#define OMOD (3 * ORIENT)

// Number of corners in cube
#define NCORNERS 8

struct move_t {
    uint8_t shuffle[NCORNERS]; // position shuffle
    uint8_t orientation[NCORNERS]; // post-orientation modification (x ORIENT)
    char face;
    int turn_count; // a number mod 4, 1 is normal turn, 3 is inverted, 2 is double turn
};

typedef uint64_t cube_t;

// Non-portable representation
constexpr cube_t INITIAL_STATE = 0x0706050403020100ULL;

const std::string VALID_FACES = "UFR";

move_t moves[] = {
    move_t {
        //              0, 1, 2, 3, 4, 5, 6, 7
        .shuffle =     {2, 0, 3, 1, 4, 5, 6, 7}, // Permutation: (0 1 3 2)
        .orientation = {0, 0, 0, 0, 0, 0, 0, 0},
        .face = 'U',
        .turn_count = 1,
    },
    move_t {
        //               0, 1, 2, 3, 4, 5, 6, 7
        .shuffle =     { 1, 5, 2, 3, 0, 4, 6, 7}, // Permutation: (0 4 5 1)
        .orientation = {O2,O1, 0, 0,O1,O2, 0, 0},
        .face = 'F',
        .turn_count = 1,
    },
    move_t {
        //               0, 1, 2, 3, 4, 5, 6, 7
        .shuffle =     { 4, 1, 0, 3, 6, 5, 2, 7}, // Permutation: (0 2 6 4)
        .orientation = {O1, 0,O2, 0,O2, 0,O1, 0},
        .face = 'R',
        .turn_count = 1,
    },
};

std::vector<move_t> allowed_moves;
std::vector<move_t> all_moves;

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

#ifdef SIMD

#include <immintrin.h>
cube_t move_cube(cube_t cube, const move_t &move) {
    uint64_t shuffle;
    memcpy(&shuffle, move.shuffle, sizeof(uint64_t));
    uint64_t orientation;
    memcpy(&orientation, move.orientation, sizeof(uint64_t));
    __m128i packed_cube = _mm_setr_epi64((__m64)cube, (__m64)0ULL);
    __m128i packed_shuffle = _mm_setr_epi64((__m64)shuffle, (__m64)0ULL);
    __m128i packed_orientation = _mm_setr_epi64((__m64)orientation, (__m64)0ULL);
    __m128i modulus = _mm_setr_epi8(
        OMOD,
        OMOD,
        OMOD,
        OMOD,
        OMOD,
        OMOD,
        OMOD,
        OMOD,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0
    );
    packed_cube = _mm_shuffle_epi8(packed_cube, packed_shuffle);
    packed_cube = _mm_add_epi8(packed_cube, packed_orientation);
    __m128i not_overflow = _mm_cmplt_epi8(packed_cube, modulus); // packed_cube < modulus === NOT (packed_cube >= modulus)
    modulus = _mm_andnot_si128(not_overflow, modulus);
    packed_cube = _mm_sub_epi8(packed_cube, modulus);
    return _mm_extract_epi64(packed_cube, 0);
}

#else

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

#endif

void double_move(move_t &dst, const move_t &src) {
    // Works specifically for quarter turns ONLY (turn_count = +- 1)
    for (size_t i = 0; i < 8; i++) {
        dst.shuffle[i] = src.shuffle[src.shuffle[i]];
        // Orientation is not affected by double turns
        dst.orientation[i] = 0;
    }
    dst.face = src.face;
    dst.turn_count = 2;
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
    dst.turn_count = 4 - src.turn_count;
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
        for (const move_t &move : allowed_moves) {
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
        for (const move_t &move : allowed_moves) {
            auto w = move_cube(v, move);
            if (db.count(w))
                continue;
            db[w] = std::make_pair(&move, v);
            bfs_queue.push(w);
        }
    }
}

bool query_db(cubedb &db, std::vector<movespec> &route, cube_t cube) {
    route.clear();
    cube_t current = cube;
    if (!db.count(current)) {
        return false;
    }
    while (db[current].first) {
        route.push_back(db[current].first);
        current = db[current].second;
    }
    std::reverse(route.begin(), route.end());
    return true;
}

void print_route(const std::vector<movespec> &route) {
    for (movespec spec : route) {
        // Assume non-null
        std::cout << spec->face;
        switch (spec->turn_count) {
            case 2:
                std::cout << '2';
                break;
            case 3:
                std::cout << '\'';
                break;
        }
        std::cout << ' ';
    }
}

void print_query_result(cubedb &db, cube_t cube) {
    std::vector<movespec> route;
    if (!query_db(db, route, cube)) {
        std::cout << "NO ROUTE\n";
        return;
    }
    print_route(route);
}

cube_t move_cube_route(cube_t cube, const std::vector<movespec> &route) {
    for (movespec move : route) {
        cube = move_cube(cube, *move);
    }
    return cube;
}

movespec get_movespec(char face, int turn_count) {
    for (const move_t &move : all_moves) {
        if (move.face == face && move.turn_count == turn_count) {
            return &move;
        }
    }
    return nullptr;
}

// Helper function for parse_route
void commit_lastface(std::vector<movespec> &dest, char &lastface, int turn_count = 1) {
    if (!lastface)
        return;
    movespec move = get_movespec(lastface, turn_count);
    if (!move) {
        std::cerr << "ERROR: Move not found: face=" << lastface << ", count=" << turn_count << '\n';
        exit(EXIT_FAILURE);
    }
    dest.push_back(move);
    lastface = '\0';
}

bool parse_route(std::vector<movespec> &dest, std::string &err, const std::string &description) {
    dest.clear();
    char lastface = '\0';
    for (char c : description) {
        if (c == ' ') // filler, can occur anywhere
            continue;
        if (c == '\'') {
            if (!lastface) {
                err = "Invalid usage of prime symbol (')";
                return false;
            }
            commit_lastface(dest, lastface, 3);
        } else if (c == '2') {
            if (!lastface) {
                err = "Invalid usage of half turn specifier";
                return false;
            }
            commit_lastface(dest, lastface, 2);
        } else {
            commit_lastface(dest, lastface);
            if (VALID_FACES.find(c) == std::string::npos) {
                err = std::string("Invalid cube move: ") + c;
                return false;
            }
            lastface = c;
        }
    }
    commit_lastface(dest, lastface);
    return true;
}

void stats_demo(bool quarter_turn_metric) {
    using std::chrono::high_resolution_clock;
    using std::chrono::duration_cast;
    using std::chrono::duration;
    using std::chrono::milliseconds;

    auto t1 = high_resolution_clock::now();
    std::vector<int> stats = stat_cube(); // Computing cube stats
    auto t2 = high_resolution_clock::now();

    duration<double, std::milli> ms_double = t2 - t1;

    int total = 0;
    std::cout << (quarter_turn_metric ? "(Quarter turn metric)\n" : "(Half turn metric)\n");
    std::cout << "God's #: " << (stats.size() - 1) << "\n";
    for (size_t n = 0; n < stats.size(); n++) {
        std::cout << "n = " << n << ", count = " << stats[n] << "\n";
        total += stats[n];
    }
    std::cout << "Total combos: " << total << "\n";
    std::cout << "Took " << ms_double.count() << " ms to compute\n";
}

void cubedb_demo() {
    cubedb db;
    std::cout << "Initializing DB...\n";
    init_db(db);
    std::cout << "DB initialized!\n";

    while (true) {
        std::string input;
        std::cout << "\nInput cube #: ";
        cube_t cube;
        std::cin >> cube;
        if (std::cin.eof()) {
            std::cout << "\nBye!\n";
            break;
        }
        std::cout << "Your cube: "; print_cube(cube);
        std::cout << "Best route to config: "; print_query_result(db, cube); std::cout << "\n";
    }
}

void cubedb_optimize_demo() {
    cubedb db;
    std::cout << "Initializing DB...\n";
    init_db(db);
    std::cout << "DB initialized!\n";

    while (true) {
        std::string input;
        std::vector<movespec> route;
        std::string err;
        std::cout << "\nInput sequence of moves: ";
        std::getline(std::cin, input);
        if (std::cin.eof()) {
            std::cout << "\nBye!\n";
            break;
        }
        if (!parse_route(route, err, input)) {
            std::cout << "Error parsing moves: " << err << '\n';
            continue;
        }
        std::cout << "Your route: "; print_route(route); std::cout << "\n";

        cube_t cube = INITIAL_STATE;
        cube = move_cube_route(cube, route);
        std::cout << "Best route: "; print_query_result(db, cube); std::cout << "\n";
    }
}

void init_allowed_moves(bool quarter_turn_metric) {
    for (const move_t &move : moves) {
        all_moves.push_back(move);
        allowed_moves.push_back(move);
    }
    for (const move_t &move : moves) {
        move_t inverted;
        invert_move(inverted, move);
        all_moves.push_back(inverted);
        allowed_moves.push_back(inverted);
    }
    for (const move_t &move : moves) {
        move_t doubled;
        double_move(doubled, move);
        all_moves.push_back(doubled);
        if (!quarter_turn_metric) {
            allowed_moves.push_back(doubled);
        }
    }
}

void usage(bool full = false) {
    std::cerr << "usage: ./cube [-h] [-q] COMMAND\n";
    if (full) {
        std::cerr
            << "Help:\n"
            << "COMMAND: One of the commands (Ex. stats, cubedb, cubedb-optimize)\n"
            << "-q: Use quarter turn metric (default to half-turn)\n"
            << "-h: Show this help\n"
            ;
    }
    exit(2);
}

int main(int argc, char **argv) {
    bool quarter_turn_metric = false; // default to half-turn metric
    char **argv_end = argv + argc;
    char **curr_arg = argv + 1;
    std::string command;
    bool has_command = false;
    while (curr_arg < argv_end) {
        std::string arg = *curr_arg;
        if (arg.length() >= 1 && arg[0] == '-') {
            // Option arg
            if (arg == "-q") {
                quarter_turn_metric = true;
            } else if (arg == "-h") {
                usage(true);
            } else {
                std::cerr << "Unrecognized option: " << arg << '\n';
                usage();
            }
        } else {
            if (has_command) {
                std::cerr << "Got extra argument: " << arg << '\n';
                usage();
            }
            has_command = true;
            command = arg;
        }
        curr_arg++;
    }
    if (!has_command) {
        std::cerr << "No command specified\n";
        usage();
    }

    init_allowed_moves(quarter_turn_metric);
    if (command == "stats") {
        stats_demo(quarter_turn_metric);
    } else if (command == "cubedb") {
        cubedb_demo();
    } else if (command == "cubedb-optimize") {
        cubedb_optimize_demo();
    } else {
        std::cerr << "unknown command: " << command << "\n";
        return 2;
    }
    return 0;
}

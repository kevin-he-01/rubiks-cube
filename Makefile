cube: cube.cpp
	clang++ -Wall -Wextra -O3 $^ -o $@

cube-profile: cube.cpp
	g++ -Wall -Wextra -g -Og -pg $^ -o $@

cube-profile-o3: cube.cpp
	g++ -Wall -Wextra -g -O3 -pg $^ -o $@

cube-debug: cube.cpp
	clang++ -Wall -Wextra -g $^ -o $@

cube: cube.cpp
	clang++ -Wall -Wextra -O3 $^ -o $@

cube-profile: cube.cpp
	clang++ -Wall -Wextra -O1 -pg $^ -o $@

cube-profile-o3: cube.cpp
	clang++ -Wall -Wextra -O3 -pg $^ -o $@

cube-debug: cube.cpp
	clang++ -Wall -Wextra -g $^ -o $@

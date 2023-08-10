cube: cube.cpp
	clang++ -Wall -Wextra -O3 $^ -o $@

bin/cube-gcc: cube.cpp
	g++ -Wall -Wextra -O3 $^ -o $@

bin/cube-profile: cube.cpp
	g++ -Wall -Wextra -g -Og -pg $^ -o $@

bin/cube-profile-o3: cube.cpp
	g++ -Wall -Wextra -g -O3 -pg $^ -o $@

bin/cube-debug: cube.cpp
	clang++ -Wall -Wextra -Og -g $^ -o $@

clean:
	rm -f cube bin/cube-*

.PHONY: clean

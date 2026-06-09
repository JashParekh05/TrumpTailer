.PHONY: build test test-cpp test-py report

build:
	pip install -e .

test: test-cpp test-py

test-cpp:
	cmake -S . -B build/tests -DTT_BUILD_TESTS=ON
	cmake --build build/tests -j
	ctest --test-dir build/tests --output-on-failure

test-py:
	python3 -m pytest -q

report:
	bash scripts/reproduce.sh

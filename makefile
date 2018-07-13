MAKEFLAGS += --no-print-directory

CC	!= which clang-devel || which clang
CXX	!= which clang++-devel || which clang++
FORMAT	!= which clang-format-devel || which clang-format
SYSDBG	!= which lldb-devel || which lldb || which gdb

APPLICATION	?= ON
BENCHMARK	?= ON
TESTS		?= ON
EXCEPTIONS	?= OFF
RTTI		?= OFF

CMAKE	:= CC=$(CC) CXX=$(CXX) CPPFLAGS="-fdiagnostics-absolute-paths -fcolor-diagnostics" cmake -GNinja
CMAKE	+= -DVCPKG_TARGET_TRIPLET=${VCPKG_DEFAULT_TRIPLET}
CMAKE	+= -DCMAKE_TOOLCHAIN_FILE=${VCPKG}/scripts/buildsystems/vcpkg.cmake
CMAKE	+= -DBUILD_APPLICATION=$(APPLICATION) -DBUILD_BENCHMARK=$(BENCHMARK) -DBUILD_TESTS=$(TESTS)
CMAKE	+= -DENABLE_EXCEPTIONS=$(EXCEPTIONS) -DENABLE_RTTI=$(RTTI)
CMAKE	+= -DCMAKE_INSTALL_PREFIX=$(PWD)

all: build/llvm/debug/CMakeCache.txt
	@cmake --build build/llvm/debug

run: build/llvm/debug/CMakeCache.txt
	@cmake --build build/llvm/debug --target application
	@build/llvm/debug/src/application/ice

debug: build/llvm/debug/CMakeCache.txt
	@cmake --build build/llvm/debug --target application
	@$(SYSDBG) build/llvm/debug/src/application/ice

release: build/llvm/release/CMakeCache.txt
	@cmake --build build/llvm/release --target application
	@build/llvm/release/src/application/ice

test: build/llvm/debug/CMakeCache.txt
	@cmake --build build/llvm/debug --target tests
	@cd build/llvm/debug && ctest

benchmark: build/llvm/release/CMakeCache.txt
	@cmake --build build/llvm/release --target benchmark
	@build/llvm/release/src/benchmark/benchmark

install: build/llvm/release/CMakeCache.txt
	@cmake --build build/llvm/release --target install

build/llvm/debug/CMakeCache.txt: CMakeLists.txt
	@mkdir -p build/llvm/debug; cd build/llvm/debug && \
	  $(CMAKE) -DCMAKE_BUILD_TYPE=Debug $(PWD)

build/llvm/release/CMakeCache.txt: CMakeLists.txt
	@mkdir -p build/llvm/release; cd build/llvm/release && \
	  $(CMAKE) -GNinja -DCMAKE_BUILD_TYPE=Release $(PWD)

format:
	@$(FORMAT) -i `find src -type f -name '*.hpp' -or -name '*.cpp'`

clean:
	@rm -rf build/llvm

uninstall:
	@rm -rf bin include lib

.PHONY: all run debug test benchmark install format clean uninstall

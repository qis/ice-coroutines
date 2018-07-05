# ICE
C++20 Coroutines TS framework.

## Requirements
* [CMake](https://cmake.org/download/) version 3.12 on all platforms
* [Ninja](https://ninja-build.org/) version 1.8 on Linux and FreeBSD
* [LLVM](https://llvm.org/) and [libcxx](https://libcxx.llvm.org/) version 7.0 on Linux and FreeBSD
* [Visual Studio 2017](https://www.visualstudio.com/downloads/) version 15.7 on Windows
* [Vcpkg](https://github.com/Microsoft/vcpkg)

## Dependencies
Set the `VCPKG` and `VCPKG_DEFAULT_TRIPLET` environment variables.

| OS          | VCPKG                    | VCPKG_DEFAULT_TRIPLET |
|-------------|--------------------------|-----------------------|
| Windows     | `C:\Libraries\vcpkg`     | `x64-windows-static`  |
| Linux       | `/opt/vcpkg`             | `x64-linux-clang`     |
| FreeBSD     | `/opt/vcpkg`             | `x64-freebsd-devel`   |

### Windows
Add `%VCPKG%` to the `PATH` environment variable.

```cmd
git clone https://github.com/Microsoft/vcpkg %VCPKG%
cd %VCPKG% && bootstrap-vcpkg.bat && vcpkg integrate install
cd %UserProfile% && rd /q /s "%VCPKG%/toolsrc/Release" "%VCPKG%/toolsrc/vcpkg/Release" ^
  "%VCPKG%/toolsrc/vcpkglib/Release" "%VCPKG%/toolsrc/vcpkgmetricsuploader/Release"
```

### Linux & FreeBSD
Add `${VCPKG}` to the `PATH` environment variable.

```sh
git clone https://github.com/Microsoft/vcpkg ${VCPKG}
rm -rf ${VCPKG}/toolsrc/build.rel; mkdir ${VCPKG}/toolsrc/build.rel && cd ${VCPKG}/toolsrc/build.rel
cmake -GNinja -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_C_COMPILER=`which clang-devel || which clang` \
  -DCMAKE_CXX_COMPILER=`which clang++-devel || which clang++` ..
cmake --build . && cp vcpkg ${VCPKG}/
cat > ${VCPKG}/triplets/${VCPKG_DEFAULT_TRIPLET}.cmake <<EOF
set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)
set(VCPKG_CMAKE_SYSTEM_NAME `uname -s`)
set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE "\${CMAKE_CURRENT_LIST_DIR}/toolchains/${VCPKG_DEFAULT_TRIPLET}.cmake")
EOF
mkdir ${VCPKG}/triplets/toolchains
cat > ${VCPKG}/triplets/toolchains/${VCPKG_DEFAULT_TRIPLET}.cmake <<EOF
set(CMAKE_CROSSCOMPILING OFF CACHE STRING "")
set(CMAKE_SYSTEM_NAME `uname -s` CACHE STRING "")
set(CMAKE_C_COMPILER "`which clang-devel || which clang`" CACHE STRING "")
set(CMAKE_CXX_COMPILER "`which clang++-devel || which clang++`" CACHE STRING "")
set(HAVE_STEADY_CLOCK ON CACHE STRING "")
set(HAVE_STD_REGEX ON CACHE STRING "")
EOF
cd && rm -rf ${VCPKG}/toolsrc/build.rel
```

### Ports
Install Vcpkg ports.

```sh
vcpkg install benchmark fmt gtest openssl zlib
```

**NOTE**: Do not execute `vcpkg` in `cmd.exe` and `bash.exe` at the same time!

## Exceptions
This library uses the following rules:
1. If a function is marked `noexcept`, then it cannot throw.
2. If a function is not marked `noexcept`, then it can throw but is unlikely to.
3. If a constructor is not marked `noexcept`, then there is a `create` member function that can be used instead.

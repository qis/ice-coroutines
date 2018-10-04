@echo off
setlocal enableextensions enabledelayedexpansion

for /f "tokens=2 delims=( " %%i in ('findstr /c:"project(" %~dp0\CMakeLists.txt') do (
  set project=%%i
)

set build=%~dp0\build\msvc
md %build% 2>nul
pushd %build%

cmake -G "Visual Studio 15 2017 Win64" ^
  -DCMAKE_CONFIGURATION_TYPES="Debug;Release" ^
  -DVCPKG_TARGET_TRIPLET=%VCPKG_DEFAULT_TRIPLET% ^
  -DCMAKE_TOOLCHAIN_FILE=%VCPKG%\scripts\buildsystems\vcpkg.cmake ^
  -DBUILD_APPLICATION=ON -DBUILD_BENCHMARK=ON -DBUILD_TESTS=ON ^
  -DENABLE_EXCEPTIONS=ON -DENABLE_RTTI=ON ^
  -DCMAKE_INSTALL_PREFIX=%~dp0 %~dp0

if %errorlevel% == 0 (
  start %project%.sln
) else (
  pause
)

popd

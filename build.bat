@echo off

set flags_debug=   -std=c11 -Wall -Wextra -pedantic-errors -fextended-identifiers -g -D DEBUG
set flags_release=   -std=c11 -Wall -Wextra -pedantic-errors -fextended-identifiers

set clang_flags_debug= /TC /Z7 /DDEBUG /W4 /WX /MDd -Qunused-arguments
set clang_flags_debug_easy= /Z7 /DDEBUG /W3 /MDd /Zc:strictStrings- -Qunused-arguments -Wno-unused-variable
set clang_flags_release= /TC /O2 /W4 /MD /Zc:strictStrings- -Qunused-arguments -Wno-unused-variable

set tcc_flags_debug= -Wall -g
set tcc_flags_release= -Wall

pushd "%~dp0"
mkdir build
pushd build

rc ..\resources.rc

REM gcc %flags_debug%   ..\src\main.c -o gcc_dbg_main
REM gcc %flags_release% ..\src\main.c -o gcc_release_main

REM clang-cl %clang_flags_debug%  ..\src\main.c -o clang_dbg_main.exe
clang-cl %clang_flags_debug_easy% ..\mmain.cpp ..\platform.cpp ..\tr_math.c ..\gr_draw.cpp ..\resources.res ..\gui.cpp -o sokoban_dbg.exe /link opengl32.lib glu32.lib gdi32.lib user32.lib winmm.lib

REM clang-cl %clang_flags_release% ..\src\main.c -o clang_rel_main.exe

REM tcc %tcc_flags_debug% ..\src\main.c -o tcc_dbg_main.exe
REM tcc %tcc_flags_release% ..\src\main.c -o tcc_rel_main.exe


popd
popd



# CMakeList.txt : CMake project for NoteCollector, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project ("NoteCollector")

# Add source to this project's executable.
add_executable (NoteCollector 
  "source.cpp"
  "source.h"
  "lexer.h"
)

# TODO: Add tests and install targets if needed.

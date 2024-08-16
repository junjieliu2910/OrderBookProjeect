cmake_minimum_required(VERSION 3.22)
project(
  OrderBoook
  VERSION 2024.08
  LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
#set(CMAKE_COMPILER_FLAG -Wall -Wextra -pedantic -Werror)
set(CMAKE_COMPILER_FLAG -Wall -Wextra -pedantic)

add_subdirectory(src)
add_subdirectory(submodules/googletest)
enable_testing()
add_subdirectory(tests)
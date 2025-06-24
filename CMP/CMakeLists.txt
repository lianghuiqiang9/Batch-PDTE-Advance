cmake_minimum_required(VERSION 3.16)

project(cmp)
find_package(SEAL 4.1)

set(CMAKE_CXX_STANDARD 14)

file(GLOB SOURCE_FILES "${PROJECT_SOURCE_DIR}/src/*.cpp")

add_library(cmp SHARED ${SOURCE_FILES})

target_include_directories(cmp PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(cmp SEAL::seal)

set(CMAKE_INSTALL_PREFIX ${PROJECT_SOURCE_DIR})
install(TARGETS cmp LIBRARY DESTINATION lib)
# install(FILES ${PROJECT_SOURCE_DIR}/include/cmp.h ${PROJECT_SOURCE_DIR}/include/node.h ${PROJECT_SOURCE_DIR}/include/pdte.h ${PROJECT_SOURCE_DIR}/include/utils.h ${PROJECT_SOURCE_DIR}/include/json.hpp   DESTINATION ../include)  

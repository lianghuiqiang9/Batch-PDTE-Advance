cmake_minimum_required(VERSION 3.16)

project(cmp)

find_package(SEAL 4.1)

link_directories (${PROJECT_SOURCE_DIR}/CDTE/lib)

add_executable(tecmp_pdte_asm tecmp_pdte_asm.cpp)
target_include_directories(tecmp_pdte_asm PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/CDTE/include)
target_link_libraries (tecmp_pdte_asm SEAL::seal libcdte.so)

add_executable(tecmp_norm_pdte_asm tecmp_norm_pdte_asm.cpp)
target_include_directories(tecmp_norm_pdte_asm PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/CDTE/include)
target_link_libraries (tecmp_norm_pdte_asm SEAL::seal libcdte.so)

add_executable(rdcmp_pdte_asm rdcmp_pdte_asm.cpp)
target_include_directories(rdcmp_pdte_asm PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/CDTE/include)
target_link_libraries (rdcmp_pdte_asm SEAL::seal libcdte.so)

add_executable(cdcmp_pdte_asm cdcmp_pdte_asm.cpp)
target_include_directories(cdcmp_pdte_asm PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/CDTE/include)
target_link_libraries (cdcmp_pdte_asm SEAL::seal libcdte.so)

add_executable(tecmp_pdte_esm tecmp_pdte_esm.cpp)
target_include_directories(tecmp_pdte_esm PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/CDTE/include)
target_link_libraries (tecmp_pdte_esm SEAL::seal libcdte.so)

add_executable(tecmp_norm_pdte_esm tecmp_norm_pdte_esm.cpp)
target_include_directories(tecmp_norm_pdte_esm PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/CDTE/include)
target_link_libraries (tecmp_norm_pdte_esm SEAL::seal libcdte.so)

add_executable(rdcmp_pdte_esm rdcmp_pdte_esm.cpp)
target_include_directories(rdcmp_pdte_esm PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/CDTE/include)
target_link_libraries (rdcmp_pdte_esm SEAL::seal libcdte.so)

add_executable(cdcmp_pdte_esm cdcmp_pdte_esm.cpp)
target_include_directories(cdcmp_pdte_esm PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/CDTE/include)
target_link_libraries (cdcmp_pdte_esm SEAL::seal libcdte.so)

add_executable(new_tree_and_data new_tree_and_data.cpp)
target_include_directories(new_tree_and_data PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/CDTE/include)
target_link_libraries(new_tree_and_data SEAL::seal libcdte.so)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
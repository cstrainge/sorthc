
cmake_minimum_required(VERSION 3.5)

project(sorthc)



# Make sure we have access to llvm.
find_package(LLVM CONFIG REQUIRED)



# Make sure we're using C++ 20.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)



# Setup our build and distribution directories.
set(BUILD_DIR "${CMAKE_BINARY_DIR}")
set(DIST_DIR "${CMAKE_SOURCE_DIR}/dist")

set(SORTH_C_SRC_DIR "${CMAKE_SOURCE_DIR}/sorthc")
set(SORTH_RUNTIME_DIR "${CMAKE_SOURCE_DIR}/sorth-runtime")



# Configure our LLVM dependencies.
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})
set(LLVM_COMPONENTS
    Target
    TargetParser
    X86TargetMCA
    X86CodeGen
    X86AsmParser
    OrcJIT
    ExecutionEngine
    Support
    Passes
    Core
    X86
    Analysis
    MC
    MCParser)

llvm_map_components_to_libnames(LLVM_LIBS ${LLVM_COMPONENTS})



# Name for the sorth-runtime library.
set(SORTH_RUNTIME_NAME "sorth-runtime")



# Add the compiler target executable.
file(GLOB SORTH_C_SRCS "${SORTH_C_SRC_DIR}/*.cpp"
                       "${SORTH_C_SRC_DIR}/source/*.cpp"
                       "${SORTH_C_SRC_DIR}/compilation/*.cpp"
                       "${SORTH_C_SRC_DIR}/compilation/byte-code/*.cpp"
                       "${SORTH_C_SRC_DIR}/compilation/run-time/*.cpp"
                       "${SORTH_C_SRC_DIR}/compilation/run-time/built-in-words/*.cpp")


add_executable(${PROJECT_NAME} ${SORTH_C_SRCS})

target_include_directories(${PROJECT_NAME} PRIVATE "${SORTH_C_SRC_DIR}/"
                                                   "${SORTH_C_SRC_DIR}/source/"
                                                   "${SORTH_C_SRC_DIR}/compilation/"
                                                   "${SORTH_C_SRC_DIR}/compilation/byte-code/"
                                                   "${SORTH_C_SRC_DIR}/compilation/run-time/"
                                                   "${SORTH_C_SRC_DIR}/compilation/run-time/built-in-words/"

                                                   "${SORTH_RUNTIME_DIR}/")


set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${DIST_DIR})

target_precompile_headers(${PROJECT_NAME} PRIVATE "${SORTH_C_SRC_DIR}/sorthc.h")



# Now add the sorth-runtime static library.
file(GLOB SORTH_RUNTIME_SRCS "${SORTH_RUNTIME_DIR}/*.cpp"
                             "${SORTH_RUNTIME_DIR}/abi/*.cpp"
                             "${SORTH_RUNTIME_DIR}/abi/words/*.cpp"
                             "${SORTH_RUNTIME_DIR}/data-structures/*.cpp")

add_library(${SORTH_RUNTIME_NAME} STATIC "${SORTH_RUNTIME_SRCS}")



# The static library should be output to the dist directory, so that the compiler can find it.
set_target_properties(${SORTH_RUNTIME_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${DIST_DIR}"
                                                       ARCHIVE_OUTPUT_DIRECTORY "${DIST_DIR}"
                                                       LIBRARY_OUTPUT_DIRECTORY "${DIST_DIR}")



target_precompile_headers(${SORTH_RUNTIME_NAME} PRIVATE "${SORTH_RUNTIME_DIR}/sorth-runtime.h")



# Make sure that both targets can find their header files.
target_include_directories(${PROJECT_NAME} PRIVATE "${SORTH_RUNTIME_DIR}/")
target_include_directories(${SORTH_RUNTIME_NAME} PRIVATE "${SORTH_RUNTIME_DIR}/")



# Bring in our required LLVM libraries.
target_link_libraries(${PROJECT_NAME} PRIVATE ${LLVM_LIBS})



# Make sure the compiler target depends on the sorth-runtime target.
target_link_libraries(${PROJECT_NAME} PRIVATE ${SORTH_RUNTIME_NAME})


# Custom target to copy the standard library
add_custom_target(copy_stdlib ALL
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_SOURCE_DIR}/std.f ${DIST_DIR}/std.f
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/std ${DIST_DIR}/std
    COMMENT "Copying standard library files to the dist directory.")



# Copy the custom main that compiled programs will use.
add_custom_target(copy_abi_main ALL
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_SOURCE_DIR}/abi/main.cpp ${DIST_DIR}/main.cpp
    COMMENT "Copying the abi main.cpp to the dist directory.")



# Make sure that the compiler target and the run-time depend on the copy_stdlib target.
add_dependencies(${PROJECT_NAME} copy_stdlib)
add_dependencies(${SORTH_RUNTIME_NAME} copy_stdlib)



# Custom target to create the dist directory
add_custom_target(create_dist_dir ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory ${DIST_DIR}
    COMMENT "Creating dist directory.")



# Ensure the dist dir exists and copy_stdlib runs before the main target.
add_dependencies(copy_stdlib create_dist_dir)

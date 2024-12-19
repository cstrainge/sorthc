
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



set(SRC_DIR "src")
file(GLOB SRCS "${SRC_DIR}/*.cpp"
               "${SRC_DIR}/source/*.cpp")


add_executable(${PROJECT_NAME} ${SRCS})

target_include_directories(${PROJECT_NAME} PRIVATE "${SRC_DIR}")
target_include_directories(${PROJECT_NAME} PRIVATE "${SRC_DIR}/source")

set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${DIST_DIR})



# Bring in our required LLVM libraries.
target_link_libraries(${PROJECT_NAME} PRIVATE ${LLVM_LIBS})



# Enable link-time optimization if we're in release mode and it's supported.
if (CMAKE_BUILD_TYPE STREQUAL "Release")
    include(CheckIPOSupported)
    check_ipo_supported(RESULT ipo_supported OUTPUT ipo_output)
    if(ipo_supported)
        message(STATUS "IPO/LTO is supported: enabling for Release build.")
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
    else()
        message(status "IPO/LTO not supported: ${ipo_output}")
    endif()
endif()



# Custom target to copy the standard library
add_custom_target(copy_stdlib ALL
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_SOURCE_DIR}/std.f ${DIST_DIR}/std.f
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/std ${DIST_DIR}/std
    COMMENT "Copying standard library files to the dist directory.")



# Make sure that the compiler target depends on the copy_stdlib target.
add_dependencies(${PROJECT_NAME} copy_stdlib)



# Custom target to create the dist directory
add_custom_target(create_dist_dir ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory ${DIST_DIR}
    COMMENT "Creating dist directory.")



# Ensure the dist dir exists and copy_stdlib runs before the main target.
add_dependencies(copy_stdlib create_dist_dir)

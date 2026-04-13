
cmake_minimum_required(VERSION 3.13)
project(example_LLVM_IR CXX)

set(TARGET_NAME "example_LLVM_IR")

if (NOT WIN32 AND NOT CYGWIN)
    # Собираем все исходники в каталоге (как было в оригинале)
    file(GLOB_RECURSE SOURCES *.cpp *.h *.hpp)

    if (SOURCES STREQUAL "")
      message(FATAL_ERROR "No sources found for ${TARGET_NAME} in ${CMAKE_CURRENT_SOURCE_DIR}")
    endif()

    add_llvm_pass_plugin(${TARGET_NAME} ${SOURCES}
        DEPENDS
          intrinsics_gen
        BUILDTREE_ONLY
    )

    
    set(LLVM_TEST_DEPENDS ${TARGET_NAME} ${LLVM_TEST_DEPENDS} PARENT_SCOPE)
else()
    message(STATUS "Skipping ${TARGET_NAME} on Windows/Cygwin")
endif()

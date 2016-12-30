enable_testing()
find_package(GTest REQUIRED)

list(REMOVE_ITEM SOURCE_FILES ${PROJECT_SOURCE_DIR}/src/main.cpp)
list(REMOVE_ITEM CPP3DS_EMU_LIBS cpp3ds-emu)

set(TEST_SOURCE_FILES
    ${PROJECT_SOURCE_DIR}/test/main.cpp
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${CPP3DS}/include
    ${GTEST_INCLUDE_DIRS}
)

link_directories(
    ${CPP3DS}/lib
)

add_executable(${PROJECT_NAME}-tests ${TEST_SOURCE_FILES} ${SOURCE_FILES} ${EMU_SOURCE_FILES})
target_link_libraries(${PROJECT_NAME}-tests ${GTEST_BOTH_LIBRARIES} cpp3ds-test ${CPP3DS_EMU_LIBS})

set_target_properties(${PROJECT_NAME}-tests PROPERTIES COMPILE_FLAGS "${CMAKE_CXX_FLAGS} ${TEST_FLAGS} -std=c++11")
set_target_properties(${PROJECT_NAME}-tests PROPERTIES COMPILE_DEFINITIONS "EMULATION;TEST")
set_target_properties(${PROJECT_NAME}-tests PROPERTIES LINK_FLAGS "${CMAKE_CXX_FLAGS} ${TEST_FLAGS}")

SET(GTEST_SEARCH_PATH "${CMAKE_CURRENT_LIST_DIR}/../thirdparty/gtest/googletest")

FIND_PATH(GTEST_SOURCE_DIR
    NAMES CMakeLists.txt src/gtest_main.cc
    PATHS ${GTEST_SEARCH_PATH})

FIND_PATH(GTEST_INCLUDE_DIR
    NAMES gtest/gtest.h
	PATH_SUFFIXES include
    HINTS ${GTEST_SOURCE_DIR}
    PATHS ${GTEST_SEARCH_PATH})
	
INCLUDE(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GTestSrc DEFAULT_MSG
    GTEST_SOURCE_DIR
    GTEST_INCLUDE_DIR)
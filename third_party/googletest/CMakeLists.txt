if (NOT GTest_FOUND)
  message(STATUS "GTest package not found, will download googletest automatically. To prevent that install google test on your system (libgtest-dev)")
  include(FetchContent)
  FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        58d77fa8070e8cec2dc1ed015d66b454c8d78850 # 1.12.1
  )
  set(BUILD_GMOCK OFF)
  set(INSTALL_GTEST OFF)
  FetchContent_MakeAvailable(googletest)
  add_library(GTest::gtest ALIAS gtest)
  add_library(GTest::gtest_main ALIAS gtest_main)
endif()

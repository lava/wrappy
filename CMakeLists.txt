cmake_minimum_required (VERSION 2.8)
project (wrappy)

find_package(PythonLibs 2.7 REQUIRED)
# If i had one word to describe python versioning, it would be "broken as fuck"
if (NOT PYTHON_VERSION VERSION_LESS 3.0)
  error("Requires python 2.x")
endif()

find_package(Boost COMPONENTS unit_test_framework)

add_library(wrappy SHARED wrappy.cpp)

target_include_directories(wrappy PRIVATE ${PYTHON_INCLUDE_DIRS})
target_include_directories(wrappy PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include/)

if(${CMAKE_VERSION} VERSION_LESS 3.1)
  set(CMAKE_CXX_FLAGS "-std=c++0x")
else()
  target_compile_features(wrappy PUBLIC cxx_variadic_templates)
  target_compile_features(wrappy PUBLIC cxx_rvalue_references)
  target_compile_features(wrappy PUBLIC cxx_auto_type)
endif()

target_link_libraries(wrappy ${PYTHON_LIBRARIES})

if(Boost_UNIT_TEST_FRAMEWORK_FOUND)
  enable_testing()
  add_executable(test_stdlib tests/stdlib.cpp)
  add_executable(test_sugar tests/sugar.cpp)
  target_link_libraries(test_stdlib wrappy ${Boost_UNIT_TEST_FRAMEWORK_LIBRARIES})
  target_link_libraries(test_sugar wrappy ${Boost_UNIT_TEST_FRAMEWORK_LIBRARIES})
  add_test(NAME stdlib COMMAND test_stdlib)
  add_test(NAME sugar  COMMAND test_sugar)
else()
  message("Boost Unit testing libraries not found, not compiling tests")
endif()

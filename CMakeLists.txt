cmake_minimum_required (VERSION 3.1)
project (wrappy)

add_library(wrappy SHARED wrappy.cpp)

find_package(PythonLibs REQUIRED)

target_include_directories(wrappy PRIVATE ${PYTHON_INCLUDE_DIRS})
target_include_directories(wrappy PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include/)

target_compile_features(wrappy PUBLIC cxx_variadic_templates)
target_compile_features(wrappy PUBLIC cxx_rvalue_references)
target_compile_features(wrappy PUBLIC cxx_auto_type)

target_link_libraries(wrappy ${PYTHON_LIBRARIES})

find_package(Boost COMPONENTS unit_test_framework)

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

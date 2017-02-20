cmake_minimum_required (VERSION 2.8)
project (wrappy)

# Dependencies
find_package(PythonLibs 2.7 REQUIRED)
# If i had one word to describe python versioning,
# it would be "broken as fuck"
if (NOT PYTHON_VERSION VERSION_LESS 3.0)
  error("Requires python 2.x")
endif()

option( WRAPPY_BUILD_DEMOS "Build the wrappy tests" ON)

if (WRAPPY_BUILD_DEMOS)
  find_package(Boost COMPONENTS unit_test_framework)
endif()

# wrappy library target
add_library(wrappy SHARED wrappy.cpp)
set_target_properties(wrappy PROPERTIES VERSION 1.0.0)
set_target_properties(wrappy PROPERTIES SOVERSION 1)

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

# Examples
add_executable(example_email examples/email.cpp)
add_executable(example_turtle examples/turtle.cpp)
add_executable(example_plot examples/plot.cpp)
target_link_libraries(example_email wrappy)
target_link_libraries(example_turtle wrappy)
target_link_libraries(example_plot wrappy)

# Tests
if(WRAPPY_BUILD_DEMOS AND Boost_UNIT_TEST_FRAMEWORK_FOUND)
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

# Installation
install(TARGETS wrappy DESTINATION lib/)
install(DIRECTORY include/ DESTINATION include/)
install(DIRECTORY examples/ DESTINATION share/doc/libwrappy/examples/)
install(FILES README.md DESTINATION share/doc/libwrappy/)

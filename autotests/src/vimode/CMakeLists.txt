set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})

include_directories(
  ${CMAKE_SOURCE_DIR}/src/vimode
)

set (VIMODE_TEST_LINK_LIBS KF5TextEditor
  KF5::I18n
  Qt5::Script
  Qt5::Test
)

macro(vimode_unit_tests)
  foreach(_testname ${ARGN})
    add_executable(${_testname} ${_testname}.cpp)
    add_test(vimode/${_testname} ${_testname})
    target_link_libraries(${_testname} ${VIMODE_TEST_LINK_LIBS})
    ecm_mark_as_test(${_testname})
  endforeach()
endmacro()

vimode_unit_tests(
  main
)

add_custom_target(vimode_tests ${CMAKE_CTEST_COMMAND} --force-new-ctest-process -R "vimode/")

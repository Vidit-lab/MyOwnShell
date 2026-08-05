# Black-box smoke tests: drive the built shell over a pipe and diff its output.
add_test(
  NAME smoke
  COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/run_tests.sh $<TARGET_FILE:myshell>)

# Tab completion and line editing need a real terminal, so these drive the
# shell under a PTY. Skipped when python3 is unavailable.
find_package(Python3 COMPONENTS Interpreter)
if(Python3_FOUND)
  add_test(
    NAME completion
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test_completion.py
            $<TARGET_FILE:myshell>)
endif()

# Black-box smoke tests: drive the built shell over a pipe and diff its output.
add_test(
  NAME smoke
  COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/run_tests.sh $<TARGET_FILE:myshell>)

# TODO: parameterize for reuse
add_custom_target(coverage_report
  COMMAND lcov --directory src/CMakeFiles/cn-cbor.dir --capture --output-file cn-cbor.info
  COMMAND genhtml --output-directory lcov cn-cbor.info
  COMMAND echo "Coverage report in: file://${CMAKE_BINARY_DIR}/lcov/index.html"
)

#
#
# Compiling/running tests

if (use_context)
  add_definitions(-DUSE_CBOR_CONTEXT)
endif()

set ( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${dist_dir}/test )

function (create_test name)
  add_executable ( ${name}_test ${name}_test.c )
  target_link_libraries ( ${name}_test PRIVATE cn-cbor )
  target_include_directories ( ${name}_test PRIVATE ../include )
  add_test ( NAME ${name} COMMAND ${name}_test )
endfunction()

create_test ( cbor )

include ( CTest )

execute_process(
    COMMAND "${GIT_EXECUTABLE}"
        describe --always --dirty --abbrev=12
    WORKING_DIRECTORY "${SOURCE_DIR}"
    OUTPUT_VARIABLE GIT_REVISION
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE GIT_RESULT
)

if(NOT GIT_RESULT EQUAL 0)
    message(FATAL_ERROR "Cannot determine firmware Git revision")
endif()

file(MAKE_DIRECTORY "${OUTPUT_DIR}")

# Only update the timestamp when the revision changes, avoiding needless rebuilds.
file(CONFIGURE
    OUTPUT "${OUTPUT_DIR}/build_version.hpp"
    CONTENT "#pragma once\n#define FW_REVISION \"@GIT_REVISION@\"\n"
    @ONLY
)

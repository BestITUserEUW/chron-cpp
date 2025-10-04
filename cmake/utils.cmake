
function(_internal_extract_version_fail_message tag)
    message(FATAL_ERROR "Could not extract ${tag} version number from chron/version.hpp")
endfunction()

function(oryx_extract_version)
    file(READ "${CMAKE_CURRENT_LIST_DIR}/include/oryx/chron/version.hpp" file_contents)

    string(REGEX MATCH "kVersionMajor[ \t]*=[ \t]*([0-9]+)" _ "${file_contents}")
    if(NOT CMAKE_MATCH_COUNT EQUAL 1)
        _internal_extract_version_fail_message(major)
    endif()
    set(ver_major ${CMAKE_MATCH_1})

    string(REGEX MATCH "KVersionMinor[ \t]*=[ \t]*([0-9]+)" _ "${file_contents}")
    if(NOT CMAKE_MATCH_COUNT EQUAL 1)
        _internal_extract_version_fail_message(minor)
    endif()

    set(ver_minor ${CMAKE_MATCH_1})
    string(REGEX MATCH "kVersionPatch[ \t]*=[ \t]*([0-9]+)" _ "${file_contents}")
    if(NOT CMAKE_MATCH_COUNT EQUAL 1)
        _internal_extract_version_fail_message(patch)
    endif()
    set(ver_patch ${CMAKE_MATCH_1})

    set(ORYX_CHRON_VERSION_MAJOR ${ver_major} PARENT_SCOPE)
    set(ORYX_CHRON_VERSION_MINOR ${ver_minor} PARENT_SCOPE)
    set(ORYX_CHRON_VERSION_PATCH ${ver_patch} PARENT_SCOPE)
    set(ORYX_CHRON_VERSION "${ver_major}.${ver_minor}.${ver_patch}" PARENT_SCOPE)
endfunction()

function(oryx_enable_addr_sanitizer target_name)
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        message(FATAL_ERROR "Sanitizer supported only for gcc/clang!")
    endif()

    # Fix for asan having trouble with <regex>
    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(${target_name} PRIVATE -Wno-maybe-uninitialized)
    endif()

    message(STATUS "Address sanitizer enabled!")
    target_compile_options(${target_name} PRIVATE -fsanitize=address,undefined)
    target_compile_options(${target_name} PRIVATE -fno-sanitize=signed-integer-overflow)
    target_compile_options(${target_name} PRIVATE -fno-sanitize-recover=all)
    target_compile_options(${target_name} PRIVATE -fno-omit-frame-pointer)
    target_link_libraries(${target_name} PRIVATE -fsanitize=address,undefined)
endfunction()

# Enable thread sanitizer (gcc/clang only)
function(oryx_enable_thread_sanitizer target_name)
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        message(FATAL_ERROR "Sanitizer supported only for gcc/clang!")
    endif()

    message(STATUS "Thread sanitizer enabled!")
    target_compile_options(${target_name} PRIVATE -fsanitize=thread)
    target_compile_options(${target_name} PRIVATE -fno-omit-frame-pointer)
    target_link_libraries(${target_name} PRIVATE -fsanitize=thread)
endfunction()
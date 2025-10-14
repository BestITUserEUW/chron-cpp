cmake_minimum_required(VERSION 3.24)

include(cmake/utils.cmake)

oryx_extract_version()

project(chron-cpp VERSION ${ORYX_CHRON_VERSION} LANGUAGES CXX)

message(STATUS "Build ${PROJECT_NAME}: ${PROJECT_VERSION}")

option(ORYX_CHRON_BUILD_SHARED_LIBS "Build shared library" ${BUILD_SHARED_LIBS})
option(ORYX_CHRON_BUILD_TESTS "Build tests" OFF)
option(ORYX_CHRON_INSTALL "Install the project" OFF)
option(ORYX_CHRON_SANITIZE_ADDRESS "Enable address sanitizer in tests" OFF)
option(ORYX_CHRON_SANITIZE_THREAD "Enable thread sanitizer in tests" OFF)

if(ORYX_SANITIZE_ADDRESS AND ORYX_SANITIZE_THREAD)
    message(FATAL_ERROR "ORYX_SANITIZE_ADDRESS and ORYX_SANITIZE_THREAD are mutually exclusive")
endif()

if (PROJECT_IS_TOP_LEVEL)
    set(ORYX_CHRON_INSTALL ON)
endif()

set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 23)
endif()

if (ORYX_CHRON_BUILD_SHARED_LIBS)
    add_library(${PROJECT_NAME} SHARED)
    set_target_properties(${PROJECT_NAME} PROPERTIES SOVERSION ${PROJECT_VERSION})
else()
    add_library(${PROJECT_NAME} STATIC)
endif()

add_library("oryx::${PROJECT_NAME}" ALIAS ${PROJECT_NAME})

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(${PROJECT_NAME} 
        PRIVATE
            /W4
    )
else()
    target_compile_options(${PROJECT_NAME} 
        PRIVATE
            -Wall 
            -Wextra
            -Werror
            -Wuninitialized 
            -Wno-unused-function 
            -Wunused-variable
    )
endif()

target_include_directories(${PROJECT_NAME} 
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

file(GLOB_RECURSE ORYX_CHRON_HEADERS
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*"
)

target_sources(${PROJECT_NAME} 
    PRIVATE 
        src/clock.cpp
        src/preprocessor.cpp
        src/randomization.cpp
        src/schedule.cpp
        src/task.cpp
        src/parser.cpp
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/include"
        FILES ${ORYX_CHRON_HEADERS}
)

# Experimental
target_precompile_headers(${PROJECT_NAME} PUBLIC
    <string>
    <chrono>
)

if(ORYX_CHRON_SANITIZE_ADDRESS)
    oryx_enable_addr_sanitizer(${PROJECT_NAME})
elseif(ORYX_CHRON_SANITIZE_THREAD)
    oryx_enable_thread_sanitizer(${PROJECT_NAME})
endif()


if(ORYX_CHRON_BUILD_TESTS)
    file(GLOB_RECURSE TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp")
    set(test_exe ${PROJECT_NAME}_tests)
    add_executable(${test_exe} ${TEST_SOURCES})
    target_link_libraries(${test_exe} PRIVATE ${PROJECT_NAME})
    
    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        target_compile_definitions(${test_exe} PUBLIC DOCTEST_CONFIG_USE_STD_HEADERS)
    endif()
endif()

if (ORYX_CHRON_INSTALL)
    include(GNUInstallDirs)
    include(CMakePackageConfigHelpers)

    configure_package_config_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/${PROJECT_NAME}-config.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config.cmake"
        INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}"
    )

    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-version.cmake"
        COMPATIBILITY ExactVersion
    )

    install(
        TARGETS ${PROJECT_NAME}
        EXPORT ${PROJECT_NAME}-targets
        FILE_SET HEADERS
    )

    install(
        FILES 
            "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config.cmake"
            "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-version.cmake"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}"
        COMPONENT ${PROJECT_NAME}
    )

    install(
        EXPORT ${PROJECT_NAME}-targets
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}"
        NAMESPACE oryx::
        FILE ${PROJECT_NAME}-targets.cmake
        COMPONENT ${PROJECT_NAME}
    )
endif ()
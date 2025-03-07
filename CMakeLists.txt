cmake_minimum_required(VERSION 3.16)

# Project name and language
project(map
    VERSION 1.0.0
    DESCRIPTION "A C23 library with strict warning flags"
    LANGUAGES C
)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)

# Set warning flags based on compiler
if(CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID MATCHES "Clang")
    set(WARNING_FLAGS
        -Wall                     # Enable all common warnings
        -Wextra                   # Enable extra warnings
        -Wpedantic               # Ensure strict ISO C compliance
        -Werror                  # Treat warnings as errors
        -Wcast-align            # Warn about pointer casts which increase alignment
        -Wcast-qual             # Warn about casts which remove type qualifiers
        -Wconversion            # Warn about implicit conversions
        -Wdouble-promotion      # Warn about float being implicitly promoted to double
        -Wformat=2              # Warn about printf/scanf format string issues
        -Wformat-security       # Warn about printf/scanf security issues
        -Wnull-dereference      # Warn about NULL pointer dereference
        -Winit-self             # Warn about uninitialized variables
        -Wmissing-include-dirs  # Warn about missing include directories
        -Wpointer-arith         # Warn about suspicious pointer arithmetic
        -Wshadow               # Warn about variable shadowing
        -Wswitch-default       # Warn about switch statements without default case
        -Wswitch-enum          # Warn about switch statements not handling all enum values
        -Wundef                # Warn about undefined preprocessor symbols
        -Wuninitialized        # Warn about uninitialized variables
        -Wunused              # Warn about unused variables/functions
    )
elseif(CMAKE_C_COMPILER_ID STREQUAL "MSVC")
    set(WARNING_FLAGS
        /W4          # Set warning level 4 (highest)
        /WX          # Treat warnings as errors
        /permissive- # Enforce strict C++ standards compliance
        /w14242     # Warn about conversion possible loss of data
        /w14254     # Warn about larger bit field truncation
        /w14263     # Warn about member function does not override any base class virtual member function
        /w14265     # Warn about class has virtual functions, but destructor is not virtual
        /w14287     # Warn about unsigned/negative constant mismatch
        /we4289     # Warn about using loop variable in post-loop code
        /w14296     # Warn about expression is always false/true
        /w14311     # Warn about pointer truncation
        /w14545     # Warn about expression before comma evaluates to a function
        /w14546     # Warn about function call before comma missing argument list
        /w14547     # Warn about operator before comma has no effect
        /w14549     # Warn about operator before comma has no effect
        /w14555     # Warn about expression has no effect
        /w14619     # Warn about pragma warning invalid warning number
        /w14640     # Warn about thread un-safe static member initialization
        /w14826     # Warn about conversion from 'type1' to 'type2' is sign-extended
        /w14905     # Warn about wide string literal cast to 'LPSTR'
        /w14906     # Warn about string literal cast to 'LPWSTR'
        /analyze    # Enable code analysis
    )
endif()

# Add library target
add_library(${PROJECT_NAME} 
  src/map.c
)

# Apply warning flags
target_compile_options(${PROJECT_NAME} PRIVATE ${WARNING_FLAGS})

# Specify include directories
target_include_directories(${PROJECT_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include/map
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Add option for testing (OFF by default)
option(BUILD_TESTING "Build the testing tree." OFF)

# Only include Unity if testing is enabled
if(BUILD_TESTING)
    include(FetchContent)
    FetchContent_Declare(
        unity
        GIT_REPOSITORY https://github.com/ThrowTheSwitch/Unity.git
        GIT_TAG v2.5.2
    )
    FetchContent_MakeAvailable(unity)

    # Add test executable
    add_executable(test_map
        tests/test_map.c
        src/map.c
    )

    target_link_libraries(test_map
        PRIVATE
        unity
    )

    target_include_directories(test_map
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include/map
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )
endif()

# Installation rules
include(GNUInstallDirs)
install(TARGETS ${PROJECT_NAME}
    EXPORT ${PROJECT_NAME}Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Export targets
install(EXPORT ${PROJECT_NAME}Targets
    FILE ${PROJECT_NAME}Targets.cmake
    NAMESPACE ${PROJECT_NAME}::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)

# Create and install package configuration files
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${PROJECT_NAME}ConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/${PROJECT_NAME}Config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)

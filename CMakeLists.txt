
# Set the minimum version
cmake_minimum_required(VERSION 3.10)

# Define our project name
# Also worth remembering, we can reference the project name later
# with the ${PROJECT_NAME} variable 
project (easykeydb VERSION 1.0.0)

# Define the C++ Standard
set(CMAKE_CXX_STANDARD 14)

# Set that it is required
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Create a sources variable with will link to all our cpp files
# Also worth remembering that it will create a variable called ${SOURCES}
set(
    SOURCES
    source/know_nothing.cpp
    source/easykey.cpp
    source/server.cpp
    main.cpp

)

# Add an executable that will link all our cpp files to one
add_executable(
    ${PROJECT_NAME} 
    ${SOURCES}
)

# Set the directories that should be included in the build command for this target
target_include_directories(
    ${PROJECT_NAME}
    PRIVATE ${PROJECT_SOURCE_DIR}/include
)
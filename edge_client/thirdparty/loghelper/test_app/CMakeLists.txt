project(test_app)
cmake_minimum_required( VERSION 2.8 )

# set default build type
if(NOT DEFINED CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g -ggdb")
    set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O2 -Wall")
endif()

include( CheckCXXCompilerFlag )
check_cxx_compiler_flag( "-std=c++14"   COMPILER_SUPPORTS_CXX14 )
if( COMPILER_SUPPORTS_CXX14 )
    if( CMAKE_COMPILER_IS_GNUCXX )
        set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++14" )
    else()
        set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14" )
    endif()
else()
    if( CMAKE_COMPILER_IS_GNUCXX )
        set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11" )
    else()
        set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11" )
    endif()
endif()

#########################

aux_source_directory(. DIR_SRCS)
link_directories(${PROJECT_SOURCE_DIR}/lib)
link_directories(${PROJECT_SOURCE_DIR}/../lib)
include_directories(${PROJECT_SOURCE_DIR}/../include)
add_executable(${PROJECT_NAME} ${DIR_SRCS})


if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    MESSAGE(STATUS "current platform: Linux")  
    add_definitions(-DROCK_LINUX_PLATFORM -DBOOST_LOG_DYN_LINK)
    target_link_libraries(${PROJECT_NAME}
        loghelper
        pthread
        boost_filesystem
    )

elseif(CMAKE_SYSTEM_NAME MATCHES "Windows")
    target_link_libraries(${PROJECT_NAME}
        loghelper 
    )
endif()
########################

# set the output dir
INSTALL(TARGETS ${PROJECT_NAME} 
    RUNTIME DESTINATION ${PROJECT_SOURCE_DIR}/../bin
    LIBRARY DESTINATION ${PROJECT_SOURCE_DIR}/../lib
    ARCHIVE DESTINATION ${PROJECT_SOURCE_DIR}/../lib
)

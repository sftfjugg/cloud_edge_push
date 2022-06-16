project( edge_app )
cmake_minimum_required( VERSION 3.0 )

# set default build type
if(NOT DEFINED CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g -ggdb")
    set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O2 -Wall")
    
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

    ADD_DEFINITIONS(-fpermissive -fno-omit-frame-pointer -Wunused-variable -Wunused-parameter -Wunused-function -w) 
endif()

if(CMAKE_SYSTEM_NAME MATCHES "Windows")
    #ģ�����SAFESEHӳ���ǲ���ȫ
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SAFESEH:NO")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /SAFESEH:NO")
    set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} /SAFESEH:NO")

    #����Ԥ����������
    ADD_DEFINITIONS(-D_CRT_SECURE_NO_WARNINGS)
    ADD_DEFINITIONS(-D_WINSOCK_DEPRECATED_NO_WARNINGS)
endif()

add_definitions(-DBOOST_ERROR_CODE_HEADER_ONLY)
#########################

file(GLOB_RECURSE HEADER_FILES 
    ${PROJECT_SOURCE_DIR}/include/*.h 
    ${PROJECT_SOURCE_DIR}/include/*.hpp  
    ${PROJECT_SOURCE_DIR}/*.h 
    ${PROJECT_SOURCE_DIR}/*.hpp
)
source_group("Include" FILES ${HEADER_FILES})

aux_source_directory(src DIR_SRCS)
# aux_source_directory(tcpdamon DIR_SRCS)

include_directories(${PROJECT_SOURCE_DIR}/../../thirdparty/loghelper/include)
include_directories(${PROJECT_SOURCE_DIR}/../tcpclient)
include_directories(${PROJECT_SOURCE_DIR}/../recv_sys_info) 
include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/../../include)
include_directories(${PROJECT_SOURCE_DIR}/../include)


link_directories(${PROJECT_SOURCE_DIR}/../../thirdparty/loghelper/lib)
link_directories(${PROJECT_SOURCE_DIR}/../../lib)

add_executable( ${PROJECT_NAME} ${DIR_SRCS} ${HEADER_FILES})

if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    add_definitions(-DROCK_LINUX_PLATFORM -DBOOST_LOG_DYN_LINK)    

    target_link_libraries(${PROJECT_NAME}
    pthread
    rt
    dl
    cpr
    curl
    loghelper
    tcpclient
    recv_sys_info
    boost_system
    boost_filesystem
    )
endif()

########################

INSTALL(TARGETS ${PROJECT_NAME} 
    RUNTIME DESTINATION ${PROJECT_SOURCE_DIR}/../../bin
    LIBRARY DESTINATION ${PROJECT_SOURCE_DIR}/../../lib
    ARCHIVE DESTINATION ${PROJECT_SOURCE_DIR}/../../lib
)

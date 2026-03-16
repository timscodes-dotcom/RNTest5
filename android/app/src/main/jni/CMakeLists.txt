cmake_minimum_required(VERSION 3.13)

# Define the library name here.
project(appmodules)

# Enable C language support
enable_language(C)

# This file includes all the necessary to let you build your React Native application
include(${REACT_ANDROID_DIR}/cmake-utils/ReactNative-application.cmake)

# Define where the additional source code lives. We need to crawl back the jni, main, src, app, android folders
target_sources(${CMAKE_PROJECT_NAME} PRIVATE ../../../../../shared/NativeSampleModule.cpp)

# Define where CMake can find the additional header files. We need to crawl back the jni, main, src, app, android folders
target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC ../../../../../shared)

# Add compiler flags to ignore various warnings
target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE 
    -Wno-unused-variable
    -Wno-logical-op-parentheses
    -Wno-bitwise-op-parentheses
    -Wno-implicit-const-int-float-conversion
    -Wno-delete-non-abstract-non-virtual-dtor
    -Wno-unused-but-set-variable
    -Wno-format
    -Wno-parentheses-equality
)

include_directories("./json/")
include_directories("./md5/")
include_directories("./AES/")
include_directories("./sqlite/")
include_directories("./mongoose/")

# Create a separate C library for SQLite
add_library(sqlite3 STATIC sqlite/sqlite3.c)
set_target_properties(sqlite3 PROPERTIES LINKER_LANGUAGE C)
target_compile_options(sqlite3 PRIVATE -Wno-unused-variable -Wno-unused-parameter -Wno-unused-function)

add_library(mongoose STATIC mongoose/mongoose.c)
set_target_properties(mongoose PROPERTIES LINKER_LANGUAGE C)
target_compile_options(mongoose PRIVATE -Wno-unused-variable -Wno-unused-parameter -Wno-unused-function)

# Add all source files to the main appmodules library
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    json/json_reader.cpp
    json/json_value.cpp
    json/json_writer.cpp
    md5/md5.cpp
    AES/rijndael-alg-fst.cpp
    AES/rijndael-api-fst.cpp
    CMongooseWebServer.cpp
    CMongooseWebClient.cpp
    CRITICAL_SECTION.cpp
    TcpServer.cpp
    TcpClient.cpp
    TcpPacketServer.cpp
    TcpPacketHandleServer.cpp
    TcpPacketHandleClient.cpp
    sqlite/SQLiteDB.cpp
    functions.cpp
    native-lib.cpp
)

# Link the SQLite library
target_link_libraries(${CMAKE_PROJECT_NAME} sqlite3)
target_link_libraries(${CMAKE_PROJECT_NAME} mongoose)
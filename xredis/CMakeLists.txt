# CMake 最低版本号要求
cmake_minimum_required(VERSION 3.8)
#set(CMAKE_BUILD_TYPE Release)
set(CMAKE_BUILD_TYPE debug)
set(CMAKE_CXX_STANDARD 11)
# 项目信息
project (uWebsockets)

#option(BUILD_SHARED_LIBS "Build shared libraries." ON)



#设定头文件目录
INCLUDE_DIRECTORIES(
src
${OPENSSL_INCLUDE_DIRECTORIES}
)
#并且把该目录设为连接目录
LINK_DIRECTORIES(
    ${OPENSSL_LINK_DIRECTORIES}
)


# 查找目录下的所有源文件
# 并将名称保存到 DIR_SRCS 变量
set(DIR_SOURCE .)
AUX_SOURCE_DIRECTORY(src/. DIR_SOURCE)



link_libraries(
    ${OPENSSL_SSL_LIBRARY}
    ${OPENSSL_CRYPTO_LIBRARY}
)

# 指定生成目标 
#ADD_EXECUTABLE(uWS  ${DIR_SOURCE} ${DIR_WEBRTC_SOURCE})


# shared library
ADD_LIBRARY(uWS SHARED ${DIR_SOURCE})

# static library
ADD_LIBRARY(uWS_static STATIC ${DIR_SOURCE})
SET_TARGET_PROPERTIES(uWS_static PROPERTIES OUTPUT_NAME "uWS")
SET_TARGET_PROPERTIES(uWS PROPERTIES CLEAN_DIRECT_OUTPUT 1)
SET_TARGET_PROPERTIES(uWS_static PROPERTIES CLEAN_DIRECT_OUTPUT 1)



#target_include_directories(uWS_static PUBLIC
#src
#${LIBUV_INCLUDE_DIR}
#${ZLIB_INCLUDE_DIRS}
#)


#target_link_libraries(uWS 
#PUBLIC
# "-Wl,--whole-archive" #之后的库使用--whole-archive选项
 #${OPENSSL_SSL_LIBRARY}
 #${OPENSSL_CRYPTO_LIBRARY}
#"-Wl,--no-whole-archive"#之后的库不使用--whole-archive选项
#)  




# 添加链接库
if (UNIX)
target_link_libraries (uWS PUBLIC 
  pthread
   dl)
endif (UNIX)
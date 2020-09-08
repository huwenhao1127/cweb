cmake_minimum_required(VERSION 3.0)
project(cweb)

#set(CMAKE_CXX_STANDARD 11)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -o -fPIC -std=c++11 -Wall -Werror -lpthread")
#-g
#-ggdb
#-Wno-deprecated
#-Wno-unused-function 
#-Wno-builtin-macro-redefined 
#-Wno-deprecated-declarations
#-rdynamic    

set(TEST_SRC
	./tests/test_web.cc
	)
set(TEST_SERVER_SRC
	./tests/test_chat_serv.cc
	)
set(TEST_CLIENT_SRC
	./tests/test_chat_client.cc
	)
set(LIB_SRC
	./src/http_conn.cc
	./src/mutex.cc
	)



set(INC_DIR src)
set(LINK_DIR ./lib)
set(LIB_LIST ./lib/cweb)

## 设置头文件目录
include_directories(${INC_DIR})
# 设置库目录
#link_directories(${LINK_DIR})
# 设置要链接的库
#link_libraries(${LIB_LIST}

# 生成库文件
add_library(cweb SHARED ${LIB_SRC})
#add_library(cweb_static STATIC ${LIB_SRC})
#SET_TARGET_PROPERTIES (sylar_static PROPERTIES OUTPUT_NAME "sylar")

# 生成可执行测试文件
add_executable(test ${TEST_SRC})
add_executable(test_server ${TEST_SERVER_SRC})
add_executable(test_client ${TEST_CLIENT_SRC})

add_dependencies(test cweb)
# 完成库的链接
target_link_libraries(test cweb)

# 输出文件的路径
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)


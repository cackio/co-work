cmake_minimum_required(VERSION 3.10)

# 项目名称和版本
project(My_Project)



# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

#启动调试
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

# 查找所有源文件
aux_source_directory(source SOURCE)
aux_source_directory(main MAIN)

include_directories(./include)
include_directories(${MySQL_INCLUDE_DIRS})
# 查找并链接库（例如，MySQL)
# 生成可执行文件
add_executable(My_Project ${SOURCE} ${MAIN} ${MYSQL_LIBRARIES})

find_package(nlohmann_json 3.2.0 REQUIRED)

include_directories(/usr/include/mysql)
link_directories(/usr/lib/mysql)
target_link_libraries(My_Project mysqlclient)


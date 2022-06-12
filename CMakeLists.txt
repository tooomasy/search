cmake_minimum_required(VERSION 3.10)
project(search VERSION 1.0)

set(CMAKE_CXX_STANDARD 20)

set(SOURCES 
  src/main.cpp
  src/command_tokenizer.cpp
  src/directory_searcher.cpp
  src/searcher.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/include)
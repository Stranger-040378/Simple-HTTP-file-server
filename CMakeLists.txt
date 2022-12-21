cmake_minimum_required(VERSION 2.8)

project(WebServer)

set(SOURCE_EXE WebServer.cpp)

file(GLOB SOURCES "${PROJECT_SOURCE_DIR}/Modules/*.cpp")
file(GLOB HEADERS "${PROJECT_SOURCE_DIR}/Modules/*.hpp")

add_library(Modules STATIC ${HEADERS} ${SOURCES})
add_executable(WebServer ${SOURCE_EXE})

target_link_libraries(WebServer Modules)

find_package(Threads REQUIRED)
if(Threads_FOUND AND CMAKE_USE_PTHREADS_INIT)
  add_definitions(-DUSE_PTHREAD_THREADS=1)
  if(PTHREAD_LIB_NAME)
	target_link_libraries(WebServer ${PTHREAD_LIB_NAME})
  else()
	target_link_libraries(WebServer ${CMAKE_THREAD_LIBS_INIT})
  endif()
else()
  message(FATAL_ERROR "no threading system pthread available")
endif()

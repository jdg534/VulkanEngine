cmake_minimum_required(VERSION 3.10.0)





set(GLM_HeadersDir ${CMAKE_CURRENT_SOURCE_DIR}/GLM CACHE INTERNAL "" FORCE)

set(GLM_STATIC_LIBRARY_ENABLE ON)
add_subdirectory(GLM)

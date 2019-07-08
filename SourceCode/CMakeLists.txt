cmake_minimum_required(VERSION 3.10.0)


file(GLOB_RECURSE VulkanEngineExeSource *.h *.cpp)

add_executable(VulkanEngineExe ${VulkanEngineExeSource})

target_include_directories(VulkanEngineExe PUBLIC ${GLM_HeadersDir})
target_include_directories(VulkanEngineExe PUBLIC ${Vulkan_INCLUDE_DIR})

# target_link_libraries(VulkanEngineExe glm_static)
target_link_libraries(VulkanEngineExe ${Vulkan_LIBRARY})

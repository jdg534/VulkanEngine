cmake_minimum_required(VERSION 3.10.0)

project(VulkanEngine)

# need Vulkan, GLM, GLFW
find_package(Vulkan REQUIRED) # refactor to require V1.1.108
add_subdirectory(Submodules)

add_subdirectory(SourceCode)


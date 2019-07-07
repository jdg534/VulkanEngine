cmake_minimum_required(VERSION 3.10.0)

project(VulkanEngine)

# need Vulkan, GLM, GLFW
find_package(Vulkan REQUIRED)

add_subdirectory(SourceCode)


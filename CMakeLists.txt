cmake_minimum_required(VERSION 3.20)
    project(VulkanTest LANGUAGES CXX)

    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

    find_package(Vulkan REQUIRED)
    find_package(glfw3 REQUIRED)
    find_package(glm REQUIRED)

    add_executable(VulkanTest main.cpp)

    target_link_libraries(VulkanTest PRIVATE
        Vulkan::Vulkan
        glfw
        glm::glm
)

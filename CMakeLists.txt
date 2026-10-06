cmake_minimum_required(VERSION 3.20)
    project(VulkanTest LANGUAGES CXX)

    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

    find_package(Vulkan REQUIRED)
    find_package(glfw3 REQUIRED)
    find_package(glm REQUIRED)

    add_executable(VulkanTest
        main.cpp
        src/application.cpp
        src/vertex.cpp
        src/utils.cpp
    )
    
    target_include_directories(VulkanTest PRIVATE src)

    target_link_libraries(VulkanTest PRIVATE
        Vulkan::Vulkan
        glfw
        glm::glm
)

# =============================================================
# 1. Tìm trình biên dịch slangc (ưu tiên PATH và Mason)
# =============================================================
find_program(SLANGC_EXECUTABLE slangc
    PATHS
        "$ENV{HOME}/.local/bin"
        "$ENV{HOME}/.local/share/nvim/mason/bin"
        ENV VULKAN_SDK
    PATH_SUFFIXES bin
)

if(NOT SLANGC_EXECUTABLE)
    message(FATAL_ERROR "Không tìm thấy slangc! Hãy chắc chắn đã cài slang hoặc tạo symlink vào ~/.local/bin.")
endif()

# =============================================================
# 2. Hàm biên dịch Slang Shader
# =============================================================
function(add_slang_shader_target TARGET)
    cmake_parse_arguments("SHADER" "" "" "SOURCES" ${ARGN})

    # Thư mục chứa shader đầu ra (ngay trong thư mục shaders/ của source code)
    set(SHADERS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/shaders")
    set(ENTRY_POINTS -entry vertMain -entry fragMain)
    set(OUTPUT_SPV "${SHADERS_DIR}/slang.spv")

    # Đảm bảo thư mục tồn tại
    file(MAKE_DIRECTORY ${SHADERS_DIR})

    # Lệnh compile chỉ chạy lại khi các file SOURCES thay đổi
    add_custom_command(
        OUTPUT ${OUTPUT_SPV}
        COMMAND ${SLANGC_EXECUTABLE} ${SHADER_SOURCES}
                -target spirv
                -profile spirv_1_4
                -emit-spirv-directly
                -fvk-use-entrypoint-name
                ${ENTRY_POINTS}
                -o ${OUTPUT_SPV}
        DEPENDS ${SHADER_SOURCES}
        COMMENT "[Slang] Compiling ${SHADER_SOURCES} -> ${OUTPUT_SPV}"
        VERBATIM
    )

    add_custom_target(${TARGET} DEPENDS ${OUTPUT_SPV})
endfunction()

# =============================================================
# 3. Áp dụng vào target VulkanTest của bạn
# =============================================================
# (foo -> SlangShaders, bar -> VulkanTest)
add_slang_shader_target(SlangShaders SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/shaders/shader.slang")
add_dependencies(VulkanTest SlangShaders)

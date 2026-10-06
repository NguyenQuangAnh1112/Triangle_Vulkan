#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include "vulkan/vulkan.hpp"
#include <glm/glm.hpp>
#include <vector>
#include <array>

struct Vertex {
    glm::vec2 pos;
    glm::vec3 color;

    static vk::VertexInputBindingDescription getBindingDescription();

    // return an array include two VertexInputAttributeDescription
    static std::array<vk::VertexInputAttributeDescription, 2> getAttributeDescription();
};


inline const std::vector<Vertex> vertices = {
    {{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}}, // Đỉnh trên: Màu đỏ
    {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},  // Đỉnh phải dưới: Màu lục
    {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}  // Đỉnh trái dưới: Màu lam;
};

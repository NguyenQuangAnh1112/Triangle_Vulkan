#include "vertex.h"


vk::VertexInputBindingDescription Vertex::getBindingDescription() {
    return vk::VertexInputBindingDescription{
        .binding = 0,
        .stride = sizeof(Vertex),
        .inputRate = vk::VertexInputRate::eVertex};
}

// return an array include two VertexInputAttributeDescription
std::array<vk::VertexInputAttributeDescription, 2> Vertex::getAttributeDescription() {
    return std::array<vk::VertexInputAttributeDescription, 2>{
        // attribute 1: position
        vk::VertexInputAttributeDescription{
            .location = 0,
            .binding = 0,
            .format = vk::Format::eR32G32Sfloat,
            .offset = static_cast<uint32_t>(offsetof(Vertex, pos))},

        // attibute 2: color
        vk::VertexInputAttributeDescription{
            .location = 1,
            .binding = 0,
            .format = vk::Format::eR32G32Sfloat,
            .offset = static_cast<uint32_t>(offsetof(Vertex, color))},
    };
}

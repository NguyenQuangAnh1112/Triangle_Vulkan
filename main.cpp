#include "application.h"
#include <iostream>
#include <cstdlib>

int main() {
    try {
        HelloTriangleApplication app;
        app.run();
    } catch (const vk::SystemError& e) {
        std::cerr << "Vulkan error: " << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

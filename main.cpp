#define GLFW_INCLUDE_VULKAN
#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS

#include <vulkan/vulkan_raii.hpp>
#include <GLFW/glfw3.h>
#include <algorithm>
#include <iostream>
#include <cstring>
#include <stdexcept>


static constexpr uint32_t WIDTH = 800;
static constexpr uint32_t HEIGHT = 600;

const std::vector<char const*> validationLayers = {
    "VK_LAYER_KHRONOS_validation",
};


std::vector<const char*> requiredDeviceExtensions = {vk::KHRSwapchainExtensionName};

#ifdef NDEBUG
constexpr bool enableValidationlayers = false;
#else
constexpr bool enableValidationlayers = true;
#endif


class HelloTriangleApplication {
  public:
    void run() {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }


  private:
    vk::raii::Context context;
    vk::raii::Instance instance{nullptr};
    vk::raii::DebugUtilsMessengerEXT debugMessenger{nullptr};
    vk::raii::PhysicalDevice physicalDevice{nullptr};
    vk::raii::Device logicalDevice{nullptr};
    vk::raii::Queue graphicsQueue{nullptr};

    GLFWwindow* mWindow = nullptr;

    static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(
        vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
        vk::DebugUtilsMessageTypeFlagsEXT type,
        const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData) {
        if (severity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning ||
            severity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eError) {
            std::cerr << "validation layer: type " << vk::to_string(type)
                      << " msg: " << pCallbackData->pMessage << std::endl;
        }

        return vk::False;
    }


    void initWindow() {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        mWindow = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
    }


    void initVulkan() {
        createInstance();
        setupDebugMessenger();
        pickPhysicalDevice();
        createLogicalDevice();
    }


    void setupDebugMessenger() {
        if (!enableValidationlayers)
            return;

        vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);

        vk::DebugUtilsMessageTypeFlagsEXT messageTypeFlags(
            vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
            vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
            vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);

        vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfoEXT{
            .messageSeverity = severityFlags,
            .messageType = messageTypeFlags,
            .pfnUserCallback = &debugCallback};

        debugMessenger = instance.createDebugUtilsMessengerEXT(debugUtilsMessengerCreateInfoEXT);
    }


    void mainLoop() {
        while (!glfwWindowShouldClose(mWindow)) {
            glfwPollEvents();
        }
    }


    void cleanup() {
        glfwDestroyWindow(mWindow);
        glfwTerminate();
    }


    void createInstance() {
        constexpr vk::ApplicationInfo appInfo{
            .pApplicationName = "Hello Triangle",
            .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
            .pEngineName = "No Engine",
            .engineVersion = VK_MAKE_VERSION(1, 0, 0),
            .apiVersion = vk::ApiVersion14};


        std::vector<char const*> requiredLayers;
        if (enableValidationlayers) {
            requiredLayers.assign(validationLayers.begin(), validationLayers.end());
        }

        std::vector<vk::LayerProperties> layerProperties = context.enumerateInstanceLayerProperties();

        const char* unsupportedLayer = nullptr;

        for (const char* requiredLayer : requiredLayers) {
            bool found = false;

            for (const auto& layerProperty : layerProperties) {
                if (std::strcmp(layerProperty.layerName, requiredLayer) == 0) {
                    found = true;
                    break;
                }
            }

            if (!found) {
                unsupportedLayer = requiredLayer;
                break;
            }
        }

        if (unsupportedLayer != nullptr) {
            throw std::runtime_error("Required layer not supported: " + std::string(unsupportedLayer));
        }


        std::vector<const char*> requiredExtensions = getRequiredInstanceExtensions();

        std::vector<vk::ExtensionProperties> extensionProperties = context.enumerateInstanceExtensionProperties();
        std::vector<const char*>::iterator unsupportedPropertyIt = std::ranges::find_if(requiredExtensions,
                                                                                        [&extensionProperties](auto const& requiredExtension) {
                                                                                            return std::ranges::none_of(extensionProperties,
                                                                                                                        [requiredExtension](auto const& extensionProperty) {
                                                                                                                            return strcmp(extensionProperty.extensionName, requiredExtension) == 0;
                                                                                                                        });
                                                                                        });

        if (unsupportedPropertyIt != requiredExtensions.end()) {
            throw std::runtime_error("Required extension not supported: " + std::string(*unsupportedPropertyIt));
        }

        vk::InstanceCreateInfo createInfo{
            .pApplicationInfo = &appInfo,
            .enabledLayerCount = static_cast<uint32_t>(requiredLayers.size()),
            .ppEnabledLayerNames = requiredLayers.data(),
            .enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size()),
            .ppEnabledExtensionNames = requiredExtensions.data()};

        instance = vk::raii::Instance(context, createInfo);
        std::cout << "Vulkan instance created successfully!" << std::endl;
    }


    std::vector<const char*> getRequiredInstanceExtensions() {
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector<const char*> extensions;

        for (uint32_t i = 0; i < glfwExtensionCount; ++i) {
            extensions.push_back(glfwExtensions[i]);
        }

        if (enableValidationlayers) {
            extensions.push_back(vk::EXTDebugUtilsExtensionName);
        }

        return extensions;
    }

    void pickPhysicalDevice() {
        std::vector<vk::raii::PhysicalDevice> physicalDevices = instance.enumeratePhysicalDevices();
        std::vector<vk::raii::PhysicalDevice>::iterator const devIt = std::ranges::find_if(physicalDevices, [&](auto const& physicalDevice) {
            return isDeviceSuitable(physicalDevice);
        });

        if (devIt == physicalDevices.end()) {
            throw std::runtime_error("failed to find a suitable GPU!");
        }

        physicalDevice = *devIt;
    }

    bool isDeviceSuitable(vk::raii::PhysicalDevice const& physicalDevice) {
        bool supprortVulkan1_3 = physicalDevice.getProperties().apiVersion >= VK_API_VERSION_1_3;

        std::vector<vk::QueueFamilyProperties> queueFamilies = physicalDevice.getQueueFamilyProperties();
        bool supportGraphics = std::ranges::any_of(queueFamilies, [](auto const& queueFamily) { return !!(queueFamily.queueFlags & vk::QueueFlagBits::eGraphics); });

        std::vector<vk::ExtensionProperties> avaiableDeviceExtensions = physicalDevice.enumerateDeviceExtensionProperties();
        bool supportsAllRequiredExtensions = std::ranges::all_of(requiredDeviceExtensions,
                                                                 [&avaiableDeviceExtensions](auto const* requiredDeviceExtension) {
                                                                     return std::ranges::any_of(avaiableDeviceExtensions, [requiredDeviceExtension](auto const& avaiableDeviceExtension) {
                                                                         return std::strcmp(avaiableDeviceExtension.extensionName, requiredDeviceExtension) == 0;
                                                                     });
                                                                 });

        auto features = physicalDevice.template getFeatures2<vk::PhysicalDeviceFeatures2,
                                                             vk::PhysicalDeviceVulkan11Features,
                                                             vk::PhysicalDeviceVulkan13Features,
                                                             vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>();
        bool supportsRequiredFeatures = features.template get<vk::PhysicalDeviceVulkan11Features>().shaderDrawParameters &&
                                        features.template get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering &&
                                        features.template get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState;

        return supprortVulkan1_3 && supportGraphics && supportsAllRequiredExtensions && supportsRequiredFeatures;
    }


    void createLogicalDevice() {
        std::vector<vk::QueueFamilyProperties> queueFamilyProperties = physicalDevice.getQueueFamilyProperties();
        std::vector<vk::QueueFamilyProperties>::iterator graphicsQueueFamilyPropertyIt = std::ranges::find_if(queueFamilyProperties, [](auto const& queueFamilyProperty) {
            return !!(queueFamilyProperty.queueFlags & vk::QueueFlagBits::eGraphics);
        });

        uint32_t graphicsIndex = static_cast<uint32_t>(std::distance(queueFamilyProperties.begin(), graphicsQueueFamilyPropertyIt));

        float queuePriority = 0.5f;

        vk::DeviceQueueCreateInfo deviceQueueCreateInfo{
            .queueFamilyIndex = graphicsIndex,
            .queueCount = 1,
            .pQueuePriorities = &queuePriority,
        };

        vk::StructureChain<vk::PhysicalDeviceFeatures2,
                           vk::PhysicalDeviceVulkan11Features,
                           vk::PhysicalDeviceVulkan13Features,
                           vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>
            featureChain = {
                {},
                {.shaderDrawParameters = true},
                {.dynamicRendering = true},
                {.extendedDynamicState = true},
            };

        vk::DeviceCreateInfo deviceCreateInfo{
            .pNext = &featureChain.get<vk::PhysicalDeviceFeatures2>(),
            .queueCreateInfoCount = 1,
            .pQueueCreateInfos = &deviceQueueCreateInfo,
            .enabledExtensionCount = static_cast<uint32_t>(requiredDeviceExtensions.size()),
            .ppEnabledExtensionNames = requiredDeviceExtensions.data(),
        };

        logicalDevice = vk::raii::Device(physicalDevice, deviceCreateInfo);
        graphicsQueue = vk::raii::Queue(logicalDevice, graphicsIndex, 0);
    }
};

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

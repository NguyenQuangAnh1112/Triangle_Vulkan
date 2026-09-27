#define GLFW_INCLUDE_VULKAN
#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS

#include <vulkan/vulkan_raii.hpp>
#include <GLFW/glfw3.h>
#include <algorithm>
#include <iostream>
#include <cstring>
#include <stdexcept>
#include <cstdint>
#include <limits>
#include <algorithm>


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
    vk::raii::Context mContext;
    vk::raii::Instance mInstance{nullptr};
    vk::raii::DebugUtilsMessengerEXT mDebugMessenger{nullptr};
    vk::raii::SurfaceKHR mSurface{nullptr};
    vk::raii::PhysicalDevice mPhysicalDevice{nullptr};
    vk::raii::Device mLogicalDevice{nullptr};
    vk::raii::Queue graphicsQueue{nullptr};
    vk::raii::SwapchainKHR mSwapChain = nullptr;
    std::vector<vk::Image> mSwapChainImages;
    std::vector<vk::raii::ImageView> mSwapChainImageViews;
    GLFWwindow* mWindow = nullptr;
    vk::Extent2D mSwapChainExtent;
    vk::SurfaceFormatKHR mSwapChainSurfaceFormat;
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
        createSurface();
        pickPhysicalDevice();
        createLogicalDevice();
        createSwapChain();
        createImageViews();
        createGraphicsPipeline();
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

        mDebugMessenger = mInstance.createDebugUtilsMessengerEXT(debugUtilsMessengerCreateInfoEXT);
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

        std::vector<vk::LayerProperties> layerProperties = mContext.enumerateInstanceLayerProperties();

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

        std::vector<vk::ExtensionProperties> extensionProperties = mContext.enumerateInstanceExtensionProperties();
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

        mInstance = vk::raii::Instance(mContext, createInfo);
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
        std::vector<vk::raii::PhysicalDevice> physicalDevices = mInstance.enumeratePhysicalDevices();
        std::vector<vk::raii::PhysicalDevice>::iterator const devIt = std::ranges::find_if(physicalDevices, [&](auto const& physicalDevice) {
            return isDeviceSuitable(physicalDevice);
        });

        if (devIt == physicalDevices.end()) {
            throw std::runtime_error("failed to find a suitable GPU!");
        }

        mPhysicalDevice = *devIt;
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
        std::vector<vk::QueueFamilyProperties> queueFamilyProperties = mPhysicalDevice.getQueueFamilyProperties();

        uint32_t queueIndex = ~0;
        for (uint32_t qfpIndex = 0; qfpIndex < queueFamilyProperties.size(); qfpIndex++) {
            if ((queueFamilyProperties[qfpIndex].queueFlags & vk::QueueFlagBits::eGraphics) && mPhysicalDevice.getSurfaceSupportKHR(qfpIndex, *mSurface)) {
                queueIndex = qfpIndex;
                break;
            }
        }

        if (queueIndex == ~0) {
            throw std::runtime_error("Could not find a queue for graphics and present -> terminating");
        }


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

        float queuePriority = 0.5f;

        vk::DeviceQueueCreateInfo deviceQueueCreateInfo{
            .queueFamilyIndex = queueIndex,
            .queueCount = 1,
            .pQueuePriorities = &queuePriority,
        };

        vk::DeviceCreateInfo deviceCreateInfo{
            .pNext = &featureChain.get<vk::PhysicalDeviceFeatures2>(),
            .queueCreateInfoCount = 1,
            .pQueueCreateInfos = &deviceQueueCreateInfo,
            .enabledExtensionCount = static_cast<uint32_t>(requiredDeviceExtensions.size()),
            .ppEnabledExtensionNames = requiredDeviceExtensions.data(),
        };

        mLogicalDevice = vk::raii::Device(mPhysicalDevice, deviceCreateInfo);
        graphicsQueue = vk::raii::Queue(mLogicalDevice, queueIndex, 0);
    }


    void createSurface() {
        VkSurfaceKHR _surface;

        if (glfwCreateWindowSurface(*mInstance, mWindow, nullptr, &_surface) != 0) {
            throw std::runtime_error("failed to create window surface!");
        }

        mSurface = vk::raii::SurfaceKHR(mInstance, _surface);
    }


    vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats) {
        const std::vector<vk::SurfaceFormatKHR>::const_iterator formatIt = std::ranges::find_if(availableFormats, [](const vk::SurfaceFormatKHR& format) {
            return format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
        });

        return formatIt != availableFormats.end() ? *formatIt : availableFormats[0];
    }


    vk::PresentModeKHR chooseSwapPresentMode(std::vector<vk::PresentModeKHR> const& availablePresentModes) {
        return vk::PresentModeKHR::eFifo;
    }


    vk::Extent2D chooseSwapExtent(vk::SurfaceCapabilitiesKHR const& surfaceCapabilities) {
        if (surfaceCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return surfaceCapabilities.currentExtent;
        }

        int width, height;

        glfwGetFramebufferSize(mWindow, &width, &height);

        return {
            std::clamp<uint32_t>(width, surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width),
            std::clamp<uint32_t>(height, surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height)};
    }


    uint32_t chooseSwapMinImageCount(vk::SurfaceCapabilitiesKHR const& surfaceCapabilities) {
        auto minImageCount = std::max(3u, surfaceCapabilities.minImageCount);

        if ((surfaceCapabilities.maxImageCount > 0) && (surfaceCapabilities.maxImageCount < minImageCount)) {
            minImageCount = surfaceCapabilities.maxImageCount;
        }

        return minImageCount;
    }


    void createSwapChain() {
        vk::SurfaceCapabilitiesKHR surfaceCapabilities = mPhysicalDevice.getSurfaceCapabilitiesKHR(*mSurface);
        std::vector<vk::SurfaceFormatKHR> availableFormats = mPhysicalDevice.getSurfaceFormatsKHR(*mSurface);
        std::vector<vk::PresentModeKHR> availablePresentModes = mPhysicalDevice.getSurfacePresentModesKHR(*mSurface);

        mSwapChainExtent = chooseSwapExtent(surfaceCapabilities);
        mSwapChainSurfaceFormat = chooseSwapSurfaceFormat(availableFormats);
        uint32_t minImageCount = chooseSwapMinImageCount(surfaceCapabilities);

        vk::SwapchainCreateInfoKHR swapChainCreateInfo{.surface = *mSurface,
                                                       .minImageCount = minImageCount,
                                                       .imageFormat = mSwapChainSurfaceFormat.format,
                                                       .imageColorSpace = mSwapChainSurfaceFormat.colorSpace,
                                                       .imageExtent = mSwapChainExtent,
                                                       .imageArrayLayers = 1,
                                                       .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
                                                       .imageSharingMode = vk::SharingMode::eExclusive,
                                                       .preTransform = surfaceCapabilities.currentTransform,
                                                       .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
                                                       .presentMode = chooseSwapPresentMode(availablePresentModes),
                                                       .clipped = true};

        mSwapChain = vk::raii::SwapchainKHR(mLogicalDevice, swapChainCreateInfo);
        mSwapChainImages = mSwapChain.getImages();
    }


    void createImageViews() {
        assert(mSwapChainImageViews.empty());

        vk::ImageViewCreateInfo imageViewCreateInfo{
            .viewType = vk::ImageViewType::e2D,
            .format = mSwapChainSurfaceFormat.format,
            .subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}};

        for (auto& image : mSwapChainImages) {
            imageViewCreateInfo.image = image;
            mSwapChainImageViews.emplace_back(mLogicalDevice, imageViewCreateInfo);
        }
    }


    void createGraphicsPipeline() {
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

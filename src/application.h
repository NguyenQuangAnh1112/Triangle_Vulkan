#define GLFW_INCLUDE_VULKAN
#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#define VULKAN_HPP_HANDLE_EROR_OUT_STRUCT_CRUCTORS

#include <vulkan/vulkan_raii.hpp>
#include <GLFW/glfw3.h>
#include <iostream>
#include <cstdint>
#include <glm/glm.hpp>

inline std::vector<const char*> requiredDeviceExtensions = {vk::KHRSwapchainExtensionName};

#ifdef NDEBUG
constexpr bool enableValidationlayers = false;
#else
constexpr bool enableValidationlayers = true;
#endif

static constexpr uint32_t WIDTH = 800;
static constexpr uint32_t HEIGHT = 600;
constexpr int MAX_FRAME_IN_FLIGHT = 2;

inline const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"};


class HelloTriangleApplication {
  public:
    void run();

  private:
    vk::raii::Context mContext;
    vk::raii::Instance mInstance{nullptr};
    vk::raii::DebugUtilsMessengerEXT mDebugMessenger{nullptr};
    vk::raii::SurfaceKHR mSurface{nullptr};
    vk::raii::PhysicalDevice mPhysicalDevice{nullptr};
    vk::raii::Device mLogicalDevice{nullptr};
    vk::raii::Queue mGraphicsQueue{nullptr};
    vk::raii::SwapchainKHR mSwapChain = nullptr;
    std::vector<vk::Image> mSwapChainImages;
    std::vector<vk::raii::ImageView> mSwapChainImageViews;
    GLFWwindow* mWindow = nullptr;
    vk::Extent2D mSwapChainExtent;
    vk::SurfaceFormatKHR mSwapChainSurfaceFormat;
    vk::raii::PipelineLayout mPipelineLayout = nullptr;
    vk::raii::Pipeline mGraphicsPipeline = nullptr;
    vk::raii::CommandPool mCommandPool = nullptr;
    uint32_t mQueueIndex = ~0;
    uint32_t mCurrentFrame = 0;
    std::vector<vk::raii::CommandBuffer> mCommandBuffers;
    std::vector<vk::raii::Semaphore> mPresentCompleteSemaphores;
    std::vector<vk::raii::Semaphore> mRenderFinishedSemaphores;
    std::vector<vk::raii::Fence> mInFlightFences;
    bool mFramebufferResized = false;
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


    void initWindow();

    void initVulkan();

    void setupDebugMessenger();

    void mainLoop();

    void cleanup();

    void createInstance();

    std::vector<const char*> getRequiredInstanceExtensions();

    void pickPhysicalDevice();

    bool isDeviceSuitable(vk::raii::PhysicalDevice const& physicalDevice);

    void createLogicalDevice();

    void createSurface();

    vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats);

    vk::PresentModeKHR chooseSwapPresentMode(std::vector<vk::PresentModeKHR> const& availablePresentModes);

    vk::Extent2D chooseSwapExtent(vk::SurfaceCapabilitiesKHR const& surfaceCapabilities);

    uint32_t chooseSwapMinImageCount(vk::SurfaceCapabilitiesKHR const& surfaceCapabilities);

    void createSwapChain();

    void createImageViews();

    void createGraphicsPipeline();

    [[nodiscard]]
    vk::raii::ShaderModule createShaderModule(const std::vector<char>& code) const;

    void createCommandPool();

    void createCommandBuffers();

    void recordCommandBuffer(uint32_t imageIndex);

    void transition_image_layout(
        uint32_t imageIndex,
        vk::ImageLayout old_layout,
        vk::ImageLayout new_layout,
        vk::AccessFlags2 src_access_mask,
        vk::AccessFlags2 dst_access_mask,
        vk::PipelineStageFlags2 src_stage_mask,
        vk::PipelineStageFlags2 dst_stage_mask);

    void transition_image_for_rendering(uint32_t imageIndex);

    void transition_image_for_present(uint32_t imageIndex);

    void drawFrame();

    void createSyncObjects();

    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);

    void cleanupSwapChain();

    void recreateSwapChain();
};

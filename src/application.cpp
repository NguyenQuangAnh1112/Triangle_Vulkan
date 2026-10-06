#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#define VULKAN_HPP_HANDLE_EROR_OUT_STRUCT_CRUCTORS

#include "vertex.h"
#include "utils.h"
#include "application.h"
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <cstdint>
#include <limits>
#include <array>

void HelloTriangleApplication::run() {
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
}


void HelloTriangleApplication::initWindow() {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    mWindow = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
    glfwSetWindowUserPointer(mWindow, this);
    glfwSetFramebufferSizeCallback(mWindow, framebufferResizeCallback);
}


void HelloTriangleApplication::initVulkan() {
    createInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createImageViews();
    createGraphicsPipeline();
    createCommandPool();
    createCommandBuffers();
    createSyncObjects();
}


void HelloTriangleApplication::setupDebugMessenger() {
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


void HelloTriangleApplication::mainLoop() {
    while (!glfwWindowShouldClose(mWindow)) {
        glfwPollEvents();
        drawFrame();
    }

    mLogicalDevice.waitIdle();
}


void HelloTriangleApplication::cleanup() {
    cleanupSwapChain();
    glfwDestroyWindow(mWindow);
    glfwTerminate();
}


void HelloTriangleApplication::createInstance() {
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


std::vector<const char*> HelloTriangleApplication::getRequiredInstanceExtensions() {
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


void HelloTriangleApplication::pickPhysicalDevice() {
    std::vector<vk::raii::PhysicalDevice> physicalDevices = mInstance.enumeratePhysicalDevices();
    std::vector<vk::raii::PhysicalDevice>::iterator const devIt = std::ranges::find_if(physicalDevices, [&](auto const& physicalDevice) {
        return isDeviceSuitable(physicalDevice);
    });

    if (devIt == physicalDevices.end()) {
        throw std::runtime_error("failed to find a suitable GPU!");
    }

    mPhysicalDevice = *devIt;
}


bool HelloTriangleApplication::isDeviceSuitable(vk::raii::PhysicalDevice const& physicalDevice) {
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


void HelloTriangleApplication::createLogicalDevice() {
    std::vector<vk::QueueFamilyProperties> queueFamilyProperties = mPhysicalDevice.getQueueFamilyProperties();

    uint32_t queueIndex = ~0;
    for (uint32_t qfpIndex = 0; qfpIndex < queueFamilyProperties.size(); qfpIndex++) {
        if ((queueFamilyProperties[qfpIndex].queueFlags & vk::QueueFlagBits::eGraphics) && mPhysicalDevice.getSurfaceSupportKHR(qfpIndex, *mSurface)) {
            queueIndex = qfpIndex;
            break;
        }
    }

    mQueueIndex = queueIndex;

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
            {.synchronization2 = true, .dynamicRendering = true},
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
    mGraphicsQueue = vk::raii::Queue(mLogicalDevice, queueIndex, 0);
}


void HelloTriangleApplication::createSurface() {
    VkSurfaceKHR _surface;

    if (glfwCreateWindowSurface(*mInstance, mWindow, nullptr, &_surface) != 0) {
        throw std::runtime_error("failed to create window surface!");
    }

    mSurface = vk::raii::SurfaceKHR(mInstance, _surface);
}


vk::SurfaceFormatKHR HelloTriangleApplication::chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats) {
    const std::vector<vk::SurfaceFormatKHR>::const_iterator formatIt = std::ranges::find_if(availableFormats, [](const vk::SurfaceFormatKHR& format) {
        return format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
    });

    return formatIt != availableFormats.end() ? *formatIt : availableFormats[0];
}


vk::PresentModeKHR HelloTriangleApplication::chooseSwapPresentMode(std::vector<vk::PresentModeKHR> const& availablePresentModes) {
    return vk::PresentModeKHR::eFifo;
}


vk::Extent2D HelloTriangleApplication::chooseSwapExtent(vk::SurfaceCapabilitiesKHR const& surfaceCapabilities) {
    if (surfaceCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return surfaceCapabilities.currentExtent;
    }

    int width, height;

    glfwGetFramebufferSize(mWindow, &width, &height);

    return {
        std::clamp<uint32_t>(width, surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width),
        std::clamp<uint32_t>(height, surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height)};
}


uint32_t HelloTriangleApplication::chooseSwapMinImageCount(vk::SurfaceCapabilitiesKHR const& surfaceCapabilities) {
    auto minImageCount = std::max(3u, surfaceCapabilities.minImageCount);

    if ((surfaceCapabilities.maxImageCount > 0) && (surfaceCapabilities.maxImageCount < minImageCount)) {
        minImageCount = surfaceCapabilities.maxImageCount;
    }

    return minImageCount;
}


void HelloTriangleApplication::createSwapChain() {
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


void HelloTriangleApplication::createImageViews() {
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


void HelloTriangleApplication::createGraphicsPipeline() {
    auto shaderCode = readFile("./shaders/slang.spv");
    vk::raii::ShaderModule shaderModule = createShaderModule(shaderCode);

    vk::PipelineShaderStageCreateInfo verShaderStateInfo{
        .stage = vk::ShaderStageFlagBits::eVertex,
        .module = shaderModule,
        .pName = "vertMain"};

    vk::PipelineShaderStageCreateInfo fragShaderStageInfo{
        .stage = vk::ShaderStageFlagBits::eFragment,
        .module = shaderModule,
        .pName = "fragMain"};

    vk::PipelineShaderStageCreateInfo shaderStages[] = {verShaderStateInfo, fragShaderStageInfo};


    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescription = Vertex::getAttributeDescription();

    vk::PipelineVertexInputStateCreateInfo vertexInputInfo{
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &bindingDescription,
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescription.size()),
        .pVertexAttributeDescriptions = attributeDescription.data(),
    };

    vk::PipelineInputAssemblyStateCreateInfo inputAssemblyInfo{
        .topology = vk::PrimitiveTopology::eTriangleList};

    vk::Viewport viewport{
        0.0f, 0.0f,
        static_cast<float>(mSwapChainExtent.width),
        static_cast<float>(mSwapChainExtent.height),
        0.0f, 1.0f};

    vk::Rect2D scissor{vk::Offset2D{0, 0}, mSwapChainExtent};

    std::vector<vk::DynamicState> dynamicStates = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor};

    vk::PipelineDynamicStateCreateInfo dynamicState{
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
        .pDynamicStates = dynamicStates.data()};

    vk::PipelineViewportStateCreateInfo viewportState{
        .viewportCount = 1,
        .scissorCount = 1};

    vk::PipelineRasterizationStateCreateInfo rasterizer{
        .depthClampEnable = vk::False,
        .rasterizerDiscardEnable = vk::False,
        .polygonMode = vk::PolygonMode::eFill,
        .cullMode = vk::CullModeFlagBits::eBack,
        .frontFace = vk::FrontFace::eClockwise,
        .depthBiasEnable = vk::False,
        .lineWidth = 1.0f};

    vk::PipelineMultisampleStateCreateInfo multisampling{
        .rasterizationSamples = vk::SampleCountFlagBits::e1,
        .sampleShadingEnable = vk::False};

    vk::PipelineDepthStencilStateCreateInfo depthStencilState;

    vk::PipelineColorBlendAttachmentState colorBlendAttachment{
        .blendEnable = vk::False,
        .srcColorBlendFactor = vk::BlendFactor::eSrcAlpha,
        .dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha,
        .colorBlendOp = vk::BlendOp::eAdd,
        .srcAlphaBlendFactor = vk::BlendFactor::eOne,
        .dstAlphaBlendFactor = vk::BlendFactor::eZero,
        .alphaBlendOp = vk::BlendOp::eAdd,
        .colorWriteMask = vk::ColorComponentFlagBits::eR |
                          vk::ColorComponentFlagBits::eG |
                          vk::ColorComponentFlagBits::eB |
                          vk::ColorComponentFlagBits::eA};

    vk::PipelineColorBlendStateCreateInfo colorBlending{
        .logicOpEnable = vk::False,
        .logicOp = vk::LogicOp::eCopy,
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachment};


    vk::PipelineLayoutCreateInfo pipelineLayoutInfo{
        .setLayoutCount = 0,
        .pushConstantRangeCount = 0};

    mPipelineLayout = vk::raii::PipelineLayout(mLogicalDevice, pipelineLayoutInfo);

    // ==================================================================
    vk::StructureChain<vk::GraphicsPipelineCreateInfo, vk::PipelineRenderingCreateInfo> pipelineCreateInfoChain = {
        {.stageCount = 2,
         .pStages = shaderStages,
         .pVertexInputState = &vertexInputInfo,
         .pInputAssemblyState = &inputAssemblyInfo,
         .pViewportState = &viewportState,
         .pRasterizationState = &rasterizer,
         .pMultisampleState = &multisampling,
         .pColorBlendState = &colorBlending,
         .pDynamicState = &dynamicState,
         .layout = *mPipelineLayout,
         .renderPass = nullptr},
        {.colorAttachmentCount = 1,
         .pColorAttachmentFormats = &mSwapChainSurfaceFormat.format}};

    mGraphicsPipeline = mLogicalDevice.createGraphicsPipeline(
        nullptr,
        pipelineCreateInfoChain.get<vk::GraphicsPipelineCreateInfo>());
    // ==================================================================
}


[[nodiscard]]
vk::raii::ShaderModule HelloTriangleApplication::createShaderModule(const std::vector<char>& code) const {
    vk::ShaderModuleCreateInfo shaderModuleCreateInfo{
        .codeSize = code.size() * sizeof(char),
        .pCode = reinterpret_cast<const uint32_t*>(code.data())};

    vk::raii::ShaderModule shaderModule{mLogicalDevice, shaderModuleCreateInfo};

    return shaderModule;
}


void HelloTriangleApplication::createCommandPool() {
    vk::CommandPoolCreateInfo poolInfo{
        .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex = mQueueIndex};

    mCommandPool = vk::raii::CommandPool(mLogicalDevice, poolInfo);
}


void HelloTriangleApplication::createCommandBuffers() {
    vk::CommandBufferAllocateInfo allocInfo{
        .commandPool = mCommandPool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = MAX_FRAME_IN_FLIGHT};

    mCommandBuffers = vk::raii::CommandBuffers(mLogicalDevice, allocInfo);
};


void HelloTriangleApplication::recordCommandBuffer(uint32_t imageIndex) {
    auto& commandBuffer = mCommandBuffers[mCurrentFrame];
    commandBuffer.begin({});

    transition_image_for_rendering(imageIndex);

    vk::ClearValue clearColor = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);
    vk::RenderingAttachmentInfo attachmentInfo = {
        .imageView = mSwapChainImageViews[imageIndex],
        .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .clearValue = clearColor};

    vk::RenderingInfo renderingInfo = {
        .renderArea = {.offset = {0, 0}, .extent = mSwapChainExtent},
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &attachmentInfo};

    mCommandBuffers[mCurrentFrame].beginRendering(renderingInfo);

    mCommandBuffers[mCurrentFrame].bindPipeline(vk::PipelineBindPoint::eGraphics, *mGraphicsPipeline);

    mCommandBuffers[mCurrentFrame].setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(mSwapChainExtent.width), static_cast<float>(mSwapChainExtent.height), 0.0f, 1.0f));
    mCommandBuffers[mCurrentFrame].setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), mSwapChainExtent));

    mCommandBuffers[mCurrentFrame].draw(3, 1, 0, 0);

    mCommandBuffers[mCurrentFrame].endRendering();

    transition_image_for_present(imageIndex);

    commandBuffer.end();
}


void HelloTriangleApplication::transition_image_layout(
    uint32_t imageIndex,
    vk::ImageLayout old_layout,
    vk::ImageLayout new_layout,
    vk::AccessFlags2 src_access_mask,
    vk::AccessFlags2 dst_access_mask,
    vk::PipelineStageFlags2 src_stage_mask,
    vk::PipelineStageFlags2 dst_stage_mask) {
    vk::ImageMemoryBarrier2 barrier = {
        .srcStageMask = src_stage_mask,
        .srcAccessMask = src_access_mask,
        .dstStageMask = dst_stage_mask,
        .dstAccessMask = dst_access_mask,
        .oldLayout = old_layout,
        .newLayout = new_layout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = mSwapChainImages[imageIndex],
        .subresourceRange = {
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1}};
    vk::DependencyInfo dependency_info = {
        .dependencyFlags = {},
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &barrier};
    mCommandBuffers[mCurrentFrame].pipelineBarrier2(dependency_info);
}


void HelloTriangleApplication::transition_image_for_rendering(uint32_t imageIndex) {
    transition_image_layout(
        imageIndex,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eColorAttachmentOptimal,
        {},
        vk::AccessFlagBits2::eColorAttachmentWrite,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput);
}


void HelloTriangleApplication::transition_image_for_present(uint32_t imageIndex) {
    transition_image_layout(
        imageIndex,
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::ImageLayout::ePresentSrcKHR,
        vk::AccessFlagBits2::eColorAttachmentWrite,
        {},
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::PipelineStageFlagBits2::eBottomOfPipe);
}


void HelloTriangleApplication::drawFrame() {
    auto fenceResult = mLogicalDevice.waitForFences(*mInFlightFences[mCurrentFrame], vk::True, UINT64_MAX);
    if (fenceResult != vk::Result::eSuccess) {
        throw std::runtime_error("failed to wait for fence!");
    }

    auto [result, imageIndex] = mSwapChain.acquireNextImage(UINT64_MAX, *mPresentCompleteSemaphores[mCurrentFrame], nullptr);

    if (result == vk::Result::eErrorOutOfDateKHR) {
        recreateSwapChain();
        return; // Thoát sớm, Fence vẫn Signaled nên không bao giờ bị Deadlock!
    }

    if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR) {
        assert(result == vk::Result::eTimeout || result == vk::Result::eNotReady);
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    mLogicalDevice.resetFences(*mInFlightFences[mCurrentFrame]);

    mCommandBuffers[mCurrentFrame].reset();
    recordCommandBuffer(imageIndex);

    vk::PipelineStageFlags waitDestinationStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
    const vk::SubmitInfo submitInfo{
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &*mPresentCompleteSemaphores[mCurrentFrame],
        .pWaitDstStageMask = &waitDestinationStageMask,
        .commandBufferCount = 1,
        .pCommandBuffers = &*mCommandBuffers[mCurrentFrame],
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &*mRenderFinishedSemaphores[imageIndex]};

    mGraphicsQueue.submit(submitInfo, *mInFlightFences[mCurrentFrame]);

    const vk::PresentInfoKHR presentInfoKHR{
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &*mRenderFinishedSemaphores[imageIndex],
        .swapchainCount = 1,
        .pSwapchains = &*mSwapChain,
        .pImageIndices = &imageIndex};

    result = mGraphicsQueue.presentKHR(presentInfoKHR);
    mCurrentFrame = (mCurrentFrame + 1) % MAX_FRAME_IN_FLIGHT;
}


void HelloTriangleApplication::createSyncObjects() {
    for (size_t i = 0; i < mSwapChainImages.size(); i++) {
        mRenderFinishedSemaphores.emplace_back(mLogicalDevice, vk::SemaphoreCreateInfo());
    }

    for (size_t i = 0; i < MAX_FRAME_IN_FLIGHT; i++) {
        mPresentCompleteSemaphores.emplace_back(mLogicalDevice, vk::SemaphoreCreateInfo());
        mInFlightFences.emplace_back(mLogicalDevice, vk::FenceCreateInfo{.flags = vk::FenceCreateFlagBits::eSignaled});
    }
}


void HelloTriangleApplication::framebufferResizeCallback(GLFWwindow* window, int width, int height) {
    auto app = reinterpret_cast<HelloTriangleApplication*>(glfwGetWindowUserPointer(window));
    app->mFramebufferResized = true;
}


void HelloTriangleApplication::cleanupSwapChain() {
    mSwapChainImageViews.clear();
    mSwapChain = nullptr;
}

void HelloTriangleApplication::recreateSwapChain() {
    int width = 0, height = 0;
    glfwGetFramebufferSize(mWindow, &width, &height);
    while ((width == 0 || height == 0) && !glfwWindowShouldClose(mWindow)) {
        glfwGetFramebufferSize(mWindow, &width, &height);
        glfwWaitEvents();
    }

    if (glfwWindowShouldClose(mWindow)) {
        return;
    }

    mLogicalDevice.waitIdle();

    cleanupSwapChain();

    createSwapChain();
    createImageViews();
}

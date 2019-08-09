#include <optional>
#include <iostream>
#include <fstream>
#include <string> // needed for checking validation layers
#include <algorithm>
#include <vector>
#include <set>
#include <cassert>
#include <exception>
#include <cstdlib>

#include <glm/glm.hpp>
#define GLFW_INCLUDE_VULKAN
#ifdef _WINDOWS
#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_EXPOSE_NATIVE_WGL
#define GLFW_EXPOSE_NATIVE_WIN32
#endif // _WINDOWS
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>


#ifdef _WINDOWS
#include <Windows.h>
#endif // _WINDOWS


static const std::vector<const char*> s_validationLayers = { "VK_LAYER_KHRONOS_validation" }; // following tutorial structure, refactor once we're got a triangle on screen
static const std::vector<const char*> s_requiredPhysicalDeviceExtentions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME }; // these constraints are meant to be used on a created device, not during device creation

class VulkanApp
{
public:
	VulkanApp()
		: c_windowWidth(800)
		, c_windowHeight(600)
		, m_window(nullptr)
		, m_vulkanInstance(nullptr)
		, m_vulkanPhysicalDevice(nullptr)
		, m_vulkanLogicalDevice(nullptr)
		, m_graphicsQueue(nullptr)
		, m_surfaceToDrawTo(nullptr)
		, m_presentQueue(nullptr)
		, m_swapChain(nullptr)
		, m_vertexShaderModule(nullptr)
		, m_fragmentShaderModule(nullptr)
		, m_pipeline(nullptr)
		, m_pipelineLayout(nullptr)
		, m_renderPass(nullptr)
		, m_commandPool(nullptr)
		, m_imageReadyToDrawToSemaphore(nullptr)
		, m_finishedDrawingSemaphore(nullptr)
#if (NDEBUG)
		, m_useVulkanValidationLayers(false) // release build
#else
		, m_useVulkanValidationLayers(true) // debug build
#endif

	{}

	~VulkanApp()
	{}

	void Run()
	{
		Init();
		MainLoop();
		Shutdown();
	}
private:
	struct QueueFamilyIndices
	{
		std::optional<uint32_t> m_graphicsFamilyIndex;
		std::optional<uint32_t> m_presentFamilyIndex;

		bool ValueReady()
		{
			return m_graphicsFamilyIndex.has_value() && m_presentFamilyIndex.has_value();
		}
	};

	struct SwapChainSupportDetails
	{
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	void Init()
	{
		try
		{
			InitWindow();
			InitVulkan();
		}
		catch (const std::exception& ex)
		{
#ifdef _WINDOWS
			OutputDebugString(ex.what());
#else
			// non windows code here
#endif

		}
	}
	void InitWindow()
	{
		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		m_window = glfwCreateWindow(c_windowWidth, c_windowHeight, "Vulkan window", nullptr, nullptr);
	}
	void InitVulkan()
	{
		CreateVulkanInstance();
		SetupVulkanDebugMessenger();
		CreateSurfaceToDrawTo();
		QueryVulkanExtentions();		
		SelectVulkanDevice();
		CreateLogicalVulkanDevice();
		CreateSwapChain();
		CreateImageViews();
		CreateRenderPass();
		CreateGraphicsPipeline();
		CreateFrameBuffers();
		CreateCommandPool();
		CreateCommandBuffers();
		CreateSemaphores();
	}

	bool AreVulkanValidationLayersSupported()
	{
		uint32_t nLayers = 0;
		vkEnumerateInstanceLayerProperties(&nLayers, nullptr);

		std::vector<VkLayerProperties> availableLayers(nLayers);
		vkEnumerateInstanceLayerProperties(&nLayers, availableLayers.data());

		const std::string targetLayerStr = std::string(s_validationLayers[0]);
		bool validationLayerFound = false;
		for (uint32_t i = 0; i < nLayers && !validationLayerFound; ++i)
		{
			validationLayerFound = targetLayerStr == availableLayers[i].layerName;
		}
		return validationLayerFound;
	}

	std::vector<const char*> GetRequiredVulkanExtentions()
	{
		// Message callback
		uint32_t glfwRequiredExtCount = 0;
		const char** glfwExtCStrs;
		glfwExtCStrs = glfwGetRequiredInstanceExtensions(&glfwRequiredExtCount);
		
		std::vector<const char*> extCStrs(glfwExtCStrs, glfwExtCStrs + glfwRequiredExtCount);

		if (AreVulkanValidationLayersSupported())
		{
			extCStrs.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}

		return extCStrs;
	}

	void CreateVulkanInstance()
	{
		if (m_useVulkanValidationLayers && !AreVulkanValidationLayersSupported())
		{
			throw std::runtime_error("tried to run with Vulkan validation layers, this setup doesn't support them.");
		}

		VkApplicationInfo appInfo = {};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Vulkan Triangle";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "Learning Vulkan Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_1;

		VkInstanceCreateInfo instanceCreateInfo = {};
		instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instanceCreateInfo.pApplicationInfo = &appInfo; // tutorial used stack memory, should be ok
		
		const std::vector<const char*> requiredExtentions = GetRequiredVulkanExtentions();
		
		instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtentions.size());
		instanceCreateInfo.ppEnabledExtensionNames = requiredExtentions.data();

		VkDebugUtilsMessengerCreateInfoEXT dbgCreateInfo = {};
		// declared outside the if so doesn't go out of scope on calling vkCreateInstance()

		if (m_useVulkanValidationLayers)
		{
			
			instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(s_validationLayers.size());
			instanceCreateInfo.ppEnabledLayerNames = s_validationLayers.data();
			PopulateVulkanDebugMessengerCreateInfo(dbgCreateInfo);
			instanceCreateInfo.pNext = &dbgCreateInfo;
		}
		else
		{
			instanceCreateInfo.enabledLayerCount = 0;
			instanceCreateInfo.pNext = nullptr;
		}

		VkResult instanceCreateRes = vkCreateInstance(&instanceCreateInfo, nullptr, &m_vulkanInstance); // the nullptr would be for an allocator callback function.
		if (instanceCreateRes != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create Vulkan instance");
		}
		std::cout << "Vulkan Instance created successfully" << std::endl;
	}

	void PopulateVulkanDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
	{
		createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback = VulkanDebugCallback;
	}

	void SetupVulkanDebugMessenger()
	{
		if (!m_useVulkanValidationLayers)
			return;

		VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
		PopulateVulkanDebugMessengerCreateInfo(createInfo);

		if (CreateDebugUtilsMessengerEXT(m_vulkanInstance, &createInfo, nullptr, &m_vulkanDebugMessenger) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to set up a Vulkan debug messenger!");
		}
	}

	void QueryVulkanExtentions()
	{
		// assumes m_vulkanInstance has been initialised
		uint32_t nVulkanExtentions = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &nVulkanExtentions, nullptr);
		std::vector<VkExtensionProperties> extensions(nVulkanExtentions);
		vkEnumerateInstanceExtensionProperties(nullptr, &nVulkanExtentions, extensions.data());
		std::cout << nVulkanExtentions << " Vulkan extensions detected:\n";
		for (const VkExtensionProperties& currExt: extensions)
		{
			std::cout << currExt.extensionName << std::endl;
		}
	}

	void CreateSurfaceToDrawTo()
	{
		// this needs to be called before SelectVulkanDevice()
#ifdef _WINDOWS
		// m_surfaceToDrawTo
		VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {};
		surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
		surfaceCreateInfo.hwnd = glfwGetWin32Window(m_window);
		surfaceCreateInfo.hinstance = GetModuleHandle(nullptr);
		if (vkCreateWin32SurfaceKHR(m_vulkanInstance, &surfaceCreateInfo, nullptr, &m_surfaceToDrawTo) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create surface to draw to");
		}

		// glfwCreateWindowSurface() easier way to do the above, function part of glfw...

#else
		// non windows equivelent code here
#endif // _WINDOWS
	}

	void SelectVulkanDevice()
	{
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(m_vulkanInstance, &deviceCount, nullptr);

		if (deviceCount == 0)
		{
			throw std::runtime_error("failed to find GPUs with Vulkan support!");
		}

		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(m_vulkanInstance, &deviceCount, devices.data());

		uint32_t suitabilityScoreToBeat = 0;
		uint32_t currentDeviceSuitabilityScore = 0;
		for (const VkPhysicalDevice& currentDevice : devices)
		{
			currentDeviceSuitabilityScore = CalculateVulkanDeviceSuitability(currentDevice);
			if (suitabilityScoreToBeat < currentDeviceSuitabilityScore)
			{
				m_vulkanPhysicalDevice = currentDevice;
				suitabilityScoreToBeat = currentDeviceSuitabilityScore;
			}
		}

		if (m_vulkanPhysicalDevice == nullptr)
		{
			throw std::runtime_error("Found Vulkan physical devices, but none were suitable");
		}

		m_graphicsQueueFamilyIndices = FindQueueFamilies(m_vulkanPhysicalDevice);

		if (!m_graphicsQueueFamilyIndices.ValueReady())
		{
			throw std::runtime_error("Found Vulkan physical devices, but the device didn't support queue families");
		}
	}

	QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device)
	{
		// finds command queues, just care about graphics. could expand to 
		QueueFamilyIndices indices;
		uint32_t nQueueFamilies = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &nQueueFamilies, nullptr);
		if (nQueueFamilies == 0)
		{
			throw std::runtime_error("No Vulkan Queue families found");
		}
		std::vector<VkQueueFamilyProperties> queueFamilies(nQueueFamilies);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &nQueueFamilies, queueFamilies.data());

		uint32_t i = 0;
		for (const VkQueueFamilyProperties& currentQueueFamilyProperties : queueFamilies)
		{
			VkBool32  gotPresentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_surfaceToDrawTo, &gotPresentSupport);

			if (currentQueueFamilyProperties.queueCount > 0 && currentQueueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				indices.m_graphicsFamilyIndex = i;
			}
			if (currentQueueFamilyProperties.queueCount > 0 && gotPresentSupport)
			{
				indices.m_presentFamilyIndex = i;
			}
			if (indices.ValueReady())
			{
				return indices;
			}
			++i;
		}
		return indices;
	}

	uint32_t CalculateVulkanDeviceSuitability(VkPhysicalDevice deviceToCheck)
	{
		uint32_t suitabilityScore = 0;
		VkPhysicalDeviceProperties deviceProperties = {};
		VkPhysicalDeviceFeatures deviceFeatures = {};
		vkGetPhysicalDeviceProperties(deviceToCheck, &deviceProperties);
		vkGetPhysicalDeviceFeatures(deviceToCheck, &deviceFeatures);
		if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		{
			suitabilityScore += 1000;
		}
		suitabilityScore += deviceProperties.limits.maxImageDimension2D;

		// return 0 if note good enough checks at end
		if (!deviceFeatures.geometryShader)
		{
			suitabilityScore = 0;
		}

		const bool deviceMeetsMinSupportExtentions = DeviceHasMinimumExtentionSupportLevel(deviceToCheck);
		if (!deviceMeetsMinSupportExtentions)
		{
			suitabilityScore = 0;
		}

		bool swapChainSupportNeedsMet = DeviceHasMinimumSwapChainSupportLevel(deviceToCheck);
		if (!swapChainSupportNeedsMet)
		{
			suitabilityScore = 0;
		}

		return suitabilityScore;
	}

	bool DeviceHasMinimumExtentionSupportLevel(VkPhysicalDevice deviceToCheck)
	{
		uint32_t nExtentions = 0;
		vkEnumerateDeviceExtensionProperties(deviceToCheck, nullptr, &nExtentions, nullptr);
		if (nExtentions == 0)
		{
			return false;
		}
		std::vector<VkExtensionProperties> extentionsPresent(nExtentions);
		vkEnumerateDeviceExtensionProperties(deviceToCheck, nullptr, &nExtentions, extentionsPresent.data());
		std::set<std::string> requiredExtentionsSet(s_requiredPhysicalDeviceExtentions.begin(), s_requiredPhysicalDeviceExtentions.end());

		for (const VkExtensionProperties& extention : extentionsPresent)
		{
			requiredExtentionsSet.erase(extention.extensionName);
		}

		return requiredExtentionsSet.empty(); // if the set isn't empty there's a required extention that isn't supported
	}

	bool DeviceHasMinimumSwapChainSupportLevel(VkPhysicalDevice device)
	{
		SwapChainSupportDetails swapChainSupport = QueryPhysicalDeviceSwapChainSupport(device);
		return !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
	}

	void CreateLogicalVulkanDevice()
	{
		assert(m_graphicsQueueFamilyIndices.ValueReady());
		std::set<uint32_t> queueFamilyIndices = { m_graphicsQueueFamilyIndices.m_graphicsFamilyIndex.value(), m_graphicsQueueFamilyIndices.m_presentFamilyIndex.value() };
		const float queuePriority = 1.0f;
		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		for (uint32_t queueFamilyIndex : queueFamilyIndices)
		{
			VkDeviceQueueCreateInfo queueCreateInfo = {};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = m_graphicsQueueFamilyIndices.m_graphicsFamilyIndex.value();
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;
			queueCreateInfos.push_back(queueCreateInfo);
		}

		VkPhysicalDeviceFeatures deviceFeatures = {}; // populate with stuff from vkGetPhysicalDeviceFeatures(), for now keep it simple
		
		VkDeviceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.pQueueCreateInfos = queueCreateInfos.data();
		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		createInfo.pEnabledFeatures = &deviceFeatures;
		createInfo.enabledExtensionCount = static_cast<uint32_t>(s_requiredPhysicalDeviceExtentions.size());
		createInfo.ppEnabledExtensionNames = s_requiredPhysicalDeviceExtentions.data();

		if (m_useVulkanValidationLayers)
		{
			createInfo.enabledLayerCount = static_cast<uint32_t>(s_validationLayers.size());
			createInfo.ppEnabledLayerNames = s_validationLayers.data();
		}
		else
		{
			createInfo.enabledLayerCount = 0;
		}

		if (vkCreateDevice(m_vulkanPhysicalDevice, &createInfo, nullptr, &m_vulkanLogicalDevice) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create logical vulkan device!");
		}

		vkGetDeviceQueue(m_vulkanLogicalDevice, m_graphicsQueueFamilyIndices.m_graphicsFamilyIndex.value(), 0, &m_graphicsQueue);
		vkGetDeviceQueue(m_vulkanLogicalDevice, m_graphicsQueueFamilyIndices.m_presentFamilyIndex.value(), 0, &m_presentQueue);
		if (m_graphicsQueue == nullptr || m_presentQueue == nullptr)
		{
			throw std::runtime_error("failed to get the graphics or present queue!");
		}		
		else if (m_graphicsQueue == m_presentQueue)
		{
			std::cout << "The Vulkan Graphics queue and the Present queue are the same queue" << std::endl;
		}
	}

	SwapChainSupportDetails QueryPhysicalDeviceSwapChainSupport(VkPhysicalDevice physicalDevice)
	{
		SwapChainSupportDetails details;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, m_surfaceToDrawTo, &details.capabilities);
		uint32_t nSupportedFormats = 0;
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, m_surfaceToDrawTo, &nSupportedFormats, nullptr);
		if (nSupportedFormats != 0)
		{
			details.formats.resize(nSupportedFormats);
			vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, m_surfaceToDrawTo, &nSupportedFormats, details.formats.data());
		}

		uint32_t nSupportedPresentModes = 0;
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, m_surfaceToDrawTo, &nSupportedPresentModes, nullptr);
		if (nSupportedPresentModes != 0)
		{
			details.presentModes.resize(nSupportedPresentModes);
			vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, m_surfaceToDrawTo, &nSupportedPresentModes, details.presentModes.data());
		}

		return details;
	}

	static inline VkSurfaceFormatKHR SelectSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) 
	{
		for (const VkSurfaceFormatKHR& format : availableFormats)
		{
			if (format.format == VK_FORMAT_B8G8R8A8_UNORM && VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			{
				return format;
			}
		}
		return availableFormats[0]; // return first element if can't pick the optimal surface format
	}

	static inline VkPresentModeKHR SelectPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
	{
		for (const VkPresentModeKHR& currentPresentMode: availablePresentModes)
		{
			if (currentPresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
			{
				return currentPresentMode;
			}
		}
		return VK_PRESENT_MODE_FIFO_KHR;
	}

	VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities)
	{
		// reduced due to compile errors
		VkExtent2D extentToUse = { c_windowWidth, c_windowHeight };
		return extentToUse;
	}

	void CreateSwapChain()
	{
		// validation for the swap chain support will have been used before reaching this function
		SwapChainSupportDetails supportedSwapChainDetails = QueryPhysicalDeviceSwapChainSupport(m_vulkanPhysicalDevice);
		VkSurfaceFormatKHR formatToCreateWith = SelectSwapSurfaceFormat(supportedSwapChainDetails.formats);
		VkPresentModeKHR presentModeToCreateWith = SelectPresentMode(supportedSwapChainDetails.presentModes);
		VkExtent2D extent = ChooseSwapExtent(supportedSwapChainDetails.capabilities);
		uint32_t imageCountToCreateWith = supportedSwapChainDetails.capabilities.minImageCount + 1;
		if (supportedSwapChainDetails.capabilities.maxImageCount > 0 && imageCountToCreateWith > supportedSwapChainDetails.capabilities.maxImageCount) {
			imageCountToCreateWith = supportedSwapChainDetails.capabilities.maxImageCount;
		}
		VkSwapchainCreateInfoKHR swapChainCreateInfo = {};
		swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		swapChainCreateInfo.surface = m_surfaceToDrawTo;
		swapChainCreateInfo.minImageCount = imageCountToCreateWith;
		swapChainCreateInfo.imageFormat = formatToCreateWith.format;
		swapChainCreateInfo.imageColorSpace = formatToCreateWith.colorSpace;
		swapChainCreateInfo.imageExtent = extent;
		swapChainCreateInfo.imageArrayLayers = 1;
		swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		QueueFamilyIndices indicesStruct = FindQueueFamilies(m_vulkanPhysicalDevice);
		uint32_t queueIndeices[] = { indicesStruct.m_graphicsFamilyIndex.value(), indicesStruct.m_presentFamilyIndex.value() };

		if (indicesStruct.m_graphicsFamilyIndex != indicesStruct.m_presentFamilyIndex)
		{
			swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			swapChainCreateInfo.queueFamilyIndexCount = 2;
			swapChainCreateInfo.pQueueFamilyIndices = queueIndeices;
		}
		else
		{
			swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			swapChainCreateInfo.queueFamilyIndexCount = 0; // Optional
			swapChainCreateInfo.pQueueFamilyIndices = nullptr; // Optional
		}

		swapChainCreateInfo.preTransform = supportedSwapChainDetails.capabilities.currentTransform;
		swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		swapChainCreateInfo.presentMode = presentModeToCreateWith;
		swapChainCreateInfo.clipped = VK_TRUE;
		swapChainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

#ifdef _WINDOWS
		// _putenv("DISABLE_VK_LAYER_VALVE_steam_overlay_1=1");
#endif // _WINDOWS

		// see: https://vulkan-tutorial.com/FAQ
		if (vkCreateSwapchainKHR(m_vulkanLogicalDevice, &swapChainCreateInfo, nullptr, &m_swapChain) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create swap chain");
		}

		// get additional swap chain info post creation
		uint32_t nSwapChainImagesPostCreation = 0;
		vkGetSwapchainImagesKHR(m_vulkanLogicalDevice, m_swapChain, &nSwapChainImagesPostCreation, nullptr);
		m_swapChainImages.resize(nSwapChainImagesPostCreation);
		vkGetSwapchainImagesKHR(m_vulkanLogicalDevice, m_swapChain, &nSwapChainImagesPostCreation, m_swapChainImages.data());
		m_swapChainImageFormat = formatToCreateWith.format;
		m_swapChainExtent = extent;
	}

	void CreateImageViews()
	{
		const size_t nSwapChainImages = m_swapChainImages.size();
		m_swapChainImageViews.resize(nSwapChainImages);
		for (size_t i = 0; i < nSwapChainImages; ++i)
		{
			VkImageViewCreateInfo imgViewCreateInfo = {};
			imgViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			imgViewCreateInfo.image = m_swapChainImages[i];
			imgViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			imgViewCreateInfo.format = m_swapChainImageFormat;
			imgViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			imgViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			imgViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			imgViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			imgViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imgViewCreateInfo.subresourceRange.baseMipLevel = 0;
			imgViewCreateInfo.subresourceRange.levelCount = 1;
			imgViewCreateInfo.subresourceRange.baseArrayLayer = 0;
			imgViewCreateInfo.subresourceRange.layerCount = 1;

			if (vkCreateImageView(m_vulkanLogicalDevice, &imgViewCreateInfo, nullptr, &m_swapChainImageViews[i]) != VK_SUCCESS)
			{
				throw std::runtime_error("Failed to create an image view");
			}
		}
	}

	void CreateGraphicsPipeline()
	{
		const std::vector<char> vertexShaderCode = ReadShader("Shaders/DefaultVert.spv");
		const std::vector<char> fragmentShaderCode = ReadShader("Shaders/DefaultFrag.spv");
		m_vertexShaderModule = CreateShaderModule(vertexShaderCode);
		m_fragmentShaderModule = CreateShaderModule(fragmentShaderCode);

		VkPipelineShaderStageCreateInfo vertexShaderStageCreateInfo = {};
		vertexShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertexShaderStageCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertexShaderStageCreateInfo.module = m_vertexShaderModule;
		vertexShaderStageCreateInfo.pName = "main";

		VkPipelineShaderStageCreateInfo fragmentShaderStageCreateInfo = {};
		fragmentShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragmentShaderStageCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragmentShaderStageCreateInfo.module = m_fragmentShaderModule;
		fragmentShaderStageCreateInfo.pName = "main";

		VkPipelineShaderStageCreateInfo piplineStagesCreateInfo[] = { vertexShaderStageCreateInfo, fragmentShaderStageCreateInfo };

		VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = {};
		vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputStateCreateInfo.vertexBindingDescriptionCount = 0;
		vertexInputStateCreateInfo.pVertexBindingDescriptions = nullptr;
		vertexInputStateCreateInfo.vertexAttributeDescriptionCount = 0;
		vertexInputStateCreateInfo.pVertexAttributeDescriptions = nullptr;

		VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo = {};
		inputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;
		
		VkViewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(m_swapChainExtent.width);
		viewport.height = static_cast<float>(m_swapChainExtent.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissorRect = {};
		scissorRect.offset = {0, 0};
		scissorRect.extent = m_swapChainExtent;

		VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {};
		viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportStateCreateInfo.viewportCount = 1;
		viewportStateCreateInfo.pViewports = &viewport;
		viewportStateCreateInfo.scissorCount = 1;
		viewportStateCreateInfo.pScissors = &scissorRect;

		VkPipelineRasterizationStateCreateInfo rasterisationStateCreateInfo = {};
		rasterisationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterisationStateCreateInfo.depthClampEnable = VK_FALSE;
		rasterisationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE; // VK_TRUE results on dropping the results before presenting to frame buffer
		rasterisationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL; // as opposed to lines or points
		rasterisationStateCreateInfo.lineWidth = 1.0f;
		rasterisationStateCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterisationStateCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
		rasterisationStateCreateInfo.depthBiasEnable = VK_FALSE;
		rasterisationStateCreateInfo.depthBiasConstantFactor = rasterisationStateCreateInfo.depthBiasClamp = rasterisationStateCreateInfo.depthBiasSlopeFactor = 0.0f;

		VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo = {};
		multisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampleStateCreateInfo.sampleShadingEnable = VK_FALSE;
		multisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisampleStateCreateInfo.minSampleShading = 1.0f;
		multisampleStateCreateInfo.pSampleMask = nullptr;
		multisampleStateCreateInfo.alphaToCoverageEnable = VK_FALSE;
		multisampleStateCreateInfo.alphaToOneEnable = VK_FALSE;

		// add depth buffer state here once got to depth buffering stage of the tutorial

		VkPipelineColorBlendAttachmentState colourBlendAttachmentState = {}; // should be named disabled colour blend attachment state
		colourBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colourBlendAttachmentState.blendEnable = VK_FALSE;
		colourBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
		colourBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
		colourBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
		colourBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colourBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		colourBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;

		VkPipelineColorBlendStateCreateInfo colourBlendStateCreateInfo = {};
		colourBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colourBlendStateCreateInfo.logicOpEnable = VK_FALSE;
		colourBlendStateCreateInfo.logicOp = VK_LOGIC_OP_COPY; // Optional
		colourBlendStateCreateInfo.attachmentCount = 1;
		colourBlendStateCreateInfo.pAttachments = &colourBlendAttachmentState;
		// defaults to black no alpha
		colourBlendStateCreateInfo.blendConstants[0] = 0.0f; // Optional
		colourBlendStateCreateInfo.blendConstants[1] = 0.0f; // Optional
		colourBlendStateCreateInfo.blendConstants[2] = 0.0f; // Optional
		colourBlendStateCreateInfo.blendConstants[3] = 0.0f; // Optional

		VkDynamicState pipelineDynamicStates[] =
		{
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_LINE_WIDTH
		};

		VkPipelineDynamicStateCreateInfo pipelineDynamicStatesCreateInfo = {};
		pipelineDynamicStatesCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		pipelineDynamicStatesCreateInfo.dynamicStateCount = 2;
		pipelineDynamicStatesCreateInfo.pDynamicStates = pipelineDynamicStates;

		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
		pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutCreateInfo.setLayoutCount = 0; // Optional
		pipelineLayoutCreateInfo.pSetLayouts = nullptr; // Optional
		pipelineLayoutCreateInfo.pushConstantRangeCount = 0; // Optional
		pipelineLayoutCreateInfo.pPushConstantRanges = nullptr; // Optional

		if (vkCreatePipelineLayout(m_vulkanLogicalDevice, &pipelineLayoutCreateInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create pipeline layout!");
		}

		VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
		pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineCreateInfo.stageCount = 2;
		pipelineCreateInfo.pStages = piplineStagesCreateInfo;
		pipelineCreateInfo.pVertexInputState = &vertexInputStateCreateInfo;
		pipelineCreateInfo.pInputAssemblyState = &inputAssemblyStateCreateInfo;
		pipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
		pipelineCreateInfo.pRasterizationState = &rasterisationStateCreateInfo;
		pipelineCreateInfo.pMultisampleState = &multisampleStateCreateInfo;
		pipelineCreateInfo.pColorBlendState = &colourBlendStateCreateInfo;
		pipelineCreateInfo.layout = m_pipelineLayout;
		pipelineCreateInfo.renderPass = m_renderPass;
		pipelineCreateInfo.subpass = 0;
		pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
		pipelineCreateInfo.basePipelineIndex = -1;

		if (vkCreateGraphicsPipelines(m_vulkanLogicalDevice, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &m_pipeline) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create graphics pipeline.");
		}
	}

	void CreateRenderPass()
	{
		VkAttachmentDescription colourAttachment = {};
		colourAttachment.format = m_swapChainImageFormat;
		colourAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colourAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colourAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colourAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colourAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colourAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colourAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference colourAttachmentRef = {};
		colourAttachmentRef.attachment = 0;
		colourAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colourAttachmentRef;

		// VkRenderPass m_renderPass;
		VkRenderPassCreateInfo renderPassCreateInfo = {};
		renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassCreateInfo.attachmentCount = 1;
		renderPassCreateInfo.pAttachments = &colourAttachment;
		renderPassCreateInfo.subpassCount = 1;
		renderPassCreateInfo.pSubpasses = &subpass;

		if (vkCreateRenderPass(m_vulkanLogicalDevice, &renderPassCreateInfo, nullptr, &m_renderPass) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create the render pass");
		}
	}

	static std::vector<char> ReadShader(const std::string& shaderFilePath)
	{
		std::ifstream shaderFile(shaderFilePath, std::ios::binary | std::ios::ate); // opens file in binary mode at end of file
		if (!shaderFile.is_open())
		{
			throw std::runtime_error("Failed to open " + shaderFilePath);
		}
		const size_t shaderFileSize = shaderFile.tellg();
		shaderFile.seekg(0);
		std::vector<char> shaderCode(shaderFileSize);
		shaderFile.read(shaderCode.data(), shaderFileSize);
		shaderFile.close();
		return shaderCode;
	}
	
	VkShaderModule CreateShaderModule(const std::vector<char>& shaderCode)
	{
		VkShaderModuleCreateInfo moduleCreateInfo = {};
		moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		moduleCreateInfo.codeSize = shaderCode.size();
		moduleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(shaderCode.data());
		VkShaderModule resultingModule = nullptr;
		if (vkCreateShaderModule(m_vulkanLogicalDevice, &moduleCreateInfo, nullptr, &resultingModule) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create shader module");
		}
		return resultingModule;
	}

	void CreateFrameBuffers()
	{
		const size_t nImagesInSwapChainViews = m_swapChainImageViews.size();
		assert(nImagesInSwapChainViews > 0);

		m_swapChainFrameBuffers.resize(nImagesInSwapChainViews);
		for (size_t i = 0; i < nImagesInSwapChainViews; ++i)
		{
			VkFramebufferCreateInfo framebufferCreateInfo = {};
			framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferCreateInfo.renderPass = m_renderPass;
			framebufferCreateInfo.attachmentCount = 1;
			framebufferCreateInfo.pAttachments = &m_swapChainImageViews[i];
			framebufferCreateInfo.width = m_swapChainExtent.width;
			framebufferCreateInfo.height = m_swapChainExtent.height;
			framebufferCreateInfo.layers = 1;

			if (vkCreateFramebuffer(m_vulkanLogicalDevice, &framebufferCreateInfo, nullptr, &m_swapChainFrameBuffers[i]) != VK_SUCCESS)
			{
				throw std::runtime_error("Failed to create frame buffer");
			}
		}
	}

	void CreateCommandPool()
	{
		QueueFamilyIndices queueFamilyIndices = FindQueueFamilies(m_vulkanPhysicalDevice);
		VkCommandPoolCreateInfo cmdPoolCreateInfo = {};
		cmdPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		cmdPoolCreateInfo.queueFamilyIndex = queueFamilyIndices.m_graphicsFamilyIndex.value();
		cmdPoolCreateInfo.flags = 0;

		if (vkCreateCommandPool(m_vulkanLogicalDevice, &cmdPoolCreateInfo, nullptr, &m_commandPool))
		{
			throw std::runtime_error("Failed to create command queue");
		}
	}

	void CreateCommandBuffers()
	{
		const size_t nFrameBuffers = m_swapChainFrameBuffers.size();
		m_commandBuffers.resize(nFrameBuffers);
		VkCommandBufferAllocateInfo cmdBuffersAllocInfo = {};
		cmdBuffersAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmdBuffersAllocInfo.commandPool = m_commandPool;
		cmdBuffersAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		cmdBuffersAllocInfo.commandBufferCount = static_cast<uint32_t>(nFrameBuffers);

		if (vkAllocateCommandBuffers(m_vulkanLogicalDevice, &cmdBuffersAllocInfo, m_commandBuffers.data()))
		{
			throw std::runtime_error("Failed to allocate Vulkan Command buffers");
		}


		// check if this is in the correct place.... sound like something that should be in a Draw() function
		for (size_t i = 0; i < nFrameBuffers; ++i)
		{
			VkCommandBufferBeginInfo cmdBuffBeginInfo = {};
			cmdBuffBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			cmdBuffBeginInfo.flags = 0;
			cmdBuffBeginInfo.pInheritanceInfo = nullptr;

			if (vkBeginCommandBuffer(m_commandBuffers[i], &cmdBuffBeginInfo) != VK_SUCCESS)
			{
				throw std::runtime_error("Failed the start recording a command buffer!");
			}

			
			VkRenderPassBeginInfo renderPassBeginInfo = {};
			renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassBeginInfo.framebuffer = m_swapChainFrameBuffers[i];
			renderPassBeginInfo.renderPass = m_renderPass;
			renderPassBeginInfo.renderArea.offset = { 0, 0 };
			renderPassBeginInfo.renderArea.extent = m_swapChainExtent;
			VkClearValue clearColour = { 0.0f, 0.0f, 0.0f, 1.0f}; // RGBA?
			renderPassBeginInfo.pClearValues = &clearColour;
			renderPassBeginInfo.clearValueCount = 1;
			vkCmdBeginRenderPass(m_commandBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
			// start draw commands
			vkCmdBindPipeline(m_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
			vkCmdDraw(m_commandBuffers[i], 3, 1, 0, 0);
			// end draw commands
			vkCmdEndRenderPass(m_commandBuffers[i]);
			if (vkEndCommandBuffer(m_commandBuffers[i]) != VK_SUCCESS)
			{
				throw std::runtime_error("Failed to finish recording commands to buffer");
			}
		}
	}

	void CreateSemaphores()
	{
		VkSemaphoreCreateInfo semaphoneCreateInfo = {};
		semaphoneCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		if (vkCreateSemaphore(m_vulkanLogicalDevice, &semaphoneCreateInfo, nullptr, &m_imageReadyToDrawToSemaphore) != VK_SUCCESS || vkCreateSemaphore(m_vulkanLogicalDevice, &semaphoneCreateInfo, nullptr, &m_finishedDrawingSemaphore) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to CreateSemaphores.");
		}
	}

	void MainLoop()
	{
		while (!glfwWindowShouldClose(m_window))
		{
			glfwPollEvents();
		}
	}

	void Shutdown()
	{
		if (m_imageReadyToDrawToSemaphore || m_finishedDrawingSemaphore)
		{
			vkDestroySemaphore(m_vulkanLogicalDevice, m_imageReadyToDrawToSemaphore, nullptr);
			vkDestroySemaphore(m_vulkanLogicalDevice, m_finishedDrawingSemaphore, nullptr);
		}
		if (m_commandBuffers.size() > 0)
		{
			for (VkCommandBuffer& cmdBuffer : m_commandBuffers)
			{
				// vkDestroyCommandBuff?
				// tutorial says they don't need clean up, check when refactoring
			}
		}
		if (m_commandPool)
		{
			vkDestroyCommandPool(m_vulkanLogicalDevice, m_commandPool, nullptr);
		}

		if (m_swapChainFrameBuffers.size() > 0)
		{
			for (const VkFramebuffer& fb : m_swapChainFrameBuffers)
			{
				vkDestroyFramebuffer(m_vulkanLogicalDevice, fb, nullptr);
			}
		}
		if (m_swapChainImageViews.size() > 0)
		{
			for (const VkImageView& imgView : m_swapChainImageViews)
			{
				vkDestroyImageView(m_vulkanLogicalDevice, imgView, nullptr);
			}
			m_swapChainImageViews.clear();
		}
		/* these get deleted when the swap chain are deleted
		if (m_swapChainImages.size() > 0)
		{
			for (const VkImage& img : m_swapChainImages)
			{
				vkDestroyImage(m_vulkanLogicalDevice, img, nullptr);
			}
			m_swapChainImages.clear();
		}*/
		if (m_pipeline)
		{
			vkDestroyPipeline(m_vulkanLogicalDevice, m_pipeline, nullptr);
		}
		if (m_pipelineLayout)
		{
			vkDestroyPipelineLayout(m_vulkanLogicalDevice, m_pipelineLayout, nullptr);
		}
		if (m_renderPass)
		{
			vkDestroyRenderPass(m_vulkanLogicalDevice, m_renderPass, nullptr);
		}
		if (m_vertexShaderModule)
		{
			vkDestroyShaderModule(m_vulkanLogicalDevice, m_vertexShaderModule, nullptr);
		}
		if (m_fragmentShaderModule)
		{
			vkDestroyShaderModule(m_vulkanLogicalDevice, m_fragmentShaderModule, nullptr);
		}

		if (m_useVulkanValidationLayers)
		{
			DestroyDebugUtilsMessengerEXT(m_vulkanInstance, m_vulkanDebugMessenger, nullptr);
		}
		if (m_swapChain)
		{
			vkDestroySwapchainKHR(m_vulkanLogicalDevice, m_swapChain, nullptr);
		}
		if (m_vulkanLogicalDevice)
		{
			vkDestroyDevice(m_vulkanLogicalDevice, nullptr); // note that this also deletes the graphics queue
		}
		if (m_surfaceToDrawTo)
		{
			vkDestroySurfaceKHR(m_vulkanInstance, m_surfaceToDrawTo, nullptr);
		}
		vkDestroyInstance(m_vulkanInstance, nullptr);
		glfwDestroyWindow(m_window);
		glfwTerminate();
		m_window = nullptr;
	}

	// Debug functions
	VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
		auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
		if (func != nullptr) {
			return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
		}
		else {
			return VK_ERROR_EXTENSION_NOT_PRESENT;
		}
	}

	static VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) 
	{
		if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
		{
			// std::cout << ?
		}
	    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

	    return VK_FALSE;
	}

	void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) 
	{
		auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
		if (func != nullptr)
		{
			func(instance, debugMessenger, pAllocator);
		}
	}
	// end of debug functions

	const uint32_t c_windowWidth;
	const uint32_t c_windowHeight;
	GLFWwindow* m_window;
	VkInstance m_vulkanInstance;
	VkPhysicalDevice m_vulkanPhysicalDevice; //note that this gets deleted when destroying m_vulkanInstance
	VkDevice m_vulkanLogicalDevice;
	VkQueue m_graphicsQueue;
	QueueFamilyIndices m_graphicsQueueFamilyIndices;
	VkDebugUtilsMessengerEXT m_vulkanDebugMessenger;
	const bool m_useVulkanValidationLayers;

	// window surface creation variables
	VkSurfaceKHR m_surfaceToDrawTo;
	VkQueue m_presentQueue;
	
	// swap chain variables, note need the enable to extensions
	VkSwapchainKHR m_swapChain;
	std::vector<VkImage> m_swapChainImages;
	VkFormat m_swapChainImageFormat;
	VkExtent2D m_swapChainExtent;
	std::vector<VkImageView> m_swapChainImageViews;

	VkShaderModule m_vertexShaderModule;
	VkShaderModule m_fragmentShaderModule;

	VkPipeline m_pipeline;
	VkPipelineLayout m_pipelineLayout;
	VkRenderPass m_renderPass;
	std::vector<VkFramebuffer> m_swapChainFrameBuffers;

	// use these to "send drawing commands"
	VkCommandPool m_commandPool;
	std::vector<VkCommandBuffer> m_commandBuffers;

	VkSemaphore m_imageReadyToDrawToSemaphore;
	VkSemaphore m_finishedDrawingSemaphore;
};


int main()
{

	VulkanApp vkApp;

	vkApp.Run();

	return 0;
}
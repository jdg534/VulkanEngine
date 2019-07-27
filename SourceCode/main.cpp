#include <optional>
#include <iostream>
#include <string> // needed for checking validation layers
#include <vector>
#include <cassert>
#include <exception>

#include <glm/glm.hpp>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#ifdef _WINDOWS
#include <Windows.h>
#endif // _WINDOWS


static const std::vector<const char*> s_validationLayers = { "VK_LAYER_KHRONOS_validation" }; // following tutorial structure, refactor once we're got a triangle on screen
// VK_LAYER_LUNARG_standard_validation found, check if that can be used as an alternative. could need to update drivers
class VulkanApp
{
public:
	VulkanApp()
		: m_window(nullptr)
		, m_vulkanInstance(nullptr)
		, m_vulkanPhysicalDevice(nullptr)
		, m_vulkanLogicalDevice(nullptr)
		, m_graphicsQueue(nullptr)
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

		bool ValueReady()
		{
			return m_graphicsFamilyIndex.has_value();
		}
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
		m_window = glfwCreateWindow(800, 600, "Vulkan window", nullptr, nullptr);
	}
	void InitVulkan()
	{
		CreateVulkanInstance();
		QueryVulkanExtentions();
		SelectVulkanDevice();
		CreateLogicalVulkanDevice();
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

		CreateVulkanInstance();
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
			if (currentQueueFamilyProperties.queueCount > 0 && currentQueueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				indices.m_graphicsFamilyIndex = i;
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

		return suitabilityScore;
	}

	void CreateLogicalVulkanDevice()
	{
		assert(m_graphicsQueueFamilyIndices.ValueReady());
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = m_graphicsQueueFamilyIndices.m_graphicsFamilyIndex.value();
		queueCreateInfo.queueCount = 1;

		float queuePriority = 1.0f;
		queueCreateInfo.pQueuePriorities = &queuePriority;

		VkPhysicalDeviceFeatures deviceFeatures = {}; // populate with stuff from vkGetPhysicalDeviceFeatures(), for now keep it simple
		
		VkDeviceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.pQueueCreateInfos = &queueCreateInfo;
		createInfo.queueCreateInfoCount = 1;
		createInfo.pEnabledFeatures = &deviceFeatures;
		createInfo.enabledExtensionCount = 0;

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
		if (m_graphicsQueue == nullptr)
		{
			throw std::runtime_error("failed to get the graphics queue!");
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
		if (m_useVulkanValidationLayers)
		{
			DestroyDebugUtilsMessengerEXT(m_vulkanInstance, m_vulkanDebugMessenger, nullptr);
		}
		if (m_vulkanLogicalDevice)
		{
			vkDestroyDevice(m_vulkanLogicalDevice, nullptr); // note that this also deletes the graphics queue
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

	GLFWwindow* m_window;
	VkInstance m_vulkanInstance;
	VkPhysicalDevice m_vulkanPhysicalDevice; //note that this gets deleted when destroying m_vulkanInstance
	VkDevice m_vulkanLogicalDevice;
	VkQueue m_graphicsQueue;
	QueueFamilyIndices m_graphicsQueueFamilyIndices;
	VkDebugUtilsMessengerEXT m_vulkanDebugMessenger;
	const bool m_useVulkanValidationLayers;
};


int main()
{

	VulkanApp vkApp;

	vkApp.Run();

	return 0;
}
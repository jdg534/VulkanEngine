#include <iostream>
#include <string> // needed for checking validation layers
#include <vector>
#include <cassert>
#include <exception>

#include <glm/glm.hpp>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

static const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" }; // following tutorial structure, refactor once we're got a triangle on screen
// VK_LAYER_LUNARG_standard_validation found, check if that can be used as an alternative. could need to update drivers
class VulkanApp
{
public:
	VulkanApp()
		: m_window(nullptr)
		, m_vulkanInstance(nullptr)
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
	void Init()
	{
		try
		{
			InitWindow();
			InitVulkan();
		}
		catch (const std::exception& ex)
		{
			// if WIN32 OutputDebugString(), figure out Unix like equivelent

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
	}

	bool AreVulkanValidationLayersSupported()
	{
		uint32_t nLayers = 0;
		vkEnumerateInstanceLayerProperties(&nLayers, nullptr);

		std::vector<VkLayerProperties> availableLayers(nLayers);
		vkEnumerateInstanceLayerProperties(&nLayers, availableLayers.data());

		const std::string targetLayerStr = std::string(validationLayers[0]);
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

		if (m_useVulkanValidationLayers)
		{
			
			instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			instanceCreateInfo.ppEnabledLayerNames = validationLayers.data();
		}
		else
		{
			instanceCreateInfo.enabledLayerCount = 0;
		}
		 

		

		VkResult instanceCreateRes = vkCreateInstance(&instanceCreateInfo, nullptr, &m_vulkanInstance); // the nullptr would be for an allocator callback function.
		if (instanceCreateRes != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create Vulkan instance");
		}
		std::cout << "Vulkan Instance created successfully" << std::endl;
	}

	void SetupVulkanDebugMessenger()
	{
		if (!m_useVulkanValidationLayers)
			return;

		VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback = VulkanDebugCallback;
		createInfo.pUserData = nullptr; // Optional

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
	VkDebugUtilsMessengerEXT m_vulkanDebugMessenger;
	const bool m_useVulkanValidationLayers;
};


int main()
{

	VulkanApp vkApp;

	vkApp.Run();

	return 0;
}
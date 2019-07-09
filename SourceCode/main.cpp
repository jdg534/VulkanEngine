#include <iostream>
#include <vector>
#include <cassert>
#include <exception>

#include <glm/glm.hpp>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class VulkanApp
{
public:
	VulkanApp()
		: m_window(nullptr)
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

	void CreateVulkanInstance()
	{
		VkApplicationInfo appInfo = {};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Vulkan Triangle";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "Learning Vulkan Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;

		VkInstanceCreateInfo instanceCreateInfo = {};
		instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instanceCreateInfo.pApplicationInfo = &appInfo; // tutorial used stack memory, should be ok
		
		uint32_t glfwRequiredExtCount = 0;
		const char** glfwExtCStrs;
		glfwExtCStrs = glfwGetRequiredInstanceExtensions(&glfwRequiredExtCount);
		instanceCreateInfo.enabledExtensionCount = glfwRequiredExtCount;
		instanceCreateInfo.ppEnabledExtensionNames = glfwExtCStrs;
		instanceCreateInfo.enabledLayerCount = 0;

		VkResult instanceCreateRes = vkCreateInstance(&instanceCreateInfo, nullptr, &m_vulkanInstance); // the nullptr would be for an allocator callback function.
		if (instanceCreateRes != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create Vulkan instance");
		}
		std::cout << "Vulkan Instance created successfully" << std::endl;
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
		vkDestroyInstance(m_vulkanInstance, nullptr);
		glfwDestroyWindow(m_window);
		glfwTerminate();
		m_window = nullptr;
	}

	GLFWwindow* m_window;
	VkInstance m_vulkanInstance;
};


int main()
{

	VulkanApp vkApp;

	vkApp.Run();

	return 0;
}
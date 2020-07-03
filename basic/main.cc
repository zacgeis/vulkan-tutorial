#include "vulkan/vulkan_core.h"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cstdlib>
#include <iostream>
#include <optional>
#include <set>
#include <stdexcept>
#include <vector>

constexpr uint32_t kWidth = 800;
constexpr uint32_t kHeight = 600;

const std::vector<const char *> kValidationLayers = {
    "VK_LAYER_KHRONOS_validation"};

constexpr bool kEnableValidationLayers = true;

static VKAPI_ATTR VkBool32 VKAPI_CALL
DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
              VkDebugUtilsMessageTypeFlagsEXT messageType,
              const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
              void *pUserData) {

  std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

  return VK_FALSE;
}

struct QueueFamilyIndices {
  std::optional<uint32_t> graphics_family;
  std::optional<uint32_t> present_family;
};

bool IsQueueFamilyComplete(const QueueFamilyIndices &indices) {
  return indices.graphics_family.has_value() &&
         indices.present_family.has_value();
}

class HelloTriangleApplication {
public:
  void Run() {
    InitWindow();
    InitVulkan();
    MainLoop();
    Cleanup();
  }

private:
  void InitWindow() {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window_ = glfwCreateWindow(kWidth, kHeight, "Vulkan", nullptr, nullptr);
  }

  bool CheckValidationLayerSupport() {
    uint32_t layer_count = 0;
    vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

    std::vector<VkLayerProperties> available_layers(layer_count);
    vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

    for (const auto *validation_layer : kValidationLayers) {
      bool found = false;
      for (const auto &available_layer : available_layers) {
        if (strcmp(validation_layer, available_layer.layerName) == 0) {
          found = true;
          break;
        }
      }
      if (!found) {
        return false;
      }
    }
    return true;
  }

  std::vector<const char *> GetRequiredExtensions() {
    uint32_t glfw_extension_count = 0;
    const char **glfw_extensions;
    glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

    std::vector<const char *> extensions(
        glfw_extensions, glfw_extensions + glfw_extension_count);

    if (kEnableValidationLayers) {
      extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
  }

  void CreateInstance() {
    if (kEnableValidationLayers && !CheckValidationLayerSupport()) {
      throw std::runtime_error(
          "Validation layers requested, but are not available.");
    }

    VkApplicationInfo app_info;
    app_info.pNext = nullptr;
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "Hello Triangle";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "No Engine";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo create_info;
    create_info.pNext = nullptr;
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;

    const auto glfw_extensions = GetRequiredExtensions();
    create_info.enabledExtensionCount =
        static_cast<uint32_t>(glfw_extensions.size());
    create_info.ppEnabledExtensionNames = glfw_extensions.data();

    VkDebugUtilsMessengerCreateInfoEXT debug_create_info;
    if (kEnableValidationLayers) {
      create_info.enabledLayerCount =
          static_cast<uint32_t>(kValidationLayers.size());
      create_info.ppEnabledLayerNames = kValidationLayers.data();

      PopulateDebugMessengerCreateInfo(debug_create_info);
      create_info.pNext = &debug_create_info;
    } else {
      create_info.enabledLayerCount = 0;
    }

    VkResult result = vkCreateInstance(&create_info, nullptr, &instance_);
    if (result != VK_SUCCESS) {
      throw std::runtime_error("Failed to create instance.");
    }

    uint32_t extension_count = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);
    std::vector<VkExtensionProperties> extensions(extension_count);
    vkEnumerateInstanceExtensionProperties(nullptr, &extension_count,
                                           extensions.data());

    std::cout << "Available Extensions: " << std::endl;
    for (const auto &extension : extensions) {
      std::cout << "  " << extension.extensionName << std::endl;
    }

    for (const auto *required_extension : glfw_extensions) {
      bool found = false;
      for (const auto &extension : extensions) {
        if (required_extension == extension.extensionName) {
          found = true;
          break;
        }
      }
      if (!found) {
        std::string extension_str(required_extension);
        std::runtime_error("GLFW required extension is not supported: " +
                           extension_str);
      }
    }
    std::cout << "All required extensions are available." << std::endl;
  }

  QueueFamilyIndices FindQueueFamilies(const VkPhysicalDevice &device) {
    QueueFamilyIndices indices;

    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count,
                                             nullptr);
    std::vector<VkQueueFamilyProperties> queue_familes(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count,
                                             queue_familes.data());

    uint32_t i = 0;
    for (const auto &queue_family : queue_familes) {
      if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
        indices.graphics_family = i;
      }

      VkBool32 present_support = false;
      vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface_,
                                           &present_support);
      if (present_support) {
        indices.present_family = i;
      }

      if (IsQueueFamilyComplete(indices)) {
        break;
      }

      i++;
    }

    return indices;
  }

  bool IsDeviceSuitable(const VkPhysicalDevice &device) {
    const auto queue_families = FindQueueFamilies(device);

    return IsQueueFamilyComplete(queue_families);
  }

  void PickPhysicalDevice() {
    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(instance_, &device_count, nullptr);
    if (device_count == 0) {
      throw std::runtime_error("Failed to find GPU with Vulkan support.");
    }
    std::vector<VkPhysicalDevice> devices(device_count);
    vkEnumeratePhysicalDevices(instance_, &device_count, devices.data());
    for (const auto &device : devices) {
      if (IsDeviceSuitable(device)) {
        physical_device_ = device;
        break;
      }
    }
    if (physical_device_ == VK_NULL_HANDLE) {
      throw std::runtime_error("Failed to find suitable GPU.");
    }
  }

  void CreateLogicalDevice() {
    QueueFamilyIndices indices = FindQueueFamilies(physical_device_);

    std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
    std::set<uint32_t> unique_queue_families = {indices.graphics_family.value(),
                                                indices.present_family.value()};

    float queue_priority = 1.0f;
    for (uint32_t queue_family : unique_queue_families) {
      VkDeviceQueueCreateInfo queue_create_info{};
      queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
      queue_create_info.queueFamilyIndex = queue_family;
      queue_create_info.queueCount = 1;
      queue_create_info.pQueuePriorities = &queue_priority;
      queue_create_infos.push_back(queue_create_info);
    }

    VkPhysicalDeviceFeatures device_features{};
    VkDeviceCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    create_info.queueCreateInfoCount =
        static_cast<uint32_t>(queue_create_infos.size());
    create_info.pQueueCreateInfos = queue_create_infos.data();
    create_info.pEnabledFeatures = &device_features;

    if (vkCreateDevice(physical_device_, &create_info, nullptr, &device_) !=
        VK_SUCCESS) {
      throw std::runtime_error("Failed to create logical device.");
    }
    vkGetDeviceQueue(device_, *indices.graphics_family, 0, &graphics_queue_);
    vkGetDeviceQueue(device_, *indices.present_family, 0, &present_queue_);
  }

  void CreateSurface() {
    if (glfwCreateWindowSurface(instance_, window_, nullptr, &surface_) !=
        VK_SUCCESS) {
      throw std::runtime_error("Failed to create window surface.");
    }
  }

  void InitVulkan() {
    CreateInstance();
    SetupDebugMessenger();
    CreateSurface();
    PickPhysicalDevice();
    CreateLogicalDevice();
  }

  void MainLoop() {
    while (!glfwWindowShouldClose(window_)) {
      glfwPollEvents();
    }
  }

  void PopulateDebugMessengerCreateInfo(
      VkDebugUtilsMessengerCreateInfoEXT &create_info) {
    create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    create_info.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                              VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                              VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    create_info.pfnUserCallback = DebugCallback;
    create_info.pUserData = nullptr; // Optional
  }

  void SetupDebugMessenger() {
    if (!kEnableValidationLayers)
      return;
    VkDebugUtilsMessengerCreateInfoEXT create_info;
    PopulateDebugMessengerCreateInfo(create_info);
    if (CreateDebugUtilsMessengerEXT(instance_, &create_info, nullptr,
                                     &debug_messenger_) != VK_SUCCESS) {
      throw std::runtime_error("Failed to setup debug messenger.");
    }
  }

  VkResult CreateDebugUtilsMessengerEXT(
      VkInstance instance,
      const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
      const VkAllocationCallbacks *pAllocator,
      VkDebugUtilsMessengerEXT *pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
        instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
      return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
      return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
  }

  void DestroyDebugUtilsMessengerEXT(VkInstance instance,
                                     VkDebugUtilsMessengerEXT debugMessenger,
                                     const VkAllocationCallbacks *pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
        instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
      func(instance, debugMessenger, pAllocator);
    }
  }

  void Cleanup() {
    if (kEnableValidationLayers) {
      DestroyDebugUtilsMessengerEXT(instance_, debug_messenger_, nullptr);
    }
    vkDestroyDevice(device_, nullptr);

    vkDestroySurfaceKHR(instance_, surface_, nullptr);
    vkDestroyInstance(instance_, nullptr);

    glfwDestroyWindow(window_);

    glfwTerminate();
  }

  VkQueue present_queue_;
  VkQueue graphics_queue_;
  VkDevice device_;
  VkPhysicalDevice physical_device_ = VK_NULL_HANDLE;
  VkInstance instance_;
  VkDebugUtilsMessengerEXT debug_messenger_;
  VkSurfaceKHR surface_;
  GLFWwindow *window_;
};

int main() {
  HelloTriangleApplication app;

  try {
    app.Run();
  } catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

#include "vulkan/vulkan_core.h"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cstdlib>
#include <iostream>
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

  void InitVulkan() {
    CreateInstance();
    SetupDebugMessenger();
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

    vkDestroyInstance(instance_, nullptr);

    glfwDestroyWindow(window_);

    glfwTerminate();
  }

  VkInstance instance_;
  VkDebugUtilsMessengerEXT debug_messenger_;
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

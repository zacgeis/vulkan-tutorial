#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>
#include <iostream>
#include <stdexcept>
#include <cstdlib>

constexpr uint32_t kWidth = 800;
constexpr uint32_t kHeight = 600;

class HelloTriangleApplication {
  public:
    void run() {
      initWindow();
      initVulkan();
      mainLoop();
      cleanup();
    }

  private:
    void initWindow() {
      glfwInit();

      glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
      glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

      window_ = glfwCreateWindow(kWidth, kHeight, "Vulkan", nullptr, nullptr);
    }

    void createInstance() {
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

      uint32_t glfw_extension_count = 0;
      const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

      create_info.enabledExtensionCount = glfw_extension_count;
      create_info.ppEnabledExtensionNames = glfw_extensions;

      create_info.enabledLayerCount = 0;

      VkResult result = vkCreateInstance(&create_info, nullptr, &instance_);
      if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to create instance.");
      }

      uint32_t extension_count = 0;
      vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);
      std::vector<VkExtensionProperties> extensions(extension_count);
      vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, extensions.data());

      std::cout << "Available Extensions: "  << std::endl;
      for (const auto& extension : extensions) {
        std::cout << "  " << extension.extensionName << std::endl;
      }

      for (size_t i = 0; i < glfw_extension_count; i++) {
        bool found = false;
        for (const auto& extension : extensions) {
          if (glfw_extensions[i] == extension.extensionName) {
            found = true;
            break;
          }
        }
        if (!found) {
          std::string extension_str(glfw_extensions[i]);
          std::runtime_error("GLFW required extension is not supported: " + extension_str);
        }
      }
      std::cout << "All required extensions are avilable." << std::endl;
    }

    void initVulkan() {
      createInstance();
    }

    void mainLoop() {
      while (!glfwWindowShouldClose(window_)) {
        glfwPollEvents();
      }
    }

    void cleanup() {
      vkDestroyInstance(instance_, nullptr);

      glfwDestroyWindow(window_);

      glfwTerminate();
    }

    VkInstance instance_;
    GLFWwindow* window_;
};

int main() {
  HelloTriangleApplication app;

  try {
    app.run();
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

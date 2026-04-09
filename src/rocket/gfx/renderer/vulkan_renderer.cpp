#include "rocket/renderer.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <limits>
#include <numbers>
#include <optional>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#ifdef ROCKETGE__Platform_Windows
#include <windows.h>
#endif

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include "binary_stuff/splash_screen.h"
#include "internal_types.hpp"
#include "intl_macros.hpp"
#include "lib/stb/stb_image.h"
#include "lib/stb/stb_truetype.h"
#include "lib/tweeny/tweeny.h"
#include "plugin.hpp"
#include "rocket/macros.hpp"
#include "rocket/plugin/plugin.hpp"
#include "rocket/runtime.hpp"
#include "shader_provider.hpp"
#include "util.hpp"

namespace {
    constexpr std::size_t rge_vk_max_frames_in_flight = 2;

    struct rge_vk_vertex_t {
        float pos[2];
        float uv[2];
    };

    struct rge_vk_pipeline_t {
        VkPipelineLayout layout = VK_NULL_HANDLE;
        VkPipeline pipeline = VK_NULL_HANDLE;
        VkShaderModule vertex_module = VK_NULL_HANDLE;
        VkShaderModule fragment_module = VK_NULL_HANDLE;
    };

    struct rge_vk_shader_pipeline_t {
        rge_vk_pipeline_t pipeline;
        bool ready = false;
    };

    struct rge_vk_sync_frame_t {
        VkSemaphore image_available = VK_NULL_HANDLE;
        VkSemaphore render_finished = VK_NULL_HANDLE;
        VkFence in_flight = VK_NULL_HANDLE;
        VkCommandBuffer command_buffer = VK_NULL_HANDLE;
    };

    struct rge_vk_swapchain_support_t {
        VkSurfaceCapabilitiesKHR capabilities {};
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> present_modes;
    };

    struct rge_vk_native_state_t {
        VkInstance instance = VK_NULL_HANDLE;
        VkSurfaceKHR surface = VK_NULL_HANDLE;
        VkPhysicalDevice physical_device = VK_NULL_HANDLE;
        VkDevice device = VK_NULL_HANDLE;
        uint32_t graphics_family = std::numeric_limits<uint32_t>::max();
        uint32_t present_family = std::numeric_limits<uint32_t>::max();
        VkQueue graphics_queue = VK_NULL_HANDLE;
        VkQueue present_queue = VK_NULL_HANDLE;

        VkSwapchainKHR swapchain = VK_NULL_HANDLE;
        VkFormat swapchain_format = VK_FORMAT_B8G8R8A8_UNORM;
        VkExtent2D swapchain_extent {};
        std::vector<VkImage> swapchain_images;
        std::vector<VkImageView> swapchain_image_views;
        std::vector<VkFramebuffer> swapchain_framebuffers;

        VkRenderPass render_pass = VK_NULL_HANDLE;

        VkCommandPool command_pool = VK_NULL_HANDLE;
        std::array<rge_vk_sync_frame_t, rge_vk_max_frames_in_flight> frames {};
        std::vector<VkFence> images_in_flight;
        std::size_t current_frame = 0;

        VkBuffer fullscreen_vertex_buffer = VK_NULL_HANDLE;
        VkDeviceMemory fullscreen_vertex_memory = VK_NULL_HANDLE;

        VkBuffer upload_buffer = VK_NULL_HANDLE;
        VkDeviceMemory upload_memory = VK_NULL_HANDLE;
        std::size_t upload_buffer_size = 0;

        VkImage overlay_image = VK_NULL_HANDLE;
        VkDeviceMemory overlay_memory = VK_NULL_HANDLE;
        VkImageView overlay_view = VK_NULL_HANDLE;
        VkSampler overlay_sampler = VK_NULL_HANDLE;
        VkExtent2D overlay_extent {};
        VkImageLayout overlay_layout = VK_IMAGE_LAYOUT_UNDEFINED;

        VkDescriptorSetLayout overlay_set_layout = VK_NULL_HANDLE;
        VkDescriptorPool overlay_descriptor_pool = VK_NULL_HANDLE;
        VkDescriptorSet overlay_descriptor_set = VK_NULL_HANDLE;

        rge_vk_pipeline_t overlay_pipeline;

        std::vector<rocket::rgba_color> framebuffer;
        std::vector<rocket::api_object_t> queued_shaders;

        bool swapchain_needs_rebuild = false;
        bool overlay_needs_rebuild = false;
        std::optional<rocket::fbounding_box> scissor_rect;
    };

    [[nodiscard]] static std::string vk_result_to_string(VkResult result) {
        switch (result) {
            case VK_SUCCESS: return "VK_SUCCESS";
            case VK_NOT_READY: return "VK_NOT_READY";
            case VK_TIMEOUT: return "VK_TIMEOUT";
            case VK_EVENT_SET: return "VK_EVENT_SET";
            case VK_EVENT_RESET: return "VK_EVENT_RESET";
            case VK_INCOMPLETE: return "VK_INCOMPLETE";
            case VK_ERROR_OUT_OF_HOST_MEMORY: return "VK_ERROR_OUT_OF_HOST_MEMORY";
            case VK_ERROR_OUT_OF_DEVICE_MEMORY: return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
            case VK_ERROR_INITIALIZATION_FAILED: return "VK_ERROR_INITIALIZATION_FAILED";
            case VK_ERROR_DEVICE_LOST: return "VK_ERROR_DEVICE_LOST";
            case VK_ERROR_MEMORY_MAP_FAILED: return "VK_ERROR_MEMORY_MAP_FAILED";
            case VK_ERROR_LAYER_NOT_PRESENT: return "VK_ERROR_LAYER_NOT_PRESENT";
            case VK_ERROR_EXTENSION_NOT_PRESENT: return "VK_ERROR_EXTENSION_NOT_PRESENT";
            case VK_ERROR_FEATURE_NOT_PRESENT: return "VK_ERROR_FEATURE_NOT_PRESENT";
            case VK_ERROR_INCOMPATIBLE_DRIVER: return "VK_ERROR_INCOMPATIBLE_DRIVER";
            case VK_ERROR_TOO_MANY_OBJECTS: return "VK_ERROR_TOO_MANY_OBJECTS";
            case VK_ERROR_FORMAT_NOT_SUPPORTED: return "VK_ERROR_FORMAT_NOT_SUPPORTED";
            case VK_ERROR_SURFACE_LOST_KHR: return "VK_ERROR_SURFACE_LOST_KHR";
            case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR: return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
            case VK_SUBOPTIMAL_KHR: return "VK_SUBOPTIMAL_KHR";
            case VK_ERROR_OUT_OF_DATE_KHR: return "VK_ERROR_OUT_OF_DATE_KHR";
            default: return "VK_RESULT_UNKNOWN";
        }
    }

    static void vk_expect(VkResult result, std::string_view context) {
        if (result != VK_SUCCESS) {
            rocket::log(
                std::string(context) + " failed with " + vk_result_to_string(result),
                "vulkan_renderer_2d",
                "vk_expect",
                "fatal"
            );
            throw std::runtime_error(std::string(context));
        }
    }

    [[nodiscard]] static rge_vk_native_state_t &vk_state(rocket::vulkan_renderer_2d *renderer) {
        r_assert(renderer != nullptr);
        auto *impl = renderer->get_backend_impl();
        r_assert(impl != nullptr);
        r_assert(impl->native_state != nullptr);
        return *reinterpret_cast<rge_vk_native_state_t*>(impl->native_state);
    }

    [[nodiscard]] static rocket::vec2f_t resolved_viewport_size(const rocket::renderer_2d_i *renderer) {
        if (renderer->get_override_viewport_size_state() != rocket::vec2f_t { -1.f, -1.f }) {
            return renderer->get_override_viewport_size_state();
        }
        auto *window = renderer->get_window_backend();
        r_assert(window != nullptr);
        return {
            static_cast<float>(window->get_size().x),
            static_cast<float>(window->get_size().y)
        };
    }

    [[nodiscard]] static VkExtent2D to_vk_extent(const rocket::vec2f_t &size) {
        return {
            static_cast<uint32_t>(std::max(1.f, std::round(size.x))),
            static_cast<uint32_t>(std::max(1.f, std::round(size.y)))
        };
    }

    [[nodiscard]] static std::string sanitize_temp_name(std::string_view value) {
        std::string out;
        out.reserve(value.size());
        for (char ch : value) {
            if ((ch >= 'a' && ch <= 'z') ||
                (ch >= 'A' && ch <= 'Z') ||
                (ch >= '0' && ch <= '9')) {
                out.push_back(ch);
            } else {
                out.push_back('_');
            }
        }
        if (out.empty()) {
            return "shader";
        }
        return out;
    }

    [[nodiscard]] static std::filesystem::path glslc_path() {
#ifdef ROCKETGE__Platform_Windows
        if (const char *vulkan_sdk = std::getenv("VULKAN_SDK")) {
            auto candidate = std::filesystem::path(vulkan_sdk) / "Bin" / "glslc.exe";
            if (std::filesystem::exists(candidate)) {
                return candidate;
            }
        }
        return std::filesystem::path("glslc.exe");
#else
        return std::filesystem::path("glslc");
#endif
    }

#ifdef ROCKETGE__Platform_Windows
    struct process_result_t {
        DWORD exit_code = static_cast<DWORD>(-1);
        std::string output;
        bool launched = false;
    };

    [[nodiscard]] static std::wstring widen(std::string_view value) {
        if (value.empty()) {
            return {};
        }

        const int size = MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0);
        if (size <= 0) {
            return {};
        }

        std::wstring wide(static_cast<std::size_t>(size), L'\0');
        MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), wide.data(), size);
        return wide;
    }

    [[nodiscard]] static process_result_t run_process_capture(
        const std::filesystem::path &exe,
        const std::vector<std::wstring> &args
    ) {
        process_result_t result {};

        SECURITY_ATTRIBUTES security_attributes {};
        security_attributes.nLength = sizeof(security_attributes);
        security_attributes.bInheritHandle = TRUE;

        HANDLE read_pipe = nullptr;
        HANDLE write_pipe = nullptr;
        if (!CreatePipe(&read_pipe, &write_pipe, &security_attributes, 0)) {
            result.output = "CreatePipe failed with " + std::to_string(GetLastError());
            return result;
        }
        SetHandleInformation(read_pipe, HANDLE_FLAG_INHERIT, 0);

        const auto quote_arg = [](const std::wstring &arg) {
            if (arg.find_first_of(L" \t\"") == std::wstring::npos) {
                return arg;
            }

            std::wstring quoted = L"\"";
            for (const wchar_t ch : arg) {
                if (ch == L'"') {
                    quoted += L'\\';
                }
                quoted += ch;
            }
            quoted += L"\"";
            return quoted;
        };

        const std::wstring exe_w = exe.wstring();
        std::wstring command_line = quote_arg(exe_w);
        for (const auto &arg : args) {
            command_line += L" ";
            command_line += quote_arg(arg);
        }
        std::vector<wchar_t> command_buffer(command_line.begin(), command_line.end());
        command_buffer.push_back(L'\0');

        STARTUPINFOW startup_info {};
        startup_info.cb = sizeof(startup_info);
        startup_info.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
        startup_info.wShowWindow = SW_HIDE;
        startup_info.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
        startup_info.hStdOutput = write_pipe;
        startup_info.hStdError = write_pipe;

        PROCESS_INFORMATION process_info {};
        const BOOL created = CreateProcessW(
            exe_w.c_str(),
            command_buffer.data(),
            nullptr,
            nullptr,
            TRUE,
            CREATE_NO_WINDOW,
            nullptr,
            nullptr,
            &startup_info,
            &process_info
        );
        CloseHandle(write_pipe);
        result.launched = created == TRUE;

        if (!created) {
            result.output = "CreateProcessW failed with " + std::to_string(GetLastError());
            CloseHandle(read_pipe);
            return result;
        }

        WaitForSingleObject(process_info.hProcess, INFINITE);

        char buffer[4096];
        DWORD bytes_read = 0;
        while (ReadFile(read_pipe, buffer, sizeof(buffer), &bytes_read, nullptr) && bytes_read != 0) {
            result.output.append(buffer, buffer + bytes_read);
        }

        GetExitCodeProcess(process_info.hProcess, &result.exit_code);
        CloseHandle(process_info.hThread);
        CloseHandle(process_info.hProcess);
        CloseHandle(read_pipe);
        return result;
    }
#endif

    [[nodiscard]] static std::vector<uint32_t> compile_glsl_to_spirv(
        const std::string &source,
        std::string_view stage,
        std::string_view hint
    ) {
        const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
        const auto safe_hint = sanitize_temp_name(hint);
        const auto debug_dir = std::filesystem::current_path() / "build" / "vk_shader_debug";
        std::error_code ignore_ec;
        std::filesystem::create_directories(debug_dir, ignore_ec);
        const auto src_path = debug_dir / ("rocket_vk_" + safe_hint + "_" + std::to_string(nonce) + "." + std::string(stage));
        const auto spv_path = debug_dir / ("rocket_vk_" + safe_hint + "_" + std::to_string(nonce) + ".spv");
        const auto log_path = debug_dir / ("rocket_vk_" + safe_hint + "_" + std::to_string(nonce) + ".log");

        {
            std::ofstream src_file(src_path, std::ios::binary);
            if (!src_file.is_open()) {
                rocket::log("failed to open temporary shader source file", "vulkan_renderer_2d", "compile_glsl_to_spirv", "fatal");
                throw std::runtime_error("failed to open temporary shader source file");
            }
            src_file << source;
        }

        const auto exe = glslc_path();
        std::string command_text;
        int rc = -1;
        std::string compiler_output;
#ifdef ROCKETGE__Platform_Windows
        const auto result = run_process_capture(
            exe,
            {
                widen("--target-env=vulkan1.0"),
                widen(std::string("-fshader-stage=") + std::string(stage)),
                widen("-o"),
                spv_path.wstring(),
                src_path.wstring(),
            }
        );
        command_text =
            "\"" + exe.string() + "\"" +
            std::string(" --target-env=vulkan1.0 -fshader-stage=") + std::string(stage) +
            " -o \"" + spv_path.string() + "\" \"" + src_path.string() + "\"";
        rc = static_cast<int>(result.exit_code);
        compiler_output = result.output;
#else
        std::ostringstream command;
        command
            << '"' << exe.string() << '"'
            << " --target-env=vulkan1.0"
            << " -fshader-stage=" << stage
            << " -o " << '"' << spv_path.string() << '"'
            << ' ' << '"' << src_path.string() << '"';
        command_text = command.str();
        rc = std::system(command_text.c_str());
#endif
        if (rc != 0) {
            {
                std::ofstream log_file(log_path, std::ios::binary);
                log_file << compiler_output;
            }
            std::filesystem::remove(spv_path, ignore_ec);
            rocket::log(
                "glslc failed while compiling " + std::string(hint) + " (" + std::string(stage) + ")\n"
                    + "command: " + command_text + "\n"
                    + "source: " + src_path.string() + "\n"
                    + "stderr/stdout:\n" + compiler_output,
                "vulkan_renderer_2d",
                "compile_glsl_to_spirv",
                "fatal"
            );
            std::fputs(("glslc failed:\n" + command_text + "\n" + compiler_output + "\n").c_str(), stderr);
            throw std::runtime_error("glslc failed");
        }
        std::filesystem::remove(src_path, ignore_ec);
        std::filesystem::remove(log_path, ignore_ec);

        std::ifstream spv_file(spv_path, std::ios::binary | std::ios::ate);
        if (!spv_file.is_open()) {
            std::filesystem::remove(spv_path, ignore_ec);
            rocket::log("failed to open generated SPIR-V file", "vulkan_renderer_2d", "compile_glsl_to_spirv", "fatal");
            throw std::runtime_error("failed to open generated SPIR-V file");
        }

        const auto end_pos = spv_file.tellg();
        if (end_pos <= 0 || (static_cast<std::size_t>(end_pos) % sizeof(uint32_t)) != 0) {
            std::filesystem::remove(spv_path, ignore_ec);
            rocket::log("generated SPIR-V had an invalid size", "vulkan_renderer_2d", "compile_glsl_to_spirv", "fatal");
            throw std::runtime_error("invalid SPIR-V output");
        }

        std::vector<uint32_t> spirv(static_cast<std::size_t>(end_pos) / sizeof(uint32_t));
        spv_file.seekg(0);
        spv_file.read(reinterpret_cast<char*>(spirv.data()), static_cast<std::streamsize>(end_pos));
        spv_file.close();
        std::filesystem::remove(spv_path, ignore_ec);
        return spirv;
    }

    [[nodiscard]] static uint32_t find_memory_type(
        VkPhysicalDevice physical_device,
        uint32_t type_filter,
        VkMemoryPropertyFlags properties
    ) {
        VkPhysicalDeviceMemoryProperties memory_properties {};
        vkGetPhysicalDeviceMemoryProperties(physical_device, &memory_properties);

        for (uint32_t i = 0; i < memory_properties.memoryTypeCount; ++i) {
            const bool type_matches = (type_filter & (1u << i)) != 0;
            const bool property_matches =
                (memory_properties.memoryTypes[i].propertyFlags & properties) == properties;
            if (type_matches && property_matches) {
                return i;
            }
        }

        rocket::log("failed to find a matching Vulkan memory type", "vulkan_renderer_2d", "find_memory_type", "fatal");
        throw std::runtime_error("failed to find Vulkan memory type");
    }

    static void create_buffer(
        rge_vk_native_state_t &state,
        VkDeviceSize size,
        VkBufferUsageFlags usage,
        VkMemoryPropertyFlags properties,
        VkBuffer &buffer,
        VkDeviceMemory &memory
    ) {
        VkBufferCreateInfo create_info { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        create_info.size = size;
        create_info.usage = usage;
        create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        vk_expect(vkCreateBuffer(state.device, &create_info, nullptr, &buffer), "vkCreateBuffer");

        VkMemoryRequirements mem_requirements {};
        vkGetBufferMemoryRequirements(state.device, buffer, &mem_requirements);

        VkMemoryAllocateInfo alloc_info { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
        alloc_info.allocationSize = mem_requirements.size;
        alloc_info.memoryTypeIndex = find_memory_type(state.physical_device, mem_requirements.memoryTypeBits, properties);

        vk_expect(vkAllocateMemory(state.device, &alloc_info, nullptr, &memory), "vkAllocateMemory(buffer)");
        vk_expect(vkBindBufferMemory(state.device, buffer, memory, 0), "vkBindBufferMemory");
    }

    static void destroy_buffer(rge_vk_native_state_t &state, VkBuffer &buffer, VkDeviceMemory &memory) {
        if (buffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(state.device, buffer, nullptr);
            buffer = VK_NULL_HANDLE;
        }
        if (memory != VK_NULL_HANDLE) {
            vkFreeMemory(state.device, memory, nullptr);
            memory = VK_NULL_HANDLE;
        }
    }

    static void create_image(
        rge_vk_native_state_t &state,
        uint32_t width,
        uint32_t height,
        VkFormat format,
        VkImageUsageFlags usage,
        VkImage &image,
        VkDeviceMemory &memory
    ) {
        VkImageCreateInfo create_info { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
        create_info.imageType = VK_IMAGE_TYPE_2D;
        create_info.extent = { width, height, 1 };
        create_info.mipLevels = 1;
        create_info.arrayLayers = 1;
        create_info.format = format;
        create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
        create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        create_info.usage = usage;
        create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        create_info.samples = VK_SAMPLE_COUNT_1_BIT;

        vk_expect(vkCreateImage(state.device, &create_info, nullptr, &image), "vkCreateImage");

        VkMemoryRequirements mem_requirements {};
        vkGetImageMemoryRequirements(state.device, image, &mem_requirements);

        VkMemoryAllocateInfo alloc_info { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
        alloc_info.allocationSize = mem_requirements.size;
        alloc_info.memoryTypeIndex = find_memory_type(
            state.physical_device,
            mem_requirements.memoryTypeBits,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        );

        vk_expect(vkAllocateMemory(state.device, &alloc_info, nullptr, &memory), "vkAllocateMemory(image)");
        vk_expect(vkBindImageMemory(state.device, image, memory, 0), "vkBindImageMemory");
    }

    static void destroy_image(
        rge_vk_native_state_t &state,
        VkImage &image,
        VkDeviceMemory &memory,
        VkImageView &view
    ) {
        if (view != VK_NULL_HANDLE) {
            vkDestroyImageView(state.device, view, nullptr);
            view = VK_NULL_HANDLE;
        }
        if (image != VK_NULL_HANDLE) {
            vkDestroyImage(state.device, image, nullptr);
            image = VK_NULL_HANDLE;
        }
        if (memory != VK_NULL_HANDLE) {
            vkFreeMemory(state.device, memory, nullptr);
            memory = VK_NULL_HANDLE;
        }
    }

    [[nodiscard]] static VkImageView create_image_view(
        VkDevice device,
        VkImage image,
        VkFormat format
    ) {
        VkImageViewCreateInfo view_info { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
        view_info.image = image;
        view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view_info.format = format;
        view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        view_info.subresourceRange.baseMipLevel = 0;
        view_info.subresourceRange.levelCount = 1;
        view_info.subresourceRange.baseArrayLayer = 0;
        view_info.subresourceRange.layerCount = 1;

        VkImageView view = VK_NULL_HANDLE;
        vk_expect(vkCreateImageView(device, &view_info, nullptr, &view), "vkCreateImageView");
        return view;
    }

    [[nodiscard]] static VkShaderModule create_shader_module(VkDevice device, std::span<const uint32_t> code) {
        VkShaderModuleCreateInfo create_info { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
        create_info.codeSize = code.size_bytes();
        create_info.pCode = code.data();

        VkShaderModule shader_module = VK_NULL_HANDLE;
        vk_expect(vkCreateShaderModule(device, &create_info, nullptr, &shader_module), "vkCreateShaderModule");
        return shader_module;
    }

    [[nodiscard]] static rge_vk_swapchain_support_t query_swapchain_support(
        VkPhysicalDevice device,
        VkSurfaceKHR surface
    ) {
        rge_vk_swapchain_support_t details;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

        uint32_t format_count = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, nullptr);
        if (format_count > 0) {
            details.formats.resize(format_count);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, details.formats.data());
        }

        uint32_t present_mode_count = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, nullptr);
        if (present_mode_count > 0) {
            details.present_modes.resize(present_mode_count);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, details.present_modes.data());
        }

        return details;
    }

    [[nodiscard]] static std::optional<std::pair<uint32_t, uint32_t>> find_queue_families(
        VkPhysicalDevice device,
        VkSurfaceKHR surface
    ) {
        uint32_t queue_family_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);
        std::vector<VkQueueFamilyProperties> families(queue_family_count);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, families.data());

        std::optional<uint32_t> graphics_family;
        std::optional<uint32_t> present_family;

        for (uint32_t i = 0; i < queue_family_count; ++i) {
            if ((families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0) {
                graphics_family = i;
            }

            VkBool32 present_support = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &present_support);
            if (present_support == VK_TRUE) {
                present_family = i;
            }

            if (graphics_family.has_value() && present_family.has_value()) {
                return std::make_pair(*graphics_family, *present_family);
            }
        }

        return std::nullopt;
    }

    [[nodiscard]] static bool physical_device_suitable(VkPhysicalDevice device, VkSurfaceKHR surface) {
        const auto queue_families = find_queue_families(device, surface);
        if (!queue_families.has_value()) {
            return false;
        }

        uint32_t extension_count = 0;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);
        std::vector<VkExtensionProperties> available_extensions(extension_count);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, available_extensions.data());

        bool has_swapchain = false;
        for (const auto &ext : available_extensions) {
            if (std::string(ext.extensionName) == VK_KHR_SWAPCHAIN_EXTENSION_NAME) {
                has_swapchain = true;
                break;
            }
        }
        if (!has_swapchain) {
            return false;
        }

        const auto support = query_swapchain_support(device, surface);
        return !support.formats.empty() && !support.present_modes.empty();
    }

    [[nodiscard]] static VkSurfaceFormatKHR choose_surface_format(const std::vector<VkSurfaceFormatKHR> &formats) {
        for (const auto &format : formats) {
            if ((format.format == VK_FORMAT_B8G8R8A8_SRGB || format.format == VK_FORMAT_B8G8R8A8_UNORM) &&
                format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return format;
            }
        }
        return formats.front();
    }

    [[nodiscard]] static VkPresentModeKHR choose_present_mode(const std::vector<VkPresentModeKHR> &modes, bool vsync) {
        if (vsync) {
            return VK_PRESENT_MODE_FIFO_KHR;
        }
        for (const auto mode : modes) {
            if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
                return mode;
            }
        }
        for (const auto mode : modes) {
            if (mode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
                return mode;
            }
        }
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    [[nodiscard]] static VkExtent2D choose_swap_extent(
        const VkSurfaceCapabilitiesKHR &capabilities,
        const rocket::vec2i_t &window_size
    ) {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return capabilities.currentExtent;
        }

        VkExtent2D actual_extent {
            static_cast<uint32_t>(std::max(1, window_size.x)),
            static_cast<uint32_t>(std::max(1, window_size.y))
        };

        actual_extent.width = std::clamp(
            actual_extent.width,
            capabilities.minImageExtent.width,
            capabilities.maxImageExtent.width
        );
        actual_extent.height = std::clamp(
            actual_extent.height,
            capabilities.minImageExtent.height,
            capabilities.maxImageExtent.height
        );

        return actual_extent;
    }

    static void destroy_pipeline(VkDevice device, rge_vk_pipeline_t &pipeline) {
        if (pipeline.pipeline != VK_NULL_HANDLE) {
            vkDestroyPipeline(device, pipeline.pipeline, nullptr);
            pipeline.pipeline = VK_NULL_HANDLE;
        }
        if (pipeline.layout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(device, pipeline.layout, nullptr);
            pipeline.layout = VK_NULL_HANDLE;
        }
        if (pipeline.vertex_module != VK_NULL_HANDLE) {
            vkDestroyShaderModule(device, pipeline.vertex_module, nullptr);
            pipeline.vertex_module = VK_NULL_HANDLE;
        }
        if (pipeline.fragment_module != VK_NULL_HANDLE) {
            vkDestroyShaderModule(device, pipeline.fragment_module, nullptr);
            pipeline.fragment_module = VK_NULL_HANDLE;
        }
    }

    [[nodiscard]] static rge_vk_pipeline_t create_pipeline(
        rge_vk_native_state_t &state,
        std::span<const uint32_t> vertex_spirv,
        std::span<const uint32_t> fragment_spirv,
        VkDescriptorSetLayout descriptor_set_layout,
        bool alpha_blend
    ) {
        rge_vk_pipeline_t pipeline;
        pipeline.vertex_module = create_shader_module(state.device, vertex_spirv);
        pipeline.fragment_module = create_shader_module(state.device, fragment_spirv);

        VkPipelineShaderStageCreateInfo vertex_stage { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
        vertex_stage.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertex_stage.module = pipeline.vertex_module;
        vertex_stage.pName = "main";

        VkPipelineShaderStageCreateInfo fragment_stage { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
        fragment_stage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragment_stage.module = pipeline.fragment_module;
        fragment_stage.pName = "main";

        VkPipelineShaderStageCreateInfo stages[] = { vertex_stage, fragment_stage };

        const std::array<VkVertexInputBindingDescription, 1> bindings = {{
            {
                .binding = 0,
                .stride = sizeof(rge_vk_vertex_t),
                .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
            }
        }};

        const std::array<VkVertexInputAttributeDescription, 2> attributes = {{
            {
                .location = 0,
                .binding = 0,
                .format = VK_FORMAT_R32G32_SFLOAT,
                .offset = offsetof(rge_vk_vertex_t, pos)
            },
            {
                .location = 1,
                .binding = 0,
                .format = VK_FORMAT_R32G32_SFLOAT,
                .offset = offsetof(rge_vk_vertex_t, uv)
            }
        }};

        VkPipelineVertexInputStateCreateInfo vertex_input { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
        vertex_input.vertexBindingDescriptionCount = static_cast<uint32_t>(bindings.size());
        vertex_input.pVertexBindingDescriptions = bindings.data();
        vertex_input.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributes.size());
        vertex_input.pVertexAttributeDescriptions = attributes.data();

        VkPipelineInputAssemblyStateCreateInfo input_assembly { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
        input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        input_assembly.primitiveRestartEnable = VK_FALSE;

        VkPipelineViewportStateCreateInfo viewport_state { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
        viewport_state.viewportCount = 1;
        viewport_state.scissorCount = 1;

        const std::array<VkDynamicState, 2> dynamic_states = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };
        VkPipelineDynamicStateCreateInfo dynamic_state { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
        dynamic_state.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
        dynamic_state.pDynamicStates = dynamic_states.data();

        VkPipelineRasterizationStateCreateInfo rasterizer { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.f;
        rasterizer.cullMode = VK_CULL_MODE_NONE;
        rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;

        VkPipelineMultisampleStateCreateInfo multisampling { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisampling.sampleShadingEnable = VK_FALSE;

        VkPipelineColorBlendAttachmentState color_blend_attachment {};
        color_blend_attachment.colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT |
            VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT |
            VK_COLOR_COMPONENT_A_BIT;
        color_blend_attachment.blendEnable = alpha_blend ? VK_TRUE : VK_FALSE;
        color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
        color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;

        VkPipelineColorBlendStateCreateInfo color_blending { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
        color_blending.logicOpEnable = VK_FALSE;
        color_blending.attachmentCount = 1;
        color_blending.pAttachments = &color_blend_attachment;

        VkPipelineLayoutCreateInfo layout_info { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
        if (descriptor_set_layout != VK_NULL_HANDLE) {
            layout_info.setLayoutCount = 1;
            layout_info.pSetLayouts = &descriptor_set_layout;
        }
        vk_expect(vkCreatePipelineLayout(state.device, &layout_info, nullptr, &pipeline.layout), "vkCreatePipelineLayout");

        VkGraphicsPipelineCreateInfo pipeline_info { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
        pipeline_info.stageCount = static_cast<uint32_t>(std::size(stages));
        pipeline_info.pStages = stages;
        pipeline_info.pVertexInputState = &vertex_input;
        pipeline_info.pInputAssemblyState = &input_assembly;
        pipeline_info.pViewportState = &viewport_state;
        pipeline_info.pRasterizationState = &rasterizer;
        pipeline_info.pMultisampleState = &multisampling;
        pipeline_info.pColorBlendState = &color_blending;
        pipeline_info.pDynamicState = &dynamic_state;
        pipeline_info.layout = pipeline.layout;
        pipeline_info.renderPass = state.render_pass;
        pipeline_info.subpass = 0;
        pipeline_info.basePipelineHandle = VK_NULL_HANDLE;

        vk_expect(
            vkCreateGraphicsPipelines(state.device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &pipeline.pipeline),
            "vkCreateGraphicsPipelines"
        );

        return pipeline;
    }

    static void create_render_pass(rge_vk_native_state_t &state) {
        VkAttachmentDescription color_attachment {};
        color_attachment.format = state.swapchain_format;
        color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
        color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference color_attachment_ref {};
        color_attachment_ref.attachment = 0;
        color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &color_attachment_ref;

        VkSubpassDependency dependency {};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo render_pass_info { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
        render_pass_info.attachmentCount = 1;
        render_pass_info.pAttachments = &color_attachment;
        render_pass_info.subpassCount = 1;
        render_pass_info.pSubpasses = &subpass;
        render_pass_info.dependencyCount = 1;
        render_pass_info.pDependencies = &dependency;

        vk_expect(vkCreateRenderPass(state.device, &render_pass_info, nullptr, &state.render_pass), "vkCreateRenderPass");
    }

    static void destroy_swapchain_objects(rge_vk_native_state_t &state) {
        for (auto &framebuffer : state.swapchain_framebuffers) {
            if (framebuffer != VK_NULL_HANDLE) {
                vkDestroyFramebuffer(state.device, framebuffer, nullptr);
            }
        }
        state.swapchain_framebuffers.clear();

        for (auto &view : state.swapchain_image_views) {
            if (view != VK_NULL_HANDLE) {
                vkDestroyImageView(state.device, view, nullptr);
            }
        }
        state.swapchain_image_views.clear();
        state.swapchain_images.clear();
        state.images_in_flight.clear();

        destroy_pipeline(state.device, state.overlay_pipeline);

        if (state.render_pass != VK_NULL_HANDLE) {
            vkDestroyRenderPass(state.device, state.render_pass, nullptr);
            state.render_pass = VK_NULL_HANDLE;
        }

        if (state.swapchain != VK_NULL_HANDLE) {
            vkDestroySwapchainKHR(state.device, state.swapchain, nullptr);
            state.swapchain = VK_NULL_HANDLE;
        }
    }

    static void destroy_shader_native_pipeline(rge_vk_native_state_t &state, rocket::vk_object_t &object) {
        if (object.additional == nullptr) {
            return;
        }
        auto *native_pipeline = reinterpret_cast<rge_vk_shader_pipeline_t*>(object.additional);
        destroy_pipeline(state.device, native_pipeline->pipeline);
        delete native_pipeline;
        object.additional = nullptr;
    }

    static void invalidate_all_shader_pipelines(rge_vk_native_state_t &state, rocket::vulkan_renderer_2d *renderer) {
        auto *impl = renderer->get_backend_impl();
        r_assert(impl != nullptr);
        for (auto &[_, object] : impl->objects) {
            if (object.type == rocket::vk_object_type_t::shader) {
                destroy_shader_native_pipeline(state, object);
            }
        }
    }

    static void create_swapchain(rge_vk_native_state_t &state, rocket::window_backend_i *window, bool vsync) {
        const auto support = query_swapchain_support(state.physical_device, state.surface);
        const VkSurfaceFormatKHR surface_format = choose_surface_format(support.formats);
        const VkPresentModeKHR present_mode = choose_present_mode(support.present_modes, vsync);
        const VkExtent2D extent = choose_swap_extent(support.capabilities, window->get_size());

        uint32_t image_count = support.capabilities.minImageCount + 1;
        if (support.capabilities.maxImageCount > 0 && image_count > support.capabilities.maxImageCount) {
            image_count = support.capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR create_info { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
        create_info.surface = state.surface;
        create_info.minImageCount = image_count;
        create_info.imageFormat = surface_format.format;
        create_info.imageColorSpace = surface_format.colorSpace;
        create_info.imageExtent = extent;
        create_info.imageArrayLayers = 1;
        create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        const uint32_t queue_family_indices[] = {
            state.graphics_family,
            state.present_family
        };
        if (state.graphics_family != state.present_family) {
            create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            create_info.queueFamilyIndexCount = 2;
            create_info.pQueueFamilyIndices = queue_family_indices;
        } else {
            create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }

        create_info.preTransform = support.capabilities.currentTransform;
        create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        create_info.presentMode = present_mode;
        create_info.clipped = VK_TRUE;
        create_info.oldSwapchain = VK_NULL_HANDLE;

        vk_expect(vkCreateSwapchainKHR(state.device, &create_info, nullptr, &state.swapchain), "vkCreateSwapchainKHR");

        uint32_t actual_image_count = 0;
        vkGetSwapchainImagesKHR(state.device, state.swapchain, &actual_image_count, nullptr);
        state.swapchain_images.resize(actual_image_count);
        vkGetSwapchainImagesKHR(state.device, state.swapchain, &actual_image_count, state.swapchain_images.data());

        state.swapchain_format = surface_format.format;
        state.swapchain_extent = extent;

        state.swapchain_image_views.reserve(state.swapchain_images.size());
        for (auto image : state.swapchain_images) {
            state.swapchain_image_views.push_back(create_image_view(state.device, image, state.swapchain_format));
        }

        create_render_pass(state);

        state.swapchain_framebuffers.reserve(state.swapchain_image_views.size());
        for (auto view : state.swapchain_image_views) {
            VkFramebufferCreateInfo framebuffer_info { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
            framebuffer_info.renderPass = state.render_pass;
            framebuffer_info.attachmentCount = 1;
            framebuffer_info.pAttachments = &view;
            framebuffer_info.width = state.swapchain_extent.width;
            framebuffer_info.height = state.swapchain_extent.height;
            framebuffer_info.layers = 1;

            VkFramebuffer framebuffer = VK_NULL_HANDLE;
            vk_expect(vkCreateFramebuffer(state.device, &framebuffer_info, nullptr, &framebuffer), "vkCreateFramebuffer");
            state.swapchain_framebuffers.push_back(framebuffer);
        }

        state.images_in_flight.assign(state.swapchain_images.size(), VK_NULL_HANDLE);
    }

    static void create_overlay_descriptor_resources(rge_vk_native_state_t &state) {
        VkDescriptorSetLayoutBinding sampler_binding {};
        sampler_binding.binding = 0;
        sampler_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        sampler_binding.descriptorCount = 1;
        sampler_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutCreateInfo layout_info { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
        layout_info.bindingCount = 1;
        layout_info.pBindings = &sampler_binding;
        vk_expect(vkCreateDescriptorSetLayout(state.device, &layout_info, nullptr, &state.overlay_set_layout), "vkCreateDescriptorSetLayout");

        VkDescriptorPoolSize pool_size {};
        pool_size.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        pool_size.descriptorCount = 1;

        VkDescriptorPoolCreateInfo pool_info { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
        pool_info.poolSizeCount = 1;
        pool_info.pPoolSizes = &pool_size;
        pool_info.maxSets = 1;
        vk_expect(vkCreateDescriptorPool(state.device, &pool_info, nullptr, &state.overlay_descriptor_pool), "vkCreateDescriptorPool");

        VkDescriptorSetAllocateInfo alloc_info { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
        alloc_info.descriptorPool = state.overlay_descriptor_pool;
        alloc_info.descriptorSetCount = 1;
        alloc_info.pSetLayouts = &state.overlay_set_layout;
        vk_expect(vkAllocateDescriptorSets(state.device, &alloc_info, &state.overlay_descriptor_set), "vkAllocateDescriptorSets");
    }

    static void create_overlay_resources(rge_vk_native_state_t &state, const VkExtent2D extent) {
        destroy_image(state, state.overlay_image, state.overlay_memory, state.overlay_view);
        if (state.overlay_sampler != VK_NULL_HANDLE) {
            vkDestroySampler(state.device, state.overlay_sampler, nullptr);
            state.overlay_sampler = VK_NULL_HANDLE;
        }

        create_image(
            state,
            extent.width,
            extent.height,
            VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            state.overlay_image,
            state.overlay_memory
        );
        state.overlay_view = create_image_view(state.device, state.overlay_image, VK_FORMAT_R8G8B8A8_UNORM);
        state.overlay_layout = VK_IMAGE_LAYOUT_UNDEFINED;
        state.overlay_extent = extent;

        VkSamplerCreateInfo sampler_info { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
        sampler_info.magFilter = VK_FILTER_NEAREST;
        sampler_info.minFilter = VK_FILTER_NEAREST;
        sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        sampler_info.maxAnisotropy = 1.f;
        sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        sampler_info.unnormalizedCoordinates = VK_FALSE;
        sampler_info.compareEnable = VK_FALSE;
        sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        vk_expect(vkCreateSampler(state.device, &sampler_info, nullptr, &state.overlay_sampler), "vkCreateSampler");

        VkDescriptorImageInfo image_info {};
        image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        image_info.imageView = state.overlay_view;
        image_info.sampler = state.overlay_sampler;

        VkWriteDescriptorSet descriptor_write { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
        descriptor_write.dstSet = state.overlay_descriptor_set;
        descriptor_write.dstBinding = 0;
        descriptor_write.dstArrayElement = 0;
        descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptor_write.descriptorCount = 1;
        descriptor_write.pImageInfo = &image_info;
        vkUpdateDescriptorSets(state.device, 1, &descriptor_write, 0, nullptr);
    }

    static void recreate_upload_buffer(rge_vk_native_state_t &state, std::size_t required_size) {
        if (required_size <= state.upload_buffer_size && state.upload_buffer != VK_NULL_HANDLE) {
            return;
        }

        destroy_buffer(state, state.upload_buffer, state.upload_memory);
        create_buffer(
            state,
            std::max<std::size_t>(required_size, 4),
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            state.upload_buffer,
            state.upload_memory
        );
        state.upload_buffer_size = std::max<std::size_t>(required_size, 4);
    }

    static void recreate_overlay_if_needed(rocket::vulkan_renderer_2d *renderer) {
        auto &state = vk_state(renderer);
        const VkExtent2D desired_extent = to_vk_extent(resolved_viewport_size(renderer));
        if (state.overlay_extent.width == desired_extent.width &&
            state.overlay_extent.height == desired_extent.height &&
            !state.overlay_needs_rebuild) {
            return;
        }

        vk_expect(vkDeviceWaitIdle(state.device), "vkDeviceWaitIdle(overlay rebuild)");
        recreate_upload_buffer(state, static_cast<std::size_t>(desired_extent.width) * desired_extent.height * sizeof(rocket::rgba_color));
        create_overlay_resources(state, desired_extent);
        state.overlay_needs_rebuild = false;
    }

    static void ensure_framebuffer_storage(rocket::vulkan_renderer_2d *renderer) {
        auto &state = vk_state(renderer);
        const VkExtent2D extent = to_vk_extent(resolved_viewport_size(renderer));
        const std::size_t pixel_count = static_cast<std::size_t>(extent.width) * extent.height;
        if (state.framebuffer.size() != pixel_count) {
            state.framebuffer.assign(pixel_count, rocket::rgba_color::blank());
        }
    }

    static void recreate_swapchain_if_needed(rocket::vulkan_renderer_2d *renderer) {
        auto &state = vk_state(renderer);
        auto *window = renderer->get_window_backend();
        r_assert(window != nullptr);
        if (!state.swapchain_needs_rebuild &&
            state.swapchain_extent.width == static_cast<uint32_t>(std::max(1, window->get_size().x)) &&
            state.swapchain_extent.height == static_cast<uint32_t>(std::max(1, window->get_size().y))) {
            return;
        }

        const auto current_size = window->get_size();
        if (current_size.x <= 0 || current_size.y <= 0) {
            return;
        }

        vk_expect(vkDeviceWaitIdle(state.device), "vkDeviceWaitIdle(swapchain rebuild)");
        destroy_swapchain_objects(state);
        create_swapchain(state, window, renderer->get_vsync_state());
        invalidate_all_shader_pipelines(state, renderer);
        state.swapchain_needs_rebuild = false;
    }

    static void create_fullscreen_vertex_buffer(rge_vk_native_state_t &state) {
        static constexpr std::array<rge_vk_vertex_t, 6> vertices = {{
            { { -1.f, -1.f }, { 0.f, 0.f } },
            { {  1.f, -1.f }, { 1.f, 0.f } },
            { {  1.f,  1.f }, { 1.f, 1.f } },
            { { -1.f, -1.f }, { 0.f, 0.f } },
            { {  1.f,  1.f }, { 1.f, 1.f } },
            { { -1.f,  1.f }, { 0.f, 1.f } }
        }};

        create_buffer(
            state,
            sizeof(vertices),
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            state.fullscreen_vertex_buffer,
            state.fullscreen_vertex_memory
        );

        void *mapped = nullptr;
        vk_expect(vkMapMemory(state.device, state.fullscreen_vertex_memory, 0, sizeof(vertices), 0, &mapped), "vkMapMemory(fullscreen vertex)");
        std::memcpy(mapped, vertices.data(), sizeof(vertices));
        vkUnmapMemory(state.device, state.fullscreen_vertex_memory);
    }

    static void create_command_resources(rge_vk_native_state_t &state) {
        VkCommandPoolCreateInfo pool_info { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
        pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        pool_info.queueFamilyIndex = state.graphics_family;
        vk_expect(vkCreateCommandPool(state.device, &pool_info, nullptr, &state.command_pool), "vkCreateCommandPool");

        std::array<VkCommandBuffer, rge_vk_max_frames_in_flight> command_buffers {};
        VkCommandBufferAllocateInfo alloc_info { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
        alloc_info.commandPool = state.command_pool;
        alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        alloc_info.commandBufferCount = static_cast<uint32_t>(command_buffers.size());
        vk_expect(vkAllocateCommandBuffers(state.device, &alloc_info, command_buffers.data()), "vkAllocateCommandBuffers");

        for (std::size_t i = 0; i < rge_vk_max_frames_in_flight; ++i) {
            state.frames[i].command_buffer = command_buffers[i];
        }

        VkSemaphoreCreateInfo semaphore_info { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
        VkFenceCreateInfo fence_info { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
        fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (auto &frame : state.frames) {
            vk_expect(vkCreateSemaphore(state.device, &semaphore_info, nullptr, &frame.image_available), "vkCreateSemaphore(image_available)");
            vk_expect(vkCreateSemaphore(state.device, &semaphore_info, nullptr, &frame.render_finished), "vkCreateSemaphore(render_finished)");
            vk_expect(vkCreateFence(state.device, &fence_info, nullptr, &frame.in_flight), "vkCreateFence");
        }
    }

    static void create_instance(rge_vk_native_state_t &state, bool debug_context) {
        uint32_t extension_count = 0;
        const char **required_extensions = glfwGetRequiredInstanceExtensions(&extension_count);
        std::vector<const char*> extensions(required_extensions, required_extensions + extension_count);

        (void) debug_context;

        VkApplicationInfo app_info { VK_STRUCTURE_TYPE_APPLICATION_INFO };
        app_info.pApplicationName = "RocketRuntime";
        app_info.applicationVersion = VK_MAKE_API_VERSION(0, 3, 0, 0);
        app_info.pEngineName = "RocketGE";
        app_info.engineVersion = VK_MAKE_API_VERSION(0, 3, 0, 0);
        app_info.apiVersion = VK_API_VERSION_1_0;

        VkInstanceCreateInfo create_info { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
        create_info.pApplicationInfo = &app_info;
        create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        create_info.ppEnabledExtensionNames = extensions.data();

        vk_expect(vkCreateInstance(&create_info, nullptr, &state.instance), "vkCreateInstance");
    }

    static void create_surface(rge_vk_native_state_t &state, rocket::window_backend_i *window) {
        const bool created = window->create_vulkan_surface(
            reinterpret_cast<void*>(state.instance),
            nullptr,
            reinterpret_cast<void*>(&state.surface)
        );
        if (!created) {
            rocket::log("failed to create Vulkan surface from the active window backend", "vulkan_renderer_2d", "create_surface", "fatal");
            throw std::runtime_error("failed to create Vulkan surface");
        }
    }

    static void pick_physical_device(rge_vk_native_state_t &state) {
        uint32_t device_count = 0;
        vkEnumeratePhysicalDevices(state.instance, &device_count, nullptr);
        if (device_count == 0) {
            rocket::log("no Vulkan-compatible GPUs were found", "vulkan_renderer_2d", "pick_physical_device", "fatal");
            throw std::runtime_error("no Vulkan-compatible GPUs were found");
        }

        std::vector<VkPhysicalDevice> devices(device_count);
        vkEnumeratePhysicalDevices(state.instance, &device_count, devices.data());

        VkPhysicalDevice fallback = VK_NULL_HANDLE;
        for (auto device : devices) {
            if (!physical_device_suitable(device, state.surface)) {
                continue;
            }

            VkPhysicalDeviceProperties properties {};
            vkGetPhysicalDeviceProperties(device, &properties);
            if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
                state.physical_device = device;
                break;
            }
            if (fallback == VK_NULL_HANDLE) {
                fallback = device;
            }
        }

        if (state.physical_device == VK_NULL_HANDLE) {
            state.physical_device = fallback;
        }
        if (state.physical_device == VK_NULL_HANDLE) {
            rocket::log("failed to find a suitable Vulkan physical device", "vulkan_renderer_2d", "pick_physical_device", "fatal");
            throw std::runtime_error("failed to find Vulkan physical device");
        }

        const auto families = find_queue_families(state.physical_device, state.surface);
        r_assert(families.has_value());
        state.graphics_family = families->first;
        state.present_family = families->second;

        VkPhysicalDeviceProperties properties {};
        vkGetPhysicalDeviceProperties(state.physical_device, &properties);
        rocket::log(
            std::string("Selected GPU: ") + properties.deviceName,
            "vulkan_renderer_2d",
            "constructor",
            "info"
        );
    }

    static void create_logical_device(rge_vk_native_state_t &state) {
        const std::array<uint32_t, 2> unique_families = {
            state.graphics_family,
            state.present_family
        };
        std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
        const float queue_priority = 1.f;

        for (uint32_t family : unique_families) {
            bool already_added = false;
            for (const auto &existing : queue_create_infos) {
                if (existing.queueFamilyIndex == family) {
                    already_added = true;
                    break;
                }
            }
            if (already_added) {
                continue;
            }

            VkDeviceQueueCreateInfo queue_info { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
            queue_info.queueFamilyIndex = family;
            queue_info.queueCount = 1;
            queue_info.pQueuePriorities = &queue_priority;
            queue_create_infos.push_back(queue_info);
        }

        VkPhysicalDeviceFeatures features {};

        const std::array<const char*, 1> device_extensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };

        VkDeviceCreateInfo create_info { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
        create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
        create_info.pQueueCreateInfos = queue_create_infos.data();
        create_info.pEnabledFeatures = &features;
        create_info.enabledExtensionCount = static_cast<uint32_t>(device_extensions.size());
        create_info.ppEnabledExtensionNames = device_extensions.data();

        vk_expect(vkCreateDevice(state.physical_device, &create_info, nullptr, &state.device), "vkCreateDevice");
        vkGetDeviceQueue(state.device, state.graphics_family, 0, &state.graphics_queue);
        vkGetDeviceQueue(state.device, state.present_family, 0, &state.present_queue);
    }

    static void create_overlay_pipeline(rge_vk_native_state_t &state) {
        static const std::string vertex_source = R"(
#version 450
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aUv;
layout(location = 0) out vec2 vUv;

void main() {
    vUv = aUv;
    gl_Position = vec4(aPos, 0.0, 1.0);
}
)";
        static const std::string fragment_source = R"(
#version 450
layout(set = 0, binding = 0) uniform sampler2D uTexture;
layout(location = 0) in vec2 vUv;
layout(location = 0) out vec4 FragColor;

void main() {
    FragColor = texture(uTexture, vUv);
}
)";

        const auto vertex_spirv = compile_glsl_to_spirv(vertex_source, "vert", "rocket_overlay");
        const auto fragment_spirv = compile_glsl_to_spirv(fragment_source, "frag", "rocket_overlay");
        state.overlay_pipeline = create_pipeline(
            state,
            vertex_spirv,
            fragment_spirv,
            state.overlay_set_layout,
            true
        );
    }

    static void ensure_overlay_pipeline(rge_vk_native_state_t &state) {
        if (state.overlay_pipeline.pipeline == VK_NULL_HANDLE) {
            create_overlay_pipeline(state);
        }
    }

    static void begin_command_buffer(VkCommandBuffer command_buffer) {
        VkCommandBufferBeginInfo begin_info { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
        begin_info.flags = 0;
        begin_info.pInheritanceInfo = nullptr;
        vk_expect(vkBeginCommandBuffer(command_buffer, &begin_info), "vkBeginCommandBuffer");
    }

    static void transition_overlay_image(
        rge_vk_native_state_t &state,
        VkCommandBuffer command_buffer,
        VkImageLayout new_layout
    ) {
        if (state.overlay_layout == new_layout) {
            return;
        }

        VkImageMemoryBarrier barrier { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
        barrier.oldLayout = state.overlay_layout;
        barrier.newLayout = new_layout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = state.overlay_image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        VkPipelineStageFlags src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        VkPipelineStageFlags dst_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;

        if (state.overlay_layout == VK_IMAGE_LAYOUT_UNDEFINED &&
            new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            dst_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        } else if (state.overlay_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL &&
                   new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            src_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            dst_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        } else if (state.overlay_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
                   new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            src_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            dst_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        } else {
            rocket::log("unsupported overlay image layout transition requested", "vulkan_renderer_2d", "transition_overlay_image", "fatal");
            throw std::runtime_error("unsupported overlay layout transition");
        }

        vkCmdPipelineBarrier(
            command_buffer,
            src_stage,
            dst_stage,
            0,
            0,
            nullptr,
            0,
            nullptr,
            1,
            &barrier
        );

        state.overlay_layout = new_layout;
    }

    static void upload_framebuffer_to_overlay(rocket::vulkan_renderer_2d *renderer, VkCommandBuffer command_buffer) {
        auto &state = vk_state(renderer);
        const std::size_t upload_size = state.framebuffer.size() * sizeof(rocket::rgba_color);
        if (upload_size == 0) {
            return;
        }

        void *mapped = nullptr;
        vk_expect(vkMapMemory(state.device, state.upload_memory, 0, upload_size, 0, &mapped), "vkMapMemory(upload)");
        std::memcpy(mapped, state.framebuffer.data(), upload_size);
        vkUnmapMemory(state.device, state.upload_memory);

        transition_overlay_image(state, command_buffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        VkBufferImageCopy region {};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = { 0, 0, 0 };
        region.imageExtent = { state.overlay_extent.width, state.overlay_extent.height, 1 };

        vkCmdCopyBufferToImage(
            command_buffer,
            state.upload_buffer,
            state.overlay_image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &region
        );

        transition_overlay_image(state, command_buffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    static void bind_default_viewport_and_scissor(const rge_vk_native_state_t &state, VkCommandBuffer command_buffer) {
        VkViewport viewport {};
        viewport.x = 0.f;
        viewport.y = 0.f;
        viewport.width = static_cast<float>(state.swapchain_extent.width);
        viewport.height = static_cast<float>(state.swapchain_extent.height);
        viewport.minDepth = 0.f;
        viewport.maxDepth = 1.f;
        vkCmdSetViewport(command_buffer, 0, 1, &viewport);

        VkRect2D scissor {};
        scissor.offset = { 0, 0 };
        scissor.extent = state.swapchain_extent;
        vkCmdSetScissor(command_buffer, 0, 1, &scissor);
    }

    static void render_queued_shaders(rocket::vulkan_renderer_2d *renderer, VkCommandBuffer command_buffer) {
        auto &state = vk_state(renderer);
        if (state.queued_shaders.empty()) {
            return;
        }
        auto *impl = renderer->get_backend_impl();
        r_assert(impl != nullptr);

        VkDeviceSize vertex_offset = 0;
        vkCmdBindVertexBuffers(command_buffer, 0, 1, &state.fullscreen_vertex_buffer, &vertex_offset);

        for (const auto shader_handle : state.queued_shaders) {
            auto it = impl->objects.find(shader_handle);
            if (it == impl->objects.end() || it->second.type != rocket::vk_object_type_t::shader) {
                continue;
            }

            auto &object = it->second;
            auto &shader = std::get<rocket::vk_shader_t>(object.value);

            auto *native_pipeline = reinterpret_cast<rge_vk_shader_pipeline_t*>(object.additional);
            if (native_pipeline == nullptr) {
                native_pipeline = new rge_vk_shader_pipeline_t();
                object.additional = native_pipeline;
            }

            if (!native_pipeline->ready) {
                native_pipeline->pipeline = create_pipeline(
                    state,
                    shader.vertex_spirv,
                    shader.fragment_spirv,
                    VK_NULL_HANDLE,
                    false
                );
                native_pipeline->ready = true;
            }

            vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, native_pipeline->pipeline.pipeline);
            bind_default_viewport_and_scissor(state, command_buffer);
            vkCmdDraw(command_buffer, 6, 1, 0, 0);
        }
    }

    static void render_overlay(VkCommandBuffer command_buffer, rge_vk_native_state_t &state) {
        ensure_overlay_pipeline(state);

        VkDeviceSize vertex_offset = 0;
        vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, state.overlay_pipeline.pipeline);
        bind_default_viewport_and_scissor(state, command_buffer);
        vkCmdBindVertexBuffers(command_buffer, 0, 1, &state.fullscreen_vertex_buffer, &vertex_offset);
        vkCmdBindDescriptorSets(
            command_buffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            state.overlay_pipeline.layout,
            0,
            1,
            &state.overlay_descriptor_set,
            0,
            nullptr
        );
        vkCmdDraw(command_buffer, 6, 1, 0, 0);
    }

    static void record_command_buffer(
        rocket::vulkan_renderer_2d *renderer,
        VkCommandBuffer command_buffer,
        uint32_t image_index
    ) {
        auto &state = vk_state(renderer);
        vkResetCommandBuffer(command_buffer, 0);
        begin_command_buffer(command_buffer);
        upload_framebuffer_to_overlay(renderer, command_buffer);

        VkClearValue clear_color {};
        clear_color.color.float32[0] = 0.f;
        clear_color.color.float32[1] = 0.f;
        clear_color.color.float32[2] = 0.f;
        clear_color.color.float32[3] = 1.f;

        VkRenderPassBeginInfo render_pass_info { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
        render_pass_info.renderPass = state.render_pass;
        render_pass_info.framebuffer = state.swapchain_framebuffers[image_index];
        render_pass_info.renderArea.offset = { 0, 0 };
        render_pass_info.renderArea.extent = state.swapchain_extent;
        render_pass_info.clearValueCount = 1;
        render_pass_info.pClearValues = &clear_color;

        vkCmdBeginRenderPass(command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
        render_queued_shaders(renderer, command_buffer);
        render_overlay(command_buffer, state);
        vkCmdEndRenderPass(command_buffer);
        vk_expect(vkEndCommandBuffer(command_buffer), "vkEndCommandBuffer");
    }

    static void initialize_vulkan_backend(rocket::vulkan_renderer_2d *renderer) {
        auto &state = vk_state(renderer);
        auto *window = renderer->get_window_backend();
        r_assert(window != nullptr);
        create_instance(state, window->get_flags().graphics_ctx.debug_context);
        create_surface(state, window);
        pick_physical_device(state);
        create_logical_device(state);
        create_command_resources(state);
        create_fullscreen_vertex_buffer(state);
        create_overlay_descriptor_resources(state);
        create_swapchain(state, window, renderer->get_vsync_state());
        recreate_upload_buffer(
            state,
            static_cast<std::size_t>(std::max(1, window->get_size().x)) *
            std::max(1, window->get_size().y) *
            sizeof(rocket::rgba_color)
        );
        create_overlay_resources(state, to_vk_extent(resolved_viewport_size(renderer)));
    }

    static void destroy_native_backend(rocket::vulkan_renderer_2d *renderer) {
        auto *impl = renderer != nullptr ? renderer->get_backend_impl() : nullptr;
        if (renderer == nullptr || impl == nullptr || impl->native_state == nullptr) {
            return;
        }

        auto &state = vk_state(renderer);
        if (state.device != VK_NULL_HANDLE) {
            vkDeviceWaitIdle(state.device);
        }

        invalidate_all_shader_pipelines(state, renderer);
        destroy_pipeline(state.device, state.overlay_pipeline);

        if (state.overlay_descriptor_pool != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(state.device, state.overlay_descriptor_pool, nullptr);
            state.overlay_descriptor_pool = VK_NULL_HANDLE;
        }
        if (state.overlay_set_layout != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(state.device, state.overlay_set_layout, nullptr);
            state.overlay_set_layout = VK_NULL_HANDLE;
        }

        destroy_image(state, state.overlay_image, state.overlay_memory, state.overlay_view);
        if (state.overlay_sampler != VK_NULL_HANDLE) {
            vkDestroySampler(state.device, state.overlay_sampler, nullptr);
            state.overlay_sampler = VK_NULL_HANDLE;
        }

        destroy_buffer(state, state.upload_buffer, state.upload_memory);
        destroy_buffer(state, state.fullscreen_vertex_buffer, state.fullscreen_vertex_memory);

        for (auto &frame : state.frames) {
            if (frame.image_available != VK_NULL_HANDLE) {
                vkDestroySemaphore(state.device, frame.image_available, nullptr);
                frame.image_available = VK_NULL_HANDLE;
            }
            if (frame.render_finished != VK_NULL_HANDLE) {
                vkDestroySemaphore(state.device, frame.render_finished, nullptr);
                frame.render_finished = VK_NULL_HANDLE;
            }
            if (frame.in_flight != VK_NULL_HANDLE) {
                vkDestroyFence(state.device, frame.in_flight, nullptr);
                frame.in_flight = VK_NULL_HANDLE;
            }
        }

        if (state.command_pool != VK_NULL_HANDLE) {
            vkDestroyCommandPool(state.device, state.command_pool, nullptr);
            state.command_pool = VK_NULL_HANDLE;
        }

        destroy_swapchain_objects(state);

        if (state.device != VK_NULL_HANDLE) {
            vkDestroyDevice(state.device, nullptr);
            state.device = VK_NULL_HANDLE;
        }
        if (state.surface != VK_NULL_HANDLE) {
            vkDestroySurfaceKHR(state.instance, state.surface, nullptr);
            state.surface = VK_NULL_HANDLE;
        }
        if (state.instance != VK_NULL_HANDLE) {
            vkDestroyInstance(state.instance, nullptr);
            state.instance = VK_NULL_HANDLE;
        }
    }

    [[nodiscard]] static rocket::rgba_color sample_texture_nearest(
        const rocket::vk_texture_t &texture,
        float u,
        float v,
        rocket::rgba_color tint = rocket::rgba_color::white()
    ) {
        if (texture.pixels.empty() || texture.size.x <= 0 || texture.size.y <= 0) {
            return rocket::rgba_color::blank();
        }

        const float clamped_u = std::clamp(u, 0.f, 0.999999f);
        const float clamped_v = std::clamp(v, 0.f, 0.999999f);
        const int x = std::clamp(static_cast<int>(clamped_u * texture.size.x), 0, texture.size.x - 1);
        const int y = std::clamp(static_cast<int>(clamped_v * texture.size.y), 0, texture.size.y - 1);
        const std::size_t index = static_cast<std::size_t>(y * texture.size.x + x) * texture.channels;

        if (texture.alpha_mask) {
            const uint8_t alpha = texture.pixels[index];
            return {
                tint.x,
                tint.y,
                tint.z,
                static_cast<uint8_t>((static_cast<uint16_t>(alpha) * tint.w) / 255)
            };
        }

        rocket::rgba_color color = rocket::rgba_color::blank();
        color.x = texture.pixels[index + 0];
        color.y = texture.channels >= 2 ? texture.pixels[index + 1] : texture.pixels[index + 0];
        color.z = texture.channels >= 3 ? texture.pixels[index + 2] : texture.pixels[index + 0];
        color.w = texture.channels >= 4 ? texture.pixels[index + 3] : 255;
        return color;
    }

    static void blend_pixel(
        std::vector<rocket::rgba_color> &framebuffer,
        int width,
        int height,
        int x,
        int y,
        rocket::rgba_color src
    ) {
        if (x < 0 || y < 0 || x >= width || y >= height) {
            return;
        }

        rocket::rgba_color &dst = framebuffer[static_cast<std::size_t>(y * width + x)];
        const float src_a = src.w / 255.f;
        const float inv_a = 1.f - src_a;

        dst.x = static_cast<uint8_t>(std::clamp(src.x * src_a + dst.x * inv_a, 0.f, 255.f));
        dst.y = static_cast<uint8_t>(std::clamp(src.y * src_a + dst.y * inv_a, 0.f, 255.f));
        dst.z = static_cast<uint8_t>(std::clamp(src.z * src_a + dst.z * inv_a, 0.f, 255.f));
        dst.w = static_cast<uint8_t>(std::clamp((src_a + (dst.w / 255.f) * inv_a) * 255.f, 0.f, 255.f));
    }

    [[nodiscard]] static bool pixel_in_scissor(const rge_vk_native_state_t &state, float x, float y) {
        if (!state.scissor_rect.has_value()) {
            return true;
        }
        const auto &rect = *state.scissor_rect;
        return x >= rect.pos.x &&
               y >= rect.pos.y &&
               x < rect.pos.x + rect.size.x &&
               y < rect.pos.y + rect.size.y;
    }

    [[nodiscard]] static bool rounded_rect_contains(
        const rocket::vec2f_t &size,
        float roundedness,
        float local_x,
        float local_y
    ) {
        if (roundedness <= 0.f) {
            return local_x >= 0.f && local_y >= 0.f && local_x <= size.x && local_y <= size.y;
        }

        const float radius = std::min(size.x, size.y) * 0.5f * std::clamp(roundedness, 0.f, 1.f);
        const float cx = local_x - size.x * 0.5f;
        const float cy = local_y - size.y * 0.5f;
        const float qx = std::abs(cx) - (size.x * 0.5f - radius);
        const float qy = std::abs(cy) - (size.y * 0.5f - radius);
        const float dx = std::max(qx, 0.f);
        const float dy = std::max(qy, 0.f);
        return (std::min(std::max(qx, qy), 0.f) + std::sqrt(dx * dx + dy * dy)) <= radius;
    }

    static void raster_rectangle(
        rocket::vulkan_renderer_2d *renderer,
        rocket::fbounding_box rect,
        rocket::rgba_color color,
        float rotation,
        float roundedness
    ) {
        auto &state = vk_state(renderer);
        ensure_framebuffer_storage(renderer);

        const int width = static_cast<int>(to_vk_extent(renderer->get_viewport_size()).width);
        const int height = static_cast<int>(to_vk_extent(renderer->get_viewport_size()).height);

        const rocket::vec2f_t center = {
            rect.pos.x + rect.size.x * 0.5f,
            rect.pos.y + rect.size.y * 0.5f
        };
        const float radians = -rotation * std::numbers::pi_v<float> / 180.f;
        const float cs = std::cos(radians);
        const float sn = std::sin(radians);
        const float forward_cs = std::cos(-radians);
        const float forward_sn = std::sin(-radians);

        const std::array<rocket::vec2f_t, 4> corners = {{
            rect.pos,
            { rect.pos.x + rect.size.x, rect.pos.y },
            { rect.pos.x + rect.size.x, rect.pos.y + rect.size.y },
            { rect.pos.x, rect.pos.y + rect.size.y }
        }};

        float min_x = std::numeric_limits<float>::max();
        float max_x = std::numeric_limits<float>::lowest();
        float min_y = std::numeric_limits<float>::max();
        float max_y = std::numeric_limits<float>::lowest();
        for (const auto &corner : corners) {
            const float dx = corner.x - center.x;
            const float dy = corner.y - center.y;
            const float rx = dx * forward_cs - dy * forward_sn + center.x;
            const float ry = dx * forward_sn + dy * forward_cs + center.y;
            min_x = std::min(min_x, rx);
            max_x = std::max(max_x, rx);
            min_y = std::min(min_y, ry);
            max_y = std::max(max_y, ry);
        }

        const int start_x = std::max(0, static_cast<int>(std::floor(min_x)));
        const int end_x = std::min(width - 1, static_cast<int>(std::ceil(max_x)));
        const int start_y = std::max(0, static_cast<int>(std::floor(min_y)));
        const int end_y = std::min(height - 1, static_cast<int>(std::ceil(max_y)));

        for (int y = start_y; y <= end_y; ++y) {
            for (int x = start_x; x <= end_x; ++x) {
                if (!pixel_in_scissor(state, static_cast<float>(x), static_cast<float>(y))) {
                    continue;
                }

                const float px = static_cast<float>(x) + 0.5f - center.x;
                const float py = static_cast<float>(y) + 0.5f - center.y;
                const float local_x = px * cs - py * sn + rect.size.x * 0.5f;
                const float local_y = px * sn + py * cs + rect.size.y * 0.5f;

                if (rounded_rect_contains(rect.size, roundedness, local_x, local_y)) {
                    blend_pixel(state.framebuffer, width, height, x, y, color);
                }
            }
        }
    }

    static void raster_textured_quad(
        rocket::vulkan_renderer_2d *renderer,
        const rocket::vk_texture_t &texture,
        rocket::fbounding_box rect,
        rocket::vec2f_t texture_origin,
        rocket::vec2f_t texture_size,
        float rotation,
        float roundedness,
        rocket::rgba_color tint = rocket::rgba_color::white()
    ) {
        auto &state = vk_state(renderer);
        ensure_framebuffer_storage(renderer);

        const int width = static_cast<int>(to_vk_extent(renderer->get_viewport_size()).width);
        const int height = static_cast<int>(to_vk_extent(renderer->get_viewport_size()).height);

        const rocket::vec2f_t center = {
            rect.pos.x + rect.size.x * 0.5f,
            rect.pos.y + rect.size.y * 0.5f
        };
        const float radians = -rotation * std::numbers::pi_v<float> / 180.f;
        const float cs = std::cos(radians);
        const float sn = std::sin(radians);
        const float forward_cs = std::cos(-radians);
        const float forward_sn = std::sin(-radians);

        const std::array<rocket::vec2f_t, 4> corners = {{
            rect.pos,
            { rect.pos.x + rect.size.x, rect.pos.y },
            { rect.pos.x + rect.size.x, rect.pos.y + rect.size.y },
            { rect.pos.x, rect.pos.y + rect.size.y }
        }};

        float min_x = std::numeric_limits<float>::max();
        float max_x = std::numeric_limits<float>::lowest();
        float min_y = std::numeric_limits<float>::max();
        float max_y = std::numeric_limits<float>::lowest();
        for (const auto &corner : corners) {
            const float dx = corner.x - center.x;
            const float dy = corner.y - center.y;
            const float rx = dx * forward_cs - dy * forward_sn + center.x;
            const float ry = dx * forward_sn + dy * forward_cs + center.y;
            min_x = std::min(min_x, rx);
            max_x = std::max(max_x, rx);
            min_y = std::min(min_y, ry);
            max_y = std::max(max_y, ry);
        }

        const int start_x = std::max(0, static_cast<int>(std::floor(min_x)));
        const int end_x = std::min(width - 1, static_cast<int>(std::ceil(max_x)));
        const int start_y = std::max(0, static_cast<int>(std::floor(min_y)));
        const int end_y = std::min(height - 1, static_cast<int>(std::ceil(max_y)));

        const rocket::vec2f_t source_size = {
            std::max(1.f, texture_size.x),
            std::max(1.f, texture_size.y)
        };

        for (int y = start_y; y <= end_y; ++y) {
            for (int x = start_x; x <= end_x; ++x) {
                if (!pixel_in_scissor(state, static_cast<float>(x), static_cast<float>(y))) {
                    continue;
                }

                const float px = static_cast<float>(x) + 0.5f - center.x;
                const float py = static_cast<float>(y) + 0.5f - center.y;
                const float local_x = px * cs - py * sn + rect.size.x * 0.5f;
                const float local_y = px * sn + py * cs + rect.size.y * 0.5f;

                if (local_x < 0.f || local_y < 0.f || local_x > rect.size.x || local_y > rect.size.y) {
                    continue;
                }
                if (!rounded_rect_contains(rect.size, roundedness, local_x, local_y)) {
                    continue;
                }

                const float u = (texture_origin.x + (local_x / std::max(rect.size.x, 1.f)) * source_size.x) / std::max(1, texture.size.x);
                const float v = (texture_origin.y + (local_y / std::max(rect.size.y, 1.f)) * source_size.y) / std::max(1, texture.size.y);
                const auto sampled = sample_texture_nearest(texture, u, v, tint);
                blend_pixel(state.framebuffer, width, height, x, y, sampled);
            }
        }
    }

    static void raster_circle(
        rocket::vulkan_renderer_2d *renderer,
        rocket::vec2f_t pos,
        float radius,
        rocket::rgba_color color,
        int thickness
    ) {
        auto &state = vk_state(renderer);
        ensure_framebuffer_storage(renderer);

        const int width = static_cast<int>(to_vk_extent(renderer->get_viewport_size()).width);
        const int height = static_cast<int>(to_vk_extent(renderer->get_viewport_size()).height);
        const int start_x = std::max(0, static_cast<int>(std::floor(pos.x - radius)));
        const int end_x = std::min(width - 1, static_cast<int>(std::ceil(pos.x + radius)));
        const int start_y = std::max(0, static_cast<int>(std::floor(pos.y - radius)));
        const int end_y = std::min(height - 1, static_cast<int>(std::ceil(pos.y + radius)));

        const float inner_radius = thickness > 0 ? std::max(0.f, radius - static_cast<float>(thickness)) : 0.f;
        const float outer_sq = radius * radius;
        const float inner_sq = inner_radius * inner_radius;

        for (int y = start_y; y <= end_y; ++y) {
            for (int x = start_x; x <= end_x; ++x) {
                if (!pixel_in_scissor(state, static_cast<float>(x), static_cast<float>(y))) {
                    continue;
                }

                const float dx = static_cast<float>(x) + 0.5f - pos.x;
                const float dy = static_cast<float>(y) + 0.5f - pos.y;
                const float dist_sq = dx * dx + dy * dy;
                const bool inside_outer = dist_sq <= outer_sq;
                const bool inside_inner = thickness > 0 ? dist_sq < inner_sq : false;
                if (inside_outer && !inside_inner) {
                    blend_pixel(state.framebuffer, width, height, x, y, color);
                }
            }
        }
    }

    static float edge_function(const rocket::vec2f_t &a, const rocket::vec2f_t &b, const rocket::vec2f_t &c) {
        return (c.x - a.x) * (b.y - a.y) - (c.y - a.y) * (b.x - a.x);
    }

    static void raster_triangle(
        rocket::vulkan_renderer_2d *renderer,
        const rocket::vec2f_t &a,
        const rocket::vec2f_t &b,
        const rocket::vec2f_t &c,
        rocket::rgba_color color
    ) {
        auto &state = vk_state(renderer);
        ensure_framebuffer_storage(renderer);

        const int width = static_cast<int>(to_vk_extent(renderer->get_viewport_size()).width);
        const int height = static_cast<int>(to_vk_extent(renderer->get_viewport_size()).height);
        const int min_x = std::max(0, static_cast<int>(std::floor(std::min({ a.x, b.x, c.x }))));
        const int max_x = std::min(width - 1, static_cast<int>(std::ceil(std::max({ a.x, b.x, c.x }))));
        const int min_y = std::max(0, static_cast<int>(std::floor(std::min({ a.y, b.y, c.y }))));
        const int max_y = std::min(height - 1, static_cast<int>(std::ceil(std::max({ a.y, b.y, c.y }))));

        const float area = edge_function(a, b, c);
        if (std::abs(area) < std::numeric_limits<float>::epsilon()) {
            return;
        }

        for (int y = min_y; y <= max_y; ++y) {
            for (int x = min_x; x <= max_x; ++x) {
                if (!pixel_in_scissor(state, static_cast<float>(x), static_cast<float>(y))) {
                    continue;
                }

                const rocket::vec2f_t p = {
                    static_cast<float>(x) + 0.5f,
                    static_cast<float>(y) + 0.5f
                };
                const float w0 = edge_function(b, c, p);
                const float w1 = edge_function(c, a, p);
                const float w2 = edge_function(a, b, p);
                if ((w0 >= 0.f && w1 >= 0.f && w2 >= 0.f && area > 0.f) ||
                    (w0 <= 0.f && w1 <= 0.f && w2 <= 0.f && area < 0.f)) {
                    blend_pixel(state.framebuffer, width, height, x, y, color);
                }
            }
        }
    }

    static void raster_polygon(
        rocket::vulkan_renderer_2d *renderer,
        rocket::vec2f_t pos,
        float radius,
        rocket::rgba_color color,
        int sides,
        float rotation
    ) {
        const int segment_count = std::max(3, sides);
        std::vector<rocket::vec2f_t> points;
        points.reserve(static_cast<std::size_t>(segment_count));

        const float rotation_rad = rotation * std::numbers::pi_v<float> / 180.f;
        for (int i = 0; i < segment_count; ++i) {
            const float angle = rotation_rad + (static_cast<float>(i) / segment_count) * 2.f * std::numbers::pi_v<float>;
            points.push_back({
                pos.x + std::cos(angle) * radius,
                pos.y + std::sin(angle) * radius
            });
        }

        for (int i = 1; i < segment_count - 1; ++i) {
            raster_triangle(renderer, points[0], points[i], points[i + 1], color);
        }
    }
}

namespace rocket {
    api_object_t vulkan_renderer_2d::allocate_object_handle() {
        return ++this->impl->current_object_handle;
    }

    vulkan_renderer_2d::vulkan_renderer_2d(window_backend_i *window, int fps, renderer_flags_t flags) {
        r_assert(window != nullptr);
        r_assert(fps != 0);

        this->impl = new renderer_2d_impl_t;
        this->impl->obj = this;
        this->bk_impl = new vulkan_renderer_2d_impl_t;
        this->bk_impl->native_state = new rge_vk_native_state_t;
        this->window = window;
        this->fps = fps;
        this->flags = flags;
        window->wbi_impl->bound_renderer2d = this;

        const auto cli_args = util::get_clistate();
        if (cli_args.framerate != -1) {
            this->fps = cli_args.framerate;
        } else {
            this->fps = 2147483647;
        }

        if (flags.share_renderer_as_global) {
            util::set_global_renderer_2d(this);
        }

        this->vsync = window->flags.vsync;

        initialize_vulkan_backend(this);
        ensure_framebuffer_storage(this);

        if (flags.show_splash && !this->splash_shown) {
            this->show_splash();
        }
    }

    vulkan_renderer_2d::gfx_chk_result vulkan_renderer_2d::check_graphics_settings(rocket::vec2f_t pos, rocket::vec2f_t sz) {
        if (!this->frame_started) {
            return gfx_chk_result::not_drawable;
        }
        if (!this->graphics_settings.viewport_culling) {
            return gfx_chk_result::drawable;
        }
        if (pos == rocket::vec2f_t { -1.f, -1.f } || sz == rocket::vec2f_t { -1.f, -1.f }) {
            return gfx_chk_result::drawable;
        }

        const rocket::fbounding_box object_rect = { pos, sz };
        const rocket::fbounding_box viewport_rect = { { 0.f, 0.f }, this->get_viewport_size() };
        return object_rect.intersects(viewport_rect) ? gfx_chk_result::drawable : gfx_chk_result::not_drawable;
    }

    api_object_t vulkan_renderer_2d::upload_font_texture_to_gpu(rocket::vec2i_t size, const std::vector<uint8_t> &bitmap) {
        api_object_t handle = ++this->impl->current_object_handle;
        vk_object_t object {};
        object.type = vk_object_type_t::texture;

        vk_texture_t texture {};
        texture.size = size;
        texture.channels = 1;
        texture.alpha_mask = true;
        texture.pixels = bitmap;
        object.value = std::move(texture);

        this->bk_impl->objects[handle] = std::move(object);
        return handle;
    }

    void vulkan_renderer_2d::clean_gpu_resource(api_object_t object_handle) {
        auto it = this->bk_impl->objects.find(object_handle);
        if (it == this->bk_impl->objects.end()) {
            return;
        }

        if (it->second.type == vk_object_type_t::shader) {
            destroy_shader_native_pipeline(vk_state(this), it->second);
        }
        this->bk_impl->objects.erase(it);
    }

    bool vulkan_renderer_2d::has_frame_began() {
        return this->frame_started;
    }

    void vulkan_renderer_2d::begin_frame() {
        recreate_swapchain_if_needed(this);
        recreate_overlay_if_needed(this);
        ensure_framebuffer_storage(this);

        this->frame_started = true;
        this->frame_start_time = clock::now();
        if (this->frame_counter == 0) {
            this->delta_time = 0.0;
        } else {
            this->delta_time = std::chrono::duration<double>(this->frame_start_time - this->last_time).count();
        }
        this->last_time = this->frame_start_time;
        vk_state(this).queued_shaders.clear();
    }

    void vulkan_renderer_2d::show_splash() {
        const auto cli_args = util::get_clistate();
        if (cli_args.nosplash) {
            this->splash_shown = true;
            return;
        }
        this->splash_shown = true;

        constexpr float duration = 16384.f;
        auto tween = tweeny::from(0).to(0).during(duration / 2).via(tweeny::easing::cubicOut);

        std::vector<uint8_t> splash_png(splash_screen_png, splash_screen_png + splash_screen_png_len);
        int width = 0;
        int height = 0;
        int channels = 0;
        stbi_uc *img_data = stbi_load_from_memory(
            splash_png.data(),
            static_cast<int>(splash_png.size()),
            &width,
            &height,
            &channels,
            4
        );
        if (img_data == nullptr) {
            rocket::log("failed to load embedded splash texture", "vulkan_renderer_2d", "show_splash", "error");
            return;
        }

        auto texture = std::make_shared<texture_t>();
        texture->size = { width, height };
        texture->channels = 4;
        texture->data.assign(img_data, img_data + (width * height * 4));
        stbi_image_free(img_data);

        bool final_stage = false;
        bool splash_finished = false;
        while (window->is_running() && !splash_finished) {
            this->begin_frame();
            this->clear(rgba_color::black());
            {
                const float alpha = tween.step(static_cast<float>(this->get_delta_time()));
                const float center_x = this->get_viewport_size().x / 2.f - window->get_size().y / 2.f;
                this->draw_texture(texture, { { center_x, 0.f }, { static_cast<float>(window->get_size().y), static_cast<float>(window->get_size().y) } });

                rocket::text_t version_text = { "Version: " ROCKETGE__VERSION, 24, rgb_color::white(), rGE__FONT_DEFAULT_MONOSPACED };
                this->draw_rectangle({ 0.f, 0.f }, this->get_viewport_size(), { 0, 0, 0, static_cast<uint8_t>(alpha) });
                this->draw_text(version_text, { 0.f, this->get_viewport_size().y - version_text.measure().y });

                if (tween.progress() >= 1.f) {
                    if (final_stage) {
                        splash_finished = true;
                    }
                    final_stage = true;
                    tween = tweeny::from(0).to(255).during(duration).via(tweeny::easing::cubicOut);
                }
            }
            this->end_frame();
            this->window->poll_events();
        }
    }

    void vulkan_renderer_2d::begin_render_mode(render_mode_t mode) {
        this->active_render_modes.push_back(mode);
    }

    std::vector<rgba_color> vulkan_renderer_2d::get_framebuffer() {
        ensure_framebuffer_storage(this);
        return vk_state(this).framebuffer;
    }

    void vulkan_renderer_2d::push_framebuffer(const std::vector<rgba_color> &framebuffer) {
        ensure_framebuffer_storage(this);
        auto &dst = vk_state(this).framebuffer;
        if (framebuffer.size() == dst.size()) {
            dst = framebuffer;
        }
    }

    vec2f_t vulkan_renderer_2d::get_viewport_size() {
        return resolved_viewport_size(this);
    }

    void vulkan_renderer_2d::begin_scissor_mode(rocket::fbounding_box rect) {
        vk_state(this).scissor_rect = rect;
    }

    void vulkan_renderer_2d::begin_scissor_mode(rocket::vec2f_t pos, rocket::vec2f_t size) {
        this->begin_scissor_mode({ pos, size });
    }

    void vulkan_renderer_2d::begin_scissor_mode(float x, float y, float sx, float sy) {
        this->begin_scissor_mode({ { x, y }, { sx, sy } });
    }

    void vulkan_renderer_2d::clear(rocket::rgba_color color) {
        ensure_framebuffer_storage(this);
        std::fill(vk_state(this).framebuffer.begin(), vk_state(this).framebuffer.end(), color);
        if (flags.share_renderer_as_global) {
            __rallframestart();
        }
    }

    void vulkan_renderer_2d::draw_shader(const shader_i &shader) {
        if (this->check_graphics_settings({ -1.f, -1.f }, { -1.f, -1.f }) == gfx_chk_result::not_drawable) {
            rgl::add_frame_metrics_data_skipped_drawcalls(1);
            return;
        }

        const auto *vk_shader = dynamic_cast<const vulkan_shader_t*>(&shader);
        if (vk_shader == nullptr || vk_shader->hdl == 0) {
            rocket::log("attempted to draw a non-vulkan shader on the Vulkan backend", "vulkan_renderer_2d", "draw_shader", "error");
            return;
        }

        vk_state(this).queued_shaders.push_back(vk_shader->hdl);
        rgl::add_frame_metrics_data_drawcalls(1);
        rgl::add_frame_metrics_data_tricount(2);
    }

    void vulkan_renderer_2d::draw_rectangle(rocket::fbounding_box rect, rocket::rgba_color color, float rotation, float roundedness, bool lines) {
        if (this->check_graphics_settings(rect.pos, rect.size) == gfx_chk_result::not_drawable) {
            rgl::add_frame_metrics_data_skipped_drawcalls(1);
            return;
        }

        if (lines) {
            constexpr float thickness = 1.f;
            this->draw_rectangle({ rect.pos, { rect.size.x, thickness } }, color);
            this->draw_rectangle({ { rect.pos.x, rect.pos.y + rect.size.y - thickness }, { rect.size.x, thickness } }, color);
            this->draw_rectangle({ rect.pos, { thickness, rect.size.y } }, color);
            this->draw_rectangle({ { rect.pos.x + rect.size.x - thickness, rect.pos.y }, { thickness, rect.size.y } }, color);
            return;
        }

        raster_rectangle(this, rect, color, rotation, roundedness);
        rgl::add_frame_metrics_data_drawcalls(1);
        rgl::add_frame_metrics_data_tricount(2);
    }

    void vulkan_renderer_2d::draw_rectangle(rocket::vec2f_t pos, rocket::vec2f_t size, rocket::rgba_color color, float rotation, float roundedness, bool lines) {
        this->draw_rectangle({ pos, size }, color, rotation, roundedness, lines);
    }

    void vulkan_renderer_2d::draw_circle(rocket::vec2f_t pos, float radius, rocket::rgba_color color, int thickness) {
        const rocket::vec2f_t center_pos = {
            pos.x - radius,
            pos.y - radius
        };
        if (this->check_graphics_settings(center_pos, { radius * 2.f, radius * 2.f }) == gfx_chk_result::not_drawable) {
            rgl::add_frame_metrics_data_skipped_drawcalls(1);
            return;
        }

        raster_circle(this, pos, radius, color, thickness);
        rgl::add_frame_metrics_data_drawcalls(1);
        rgl::add_frame_metrics_data_tricount(std::max(2, static_cast<int>(radius)));
    }

    void vulkan_renderer_2d::draw_polygon(rocket::vec2f_t pos, float radius, rocket::rgba_color color, int sides, float rotation) {
        const rocket::vec2f_t center_pos = {
            pos.x - radius,
            pos.y - radius
        };
        if (this->check_graphics_settings(center_pos, { radius * 2.f, radius * 2.f }) == gfx_chk_result::not_drawable) {
            rgl::add_frame_metrics_data_skipped_drawcalls(1);
            return;
        }

        raster_polygon(this, pos, radius, color, sides, rotation);
        rgl::add_frame_metrics_data_drawcalls(1);
        rgl::add_frame_metrics_data_tricount(std::max(1, sides - 2));
    }

    void vulkan_renderer_2d::draw_texture(std::shared_ptr<rocket::texture_t> texture, rocket::fbounding_box rect, float rotation, float roundedness) {
        if (this->check_graphics_settings(rect.pos, rect.size) == gfx_chk_result::not_drawable) {
            rgl::add_frame_metrics_data_skipped_drawcalls(1);
            return;
        }
        if (texture == nullptr) {
            return;
        }

        this->make_ready_texture(texture);
        auto object_it = this->bk_impl->objects.find(texture->hdl);
        if (object_it == this->bk_impl->objects.end()) {
            return;
        }
        const auto &vk_texture = std::get<vk_texture_t>(object_it->second.value);
        raster_textured_quad(
            this,
            vk_texture,
            rect,
            { 0.f, 0.f },
            { static_cast<float>(vk_texture.size.x), static_cast<float>(vk_texture.size.y) },
            rotation,
            roundedness
        );
        rgl::add_frame_metrics_data_drawcalls(1);
        rgl::add_frame_metrics_data_tricount(2);
    }

    void vulkan_renderer_2d::draw_atlas_texture(
        std::shared_ptr<rocket::texture_t> texture,
        rocket::fbounding_box rect,
        rocket::vec2f_t texture_position_in_atlas,
        rocket::vec2f_t texture_size_in_atlas,
        float rotation,
        float roundedness
    ) {
        if (this->check_graphics_settings(rect.pos, rect.size) == gfx_chk_result::not_drawable) {
            rgl::add_frame_metrics_data_skipped_drawcalls(1);
            return;
        }
        if (texture == nullptr) {
            return;
        }

        this->make_ready_texture(texture);
        auto object_it = this->bk_impl->objects.find(texture->hdl);
        if (object_it == this->bk_impl->objects.end()) {
            return;
        }
        const auto &vk_texture = std::get<vk_texture_t>(object_it->second.value);
        raster_textured_quad(
            this,
            vk_texture,
            rect,
            texture_position_in_atlas,
            texture_size_in_atlas,
            rotation,
            roundedness
        );
        rgl::add_frame_metrics_data_drawcalls(1);
        rgl::add_frame_metrics_data_tricount(2);
    }

    void vulkan_renderer_2d::make_ready_texture(std::shared_ptr<rocket::texture_t> texture) {
        if (texture == nullptr || texture->hdl != 0) {
            return;
        }

        api_object_t handle = ++this->impl->current_object_handle;
        vk_object_t object {};
        object.type = vk_object_type_t::texture;

        vk_texture_t vk_texture {};
        vk_texture.size = texture->size;
        vk_texture.channels = texture->channels;
        vk_texture.alpha_mask = false;
        vk_texture.pixels = texture->data;
        object.value = std::move(vk_texture);

        this->bk_impl->objects[handle] = std::move(object);
        texture->hdl = handle;
    }

    void vulkan_renderer_2d::draw_text(const rocket::text_t &text_value, vec2f_t position) {
        rocket::text_t text = text_value;
        if (check_graphics_settings(position, text.measure()) == gfx_chk_result::not_drawable) {
            rgl::add_frame_metrics_data_skipped_drawcalls(static_cast<int>(text.text.size()));
            return;
        }
        if (util::get_clistate().notext) {
            return;
        }

        if (text.font == nullptr) {
            text.font = font_t::font_default(static_cast<int>(text.size));
        } else if (text.font.get() == reinterpret_cast<font_t*>(0x01)) {
            text.font = font_t::font_default_monospace(static_cast<int>(text.size));
        }
        if (text.font == nullptr || text.font->hdl == 0) {
            return;
        }

        auto font_it = this->bk_impl->objects.find(text.font->hdl);
        if (font_it == this->bk_impl->objects.end()) {
            return;
        }

        const auto &font_texture = std::get<vk_texture_t>(font_it->second.value);
        float x = position.x;
        float y = position.y;
        for (const char raw_ch : text.text) {
            const unsigned char ch = static_cast<unsigned char>(raw_ch);
            if (ch < 32 || ch > 127) {
                continue;
            }

            stbtt_aligned_quad quad {};
            stbtt_GetBakedQuad(text.font->cdata->a, text.font->sttex_size.x, text.font->sttex_size.y, ch - 32, &x, &y, &quad, 1);
            const rocket::fbounding_box rect = {
                { quad.x0, quad.y0 },
                { quad.x1 - quad.x0, quad.y1 - quad.y0 }
            };

            raster_textured_quad(
                this,
                font_texture,
                rect,
                {
                    quad.s0 * static_cast<float>(font_texture.size.x),
                    quad.t0 * static_cast<float>(font_texture.size.y)
                },
                {
                    (quad.s1 - quad.s0) * static_cast<float>(font_texture.size.x),
                    (quad.t1 - quad.t0) * static_cast<float>(font_texture.size.y)
                },
                0.f,
                0.f,
                static_cast<rocket::rgba_color>(text.color)
            );
        }
        rgl::add_frame_metrics_data_drawcalls(1);
        rgl::add_frame_metrics_data_tricount(static_cast<int>(text.text.size()) * 2);
    }

    void vulkan_renderer_2d::draw_pixel(rocket::vec2f_t pos, rocket::rgba_color color) {
        if (this->check_graphics_settings(pos, { 1.f, 1.f }) == gfx_chk_result::not_drawable) {
            rgl::add_frame_metrics_data_skipped_drawcalls(1);
            return;
        }
        ensure_framebuffer_storage(this);
        auto &state = vk_state(this);
        const int width = static_cast<int>(to_vk_extent(this->get_viewport_size()).width);
        const int height = static_cast<int>(to_vk_extent(this->get_viewport_size()).height);
        blend_pixel(state.framebuffer, width, height, static_cast<int>(pos.x), static_cast<int>(pos.y), color);
        rgl::add_frame_metrics_data_drawcalls(1);
        rgl::add_frame_metrics_data_tricount(1);
    }

    void vulkan_renderer_2d::draw_fps(vec2f_t pos) {
        const std::string fps_text = "FPS: " + std::to_string(static_cast<int>(std::round(get_current_fps())));
        rocket::text_t fps = rocket::text_t(fps_text, 24, rocket::rgb_color::green());
        this->draw_text(fps, pos);
    }

    void vulkan_renderer_2d::set_wireframe(bool enabled) {
        this->wireframe = enabled;
    }

    void vulkan_renderer_2d::set_vsync(bool enabled) {
        this->vsync = enabled;
        vk_state(this).swapchain_needs_rebuild = true;
    }

    void vulkan_renderer_2d::set_fps(int fps_value) {
        this->fps = fps_value;
    }

    void vulkan_renderer_2d::end_scissor_mode() {
        vk_state(this).scissor_rect.reset();
    }

    void vulkan_renderer_2d::end_render_mode(render_mode_t mode) {
        for (auto it = this->active_render_modes.begin(); it != this->active_render_modes.end(); ++it) {
            if (*it == mode) {
                this->active_render_modes.erase(it);
                break;
            }
        }
    }

    void vulkan_renderer_2d::end_frame() {
        if (!this->frame_started) {
            return;
        }

        if (flags.share_renderer_as_global) {
            __rallframeend();
        }
        if (util::get_clistate().debugoverlay) {
            util::draw_debug_overlay(this);
        }

        recreate_swapchain_if_needed(this);
        recreate_overlay_if_needed(this);
        ensure_framebuffer_storage(this);

        auto &state = vk_state(this);
        auto &frame = state.frames[state.current_frame];
        vk_expect(vkWaitForFences(state.device, 1, &frame.in_flight, VK_TRUE, UINT64_MAX), "vkWaitForFences");

        uint32_t image_index = 0;
        VkResult acquire_result = vkAcquireNextImageKHR(
            state.device,
            state.swapchain,
            UINT64_MAX,
            frame.image_available,
            VK_NULL_HANDLE,
            &image_index
        );
        if (acquire_result == VK_ERROR_OUT_OF_DATE_KHR) {
            state.swapchain_needs_rebuild = true;
            this->frame_started = false;
            recreate_swapchain_if_needed(this);
            return;
        }
        if (acquire_result != VK_SUCCESS && acquire_result != VK_SUBOPTIMAL_KHR) {
            vk_expect(acquire_result, "vkAcquireNextImageKHR");
        }

        if (state.images_in_flight[image_index] != VK_NULL_HANDLE) {
            vk_expect(vkWaitForFences(state.device, 1, &state.images_in_flight[image_index], VK_TRUE, UINT64_MAX), "vkWaitForFences(image)");
        }
        state.images_in_flight[image_index] = frame.in_flight;

        vk_expect(vkResetFences(state.device, 1, &frame.in_flight), "vkResetFences");
        record_command_buffer(this, frame.command_buffer, image_index);

        const VkPipelineStageFlags wait_stages[] = {
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
        };
        VkSubmitInfo submit_info { VK_STRUCTURE_TYPE_SUBMIT_INFO };
        submit_info.waitSemaphoreCount = 1;
        submit_info.pWaitSemaphores = &frame.image_available;
        submit_info.pWaitDstStageMask = wait_stages;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &frame.command_buffer;
        submit_info.signalSemaphoreCount = 1;
        submit_info.pSignalSemaphores = &frame.render_finished;

        vk_expect(vkQueueSubmit(state.graphics_queue, 1, &submit_info, frame.in_flight), "vkQueueSubmit");

        VkPresentInfoKHR present_info { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
        present_info.waitSemaphoreCount = 1;
        present_info.pWaitSemaphores = &frame.render_finished;
        present_info.swapchainCount = 1;
        present_info.pSwapchains = &state.swapchain;
        present_info.pImageIndices = &image_index;

        const VkResult present_result = vkQueuePresentKHR(state.present_queue, &present_info);
        if (present_result == VK_ERROR_OUT_OF_DATE_KHR || present_result == VK_SUBOPTIMAL_KHR) {
            state.swapchain_needs_rebuild = true;
        } else if (present_result != VK_SUCCESS) {
            vk_expect(present_result, "vkQueuePresentKHR");
        }

        this->frame_started = false;
        auto frame_end_time = clock::now();
        this->frame_counter++;
        state.current_frame = (state.current_frame + 1) % rge_vk_max_frames_in_flight;

        rgl::frame_metrics_t metrics = rgl::get_frame_metrics();
        if (metrics.drawcalls > rGL_MAX_RECOMMENDED_DRAWCALLS) {
            rocket::log("Too many drawcalls! (" + std::to_string(metrics.drawcalls) + ") Frames may suffer", "vulkan_renderer_2d", "end_frame", "warning");
        }
        if (metrics.tricount > rGL_MAX_RECOMMENDED_TRICOUNT) {
            rocket::log("Too many triangles! (" + std::to_string(metrics.tricount) + ") Frames may suffer", "vulkan_renderer_2d", "end_frame", "warning");
        }

        rgl::reset_frame_metrics();

        if (this->fps != rocket::fps_uncapped && !this->vsync) {
            const double frame_duration = std::chrono::duration<double>(frame_end_time - frame_start_time).count();
            if (this->fps == 0) {
                rocket::log("Target FPS 0 is too low", "vulkan_renderer_2d", "end_frame", "fatal");
                rocket::exit(1);
            }
            const double frametime_limit = 1.0 / this->fps;
            util::frame_timer_wait_for(
                frame_duration,
                frametime_limit,
                util::get_clistate().software_frame_timer,
                this->fps,
                this->frame_start_time
            );
            const double measured_time = frame_duration + std::chrono::duration<double>(clock::now() - frame_end_time).count();
            rgl::update_draw_metrics_data(static_cast<float>(measured_time), this->delta_time > 0.0 ? static_cast<float>(1.0 / this->delta_time) : 0.f);
        } else {
            const double frame_duration = std::chrono::duration<double>(frame_end_time - frame_start_time).count();
            rgl::update_draw_metrics_data(static_cast<float>(frame_duration), this->delta_time > 0.0 ? static_cast<float>(1.0 / this->delta_time) : 0.f);
        }
    }

    bool vulkan_renderer_2d::has_frame_ended() {
        return !this->frame_started;
    }

    void vulkan_renderer_2d::set_graphics_settings(graphics_settings_t graphics) {
        this->graphics_settings = graphics;
    }

    void vulkan_renderer_2d::set_viewport_size(vec2f_t size) {
        this->override_viewport_size = size;
        vk_state(this).overlay_needs_rebuild = true;
        ensure_framebuffer_storage(this);
    }

    void vulkan_renderer_2d::set_viewport_offset(vec2f_t zero_pos) {
        this->override_viewport_offset = zero_pos;
    }

    void vulkan_renderer_2d::set_camera(camera_2d *cam) {
        this->cam = cam;
        rocket::log("cameras are not implemented", "vulkan_renderer_2d", "set_camera", "fixme");
    }

    void vulkan_renderer_2d::close() {
        if (this->window != nullptr && this->window->wbi_impl != nullptr && this->window->wbi_impl->bound_renderer2d == this) {
            this->window->wbi_impl->bound_renderer2d = nullptr;
        }

        if (util::get_global_renderer_2d() == this) {
            util::set_global_renderer_2d(nullptr);
        }

        destroy_native_backend(this);
        shader_provider_reset();

        if (this->bk_impl != nullptr) {
            delete reinterpret_cast<rge_vk_native_state_t*>(this->bk_impl->native_state);
            this->bk_impl->native_state = nullptr;
            delete this->bk_impl;
            this->bk_impl = nullptr;
        }

        if (this->impl != nullptr) {
            delete this->impl;
            this->impl = nullptr;
        }

        this->window = nullptr;
    }

    bool vulkan_renderer_2d::get_wireframe() {
        return this->wireframe;
    }

    bool vulkan_renderer_2d::get_vsync() {
        return this->vsync;
    }

    int vulkan_renderer_2d::get_fps() {
        return this->fps;
    }

    double vulkan_renderer_2d::get_delta_time() {
        return this->delta_time;
    }

    uint64_t vulkan_renderer_2d::get_framecount() {
        return this->frame_counter;
    }

    int vulkan_renderer_2d::get_drawcalls() {
        return rgl::get_frame_metrics().drawcalls;
    }

    rgl::draw_metrics_t vulkan_renderer_2d::get_draw_metrics() {
        return rgl::get_draw_metrics();
    }

    const graphics_settings_t &vulkan_renderer_2d::get_graphics_settings() {
        return this->graphics_settings;
    }

    api_object_t vulkan_renderer_2d::get_framebuffer_texture() {
        ensure_framebuffer_storage(this);

        api_object_t handle = ++this->impl->current_object_handle;
        vk_object_t object {};
        object.type = vk_object_type_t::texture;

        vk_texture_t texture {};
        const auto viewport = to_vk_extent(this->get_viewport_size());
        texture.size = {
            static_cast<int32_t>(viewport.width),
            static_cast<int32_t>(viewport.height)
        };
        texture.channels = 4;
        texture.alpha_mask = false;
        texture.pixels.resize(vk_state(this).framebuffer.size() * sizeof(rgba_color));
        std::memcpy(texture.pixels.data(), vk_state(this).framebuffer.data(), texture.pixels.size());
        object.value = std::move(texture);
        this->bk_impl->objects[handle] = std::move(object);

        return handle;
    }

    camera_2d *vulkan_renderer_2d::get_camera() {
        return this->cam;
    }

    glm::mat4 vulkan_renderer_2d::get_camera_matrix() {
        return {};
    }

    float vulkan_renderer_2d::get_current_fps() {
        return rgl::get_draw_metrics().avg_fps;
    }

    render_cache_t *vulkan_renderer_2d::create_render_cache(std::function<void(renderer_2d_i *ren)> draw_cb) {
        (void) draw_cb;
        return nullptr;
    }

    void vulkan_renderer_2d::invalidate_render_cache(render_cache_t *c) {
        (void) c;
    }

    void vulkan_renderer_2d::begin_render_cache(render_cache_t *c) {
        (void) c;
    }

    void vulkan_renderer_2d::end_render_cache(render_cache_t *c) {
        (void) c;
    }

    void vulkan_renderer_2d::draw_render_cache(render_cache_t *c, rocket::vec2f_t pos, rocket::vec2f_t sz) {
        (void) c;
        (void) pos;
        (void) sz;
    }

    void vulkan_renderer_2d::draw_render_cache(render_cache_t *c, rocket::fbounding_box bbox) {
        (void) c;
        (void) bbox;
    }

    void vulkan_renderer_2d::destroy_render_cache(render_cache_t *&c) {
        c = nullptr;
    }

    vulkan_renderer_2d::~vulkan_renderer_2d() {
        this->close();
    }
}

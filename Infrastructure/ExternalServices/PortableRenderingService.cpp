#include "PortableRenderingService.h"
#include "../../Domain/ValueObjects/CharacterASCIIData.h"

#include <SDL.h>
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <limits>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

#ifdef __EMSCRIPTEN__
#include <GLES3/gl3.h>
#else
#include <SDL_vulkan.h>
#include <vulkan/vulkan.h>
#endif

namespace {
// 720p、1080p、1440p、4Kのいずれでも整数倍で拡大できる解像度
constexpr int LowWidth = 640;
constexpr int LowHeight = 360;
enum Shader { Object, Background, Spell, Shot, Explosion, Railgun, Font, Upscale, ShaderCount };
enum Blend { Alpha, Opaque, Additive, Smoke };
constexpr const char* ShaderNames[] = {"object", "background", "spell", "shot", "explosion", "railgun", "font", "upscale"};

/** @brief HLSLのb0に対応する96バイトの定数 */
struct Uniforms {
    Matrix4x4 matrix = Matrix4x4::Identity;
    ColorF color = ColorF::White();
    float time = 0;
    float shape = 0;
    float rotation = 0;
    float padding = 0;
};
static_assert(sizeof(Uniforms) == 96, "HLSL constant buffer layout changed");
static_assert(offsetof(Uniforms, color) == 64 && offsetof(Uniforms, time) == 80,
    "HLSL constant buffer offsets changed");

/** @brief 1回の描画に必要なGPU状態 */
struct Draw {
    Uniforms uniforms;
    Viewport viewport;
    Shader shader = Object;
    Blend blend = Alpha;
    int vertices = 4;
    bool strip = true;
    bool depth = false;
    bool depthWrite = false;
    bool cull = false;
};

/** @brief 配布物のシェーダーを読む @param name ファイル名 @return ファイル内容 */
std::string ReadShader(const std::string& name) {
    // 作業ディレクトリに依存せず、実行ファイル横のリソースを優先する
    char* base = SDL_GetBasePath();
    const std::string directory = base ? base : "";
    SDL_free(base);
    std::ifstream stream(directory + "Shaders/" + name, std::ios::binary);
    if (!stream) stream.open("Shaders/" + name, std::ios::binary);
    if (!stream) throw std::runtime_error("Cannot read shader: " + name);
    return {std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
}

/** @brief 描画領域をターゲット寸法へ写す @param area 元領域 @param width 出力幅 @param height 出力高さ @param sourceWidth 元幅 @param sourceHeight 元高さ @return 有効範囲に収めた領域 */
Viewport ScaledViewport(const Viewport& area, int width, int height, int sourceWidth, int sourceHeight) {
    const int left = std::clamp(static_cast<int>(static_cast<int64_t>(area.x) * width / sourceWidth), 0, width);
    const int top = std::clamp(static_cast<int>(static_cast<int64_t>(area.y) * height / sourceHeight), 0, height);
    const int right = std::clamp(static_cast<int>((static_cast<int64_t>(area.x) + area.width) * width / sourceWidth), left, width);
    const int bottom = std::clamp(static_cast<int>((static_cast<int64_t>(area.y) + area.height) * height / sourceHeight), top, height);
    return {left, top, right - left, bottom - top};
}
}

struct PortableRenderingService::Impl {
    SDL_Window* window = nullptr;
    int width = 0;
    int height = 0;
    bool retro = true;
    bool frameRetro = true;
    bool initialized = false;
    PipelineId pipeline = PipelineId::Object;
    Viewport viewport;
    std::vector<Draw> draws;
    size_t hudStart = std::numeric_limits<size_t>::max();

    /** @brief 現在のパイプライン状態で描画命令を作る @return 描画命令 */
    Draw MakeDraw() const {
        Draw draw;
        draw.viewport = viewport;
        draw.shader = pipeline == PipelineId::Background ? Background : pipeline == PipelineId::SpellCircle ? Spell : Object;
        draw.blend = pipeline == PipelineId::Background ? Opaque : Alpha;
        draw.depth = draw.depthWrite = draw.cull = pipeline == PipelineId::Model3D;
        return draw;
    }

    /** @brief HUDへ切り替わる位置を記録する */
    void MarkHud() {
        if (hudStart == std::numeric_limits<size_t>::max()) hudStart = draws.size();
    }

#ifdef __EMSCRIPTEN__
    SDL_GLContext context = nullptr;
    GLuint programs[ShaderCount]{};
    GLuint uniformBuffer = 0;
    GLuint vertexArray = 0;
    GLuint lowFramebuffer = 0;
    GLuint lowTexture = 0;
    GLuint lowDepth = 0;

    /** @brief GLSLをコンパイルする @param kind シェーダー種別 @param source ソース @return シェーダーID */
    GLuint Compile(GLenum kind, const std::string& source) {
        const GLuint shader = glCreateShader(kind);
        const char* text = source.c_str();
        glShaderSource(shader, 1, &text, nullptr);
        glCompileShader(shader);
        GLint success = GL_FALSE;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            std::array<char, 8192> log{};
            glGetShaderInfoLog(shader, static_cast<GLsizei>(log.size()), nullptr, log.data());
            glDeleteShader(shader);
            throw std::runtime_error(log.data());
        }
        return shader;
    }

    /** @brief WebGLのプログラムと低解像度ターゲットを作る */
    void InitializeGpu() {
        context = SDL_GL_CreateContext(window);
        if (!context) throw std::runtime_error(SDL_GetError());
        SDL_GL_MakeCurrent(window, context);
        SDL_GL_SetSwapInterval(1);
        for (int i = 0; i < ShaderCount; ++i) {
            const GLuint vertex = Compile(GL_VERTEX_SHADER, ReadShader(std::string(ShaderNames[i]) + ".vert.glsl"));
            GLuint fragment = 0;
            try { fragment = Compile(GL_FRAGMENT_SHADER, ReadShader(std::string(ShaderNames[i]) + ".frag.glsl")); }
            catch (...) { glDeleteShader(vertex); throw; }
            programs[i] = glCreateProgram();
            glAttachShader(programs[i], vertex);
            glAttachShader(programs[i], fragment);
            glLinkProgram(programs[i]);
            glDeleteShader(vertex);
            glDeleteShader(fragment);
            GLint success = GL_FALSE;
            glGetProgramiv(programs[i], GL_LINK_STATUS, &success);
            if (!success) {
                std::array<char, 8192> log{};
                glGetProgramInfoLog(programs[i], static_cast<GLsizei>(log.size()), nullptr, log.data());
                throw std::runtime_error(log.data());
            }
            // 変換ツールが生成する名前に依存せず、b0とテクスチャを接続する
            GLint count = 0;
            glGetProgramiv(programs[i], GL_ACTIVE_UNIFORM_BLOCKS, &count);
            for (GLint block = 0; block < count; ++block) glUniformBlockBinding(programs[i], block, 0);
            glUseProgram(programs[i]);
            glGetProgramiv(programs[i], GL_ACTIVE_UNIFORMS, &count);
            for (GLint u = 0; u < count; ++u) {
                std::array<char, 256> name{};
                GLenum type = 0;
                GLint size = 0;
                glGetActiveUniform(programs[i], u, static_cast<GLsizei>(name.size()), nullptr, &size, &type, name.data());
                if (type == GL_SAMPLER_2D) glUniform1i(glGetUniformLocation(programs[i], name.data()), 0);
            }
        }
        glGenVertexArrays(1, &vertexArray);
        glBindVertexArray(vertexArray);
        glGenBuffers(1, &uniformBuffer);
        glBindBuffer(GL_UNIFORM_BUFFER, uniformBuffer);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(Uniforms), nullptr, GL_STREAM_DRAW);
        glBindBufferBase(GL_UNIFORM_BUFFER, 0, uniformBuffer);
        glGenTextures(1, &lowTexture);
        glBindTexture(GL_TEXTURE_2D, lowTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, LowWidth, LowHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glGenRenderbuffers(1, &lowDepth);
        glBindRenderbuffer(GL_RENDERBUFFER, lowDepth);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, LowWidth, LowHeight);
        glGenFramebuffers(1, &lowFramebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, lowFramebuffer);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, lowTexture, 0);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, lowDepth);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            throw std::runtime_error("WebGL low resolution framebuffer is incomplete");
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    /** @brief WebGLリソースを解放する */
    void CleanupGpu() {
        if (!context) return;
        SDL_GL_MakeCurrent(window, context);
        for (GLuint program : programs) if (program) glDeleteProgram(program);
        glDeleteBuffers(1, &uniformBuffer);
        glDeleteVertexArrays(1, &vertexArray);
        glDeleteFramebuffers(1, &lowFramebuffer);
        glDeleteTextures(1, &lowTexture);
        glDeleteRenderbuffers(1, &lowDepth);
        SDL_GL_DeleteContext(context);
        context = nullptr;
    }

    /** @brief 1命令をWebGLへ発行する @param draw 描画命令 @param targetWidth 描画幅 @param targetHeight 描画高さ */
    void Execute(const Draw& draw, int targetWidth, int targetHeight) {
        const auto area = ScaledViewport(draw.viewport, targetWidth, targetHeight, width, height);
        if (!area.IsValid()) return;
        glViewport(area.x, targetHeight - area.y - area.height, area.width, area.height);
        glScissor(area.x, targetHeight - area.y - area.height, area.width, area.height);
        glEnable(GL_SCISSOR_TEST);
        if (draw.depth) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);
        glDepthMask(draw.depthWrite ? GL_TRUE : GL_FALSE);
        if (draw.cull) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        // 既存HLSLの表面はNDCで時計回り、Web用VSではYを反転しない
        glFrontFace(GL_CW);
        if (draw.blend == Opaque) glDisable(GL_BLEND);
        else {
            glEnable(GL_BLEND);
            glBlendEquation(GL_FUNC_ADD);
            glBlendFuncSeparate(GL_SRC_ALPHA, draw.blend == Additive ? GL_ONE : GL_ONE_MINUS_SRC_ALPHA,
                GL_ONE, draw.blend == Additive ? GL_ONE : draw.blend == Smoke ? GL_ONE_MINUS_SRC_ALPHA : GL_ZERO);
        }
        glUseProgram(programs[draw.shader]);
        glBindBuffer(GL_UNIFORM_BUFFER, uniformBuffer);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(Uniforms), &draw.uniforms);
        glDrawArrays(draw.strip ? GL_TRIANGLE_STRIP : GL_TRIANGLES, 0, draw.vertices);
    }

    /** @brief WebGLフレームを送信する */
    void Render() {
        SDL_GL_MakeCurrent(window, context);
        glBindVertexArray(vertexArray);
        glBindBufferBase(GL_UNIFORM_BUFFER, 0, uniformBuffer);
        const size_t boundary = frameRetro ? std::min(hudStart, draws.size()) : 0;
        glBindFramebuffer(GL_FRAMEBUFFER, frameRetro ? lowFramebuffer : 0);
        glDisable(GL_SCISSOR_TEST);
        glDepthMask(GL_TRUE);
        glClearColor(0.05f, 0.05f, 0.1f, 1.0f);
        glClearDepthf(1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        for (size_t i = 0; i < boundary; ++i) Execute(draws[i], LowWidth, LowHeight);
        if (frameRetro) {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glDisable(GL_SCISSOR_TEST);
            glDepthMask(GL_TRUE);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, lowTexture);
            Draw upscale;
            upscale.viewport = {0, 0, width, height};
            upscale.shader = Upscale;
            upscale.blend = Opaque;
            upscale.vertices = 3;
            upscale.strip = false;
            Execute(upscale, width, height);
        }
        for (size_t i = boundary; i < draws.size(); ++i) Execute(draws[i], width, height);
        SDL_GL_SwapWindow(window);
    }
#else
    struct Image {
        VkImage image = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        VkImageView view = VK_NULL_HANDLE;
    };
    VkInstance instance = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkPhysicalDevice physical = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue queue = VK_NULL_HANDLE;
    uint32_t queueFamily = 0;
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    VkFormat colorFormat = VK_FORMAT_UNDEFINED;
    VkFormat depthFormat = VK_FORMAT_UNDEFINED;
    VkExtent2D extent{};
    std::vector<VkImage> swapImages;
    std::vector<VkImageView> swapViews;
    std::vector<VkFramebuffer> framebuffers;
    std::vector<VkSemaphore> rendered;
    Image lowColor, lowDepth, screenDepth;
    VkFramebuffer lowFramebuffer = VK_NULL_HANDLE;
    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkCommandPool commandPool = VK_NULL_HANDLE;
    VkCommandBuffer command = VK_NULL_HANDLE;
    VkSemaphore acquired = VK_NULL_HANDLE;
    VkFence fence = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptorLayout = VK_NULL_HANDLE;
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkSampler sampler = VK_NULL_HANDLE;
    VkShaderModule shaders[ShaderCount][2]{};
    std::map<unsigned, VkPipeline> pipelines;
    VkBuffer uniformBuffer = VK_NULL_HANDLE;
    VkDeviceMemory uniformMemory = VK_NULL_HANDLE;
    void* uniformData = nullptr;
    VkDeviceSize uniformStride = 0;
    size_t uniformCapacity = 0;
    bool resizePending = true;

    /** @brief Vulkan失敗を呼出元へ伝える @param result API結果 @param operation 操作名 */
    static void Check(VkResult result, const char* operation) {
        if (result != VK_SUCCESS) throw std::runtime_error(std::string(operation) + ": " + std::to_string(result));
    }

    /** @brief 要求を満たすメモリ種別を探す @param bits 利用可能な種別 @param flags 必須属性 @return 種別番号 */
    uint32_t MemoryType(uint32_t bits, VkMemoryPropertyFlags flags) const {
        VkPhysicalDeviceMemoryProperties properties{};
        vkGetPhysicalDeviceMemoryProperties(physical, &properties);
        for (uint32_t i = 0; i < properties.memoryTypeCount; ++i)
            if ((bits & (1u << i)) && (properties.memoryTypes[i].propertyFlags & flags) == flags) return i;
        throw std::runtime_error("No compatible Vulkan memory type");
    }

    /** @brief 画像ビューを生成する @param image GPU画像 @param format 形式 @param aspect 使用面 @return ビュー */
    VkImageView CreateView(VkImage image, VkFormat format, VkImageAspectFlags aspect) {
        VkImageViewCreateInfo info{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        info.image = image;
        info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        info.format = format;
        info.subresourceRange = {aspect, 0, 1, 0, 1};
        VkImageView view = VK_NULL_HANDLE;
        Check(vkCreateImageView(device, &info, nullptr, &view), "vkCreateImageView");
        return view;
    }

    /** @brief GPU画像を生成する @param result 格納先 @param w 幅 @param h 高さ @param format 形式 @param usage 用途 @param aspect 使用面 */
    void CreateImage(Image& result, int w, int h, VkFormat format, VkImageUsageFlags usage, VkImageAspectFlags aspect) {
        VkImageCreateInfo info{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
        info.imageType = VK_IMAGE_TYPE_2D;
        info.format = format;
        info.extent = {static_cast<uint32_t>(w), static_cast<uint32_t>(h), 1};
        info.mipLevels = info.arrayLayers = 1;
        info.samples = VK_SAMPLE_COUNT_1_BIT;
        info.tiling = VK_IMAGE_TILING_OPTIMAL;
        info.usage = usage;
        info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        Check(vkCreateImage(device, &info, nullptr, &result.image), "vkCreateImage");
        VkMemoryRequirements requirements{};
        vkGetImageMemoryRequirements(device, result.image, &requirements);
        VkMemoryAllocateInfo allocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        allocation.allocationSize = requirements.size;
        allocation.memoryTypeIndex = MemoryType(requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        Check(vkAllocateMemory(device, &allocation, nullptr, &result.memory), "vkAllocateMemory(image)");
        Check(vkBindImageMemory(device, result.image, result.memory, 0), "vkBindImageMemory");
        result.view = CreateView(result.image, format, aspect);
    }

    /** @brief GPU画像を解放する @param image 対象 */
    void DestroyImage(Image& image) {
        if (image.view) vkDestroyImageView(device, image.view, nullptr);
        if (image.image) vkDestroyImage(device, image.image, nullptr);
        if (image.memory) vkFreeMemory(device, image.memory, nullptr);
        image = {};
    }

    /** @brief Vulkanデバイスと描画用オブジェクトを生成する */
    void InitializeGpu() {
        uint32_t count = 0;
        if (!SDL_Vulkan_GetInstanceExtensions(window, &count, nullptr)) throw std::runtime_error(SDL_GetError());
        std::vector<const char*> extensions(count);
        if (!SDL_Vulkan_GetInstanceExtensions(window, &count, extensions.data())) throw std::runtime_error(SDL_GetError());
        Check(vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr), "vkEnumerateInstanceExtensionProperties");
        std::vector<VkExtensionProperties> available(count);
        Check(vkEnumerateInstanceExtensionProperties(nullptr, &count, available.data()), "vkEnumerateInstanceExtensionProperties");
        VkInstanceCreateFlags instanceFlags = 0;
        for (const auto& extension : available) {
            if (std::strcmp(extension.extensionName, "VK_KHR_portability_enumeration") == 0) {
                extensions.push_back("VK_KHR_portability_enumeration");
                instanceFlags |= static_cast<VkInstanceCreateFlags>(0x00000001);
            }
        }
        VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};
        app.pApplicationName = "SPACE YAKUZA";
        app.apiVersion = VK_API_VERSION_1_1;
        VkInstanceCreateInfo instanceInfo{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
        instanceInfo.flags = instanceFlags;
        instanceInfo.pApplicationInfo = &app;
        instanceInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        instanceInfo.ppEnabledExtensionNames = extensions.data();
        Check(vkCreateInstance(&instanceInfo, nullptr, &instance), "vkCreateInstance");
        if (!SDL_Vulkan_CreateSurface(window, instance, &surface)) throw std::runtime_error(SDL_GetError());
        Check(vkEnumeratePhysicalDevices(instance, &count, nullptr), "vkEnumeratePhysicalDevices");
        std::vector<VkPhysicalDevice> devices(count);
        Check(vkEnumeratePhysicalDevices(instance, &count, devices.data()), "vkEnumeratePhysicalDevices");
        // 描画とPresentを共に処理できるQueueを持つデバイスを選ぶ
        for (VkPhysicalDevice candidate : devices) {
            uint32_t queueCount = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(candidate, &queueCount, nullptr);
            std::vector<VkQueueFamilyProperties> queues(queueCount);
            vkGetPhysicalDeviceQueueFamilyProperties(candidate, &queueCount, queues.data());
            for (uint32_t i = 0; i < queueCount; ++i) {
                VkBool32 present = VK_FALSE;
                Check(vkGetPhysicalDeviceSurfaceSupportKHR(candidate, i, surface, &present), "vkGetPhysicalDeviceSurfaceSupportKHR");
                if ((queues[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && present) { physical = candidate; queueFamily = i; break; }
            }
            if (physical) break;
        }
        if (!physical) throw std::runtime_error("No Vulkan graphics queue can present to this window");
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(physical, &properties);
        const VkDeviceSize alignment = std::max<VkDeviceSize>(16, properties.limits.minUniformBufferOffsetAlignment);
        uniformStride = (sizeof(Uniforms) + alignment - 1) / alignment * alignment;
        Check(vkEnumerateDeviceExtensionProperties(physical, nullptr, &count, nullptr), "vkEnumerateDeviceExtensionProperties");
        available.resize(count);
        Check(vkEnumerateDeviceExtensionProperties(physical, nullptr, &count, available.data()), "vkEnumerateDeviceExtensionProperties");
        extensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
        for (const auto& extension : available)
            if (std::strcmp(extension.extensionName, "VK_KHR_portability_subset") == 0) extensions.push_back("VK_KHR_portability_subset");
        const float priority = 1;
        VkDeviceQueueCreateInfo queueInfo{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
        queueInfo.queueFamilyIndex = queueFamily;
        queueInfo.queueCount = 1;
        queueInfo.pQueuePriorities = &priority;
        VkDeviceCreateInfo deviceInfo{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
        deviceInfo.queueCreateInfoCount = 1;
        deviceInfo.pQueueCreateInfos = &queueInfo;
        deviceInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        deviceInfo.ppEnabledExtensionNames = extensions.data();
        Check(vkCreateDevice(physical, &deviceInfo, nullptr, &device), "vkCreateDevice");
        vkGetDeviceQueue(device, queueFamily, 0, &queue);
        VkCommandPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = queueFamily;
        Check(vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool), "vkCreateCommandPool");
        VkCommandBufferAllocateInfo commandInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
        commandInfo.commandPool = commandPool;
        commandInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        commandInfo.commandBufferCount = 1;
        Check(vkAllocateCommandBuffers(device, &commandInfo, &command), "vkAllocateCommandBuffers");
        VkSemaphoreCreateInfo semaphoreInfo{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        Check(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &acquired), "vkCreateSemaphore");
        VkFenceCreateInfo fenceInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        Check(vkCreateFence(device, &fenceInfo, nullptr, &fence), "vkCreateFence");
        const VkDescriptorSetLayoutBinding bindings[] = {
            {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
            {1, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
            {2, VK_DESCRIPTOR_TYPE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}};
        VkDescriptorSetLayoutCreateInfo layoutInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
        layoutInfo.bindingCount = 3;
        layoutInfo.pBindings = bindings;
        Check(vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorLayout), "vkCreateDescriptorSetLayout");
        VkPipelineLayoutCreateInfo pipelineInfo{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
        pipelineInfo.setLayoutCount = 1;
        pipelineInfo.pSetLayouts = &descriptorLayout;
        Check(vkCreatePipelineLayout(device, &pipelineInfo, nullptr, &pipelineLayout), "vkCreatePipelineLayout");
        const VkDescriptorPoolSize sizes[] = {{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1}, {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1}, {VK_DESCRIPTOR_TYPE_SAMPLER, 1}};
        VkDescriptorPoolCreateInfo descriptorInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
        descriptorInfo.maxSets = 1;
        descriptorInfo.poolSizeCount = 3;
        descriptorInfo.pPoolSizes = sizes;
        Check(vkCreateDescriptorPool(device, &descriptorInfo, nullptr, &descriptorPool), "vkCreateDescriptorPool");
        VkDescriptorSetAllocateInfo setInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
        setInfo.descriptorPool = descriptorPool;
        setInfo.descriptorSetCount = 1;
        setInfo.pSetLayouts = &descriptorLayout;
        Check(vkAllocateDescriptorSets(device, &setInfo, &descriptorSet), "vkAllocateDescriptorSets");
        VkSamplerCreateInfo samplerInfo{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
        samplerInfo.magFilter = samplerInfo.minFilter = VK_FILTER_NEAREST;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        samplerInfo.addressModeU = samplerInfo.addressModeV = samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        Check(vkCreateSampler(device, &samplerInfo, nullptr, &sampler), "vkCreateSampler");
        for (int shader = 0; shader < ShaderCount; ++shader) {
            for (int stage = 0; stage < 2; ++stage) {
                const auto bytes = ReadShader(std::string(ShaderNames[shader]) + (stage == 0 ? ".vert.spv" : ".frag.spv"));
                if (bytes.empty() || bytes.size() % 4 != 0) throw std::runtime_error("Invalid SPIR-V size");
                std::vector<uint32_t> words(bytes.size() / 4);
                std::memcpy(words.data(), bytes.data(), bytes.size());
                VkShaderModuleCreateInfo shaderInfo{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
                shaderInfo.codeSize = bytes.size();
                shaderInfo.pCode = words.data();
                Check(vkCreateShaderModule(device, &shaderInfo, nullptr, &shaders[shader][stage]), "vkCreateShaderModule");
            }
        }
        RecreateSwapchain();
    }

    /** @brief 寸法に依存する描画リソースを解放する */
    void DestroySwapchain() {
        for (auto item : pipelines) vkDestroyPipeline(device, item.second, nullptr);
        pipelines.clear();
        for (auto framebuffer : framebuffers) vkDestroyFramebuffer(device, framebuffer, nullptr);
        framebuffers.clear();
        if (lowFramebuffer) vkDestroyFramebuffer(device, lowFramebuffer, nullptr);
        lowFramebuffer = VK_NULL_HANDLE;
        if (renderPass) vkDestroyRenderPass(device, renderPass, nullptr);
        renderPass = VK_NULL_HANDLE;
        DestroyImage(lowColor);
        DestroyImage(lowDepth);
        DestroyImage(screenDepth);
        for (auto view : swapViews) vkDestroyImageView(device, view, nullptr);
        swapViews.clear();
        for (auto semaphore : rendered) vkDestroySemaphore(device, semaphore, nullptr);
        rendered.clear();
        if (swapchain) vkDestroySwapchainKHR(device, swapchain, nullptr);
        swapchain = VK_NULL_HANDLE;
    }

    /** @brief Swapchainと低解像度ターゲットを現在のウィンドウ寸法へ作り直す */
    void RecreateSwapchain() {
        Check(vkDeviceWaitIdle(device), "vkDeviceWaitIdle(resize)");
        DestroySwapchain();
        VkSurfaceCapabilitiesKHR capabilities{};
        Check(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical, surface, &capabilities), "vkGetPhysicalDeviceSurfaceCapabilitiesKHR");
        uint32_t count = 0;
        Check(vkGetPhysicalDeviceSurfaceFormatsKHR(physical, surface, &count, nullptr), "vkGetPhysicalDeviceSurfaceFormatsKHR");
        std::vector<VkSurfaceFormatKHR> formats(count);
        Check(vkGetPhysicalDeviceSurfaceFormatsKHR(physical, surface, &count, formats.data()), "vkGetPhysicalDeviceSurfaceFormatsKHR");
        if (formats.empty()) throw std::runtime_error("No Vulkan surface format");
        VkSurfaceFormatKHR format = formats.front();
        if (format.format == VK_FORMAT_UNDEFINED) format.format = VK_FORMAT_B8G8R8A8_UNORM;
        for (const auto& candidate : formats)
            if (candidate.format == VK_FORMAT_B8G8R8A8_UNORM || candidate.format == VK_FORMAT_R8G8B8A8_UNORM) { format = candidate; break; }
        colorFormat = format.format;
        depthFormat = VK_FORMAT_UNDEFINED;
        for (VkFormat candidate : {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D16_UNORM}) {
            VkFormatProperties properties{};
            vkGetPhysicalDeviceFormatProperties(physical, candidate, &properties);
            if (properties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) { depthFormat = candidate; break; }
        }
        if (depthFormat == VK_FORMAT_UNDEFINED) throw std::runtime_error("No Vulkan depth format");
        extent = capabilities.currentExtent;
        if (extent.width == UINT32_MAX) {
            extent.width = std::clamp(static_cast<uint32_t>(width), capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            extent.height = std::clamp(static_cast<uint32_t>(height), capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
        }
        if (extent.width == 0 || extent.height == 0) return;
        count = capabilities.minImageCount + 1;
        if (capabilities.maxImageCount) count = std::min(count, capabilities.maxImageCount);
        VkSwapchainCreateInfoKHR swapInfo{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
        swapInfo.surface = surface;
        swapInfo.minImageCount = count;
        swapInfo.imageFormat = colorFormat;
        swapInfo.imageColorSpace = format.colorSpace;
        swapInfo.imageExtent = extent;
        swapInfo.imageArrayLayers = 1;
        swapInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        swapInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapInfo.preTransform = capabilities.currentTransform;
        for (auto alpha : {VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR, VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR, VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR, VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR})
            if (capabilities.supportedCompositeAlpha & alpha) { swapInfo.compositeAlpha = alpha; break; }
        swapInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
        swapInfo.clipped = VK_TRUE;
        Check(vkCreateSwapchainKHR(device, &swapInfo, nullptr, &swapchain), "vkCreateSwapchainKHR");
        Check(vkGetSwapchainImagesKHR(device, swapchain, &count, nullptr), "vkGetSwapchainImagesKHR");
        swapImages.resize(count);
        Check(vkGetSwapchainImagesKHR(device, swapchain, &count, swapImages.data()), "vkGetSwapchainImagesKHR");
        for (auto image : swapImages) swapViews.push_back(CreateView(image, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT));
        CreateImage(lowColor, LowWidth, LowHeight, colorFormat, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
        CreateImage(lowDepth, LowWidth, LowHeight, depthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_ASPECT_DEPTH_BIT);
        CreateImage(screenDepth, extent.width, extent.height, depthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_ASPECT_DEPTH_BIT);
        VkAttachmentDescription attachments[2]{};
        attachments[0].format = colorFormat;
        attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        attachments[1] = attachments[0];
        attachments[1].format = depthFormat;
        attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        const VkAttachmentReference colorReference{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
        const VkAttachmentReference depthReference{1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorReference;
        subpass.pDepthStencilAttachment = &depthReference;
        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstStageMask = dependency.srcStageMask;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        VkRenderPassCreateInfo passInfo{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
        passInfo.attachmentCount = 2;
        passInfo.pAttachments = attachments;
        passInfo.subpassCount = 1;
        passInfo.pSubpasses = &subpass;
        passInfo.dependencyCount = 1;
        passInfo.pDependencies = &dependency;
        Check(vkCreateRenderPass(device, &passInfo, nullptr, &renderPass), "vkCreateRenderPass");
        for (auto view : swapViews) framebuffers.push_back(CreateFramebuffer(view, screenDepth.view, extent.width, extent.height));
        lowFramebuffer = CreateFramebuffer(lowColor.view, lowDepth.view, LowWidth, LowHeight);
        VkSemaphoreCreateInfo semaphoreInfo{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        for (size_t i = 0; i < swapImages.size(); ++i) {
            VkSemaphore semaphore = VK_NULL_HANDLE;
            Check(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &semaphore), "vkCreateSemaphore(present)");
            rendered.push_back(semaphore);
        }
        const VkDescriptorImageInfo imageInfo{VK_NULL_HANDLE, lowColor.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
        const VkDescriptorImageInfo samplerInfo{sampler, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_UNDEFINED};
        VkWriteDescriptorSet writes[2]{};
        for (int i = 0; i < 2; ++i) {
            writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[i].dstSet = descriptorSet;
            writes[i].dstBinding = i + 1;
            writes[i].descriptorCount = 1;
        }
        writes[0].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        writes[0].pImageInfo = &imageInfo;
        writes[1].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
        writes[1].pImageInfo = &samplerInfo;
        vkUpdateDescriptorSets(device, 2, writes, 0, nullptr);
        resizePending = false;
    }

    /** @brief Framebufferを作る @param color カラー @param depth 深度 @param w 幅 @param h 高さ @return Framebuffer */
    VkFramebuffer CreateFramebuffer(VkImageView color, VkImageView depth, uint32_t w, uint32_t h) {
        const VkImageView attachments[] = {color, depth};
        VkFramebufferCreateInfo info{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        info.renderPass = renderPass;
        info.attachmentCount = 2;
        info.pAttachments = attachments;
        info.width = w;
        info.height = h;
        info.layers = 1;
        VkFramebuffer framebuffer = VK_NULL_HANDLE;
        Check(vkCreateFramebuffer(device, &info, nullptr, &framebuffer), "vkCreateFramebuffer");
        return framebuffer;
    }

    /** @brief 描画状態に対応するPSOを取得する @param draw 描画命令 @return パイプライン */
    VkPipeline Pipeline(const Draw& draw) {
        const unsigned key = draw.shader | (draw.blend << 4) | (draw.strip << 6) | (draw.depth << 7) | (draw.depthWrite << 8) | (draw.cull << 9);
        auto found = pipelines.find(key);
        if (found != pipelines.end()) return found->second;
        VkPipelineShaderStageCreateInfo stages[2]{};
        for (int i = 0; i < 2; ++i) {
            stages[i].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            stages[i].stage = i == 0 ? VK_SHADER_STAGE_VERTEX_BIT : VK_SHADER_STAGE_FRAGMENT_BIT;
            stages[i].module = shaders[draw.shader][i];
            stages[i].pName = i == 0 ? "VSMain" : "PSMain";
        }
        VkPipelineVertexInputStateCreateInfo vertex{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
        VkPipelineInputAssemblyStateCreateInfo assembly{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
        assembly.topology = draw.strip ? VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP : VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        VkPipelineViewportStateCreateInfo viewportInfo{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
        viewportInfo.viewportCount = viewportInfo.scissorCount = 1;
        VkPipelineRasterizationStateCreateInfo raster{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
        raster.polygonMode = VK_POLYGON_MODE_FILL;
        raster.cullMode = draw.cull ? VK_CULL_MODE_BACK_BIT : VK_CULL_MODE_NONE;
        raster.frontFace = VK_FRONT_FACE_CLOCKWISE;
        raster.lineWidth = 1;
        VkPipelineMultisampleStateCreateInfo multisample{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
        multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        VkPipelineDepthStencilStateCreateInfo depth{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
        depth.depthTestEnable = draw.depth;
        depth.depthWriteEnable = draw.depthWrite;
        depth.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
        VkPipelineColorBlendAttachmentState attachment{};
        attachment.blendEnable = draw.blend != Opaque;
        attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        attachment.dstColorBlendFactor = draw.blend == Additive ? VK_BLEND_FACTOR_ONE : VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        attachment.colorBlendOp = attachment.alphaBlendOp = VK_BLEND_OP_ADD;
        attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        attachment.dstAlphaBlendFactor = draw.blend == Additive ? VK_BLEND_FACTOR_ONE : draw.blend == Smoke ? VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA : VK_BLEND_FACTOR_ZERO;
        attachment.colorWriteMask = 15;
        VkPipelineColorBlendStateCreateInfo blend{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
        blend.attachmentCount = 1;
        blend.pAttachments = &attachment;
        const VkDynamicState dynamics[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
        VkPipelineDynamicStateCreateInfo dynamic{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
        dynamic.dynamicStateCount = 2;
        dynamic.pDynamicStates = dynamics;
        VkGraphicsPipelineCreateInfo info{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
        info.stageCount = 2;
        info.pStages = stages;
        info.pVertexInputState = &vertex;
        info.pInputAssemblyState = &assembly;
        info.pViewportState = &viewportInfo;
        info.pRasterizationState = &raster;
        info.pMultisampleState = &multisample;
        info.pDepthStencilState = &depth;
        info.pColorBlendState = &blend;
        info.pDynamicState = &dynamic;
        info.layout = pipelineLayout;
        info.renderPass = renderPass;
        VkPipeline result = VK_NULL_HANDLE;
        Check(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &info, nullptr, &result), "vkCreateGraphicsPipelines");
        pipelines.emplace(key, result);
        return result;
    }

    /** @brief 可変長の定数バッファを確保する @param required 必要命令数 */
    void EnsureUniforms(size_t required) {
        if (required <= uniformCapacity) return;
        if (uniformData) vkUnmapMemory(device, uniformMemory);
        if (uniformBuffer) vkDestroyBuffer(device, uniformBuffer, nullptr);
        if (uniformMemory) vkFreeMemory(device, uniformMemory, nullptr);
        uniformData = nullptr;
        uniformBuffer = VK_NULL_HANDLE;
        uniformMemory = VK_NULL_HANDLE;
        uniformCapacity = std::max(required, uniformCapacity * 2 + 2048);
        if (uniformCapacity > UINT32_MAX / uniformStride) throw std::runtime_error("Vulkan uniform offsets exceed uint32 range");
        VkBufferCreateInfo info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        info.size = uniformCapacity * uniformStride;
        info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        Check(vkCreateBuffer(device, &info, nullptr, &uniformBuffer), "vkCreateBuffer");
        VkMemoryRequirements requirements{};
        vkGetBufferMemoryRequirements(device, uniformBuffer, &requirements);
        VkMemoryAllocateInfo allocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        allocation.allocationSize = requirements.size;
        allocation.memoryTypeIndex = MemoryType(requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        Check(vkAllocateMemory(device, &allocation, nullptr, &uniformMemory), "vkAllocateMemory(uniforms)");
        Check(vkBindBufferMemory(device, uniformBuffer, uniformMemory, 0), "vkBindBufferMemory");
        Check(vkMapMemory(device, uniformMemory, 0, VK_WHOLE_SIZE, 0, &uniformData), "vkMapMemory");
        const VkDescriptorBufferInfo bufferInfo{uniformBuffer, 0, sizeof(Uniforms)};
        VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        write.dstSet = descriptorSet;
        write.dstBinding = 0;
        write.descriptorCount = 1;
        write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        write.pBufferInfo = &bufferInfo;
        vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
    }

    /** @brief RenderPassを開始する @param framebuffer 描画先 @param w 幅 @param h 高さ */
    void BeginPass(VkFramebuffer framebuffer, uint32_t w, uint32_t h) {
        VkClearValue clear[2]{};
        clear[0].color = {{0.05f, 0.05f, 0.1f, 1}};
        clear[1].depthStencil = {1, 0};
        VkRenderPassBeginInfo info{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
        info.renderPass = renderPass;
        info.framebuffer = framebuffer;
        info.renderArea.extent = {w, h};
        info.clearValueCount = 2;
        info.pClearValues = clear;
        vkCmdBeginRenderPass(command, &info, VK_SUBPASS_CONTENTS_INLINE);
    }

    /** @brief 描画命令を記録する @param draw 命令 @param index 定数位置 @param w 出力幅 @param h 出力高さ */
    void Execute(const Draw& draw, size_t index, int w, int h) {
        const auto area = ScaledViewport(draw.viewport, w, h, width, height);
        if (!area.IsValid()) return;
        const VkViewport viewportInfo{static_cast<float>(area.x), static_cast<float>(area.y), static_cast<float>(area.width), static_cast<float>(area.height), 0, 1};
        const VkRect2D scissor{{area.x, area.y}, {static_cast<uint32_t>(area.width), static_cast<uint32_t>(area.height)}};
        vkCmdSetViewport(command, 0, 1, &viewportInfo);
        vkCmdSetScissor(command, 0, 1, &scissor);
        vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline(draw));
        const uint32_t offset = static_cast<uint32_t>(index * uniformStride);
        vkCmdBindDescriptorSets(command, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 1, &offset);
        vkCmdDraw(command, draw.vertices, 1, 0, 0);
    }

    /** @brief カラー画像を次の使用状態へ遷移する @param image 画像 @param layout 次状態 @param stage 次使用ステージ @param access 次アクセス */
    void TransitionColor(VkImage image, VkImageLayout layout, VkPipelineStageFlags stage, VkAccessFlags access) {
        VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        barrier.dstAccessMask = access;
        barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        barrier.newLayout = layout;
        barrier.srcQueueFamilyIndex = barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        vkCmdPipelineBarrier(command, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, stage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    }

    /** @brief Vulkanフレームを送信する */
    void Render() {
        // ponytail: 既存D3D版同様に1フレームを同期し、必要ならFrameContextを複数化する
        Check(vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX), "vkWaitForFences");
        if (resizePending) RecreateSwapchain();
        if (!swapchain) return;
        uint32_t imageIndex = 0;
        const VkResult acquire = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, acquired, VK_NULL_HANDLE, &imageIndex);
        if (acquire == VK_ERROR_OUT_OF_DATE_KHR) { resizePending = true; return; }
        if (acquire != VK_SUBOPTIMAL_KHR) Check(acquire, "vkAcquireNextImageKHR");
        EnsureUniforms(draws.size() + 1);
        for (size_t i = 0; i < draws.size(); ++i)
            std::memcpy(static_cast<char*>(uniformData) + i * uniformStride, &draws[i].uniforms, sizeof(Uniforms));
        std::memset(static_cast<char*>(uniformData) + draws.size() * uniformStride, 0, sizeof(Uniforms));
        Check(vkResetCommandBuffer(command, 0), "vkResetCommandBuffer");
        VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        Check(vkBeginCommandBuffer(command, &begin), "vkBeginCommandBuffer");
        const size_t boundary = frameRetro ? std::min(hudStart, draws.size()) : 0;
        if (frameRetro) {
            BeginPass(lowFramebuffer, LowWidth, LowHeight);
            for (size_t i = 0; i < boundary; ++i) Execute(draws[i], i, LowWidth, LowHeight);
            vkCmdEndRenderPass(command);
            TransitionColor(lowColor.image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT);
        }
        BeginPass(framebuffers[imageIndex], extent.width, extent.height);
        if (frameRetro) {
            Draw upscale;
            upscale.viewport = {0, 0, width, height};
            upscale.shader = Upscale;
            upscale.blend = Opaque;
            upscale.strip = false;
            upscale.vertices = 3;
            Execute(upscale, draws.size(), extent.width, extent.height);
        }
        for (size_t i = boundary; i < draws.size(); ++i) Execute(draws[i], i, extent.width, extent.height);
        vkCmdEndRenderPass(command);
        TransitionColor(swapImages[imageIndex], VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0);
        Check(vkEndCommandBuffer(command), "vkEndCommandBuffer");
        const VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
        submit.waitSemaphoreCount = 1;
        submit.pWaitSemaphores = &acquired;
        submit.pWaitDstStageMask = &waitStage;
        submit.commandBufferCount = 1;
        submit.pCommandBuffers = &command;
        submit.signalSemaphoreCount = 1;
        submit.pSignalSemaphores = &rendered[imageIndex];
        Check(vkResetFences(device, 1, &fence), "vkResetFences");
        Check(vkQueueSubmit(queue, 1, &submit, fence), "vkQueueSubmit");
        VkPresentInfoKHR present{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
        present.waitSemaphoreCount = 1;
        present.pWaitSemaphores = &rendered[imageIndex];
        present.swapchainCount = 1;
        present.pSwapchains = &swapchain;
        present.pImageIndices = &imageIndex;
        const VkResult result = vkQueuePresentKHR(queue, &present);
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || acquire == VK_SUBOPTIMAL_KHR) resizePending = true;
        else Check(result, "vkQueuePresentKHR");
    }

    /** @brief Vulkanリソースを依存順に解放する */
    void CleanupGpu() {
        if (device) {
            vkDeviceWaitIdle(device);
            DestroySwapchain();
            if (uniformData) vkUnmapMemory(device, uniformMemory);
            if (uniformBuffer) vkDestroyBuffer(device, uniformBuffer, nullptr);
            if (uniformMemory) vkFreeMemory(device, uniformMemory, nullptr);
            for (auto& pair : shaders) for (auto shader : pair) if (shader) vkDestroyShaderModule(device, shader, nullptr);
            if (sampler) vkDestroySampler(device, sampler, nullptr);
            if (descriptorPool) vkDestroyDescriptorPool(device, descriptorPool, nullptr);
            if (pipelineLayout) vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
            if (descriptorLayout) vkDestroyDescriptorSetLayout(device, descriptorLayout, nullptr);
            if (acquired) vkDestroySemaphore(device, acquired, nullptr);
            if (fence) vkDestroyFence(device, fence, nullptr);
            if (commandPool) vkDestroyCommandPool(device, commandPool, nullptr);
            vkDestroyDevice(device, nullptr);
            device = VK_NULL_HANDLE;
        }
        if (surface) vkDestroySurfaceKHR(instance, surface, nullptr);
        if (instance) vkDestroyInstance(instance, nullptr);
        instance = VK_NULL_HANDLE;
        surface = VK_NULL_HANDLE;
    }
#endif
};

PortableRenderingService::PortableRenderingService() : impl_(std::make_unique<Impl>()) {}
PortableRenderingService::~PortableRenderingService() { impl_->CleanupGpu(); }

bool PortableRenderingService::Initialize(SDL_Window* window, int width, int height) {
    Cleanup();
    if (!window || width <= 0 || height <= 0) return false;
    impl_->window = window;
    impl_->width = width;
    impl_->height = height;
    try {
        impl_->InitializeGpu();
        impl_->initialized = true;
        return true;
    } catch (const std::exception& error) {
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Renderer initialization failed: %s", error.what());
        Cleanup();
        return false;
    }
}

void PortableRenderingService::Cleanup() {
    impl_->CleanupGpu();
    impl_ = std::make_unique<Impl>();
}

void PortableRenderingService::Resize(int width, int height) {
    if (width == impl_->width && height == impl_->height) return;
    impl_->width = std::max(width, 0);
    impl_->height = std::max(height, 0);
#ifndef __EMSCRIPTEN__
    impl_->resizePending = true;
#endif
}

void PortableRenderingService::BeginFrame() {
    impl_->draws.clear();
    impl_->hudStart = std::numeric_limits<size_t>::max();
    impl_->frameRetro = impl_->retro;
    impl_->pipeline = PipelineId::Object;
    impl_->viewport = {0, 0, impl_->width, impl_->height};
}

void PortableRenderingService::EndFrame() {
    if (impl_->initialized && impl_->width > 0 && impl_->height > 0) impl_->Render();
}

void PortableRenderingService::DrawRect(const Rect& rect, const ColorF& color) {
    SetPipeline(PipelineId::Object);
    Draw draw = impl_->MakeDraw();
    draw.uniforms.matrix = Matrix4x4::Translation({rect.position.x, rect.position.y, 0}) * Matrix4x4::Scale({rect.size.x, rect.size.y, 1});
    draw.uniforms.color = color;
    draw.uniforms.shape = 6;
    impl_->draws.push_back(draw);
}

void PortableRenderingService::DrawCircle(const Circle& circle, const ColorF& color) {
    SetPipeline(PipelineId::SpellCircle);
    Draw draw = impl_->MakeDraw();
    const float diameter = std::abs(circle.radius) * 2;
    draw.uniforms.matrix = Matrix4x4::Translation({circle.center.x, circle.center.y, 0}) * Matrix4x4::Scale({diameter, diameter, 1});
    draw.uniforms.color = color;
    draw.uniforms.shape = 6;
    impl_->draws.push_back(draw);
}

void PortableRenderingService::DrawPrimitive3D(const Primitive3D& primitive) {
    Draw draw = impl_->MakeDraw();
    draw.uniforms.matrix = primitive.wvpMatrix;
    draw.uniforms.color = primitive.color;
    draw.uniforms.rotation = primitive.rotationAngle;
    switch (primitive.shape) {
    case PrimitiveShape::Plate: draw.uniforms.shape = 0; draw.vertices = 4; break;
    case PrimitiveShape::Box: draw.uniforms.shape = 1; draw.vertices = 36; break;
    case PrimitiveShape::Sphere: draw.uniforms.shape = 7; draw.vertices = 576; break;
    case PrimitiveShape::Cylinder: draw.uniforms.shape = 3; draw.vertices = 96; break;
    case PrimitiveShape::Cone: draw.uniforms.shape = 4; draw.vertices = 48; break;
    case PrimitiveShape::Prism: draw.uniforms.shape = 5; draw.vertices = 24; break;
    case PrimitiveShape::Sprite2D: draw.uniforms.shape = 6; draw.vertices = 4; break;
    }
    draw.strip = draw.vertices == 4;
    impl_->draws.push_back(draw);
}

void PortableRenderingService::DrawPlayerShot(const PlayerShotVisual& shot) {
    Draw draw;
    draw.viewport = impl_->viewport;
    draw.shader = Shot;
    draw.blend = Additive;
    draw.depth = shot.depthTest;
    draw.uniforms.matrix = Matrix4x4::Translation({shot.position.x, shot.position.y, shot.depth}) *
        Matrix4x4::RotationZ(shot.direction) * Matrix4x4::Scale({shot.size.x, shot.size.y, 1});
    draw.uniforms.color = {1, 1, 1, shot.opacity};
    draw.uniforms.time = shot.time;
    draw.uniforms.shape = static_cast<float>(shot.type);
    draw.uniforms.rotation = shot.direction;
    impl_->draws.push_back(draw);
}

void PortableRenderingService::DrawExplosion(const ExplosionVisual& explosion) {
    Draw draw;
    draw.viewport = impl_->viewport;
    draw.shader = Explosion;
    draw.blend = explosion.effectType == 0 || explosion.effectType == 3 || explosion.effectType == 4 ? Additive : Smoke;
    draw.depth = explosion.effectType == 3;
    draw.uniforms.matrix = explosion.wvpMatrix;
    draw.uniforms.time = explosion.progress;
    draw.uniforms.shape = static_cast<float>(explosion.effectType);
    impl_->draws.push_back(draw);
}

void PortableRenderingService::DrawRailgun(const RailgunVisual& railgun) {
    Draw draw;
    draw.viewport = impl_->viewport;
    draw.shader = Railgun;
    draw.blend = Additive;
    draw.uniforms.matrix = railgun.wvpMatrix;
    draw.uniforms.time = railgun.progress;
    draw.uniforms.shape = static_cast<float>(railgun.effectType);
    impl_->draws.push_back(draw);
}

void PortableRenderingService::DrawTextCommand(std::string_view text, const Vector2& position, float size,
    const ColorF& color, float characterSpacing) {
    impl_->MarkHud();
    if (impl_->height <= 0) return;
    float x = position.x;
    float y = position.y;
    for (unsigned char c : text) {
        if (c == '\n') { x = position.x; y -= size * AspectRatio() * 2.4f; continue; }
        if (c >= 128) c = '?';
        Draw draw;
        draw.shader = Font;
        draw.viewport = {0, 0, impl_->width, impl_->height};
        // Floppy版と同じビットマップを定数へ格納し、WebGL非対応のBuffer Textureを不要にする
        const float glyph[] = {x, y, size, size * AspectRatio(), color.r, color.g, color.b, color.a,
            static_cast<float>(CharacterASCIIData::g_font[c][0]), static_cast<float>(CharacterASCIIData::g_font[c][1]),
            static_cast<float>(CharacterASCIIData::g_font[c][2]), static_cast<float>(CharacterASCIIData::g_font[c][3]),
            static_cast<float>(CharacterASCIIData::g_font[c][4]), static_cast<float>(CharacterASCIIData::g_font[c][5]),
            static_cast<float>(CharacterASCIIData::g_font[c][6]), static_cast<float>(CharacterASCIIData::g_font[c][7])};
        std::memcpy(&draw.uniforms, glyph, sizeof(glyph));
        impl_->draws.push_back(draw);
        x += size * 1.5f + characterSpacing;
    }
}

void PortableRenderingService::SetPipeline(PipelineId pipeline) { impl_->pipeline = pipeline; }
void PortableRenderingService::SetCamera(const CameraMatrices&, const Viewport& viewport) {
    // WVPはRendererが各命令へ格納済みのため、バックエンドはViewportだけを反映する
    if (viewport.IsValid()) impl_->viewport = viewport;
}
void PortableRenderingService::ResetCamera() {
    impl_->MarkHud();
    impl_->viewport = {0, 0, impl_->width, impl_->height};
}
void PortableRenderingService::SetRetroEffectEnabled(bool enabled) { impl_->retro = enabled; }
int PortableRenderingService::Width() const { return impl_->width; }
int PortableRenderingService::Height() const { return impl_->height; }
float PortableRenderingService::AspectRatio() const { return impl_->height > 0 ? static_cast<float>(impl_->width) / impl_->height : 1; }

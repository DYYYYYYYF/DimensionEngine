#include <headers.hpp>
#include <vector>
#include <vulkan/vulkan_handles.hpp>

//const vk::PolygonMode polygonMode = vk::PolygonMode::eLine;
const std::vector<vk::PolygonMode> polygonModes = {vk::PolygonMode::eLine,
        vk::PolygonMode::eFill};
static vk::PolygonMode polygonMode = polygonModes[1];

class TriangleApplication{
    private:
        struct EyePos{
            float x = 0.0;
            float y = 0.0;
            float z = 0.0;
        };

        struct MousePos{
            double x = 0.0;
            double y = 0.0;
        };

    public:
        void run(){
            initWindow();
            initVulkan();
            mainLoop();
            cleanup();
        }

        struct Vertex
        {
            glm::vec3 pos;
            glm::vec3 color;
            glm::vec3 normal;
            glm::vec2 texCoord;
            
            static vk::VertexInputBindingDescription GetBindingDescription(){
                static vk::VertexInputBindingDescription description;
                description.setBinding(0)
                           .setInputRate(vk::VertexInputRate::eVertex)
                           .setStride(sizeof(Vertex));
                return description;
            }

            static std::array<vk::VertexInputAttributeDescription,4> GetAttributeDescription(){
                static std::array<vk::VertexInputAttributeDescription,4> desc;
                desc[0].setBinding(0)
                       .setLocation(0)      // .vert中的参数 -- location
                       .setFormat(vk::Format::eR32G32B32Sfloat)     //与.vert中的输入参数相同 -- Vec2
                       .setOffset(offsetof(Vertex, pos));  //偏移量
                desc[1].setBinding(0)
                       .setLocation(1)
                       .setFormat(vk::Format::eR32G32B32Sfloat)
                       .setOffset(offsetof(Vertex, color));
                desc[2].setBinding(0)
                       .setLocation(2)
                       .setFormat(vk::Format::eR32G32B32Sfloat)
                       .setOffset(offsetof(Vertex, normal));
                desc[3].setBinding(0)
                       .setLocation(3)
                       .setFormat(vk::Format::eR32G32Sfloat)
                       .setOffset(offsetof(Vertex, texCoord));
                return desc;
            }

            bool operator==(const Vertex& other) const {
                return pos == other.pos && color == other.color && texCoord == other.texCoord;
            }
        };

        struct KeyBoardScoll{
            float scale = 0.5;
            float camera = -1;
            float transX = 0.0;
            float transY = 0.0;
            float transZ = 0.0;
            float rotateX = 0.0;
            float rotateY = 0.0;
            float rotateZ = 0.0;
            float angleX = 0.0;
            float angleY = 0.0;
            MousePos mouse;
        };
     
        struct ObjModel{
            std::vector<uint32_t> indices;
            std::vector<Vertex> vertices;
            vk::Buffer vertexBuf;
            vk::Buffer indicesBuf;
        };

   Camera camera = Camera(glm::vec3(0.0f, 2.0f, 2.0f), glm::radians(-45.0f), glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        Loader loader;
        KeyBoardScoll keyBoardScoll;
  
        vk::Pipeline createPipeline(vk::ShaderModule vextexShader, vk::ShaderModule fragShader);
        vk::ShaderModule createShaderModule(const char* filename);
        void updateKeyBoard();
        void updateUniform(TriangleApplication::KeyBoardScoll val);
        void Render(vk::Pipeline curPipeline, TriangleApplication::ObjModel model);
        void WaitIdle();
    private:
        // const char* objFile = "/Users/uncled/Desktop/CMakeFiles/ComputerGraph/TestVulkanDemo/model/sponza.obj";
         const char* roomObj = "/Users/uncled/Desktop/CMakeFiles/ComputerGraph/TestVulkanDemo/model/room.obj";
        // const char* objFile = "/Users/uncled/Desktop/CMakeFiles/ComputerGraph/TestVulkanDemo/model/CornellBox.obj";
        // const char* objFile = "/Users/uncled/Desktop/CMakeFiles/ComputerGraph/TestVulkanDemo/model/untitled.obj";
         const char* shipObj = "/Users/uncled/Desktop/CMakeFiles/ComputerGraph/TestVulkanDemo/model/ship.obj";
        // const char* objFile = "/Users/uncled/Desktop/CMakeFiles/ComputerGraph/TestVulkanDemo/model/ball.obj";

        struct QueueFamilyIndices{
            std::optional<uint32_t> graphicsIndices;
            std::optional<uint32_t> presentIndices;  
        };

        struct SwapchainSupport{
            vk::SurfaceCapabilitiesKHR capabilities;      //能力
            vk::Extent2D extent;        //尺寸大小
            vk::SurfaceFormatKHR format;        //格式
            vk::PresentModeKHR presnetMode;     //显示模式
            uint32_t imageCount;
        };

        struct MemRequiredInfo{
            uint32_t index;
            size_t size;
        };

        struct UniformBufferObj{
         glm::mat4 model;            // 64
         glm::mat4 view;             // 128
         glm::mat4 projective;       // 192
         glm::vec3 baseLight = {1.0, 1.0, 1.0};        // 204 
         float intensity = 0.5f;     // 208
         glm::vec3 lightPos = {20.0f, 20.0f, 20.0f};      // 220
         float specularIntensity = 0.8f;            // 224
         glm::vec3 viewPos = {0, 0, 2};
        };
        
        QueueFamilyIndices queueIndices;
        SwapchainSupport supportInfo;
        UniformBufferObj ubo;

        GLFWwindow *window;
        vk::Instance instance;  
        vk::SurfaceKHR surface; 
        vk::PhysicalDevice phyDevice;   //物理设备GPU
        vk::Device device;      //逻辑
        vk::Queue graphicQueue;
        vk::Queue presentQueue;
        vk::SwapchainKHR swapChain;
        std::vector<vk::Image> images;
        std::vector<vk::ImageView> views;
        std::vector<vk::ShaderModule> modules;
        std::vector<vk::Framebuffer> framebuffers;

        std::vector<ObjModel> models;   //存放模型
        std::vector<vk::Pipeline> pipelines;  //存放不同类型的渲染管线
        vk::PipelineLayout layout;
        vk::RenderPass renderPass;
        vk::CommandPool cmdPool;
        vk::CommandBuffer cmdBuffer;        
        vk::Semaphore imageAvaliableSem;
        vk::Semaphore renderFinishSem;
        vk::Fence fence;
        vk::Buffer vertexBuffer;
        vk::DeviceMemory vertexMemory;
        vk::Buffer deviceBuffer;
        vk::DeviceMemory deviceMemory;
        vk::Buffer indexBuf;
        vk::DeviceMemory indexMem;
        vk::Buffer uniformBuffer;
        vk::DeviceMemory uniformMemory;
        vk::DescriptorSetLayout uniformDesLayout;
        vk::DescriptorPool descPool;
        vk::DescriptorSet descSets;
        vk::Image textureImage;
        vk::DeviceMemory textureMemory;
        vk::ImageView textureImageView;
        vk::Sampler textureSampler;

        vk::Image depthImage;
        vk::DeviceMemory depthMemory;
        vk::ImageView depthImageView;

        void initWindow();
        void initVulkan();
        void mainLoop();
        void cleanup();

        void loadTexture();
        void createTextureImage(vk::Image image, uint64_t texHeight, uint64_t texWidth, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage);
        void transitionImageLayout(vk::Image image, vk::Format format, vk::ImageLayout oldLayout, vk::ImageLayout newLayout);
        void copyBuffer2Image(vk::Buffer buf, vk::Image image, uint32_t height, uint32_t width);
        void createTextureImageView();
        void createTextureSample();
        void createVertexBuf(TriangleApplication::ObjModel &model);
        void createIndexBuf(TriangleApplication::ObjModel &model);
        void createUniformBuf();
       
        void createDepthResource();
        void createDepthImage(uint64_t depthHeight, uint64_t depthWidth, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage);

        void copyBuffer(vk::Buffer src, vk::Buffer dst, vk::DeviceSize size);
        TriangleApplication::ObjModel loadModel(const char* objPath);
        void updateDescSet();
        void endCmdBuffer(vk::CommandBuffer cmdBuf);


        TriangleApplication::MemRequiredInfo queryImageReqInfo(vk::Image image, vk::MemoryPropertyFlags flag);
        
        vk::Image createImage(uint64_t height, uint64_t width, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage);
        vk::ImageView createImageView(vk::Image image, vk::Format format, vk::ImageAspectFlags aspectFlags);
        vk::Instance createInstance();
        vk::SurfaceKHR createSurface();
        vk::PhysicalDevice pickupPhysicalDevice();
        vk::Device createDeive();
        vk::SwapchainKHR createSwapchain();
        vk::PipelineLayout createLayout();
        vk::RenderPass createRenderPass();
        vk::CommandPool createCmdPool();
        vk::CommandBuffer alloateCmdBuffer();
        vk::Semaphore createSemaphore();
        vk::Fence createFence();
        vk::Buffer createBuffer(vk::BufferUsageFlags flag, vk::SharingMode mode, uint64_t size);
        vk::DeviceMemory allocateMemory(vk::Buffer buffer, vk::MemoryPropertyFlags flag);
        vk::DescriptorSetLayout createDescriptionSetLayout();
        vk::DescriptorPool createDescPool();
        vk::DescriptorSet createDescSet();
        vk::Format findSupportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features);
        vk::Format findDepthForamt();

        std::vector<vk::ImageView> createImageViews();
        std::vector<vk::Framebuffer> createFramebuffer();

        void recordCmd(vk::CommandBuffer buf, vk::Framebuffer fbur, vk::Pipeline pipeline, TriangleApplication::ObjModel model);

        bool hasStencilComponent(vk::Format format);

        QueueFamilyIndices queryPhysicalDevice();
        SwapchainSupport querySwapchainSupport(int w, int h);
        MemRequiredInfo queryMemReqInfo(vk::Buffer, vk::MemoryPropertyFlags flag);

        void initImGUI();
};

namespace std{
    template<> struct hash<TriangleApplication::Vertex> {
        size_t operator()(TriangleApplication::Vertex const& vertex) const {
            return ((hash<glm::vec3>()(vertex.pos) ^
                    (hash<glm::vec3>()(vertex.color) << 1)) >> 1)^
                    (hash<glm::vec2>()(vertex.texCoord) << 1);
        }
    };
}

#include <cstdint>
#include <iostream>
#include <random>
#include <chrono>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "Shader.hpp"

#define INITIAL_WIDTH 1600
#define INITIAL_HEIGHT 900
#define VSYNC

static float deltaTime = 0.0f;

static std::shared_ptr<ShaderProgram> particlesProgram;
static std::shared_ptr<ShaderProgram> outputProgram;
static std::shared_ptr<SSBO> uintPixels;
static std::shared_ptr<SSBO> particleBuffer;
static glm::ivec2 particleSize{INITIAL_WIDTH, INITIAL_HEIGHT};
static glm::ivec3 dispatchSize{64, 32, 16};

static glm::mat4 projection{1.0f};
static glm::mat4 view{1.0f};
static glm::vec3 eyePos{1.5f, 5.0f, 5.0f};
static bool animateEyePos = false;
static bool animatedEyePosNormalize = false;

static float particleEMax = 1000.0f;
static float outputScalar = 5.0f;

static std::default_random_engine randomEngine(std::random_device{}());
static std::uniform_int_distribution<unsigned short> particleSeedDistribution;
static int particleSeed = 1;

static glm::vec4 attractors{-1.4f, 1.6f, 1.0f, 0.7f};

static glm::vec3 coldColor{0.25f, 0.25f, 1.0f};
static glm::vec3 hotColor{1.0f, 0.25f, 0.25f};

static std::vector<glm::vec4> attractorPresets{
    {-1.4f, 1.6f, 1.0f, 0.7f},
    {-1.7f, 1.7f, 0.6f, 1.2f},
    {-1.7f, 1.3f, 0.1f, 1.3f},
    {-1.8f, -2.0f, -0.5f, -0.9f},

};

static void clearPixelSSBO()
{
    uintPixels->Bind();
    glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_RG32UI, GL_RG, GL_UNSIGNED_INT, nullptr);
}

static void clearParticlesSSBO()
{
    particleBuffer->Bind();
    glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_RGBA32F, GL_RGBA, GL_FLOAT, nullptr);
}

static void updateColors()
{
    if (outputProgram != nullptr)
    {
        outputProgram->SetVec3("ColdColor", coldColor);
        outputProgram->SetVec3("HotColor", hotColor);
    }
}

static void updateAttractors()
{
    clearParticlesSSBO();

    if (particlesProgram != nullptr)
    {
        particlesProgram->SetVec4("attractors", attractors);
    }
}

static size_t getParticleCount()
{
    return static_cast<size_t>(16 * 16 * 1) * (static_cast<size_t>(dispatchSize.x * dispatchSize.y) * dispatchSize.z);
}

static void reloadShaders()
{
    try
    {
        auto particles = std::make_shared<Shader>("particles.comp", ShaderType::Compute);
        particlesProgram = std::make_shared<ShaderProgram>(particles);

        auto outputVert = std::make_shared<Shader>("output.vert", ShaderType::Vertex);
        auto outputFrag = std::make_shared<Shader>("output.frag", ShaderType::Fragment);
        outputProgram = std::make_shared<ShaderProgram>(outputVert, outputFrag);
    }
    catch (const std::exception &e)
    {
        particlesProgram.reset();
        outputProgram.reset();
        std::cerr << e.what() << '\n';
        return;
    }

    if (particlesProgram != nullptr)
    {
        particlesProgram->SetMat4("MVP", projection * view);
    }

    updateAttractors();
    updateColors();
}

static void recreatePixelsSSBO()
{
    {
        const auto pixelCount = particleSize.x * particleSize.y;
        const auto pixels = new uint64_t[pixelCount];
        memset(pixels, 0, pixelCount * sizeof(uint64_t));
        uintPixels->Update(pixels, pixelCount * sizeof(uint64_t));
        delete[] pixels;
    }

    if (particlesProgram != nullptr)
    {
        particlesProgram->SetIVec2("RenderTextureDimensions", particleSize);
        particlesProgram->SetSSBO("PixelBufferSSBO", uintPixels);
        particlesProgram->SetFloat("eMax", particleEMax);
    }

    if (outputProgram != nullptr)
    {
        outputProgram->SetIVec2("RenderTextureDimensions", particleSize);
        outputProgram->SetSSBO("PixelBufferSSBO", uintPixels);
        outputProgram->SetFloat("outputScalar", outputScalar);
    }
}

static void recreateParticlesSSBO()
{
    {
        const auto particleCount = getParticleCount();
        const auto particles = std::vector<glm::vec4>(particleCount, glm::vec4{0.0f, 0.0f, 0.0f, 0.0f});
        particleBuffer->Update(particles);
    }

    if (particlesProgram != nullptr)
    {
        particlesProgram->SetSSBO("ParticleBufferSSBO", particleBuffer);
    }
}

static void recreateMVP(int width, int height)
{
    const auto aspect = static_cast<float>(width) / static_cast<float>(height);
    projection = glm::perspective(glm::radians(30.0f), aspect, 0.01f, 100.0f);

    view = glm::lookAt(eyePos, {0.0f, 0.0f, 0.0f}, glm::vec3(0.0f, 1.0f, 0.0f));

    if (particlesProgram != nullptr)
    {
        particlesProgram->SetMat4("MVP", projection * view);
    }
}

static void appInit()
{
    particleSeedDistribution = std::uniform_int_distribution<unsigned short>(0);

    uintPixels = std::make_shared<SSBO>();
    particleBuffer = std::make_shared<SSBO>();

    recreateMVP(INITIAL_WIDTH, INITIAL_HEIGHT);
    reloadShaders();

    recreatePixelsSSBO();
    recreateParticlesSSBO();
}

static void appResize(int width, int height)
{
    recreateMVP(width, height);
    particleSize = glm::ivec2(width, height);
    recreatePixelsSSBO();
}

static void appRender()
{
    clearPixelSSBO();

    particleSeed = particleSeedDistribution(randomEngine);

    const auto glfwTime = static_cast<float>(glfwGetTime());

    if (particlesProgram != nullptr)
    {
        particlesProgram->Use();
        particlesProgram->SetFloat("Time", glfwTime);
        particlesProgram->SetInt("Seed", particleSeed);
        glDispatchCompute(dispatchSize.x, dispatchSize.y, dispatchSize.z);
    }

    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    if (outputProgram != nullptr)
    {
        outputProgram->Use();
        glDrawArrays(GL_TRIANGLES, 0, 3);
    }

    if (animateEyePos)
    {
        constexpr float distance = 10.0f;
        eyePos = glm::vec3(glm::sin(glfwTime * 0.35f), glm::cos(glfwTime * 0.25f), glm::sin(glfwTime * 0.2f));
        if (animatedEyePosNormalize)
        {
            eyePos = glm::normalize(eyePos);
        }
        eyePos *= distance;
        recreateMVP(particleSize.x, particleSize.y);
    }

    ImGui::SetNextWindowSize(ImVec2(340, 340), ImGuiCond_Once);
    if (ImGui::Begin("Settings"))
    {
        if (ImGui::Button("Reload Shaders"))
        {
            reloadShaders();
            recreatePixelsSSBO();
        }

        if (ImGui::InputFloat("Particle eMax", &particleEMax, 0, 0, "%.0f"))
        {
            if (particlesProgram != nullptr)
            {
                particlesProgram->SetFloat("eMax", particleEMax);
            }
        }

        if (ImGui::DragFloat("Output Scalar", &outputScalar, 0.01f, 0, 0, "%.2f"))
        {
            if (outputProgram != nullptr)
            {
                outputProgram->SetFloat("outputScalar", outputScalar);
            }
        }

        ImGui::Spacing();
        ImGui::Spacing();

        if (ImGui::DragFloat3("View Pos", glm::value_ptr(eyePos), 0.05f))
        {
            recreateMVP(particleSize.x, particleSize.y);
        }
        if (ImGui::Checkbox("Animate View Pos", &animateEyePos))
        {
        }
        ImGui::SameLine();
        if (ImGui::Checkbox("Normalized Distance", &animatedEyePosNormalize))
        {
        }

        ImGui::Spacing();
        ImGui::Spacing();

        if (ImGui::InputInt3("Dispatch Size", glm::value_ptr(dispatchSize)))
        {
            recreateParticlesSSBO();
        }

        std::stringstream ss;
        ss.imbue(std::locale(""));
        ss << getParticleCount();
        ImGui::Text("Particles: %s", ss.str().c_str());

        ImGui::Spacing();
        ImGui::Spacing();

        if (ImGui::InputFloat4("Attractors", glm::value_ptr(attractors)))
        {
            updateAttractors();
        }

        if (ImGui::BeginCombo("##Attractor Presets", "Attractor Presets"))
        {
            for (size_t i = 0; i < attractorPresets.size(); i++)
            {
                std::string label = "Preset " + std::to_string(i + 1);
                if (ImGui::Selectable(label.c_str(), false))
                {
                    attractors = attractorPresets[i];
                    updateAttractors();
                }
            }

            ImGui::EndCombo();
        }

        ImGui::Spacing();
        ImGui::Spacing();

        if (ImGui::ColorEdit3("Cold Color", glm::value_ptr(coldColor)))
        {
            updateColors();
        }
        if (ImGui::ColorEdit3("Hot Color", glm::value_ptr(hotColor)))
        {
            updateColors();
        }

        ImGui::Spacing();
        ImGui::Spacing();

        ImGui::Text("FPS: %.1f", 1.0f / deltaTime);

        ImGui::End();
    }
}

static void appCleanup()
{
    outputProgram.reset();
    particlesProgram.reset();
    uintPixels.reset();
}

static void processInput(GLFWwindow *window);
static void framebufferSizeCallback(GLFWwindow *window, int width, int height);
static void APIENTRY debugMessageCallback(GLenum source, GLenum type, unsigned int id, GLenum severity, GLsizei length, const char *message, const void *userParam);

static void app()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#if __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
#ifndef NDEBUG
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
#endif

    GLFWwindow *window = glfwCreateWindow(INITIAL_WIDTH, INITIAL_HEIGHT, "SomeParticles", nullptr, nullptr);
    if (window == nullptr)
    {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window");
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
#ifndef VSYNC
    glfwSwapInterval(0);
#endif

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
    {
        throw std::runtime_error("Failed to initialize GLAD");
    }

    // Enable debug
    int flags;
    glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
    if (flags & GL_CONTEXT_FLAG_DEBUG_BIT)
    {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(&debugMessageCallback, nullptr);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable; // IF using Docking Branch

    ImGui_ImplGlfw_InitForOpenGL(window, true); // Second param install_callback=true will install GLFW callbacks and chain to existing ones.
    ImGui_ImplOpenGL3_Init();

    appInit();

    auto lastTime = std::chrono::high_resolution_clock::now();
    while (!glfwWindowShouldClose(window))
    {
        auto currentTime = std::chrono::high_resolution_clock::now();
        deltaTime = std::chrono::duration_cast<std::chrono::duration<float>>(currentTime - lastTime).count();
        lastTime = currentTime;

        processInput(window);

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        appRender();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    appCleanup();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwTerminate();
}

static void processInput(GLFWwindow *window)
{
    const auto leftAlt = glfwGetKey(window, GLFW_KEY_LEFT_ALT);
    const auto altPressed = leftAlt == GLFW_PRESS || leftAlt == GLFW_REPEAT;

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS && altPressed)
    {
        glfwSetWindowShouldClose(window, true);
    }
}

static void framebufferSizeCallback(GLFWwindow *window, int width, int height)
{
    glViewport(0, 0, width, height);
    appResize(width, height);
}

// https://learnopengl.com/In-Practice/Debugging
void debugMessageCallback(GLenum source, GLenum type, unsigned int id, GLenum severity, GLsizei length, const char *message, const void *userParam)
{
    // ignore non-significant error/warning codes
    if (id == 131169 || id == 131185 || id == 131218 || id == 131204) return;

    // Ignore Generic vertex attribute array 0 uses a pointer with a small value (0x(nil))
    if (id == 131076) return;

    std::cout << "Debug message (" << id << "): " << message << '\n';

    switch (source)
    {
        case GL_DEBUG_SOURCE_API:
            std::cout << "Source: API";
            break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
            std::cout << "Source: Window System";
            break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER:
            std::cout << "Source: Shader Compiler";
            break;
        case GL_DEBUG_SOURCE_THIRD_PARTY:
            std::cout << "Source: Third Party";
            break;
        case GL_DEBUG_SOURCE_APPLICATION:
            std::cout << "Source: Application";
            break;
        case GL_DEBUG_SOURCE_OTHER:
            std::cout << "Source: Other";
            break;
        default:
            break;
    }
    std::cout << '\n';

    switch (type)
    {
        case GL_DEBUG_TYPE_ERROR:
            std::cout << "Type: Error";
            break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
            std::cout << "Type: Deprecated Behaviour";
            break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
            std::cout << "Type: Undefined Behaviour";
            break;
        case GL_DEBUG_TYPE_PORTABILITY:
            std::cout << "Type: Portability";
            break;
        case GL_DEBUG_TYPE_PERFORMANCE:
            std::cout << "Type: Performance";
            break;
        case GL_DEBUG_TYPE_MARKER:
            std::cout << "Type: Marker";
            break;
        case GL_DEBUG_TYPE_PUSH_GROUP:
            std::cout << "Type: Push Group";
            break;
        case GL_DEBUG_TYPE_POP_GROUP:
            std::cout << "Type: Pop Group";
            break;
        case GL_DEBUG_TYPE_OTHER:
            std::cout << "Type: Other";
            break;
        default:
            break;
    }
    std::cout << '\n';

    switch (severity)
    {
        case GL_DEBUG_SEVERITY_HIGH:
            std::cout << "Severity: high";
            break;
        case GL_DEBUG_SEVERITY_MEDIUM:
            std::cout << "Severity: medium";
            break;
        case GL_DEBUG_SEVERITY_LOW:
            std::cout << "Severity: low";
            break;
        case GL_DEBUG_SEVERITY_NOTIFICATION:
            std::cout << "Severity: notification";
            break;
        default:
            break;
    }
    std::cout << '\n';
    std::cout << std::endl;
}

int main()
{
    try
    {
        app();
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
        return -1;
    }

    return 0;
}

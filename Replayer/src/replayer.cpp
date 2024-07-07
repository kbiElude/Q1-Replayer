/* API Interceptor (c) 2024 Dominik Witczak
 *
 * This code is licensed under MIT license (see LICENSE.txt for details)
 */
#include "replayer.h"
#include "Common/logger.h"
#include "glfw/glfw3.h"
#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_opengl3.h"
#include <assert.h>
#include <thread>


static void glfw_error_callback(int         error,
                                const char* description)
{
    assert(false && "GLFW error callback received");
}

Replayer::Replayer()
    :m_worker_thread_must_die(false)
{
    /* Stub */
}

Replayer::~Replayer()
{
    m_worker_thread_must_die = true;

    m_worker_thread.join();
}

ReplayerUniquePtr Replayer::create()
{
    ReplayerUniquePtr result_ptr(new Replayer() );

    if (result_ptr != nullptr)
    {
        if (!result_ptr->init() )
        {
            result_ptr.reset();
        }
    }

    return result_ptr;
}

bool Replayer::init()
{
    m_worker_thread = std::thread(&Replayer::execute,
                                   this);

    return true;
}

void Replayer::execute()
{
    int         result     = 1;
    GLFWwindow* window_ptr = nullptr;

    APIInterceptor::g_logger_ptr->disable_logging_for_this_thread();

    glfwSetErrorCallback(glfw_error_callback);

    if (!glfwInit() )
    {
        assert(false);

        goto end;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);

    // Create window with graphics context
    window_ptr = glfwCreateWindow(1280,
                                  720,
                                  "Window",
                                  nullptr,  /* monitor */
                                  nullptr); /* share   */

    if (window_ptr == nullptr)
    {
        assert(false);

        goto end;
    }

    glfwMakeContextCurrent(window_ptr);
    glfwSwapInterval      (1); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();

    ImGui::CreateContext();

    {
        ImGuiIO& io = ImGui::GetIO();

        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.IniFilename  = nullptr;
    }

    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window_ptr,
                                 true); /* install_callbacks */

    #ifdef __EMSCRIPTEN__
    {
        ImGui_ImplGlfw_InstallEmscriptenCanvasResizeCallback("#canvas");
    }
    #endif

    ImGui_ImplOpenGL3_Init("#version 100");

    // Main loop
    while (!glfwWindowShouldClose(window_ptr) &&
           !m_worker_thread_must_die)
    {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame   ();

        ImGui::NewFrame();
        {
            int display_h = 0;
            int display_w = 0;

            glfwGetFramebufferSize(window_ptr,
                                  &display_w,
                                  &display_h);

            ImGui::Begin("Hello.", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);
            {
                const auto window_size = ImGui::GetWindowSize();

                ImGui::Text("Example message");

                ImGui::SetWindowPos(ImVec2( (display_w - window_size.x) / 2,
                                            (display_h - window_size.y) / 2) );
            }
            ImGui::End();

            ImGui::Render();

            glClear(GL_COLOR_BUFFER_BIT);
        }
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData() );

        glfwSwapBuffers(window_ptr);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown   ();
    ImGui::DestroyContext     ();

    glfwDestroyWindow(window_ptr);
    glfwTerminate    ();

end:
    ;
}
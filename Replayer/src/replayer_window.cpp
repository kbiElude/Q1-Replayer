/* API Interceptor (c) 2024 Dominik Witczak
 *
 * This code is licensed under MIT license (see LICENSE.txt for details)
 */
#include "replayer_window.h"
#include "Common/callbacks.h"
#include "Common/logger.h"
#include "glfw/glfw3.h"
#include "glfw/glfw3native.h"
#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_opengl3.h"
#include <assert.h>
#include <thread>


static void glfw_error_callback(int         error,
                                const char* description)
{
    assert(false && "GLFW error callback received");
}

ReplayerWindow::ReplayerWindow(const std::array<uint32_t, 2>& in_extents)
    :m_extents               (in_extents),
     m_window_ptr            (nullptr),
     m_worker_thread_must_die(false)
{
    /* Stub */
}

ReplayerWindow::~ReplayerWindow()
{
    m_worker_thread_must_die = true;

    m_worker_thread.join();
}

ReplayerWindowUniquePtr ReplayerWindow::create(const std::array<uint32_t, 2>& in_extents)
{
    ReplayerWindowUniquePtr result_ptr(new ReplayerWindow(in_extents) );

    if (result_ptr != nullptr)
    {
        if (!result_ptr->init() )
        {
            result_ptr.reset();
        }
    }

    return result_ptr;
}

bool ReplayerWindow::init()
{
    m_worker_thread = std::thread(&ReplayerWindow::execute,
                                   this);

    return true;
}

void ReplayerWindow::execute()
{
    int result = 1;

    APIInterceptor::disable_callbacks_for_this_thread            ();
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
    m_window_ptr = glfwCreateWindow(m_extents.at(0),
                                  m_extents.at(1),
                                  "Q1 replayer",
                                  nullptr,  /* monitor */
                                  nullptr); /* share   */

    if (m_window_ptr == nullptr)
    {
        assert(false);

        goto end;
    }

    glfwMakeContextCurrent(m_window_ptr);
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
    ImGui_ImplGlfw_InitForOpenGL(m_window_ptr,
                                 true); /* install_callbacks */

    #ifdef __EMSCRIPTEN__
    {
        ImGui_ImplGlfw_InstallEmscriptenCanvasResizeCallback("#canvas");
    }
    #endif

    ImGui_ImplOpenGL3_Init("#version 100");

    // Main loop
    while (!glfwWindowShouldClose(m_window_ptr) &&
           !m_worker_thread_must_die)
    {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame   ();

        ImGui::NewFrame();
        {
            int display_h = 0;
            int display_w = 0;

            glfwGetFramebufferSize(m_window_ptr,
                                  &display_w,
                                  &display_h);

            ImGui::Begin("Hello.", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);
            {
                const auto window_size = ImGui::GetWindowSize();

                ImGui::Text("Press ___ to capture a frame for replay.");

                ImGui::SetWindowPos(ImVec2( (display_w - window_size.x) / 2,
                                            (display_h - window_size.y) / 2) );
            }
            ImGui::End();

            ImGui::Render();

            glClear(GL_COLOR_BUFFER_BIT);
        }
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData() );

        glfwSwapBuffers(m_window_ptr);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown   ();
    ImGui::DestroyContext     ();

    glfwDestroyWindow(m_window_ptr);
    glfwTerminate    ();

end:
    ;
}

void ReplayerWindow::set_position(const std::array<uint32_t, 2>& in_x1y1)
{
    glfwSetWindowPos(m_window_ptr,
                     in_x1y1.at(0),
                     in_x1y1.at(1) );
}
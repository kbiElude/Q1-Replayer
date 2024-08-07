/* API Interceptor (c) 2024 Dominik Witczak
 *
 * This code is licensed under MIT license (see LICENSE.txt for details)
 */
#include "replayer.h"
#include "replayer_apicall_window.h"
#include "Common/callbacks.h"
#include "Common/logger.h"
#include "glfw/glfw3.h"
#include "glfw/glfw3native.h"
#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_opengl3.h"
#include <assert.h>


static void glfw_error_callback(int         error,
                                const char* description)
{
    assert(false && "GLFW error callback received");
}

ReplayerAPICallWindow::ReplayerAPICallWindow()
    :m_window_ptr            (nullptr),
     m_worker_thread_must_die(false)
{
    /* Stub */
}

ReplayerAPICallWindow::~ReplayerAPICallWindow()
{
    m_worker_thread_must_die = true;

    m_worker_thread.join();
}

ReplayerAPICallWindowUniquePtr ReplayerAPICallWindow::create()
{
    ReplayerAPICallWindowUniquePtr result_ptr(
        new ReplayerAPICallWindow()
    );

    assert(result_ptr != nullptr);
    return result_ptr;
}

void ReplayerAPICallWindow::execute()
{
    ImGuiContext* imgui_context_ptr = nullptr;
    int           result            = 1;

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
    glfwWindowHint(GLFW_DEPTH_BITS,            32);

    // Create window with graphics context
    m_window_ptr = glfwCreateWindow(m_window_extents.at(0),
                                    m_window_extents.at(1),
                                    "Q1 API call window",
                                    nullptr,  /* monitor */
                                    nullptr); /* share   */

    if (m_window_ptr == nullptr)
    {
        assert(false);

        goto end;
    }

    glfwSetWindowPos      (m_window_ptr,
                           m_window_x1y1.at(0),
                           m_window_x1y1.at(1) );
    glfwMakeContextCurrent(m_window_ptr);
    glfwSwapInterval      (1); // Enable vsync

    // Setup Dear ImGui context
    {
        std::lock_guard<std::mutex> lock(g_imgui_mutex);

        IMGUI_CHECKVERSION();

        imgui_context_ptr = ImGui::CreateContext();

        ImGui::SetCurrentContext(imgui_context_ptr);

        {
            ImGuiIO& io = ImGui::GetIO();

            io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
            io.IniFilename  = nullptr;
        }

        ImGui::StyleColorsDark();

        // Setup Platform/Renderer backends
        ImGui_ImplGlfw_InitForOpenGL(m_window_ptr,
                                     true); /* install_callbacks */

        ImGui_ImplOpenGL3_Init("#version 100");
    }

    // Main loop
    while (!glfwWindowShouldClose(m_window_ptr) &&
           !m_worker_thread_must_die)
    {
        glfwWaitEventsTimeout(0.5); /* timeout; 0.5 = half a second */

        {
            glClear(GL_COLOR_BUFFER_BIT);

            {
                std::lock_guard<std::mutex> lock(g_imgui_mutex);

                ImGui::SetCurrentContext(imgui_context_ptr);

                ImGui_ImplOpenGL3_NewFrame();
                ImGui_ImplGlfw_NewFrame   ();

                ImGui::NewFrame();
                {
                    int display_h = 0;
                    int display_w = 0;

                    glfwGetFramebufferSize(m_window_ptr,
                                          &display_w,
                                          &display_h);

                    ImGui::Begin("Hello.", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar);
                    {
                        const auto window_size = ImGui::GetWindowSize();

                        ImGui::Text("Press F11 to capture a frame..");

                        ImGui::SetWindowPos(ImVec2( (display_w - window_size.x) / 2,
                                                    (display_h - window_size.y) / 2) );
                    }
                    ImGui::End();
                }

                ImGui::Render                   ();
                ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData() );
            }
        }

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

void ReplayerAPICallWindow::set_position(const std::array<uint32_t, 2>& in_x1y1,
                                         const std::array<uint32_t, 2>& in_extents)
{
    m_window_extents = in_extents;
    m_window_x1y1    = in_x1y1;

    if (m_window_ptr == nullptr)
    {
        /* NOTE: The worker thread uses m_window_* fields when creating the window for the first time. */
        m_worker_thread = std::thread(&ReplayerAPICallWindow::execute,
                                       this);
    }
    else
    {
        glfwSetWindowPos (m_window_ptr,
                          in_x1y1.at(0),
                          in_x1y1.at(1) );
        glfwSetWindowSize(m_window_ptr,
                         in_extents.at(0),
                         in_extents.at(1) );
    }
}
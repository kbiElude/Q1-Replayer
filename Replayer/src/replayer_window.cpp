/* API Interceptor (c) 2024 Dominik Witczak
 *
 * This code is licensed under MIT license (see LICENSE.txt for details)
 */
#include "replayer_snapshot_player.h"
#include "replayer_snapshotter.h"
#include "replayer_window.h"
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

ReplayerWindow::ReplayerWindow(const std::array<uint32_t, 2>& in_extents,
                               ReplayerSnapshotter*           in_snapshotter_ptr,
                               ReplayerSnapshotPlayer*        in_snapshot_player_ptr)
    :m_extents               (in_extents),
     m_snapshot_player_ptr   (in_snapshot_player_ptr),
     m_snapshotter_ptr       (in_snapshotter_ptr),
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

ReplayerWindowUniquePtr ReplayerWindow::create(const std::array<uint32_t, 2>& in_extents,
                                               ReplayerSnapshotter*           in_snapshotter_ptr,
                                               ReplayerSnapshotPlayer*        in_snapshot_player_ptr)
{
    ReplayerWindowUniquePtr result_ptr(new ReplayerWindow(in_extents,
                                                          in_snapshotter_ptr,
                                                          in_snapshot_player_ptr) );

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

                /* If a snapshot has been captured, transfer it to the player */
                {
                    GLIDToTexturePropsMapUniquePtr new_snapshot_gl_id_to_texture_props_map_ptr;
                    ReplayerSnapshotUniquePtr      new_snapshot_ptr;

                    if (m_snapshotter_ptr->pop_snapshot(&new_snapshot_ptr,
                                                        &new_snapshot_gl_id_to_texture_props_map_ptr) )
                    {
                        m_snapshot_player_ptr->load_snapshot(std::move(new_snapshot_ptr),
                                                             std::move(new_snapshot_gl_id_to_texture_props_map_ptr) );
                    }
                }

                if (!m_snapshot_player_ptr->is_snapshot_loaded() )
                {
                    ImGui::Text("Press F11 to capture a frame for replay.");
                }

                ImGui::SetWindowPos(ImVec2( (display_w - window_size.x) / 2,
                                            (display_h - window_size.y) / 2) );
            }
            ImGui::End   ();
            ImGui::Render();

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData() );
        }

        if (m_snapshot_player_ptr->is_snapshot_loaded() )
        {
            m_snapshot_player_ptr->play_snapshot();

            // ImGui::Text("Snapshot available.");
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

void ReplayerWindow::set_position(const std::array<uint32_t, 2>& in_x1y1)
{
    glfwSetWindowPos(m_window_ptr,
                     in_x1y1.at(0),
                     in_x1y1.at(1) );
}
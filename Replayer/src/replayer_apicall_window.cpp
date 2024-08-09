/* API Interceptor (c) 2024 Dominik Witczak
 *
 * This code is licensed under MIT license (see LICENSE.txt for details)
 */
#include "replayer.h"
#include "replayer_apicall_window.h"
#include "Common/callbacks.h"
#include "Common/logger.h"
#include "Common/utils.h"
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

ReplayerAPICallWindow::ReplayerAPICallWindow(Replayer* in_replayer_ptr)
    :m_replayer_ptr                    (in_replayer_ptr),
     m_should_disable_lightmaps        (false),
     m_should_draw_screenspace_geometry(true),
     m_should_draw_weapon              (true),
     m_should_shade_3d_models          (true),
     m_snapshot_ptr                    (nullptr),
     m_window_ptr                      (nullptr),
     m_worker_thread_must_die          (false)
{
    /* Stub */
}

ReplayerAPICallWindow::~ReplayerAPICallWindow()
{
    m_worker_thread_must_die = true;

    m_worker_thread.join();
}

ReplayerAPICallWindowUniquePtr ReplayerAPICallWindow::create(Replayer* in_replayer_ptr)
{
    ReplayerAPICallWindowUniquePtr result_ptr(
        new ReplayerAPICallWindow(in_replayer_ptr)
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
                    int        display_h   = 0;
                    int        display_w   = 0;

                    glfwGetFramebufferSize(m_window_ptr,
                                          &display_w,
                                          &display_h);

                    if (m_snapshot_ptr != nullptr)
                    {
                        const auto api_call_listbox_height = m_window_extents.at(1) * 3 / 4; /* 75% */

                        ImGui::Begin("API calls",
                                     nullptr,
                                     ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize);
                        {
                            bool needs_window_refresh = false;

                            if (ImGui::BeginMenuBar() )
                            {
                                ImGui::TextUnformatted("API calls");
                                ImGui::EndMenuBar     ();
                            }

                            ImGui::SetWindowPos (ImVec2(0, 0) );
                            ImGui::SetWindowSize(ImVec2(static_cast<float>(m_window_extents.at(0) ),
                                                        static_cast<float>(m_window_extents.at(1) )));

                            ImGui::BeginListBox("##ListBox",
                                                ImVec2(static_cast<float>(display_w - 10), static_cast<float>(api_call_listbox_height) ));
                            {
                                bool       command_adjusted         = false;
                                auto       command_enabled_bool_ptr = reinterpret_cast<bool*>(m_replayer_ptr->get_current_snapshot_command_enabled_bool_as_u8_vec_ptr()->data() );
                                const auto n_api_commands           = static_cast<uint32_t>  (m_api_command_vec.size                                                 ()         );

                                for (uint32_t n_api_command = 0;
                                              n_api_command < n_api_commands;
                                            ++n_api_command)
                                {
                                    bool status = command_enabled_bool_ptr[n_api_command];

                                    command_adjusted |= ImGui::Selectable(m_api_command_vec.at(n_api_command).c_str(),
                                                                         &status);

                                    if (status != command_enabled_bool_ptr[n_api_command])
                                    {
                                        command_enabled_bool_ptr[n_api_command] = status;
                                    }
                                }

                                if (command_adjusted)
                                {
                                    m_replayer_ptr->refresh_windows();
                                }
                            }
                            ImGui::EndListBox();

                            ImGui::NewLine ();

                            if (ImGui::Checkbox("Disable lightmaps",
                                                &m_should_disable_lightmaps) )
                            {
                                needs_window_refresh = true;
                            }

                            if (ImGui::Checkbox("Draw screen-space geometry",
                                                &m_should_draw_screenspace_geometry) )
                            {
                                needs_window_refresh = true;
                            }

                            if (ImGui::Checkbox("Draw weapon",
                                                &m_should_draw_weapon) )
                            {
                                needs_window_refresh = true;
                            }

                            if (ImGui::Checkbox("Shade 3D models",
                                                &m_should_shade_3d_models) )
                            {
                                needs_window_refresh = true;
                            }

                            if (needs_window_refresh)
                            {
                                m_replayer_ptr->refresh_windows();
                            }
                        }
                        ImGui::End();
                    }
                    else
                    {
                        ImGui::Begin("Hello.", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar);
                        {
                            ImGui::Text("Press F11 to capture a frame..");
                        }
                        ImGui::End();

                        ImGui::SetWindowPos(ImVec2( static_cast<float>(display_w - m_window_extents.at(0) ) / 2,
                                                    static_cast<float>(display_h - m_window_extents.at(1) ) / 2) );
                    }
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

void ReplayerAPICallWindow::load_snapshot(ReplayerSnapshot* in_snapshot_ptr)
{
    assert(in_snapshot_ptr != nullptr);

    /* Cache the snapshot instance */
    m_snapshot_ptr = in_snapshot_ptr;

    /* Generate API command list from the snapshot in a format that is friendly for consumption
     * at frame rendering time.
     */
    update_api_command_list_vec();
}

void ReplayerAPICallWindow::lock_for_snapshot_access()
{
    m_mutex.lock();
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

void ReplayerAPICallWindow::unlock_for_snapshot_access()
{
    m_mutex.unlock();
}

void ReplayerAPICallWindow::update_api_command_list_vec()
{
    const auto  n_api_commands    = m_snapshot_ptr->get_n_api_commands();
    const auto  n_digits_required = static_cast<uint32_t>             (std::log10(n_api_commands) );

    std::string filler_string;
    std::string temp_string;

    m_api_command_vec.resize(n_api_commands);

    for (uint32_t n_api_command = 0;
                  n_api_command < n_api_commands;
                ++n_api_command)
    {
        if ( (n_api_command % 10) == 0)
        {
            const auto n_zero_digits_needed = n_digits_required - static_cast<uint32_t>(std::log10(n_api_command) );

            filler_string.clear();

            for (uint32_t n_zero = 0;
                          n_zero < n_zero_digits_needed;
                        ++n_zero)
            {
                filler_string += "0";
            }
        }

        APIInterceptor::convert_api_command_to_string(*m_snapshot_ptr->get_api_command_ptr(n_api_command),
                                                      &temp_string);

        m_api_command_vec.at(n_api_command) = filler_string + std::to_string(n_api_command) + ". " + temp_string;
    }
}

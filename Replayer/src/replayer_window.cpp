/* API Interceptor (c) 2024 Dominik Witczak
 *
 * This code is licensed under MIT license (see LICENSE.txt for details)
 */
#include "replayer.h"
#include "replayer_snapshot_player.h"
#include "replayer_snapshotter.h"
#include "replayer_window.h"
#include "Common/callbacks.h"
#include "Common/logger.h"
#include "glfw/glfw3.h"
#include "glfw/glfw3native.h"
#include <assert.h>


static void glfw_error_callback(int         error,
                                const char* description)
{
    assert(false && "GLFW error callback received");
}

ReplayerWindow::ReplayerWindow(const std::array<uint32_t, 2>& in_extents,
                               ReplayerSnapshotter*           in_snapshotter_ptr,
                               ReplayerSnapshotPlayer*        in_snapshot_player_ptr)
    :m_extents                      (in_extents),
     m_n_last_api_command_to_execute(0),
     m_snapshot_player_ptr          (in_snapshot_player_ptr),
     m_snapshotter_ptr              (in_snapshotter_ptr),
     m_window_ptr                   (nullptr),
     m_worker_thread_must_die       (false)
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
    glfwWindowHint(GLFW_DEPTH_BITS,            32);

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

    glfwSetWindowRefreshCallback(m_window_ptr,
                                 [](GLFWwindow* /* in_window_ptr */) { glfwPostEmptyEvent(); });

    glfwMakeContextCurrent(m_window_ptr);
    glfwSwapInterval      (1); // Enable vsync

    // Main loop
    while (!glfwWindowShouldClose(m_window_ptr) &&
           !m_worker_thread_must_die)
    {
        glfwWaitEventsTimeout(0.5); /* timeout; 0.5 = half a second */

        /* If a snapshot has been captured, transfer it to the player */
        {
            GLIDToTexturePropsMapUniquePtr new_snapshot_gl_id_to_texture_props_map_ptr;
            ReplayerSnapshotUniquePtr      new_snapshot_ptr;
            U8VecUniquePtr                 new_snapshot_prev_frame_depth_data_u8_vec_ptr;
            GLContextStateUniquePtr        new_snapshot_start_gl_context_state_ptr;

            if (m_snapshotter_ptr->pop_snapshot(&new_snapshot_start_gl_context_state_ptr,
                                                &new_snapshot_ptr,
                                                &new_snapshot_gl_id_to_texture_props_map_ptr,
                                                &new_snapshot_prev_frame_depth_data_u8_vec_ptr) )
            {
                m_n_last_api_command_to_execute = new_snapshot_ptr->get_n_api_commands() - 1;

                m_snapshot_player_ptr->load_snapshot(std::move(new_snapshot_start_gl_context_state_ptr),
                                                     std::move(new_snapshot_ptr),
                                                     std::move(new_snapshot_gl_id_to_texture_props_map_ptr),
                                                     std::move(new_snapshot_prev_frame_depth_data_u8_vec_ptr) );
            }
        }

        {
            const auto is_snapshot_loaded = m_snapshot_player_ptr->is_snapshot_loaded();

            if (is_snapshot_loaded)
            {
                m_snapshot_player_ptr->play_snapshot(m_n_last_api_command_to_execute);
            }
            else
            {
                glClear(GL_COLOR_BUFFER_BIT);
            }
        }

        glfwSwapBuffers(m_window_ptr);
    }

    glfwDestroyWindow(m_window_ptr);
    glfwTerminate    ();

end:
    ;
}

void ReplayerWindow::refresh()
{
    glfwPostEmptyEvent();
}

void ReplayerWindow::set_position(const std::array<uint32_t, 2>& in_x1y1)
{
    glfwSetWindowPos(m_window_ptr,
                     in_x1y1.at(0),
                     in_x1y1.at(1) );
}
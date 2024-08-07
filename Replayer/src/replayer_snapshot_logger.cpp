/* API Interceptor (c) 2024 Dominik Witczak
 *
 * This code is licensed under MIT license (see LICENSE.txt for details)
 */
#include "replayer_snapshot_logger.h"
#include "Common/utils.h"
#include "OpenGL/utils_enum.h"
#include <iomanip>

ReplayerSnapshotLogger::ReplayerSnapshotLogger()
    :m_n_snapshots_dumped(0)
{
    /* Stub */
}

ReplayerSnapshotLogger::~ReplayerSnapshotLogger()
{
    /* Stub */
}

ReplayerSnapshotLoggerUniquePtr ReplayerSnapshotLogger::create()
{
    ReplayerSnapshotLoggerUniquePtr result_ptr(new ReplayerSnapshotLogger() );

    AI_ASSERT(result_ptr != nullptr);
    return result_ptr;
}

void ReplayerSnapshotLogger::log_snapshot(const GLContextState*        in_start_context_state_ptr,
                                          const ReplayerSnapshot*      in_snapshot_ptr,
                                          const GLIDToTexturePropsMap* in_snapshot_gl_id_to_texture_props_map_ptr)
{
    std::stringstream log_sstream;

    /* Start context state: */
    log_sstream << "-- Start frame state --\n"
                   "\n"
                   "+ Alpha test enabled:   " << in_start_context_state_ptr->alpha_test_enabled   << "\n"
                   "+ Blend enabled:        " << in_start_context_state_ptr->blend_enabled        << "\n"
                   "+ Cull face enabled:    " << in_start_context_state_ptr->cull_face_enabled    << "\n"
                   "+ Depth test enabled:   " << in_start_context_state_ptr->depth_test_enabled   << "\n"
                   "+ Scissor test enabled: " << in_start_context_state_ptr->scissor_test_enabled << "\n"
                   "+ Texture 2D enabled:   " << in_start_context_state_ptr->texture_2d_enabled   << "\n"
                   "\n"
                   "* Alpha function:     "  << OpenGL::Utils::get_raw_string_for_gl_enum(in_start_context_state_ptr->alpha_func_func)    << "\n"
                   "* Alpha reference:    "  << in_start_context_state_ptr->alpha_func_ref                                                << "\n"
                   "* Blend func dfactor: "  << OpenGL::Utils::get_raw_string_for_gl_enum(in_start_context_state_ptr->blend_func_dfactor) << "\n"
                   "* Blend func sfactor: "  << OpenGL::Utils::get_raw_string_for_gl_enum(in_start_context_state_ptr->blend_func_sfactor) << "\n"
                   "* Clear color:        (" << in_start_context_state_ptr->clear_color[0]                                                << ", "
                                             << in_start_context_state_ptr->clear_color[1]                                                << ", "
                                             << in_start_context_state_ptr->clear_color[2]                                                << ", "
                                             << in_start_context_state_ptr->clear_color[3]                                                << ")\n"
                   "* Clear depth:        "  << in_start_context_state_ptr->clear_depth                                                   << "\n"
                   "* Cull face mode:     "  << OpenGL::Utils::get_raw_string_for_gl_enum(in_start_context_state_ptr->cull_face_mode)     << "\n"
                   "* Depth func:         "  << OpenGL::Utils::get_raw_string_for_gl_enum(in_start_context_state_ptr->depth_func)         << "\n"
                   "* Depth mask:         "  << in_start_context_state_ptr->depth_mask                                                    << "\n"
                   "* Depth range:       ("  << in_start_context_state_ptr->depth_range[0]                                                << ", "
                                             << in_start_context_state_ptr->depth_range[1]                                                << ")\n"
                   "* Draw buffer:        "  << OpenGL::Utils::get_raw_string_for_gl_enum(in_start_context_state_ptr->draw_buffer_mode)   << "\n"
                   "* Front face:         "  << OpenGL::Utils::get_raw_string_for_gl_enum(in_start_context_state_ptr->front_face_mode)    << "\n"
                   "* Matrix mode:        "  << OpenGL::Utils::get_raw_string_for_gl_enum(in_start_context_state_ptr->matrix_mode)        << "\n"
                   "* Shade model:        "  << OpenGL::Utils::get_raw_string_for_gl_enum(in_start_context_state_ptr->shade_model)        << "\n"
                   "* Texture env mode:   "  << OpenGL::Utils::get_raw_string_for_gl_enum(in_start_context_state_ptr->texture_env_mode)   << "\n"
                   "* Viewport extents:  ("  << in_start_context_state_ptr->viewport_extents[0]                                           << ", "
                                             << in_start_context_state_ptr->viewport_extents[1]                                           << ")\n"
                   "* Viewport X1Y1:     ("  << in_start_context_state_ptr->viewport_x1y1   [0]                                           << ", "
                                             << in_start_context_state_ptr->viewport_x1y1   [1]                                           << ")\n"
                   "\n"
                   "* Bound 2D texture:   "  << in_start_context_state_ptr->bound_2d_texture_gl_id                                        << "\n"
                   "\n"
                   "\n";

    /* Snapshot API calls: */
    log_sstream << "-- API calls --\n"
                   "\n";

    {
        std::string api_command_string;
        const auto  n_api_commands     = in_snapshot_ptr->get_n_api_commands();

        log_sstream << std::setw(static_cast<std::streamsize>(log10(n_api_commands) ));

        for (uint32_t n_api_command = 0;
                      n_api_command < n_api_commands;
                    ++n_api_command)
        {
            auto api_command_ptr = in_snapshot_ptr->get_api_command_ptr(n_api_command);

            log_sstream << "#" << n_api_command << ": ";

            APIInterceptor::convert_api_command_to_string(*api_command_ptr,
                                                          &api_command_string);

            log_sstream << api_command_string;
        }
    }

    log_sstream << "<-- end of transmission";

    /* Store the log */
    {
        const std::string log_name        = "q1_snapshot" + std::to_string(m_n_snapshots_dumped) + ".log";
        FILE*             file_handle_ptr = ::fopen(log_name.c_str(), "w");

        m_n_snapshots_dumped++;

        AI_ASSERT(file_handle_ptr != nullptr);
        if (file_handle_ptr != nullptr)
        {
            ::fwrite(log_sstream.str().data  (),
                     log_sstream.str().length(),
                     1, /* count */
                     file_handle_ptr);
            ::fclose(file_handle_ptr);
        }
    }

    /* Done */
}
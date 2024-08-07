/* API Interceptor (c) 2024 Dominik Witczak
 *
 * This code is licensed under MIT license (see LICENSE.txt for details)
 */
#include "replayer_snapshot_logger.h"
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
        const auto n_api_commands = in_snapshot_ptr->get_n_api_commands();

        log_sstream << std::setw(static_cast<std::streamsize>(log10(n_api_commands) ));

        for (uint32_t n_api_command = 0;
                      n_api_command < n_api_commands;
                    ++n_api_command)
        {
            auto api_command_ptr = in_snapshot_ptr->get_api_command_ptr(n_api_command);

            log_sstream << "#" << n_api_command << ": ";

            switch (api_command_ptr->api_func)
            {
                case APIInterceptor::APIFUNCTION_GL_GLALPHAFUNC:
                {
                    const auto arg_func = api_command_ptr->api_arg_vec.at(0).get_u32 ();
                    const auto arg_ref  = api_command_ptr->api_arg_vec.at(1).get_fp32();

                    log_sstream << "glAlphaFunc("
                                << OpenGL::Utils::get_raw_string_for_gl_enum(arg_func)
                                << ", "
                                << arg_ref
                                << ")\n";

                    break;
                }

                case APIInterceptor::APIFUNCTION_GL_GLBEGIN:
                {
                    const auto arg_mode = api_command_ptr->api_arg_vec.at(0).get_u32();

                    log_sstream << "glBegin("
                                << OpenGL::Utils::get_raw_string_for_gl_enum(arg_mode)
                                << ")\n";

                    break;
                }

                case APIInterceptor::APIFUNCTION_GL_GLBINDTEXTURE:
                {
                    const auto arg_target  = api_command_ptr->api_arg_vec.at(0).get_u32();
                    const auto arg_texture = api_command_ptr->api_arg_vec.at(1).get_u32();

                    log_sstream << "glBindTexture("
                                << OpenGL::Utils::get_raw_string_for_gl_enum(arg_target)
                                << ", "
                                << arg_texture
                                << ")\n";

                    break;
                }

                case APIInterceptor::APIFUNCTION_GL_GLBLENDFUNC:
                {
                    const auto arg_sfactor = api_command_ptr->api_arg_vec.at(0).get_u32();
                    const auto arg_dfactor = api_command_ptr->api_arg_vec.at(1).get_u32();

                    log_sstream << "glBlendFunc("
                                << OpenGL::Utils::get_raw_string_for_gl_enum(arg_sfactor)
                                << ", "
                                << OpenGL::Utils::get_raw_string_for_gl_enum(arg_dfactor)
                                << ")\n";

                    break;
                }

                case APIInterceptor::APIFUNCTION_GL_GLCLEAR:
                {
                    const auto arg_mask = api_command_ptr->api_arg_vec.at(0).get_u32();

                    log_sstream << "glClear("
                                << OpenGL::Utils::get_raw_string_for_gl_bitfield(OpenGL::BitfieldType::Clear_Buffer_Mask,
                                                                                 arg_mask)
                                << ")\n";

                    break;
                }

                case APIInterceptor::APIFUNCTION_GL_GLCLEARCOLOR:
                {
                    const auto arg_red   = api_command_ptr->api_arg_vec.at(0).get_fp32();
                    const auto arg_green = api_command_ptr->api_arg_vec.at(1).get_fp32();
                    const auto arg_blue  = api_command_ptr->api_arg_vec.at(2).get_fp32();
                    const auto arg_alpha = api_command_ptr->api_arg_vec.at(3).get_fp32();

                    log_sstream << "glClearColor("
                                << arg_red
                                << ", "
                                << arg_green
                                << ", "
                                << arg_blue
                                << ", "
                                << arg_alpha
                                << ")\n";

                    break;
                }

                case APIInterceptor::APIFUNCTION_GL_GLCLEARDEPTH:
                {
                    const auto arg_depth = api_command_ptr->api_arg_vec.at(0).get_fp64();

                    log_sstream << "glClearDepth("
                                << arg_depth
                                << ")\n";

                    break;
                }

                case APIInterceptor::APIFUNCTION_GL_GLCOLOR3F:
                {
                    const auto arg_x = api_command_ptr->api_arg_vec.at(0).get_fp32();
                    const auto arg_y = api_command_ptr->api_arg_vec.at(1).get_fp32();
                    const auto arg_z = api_command_ptr->api_arg_vec.at(2).get_fp32();

                    log_sstream << "glColor3f("
                                << arg_x
                                << ", "
                                << arg_y
                                << ", "
                                << arg_z
                                << ")\n";

                    break;
                }

                case APIInterceptor::APIFUNCTION_GL_GLCOLOR3UB:
                {
                    const auto arg_x = api_command_ptr->api_arg_vec.at(0).get_u8();
                    const auto arg_y = api_command_ptr->api_arg_vec.at(1).get_u8();
                    const auto arg_z = api_command_ptr->api_arg_vec.at(2).get_u8();

                    log_sstream << "glColor3ub("
                                << static_cast<uint32_t>(arg_x)
                                << ", "
                                << static_cast<uint32_t>(arg_y)
                                << ", "
                                << static_cast<uint32_t>(arg_z)
                                << ")\n";
                    break;
                }

                case APIInterceptor::APIFUNCTION_GL_GLCOLOR4F:
                {
                    const auto arg_x = api_command_ptr->api_arg_vec.at(0).get_fp32();
                    const auto arg_y = api_command_ptr->api_arg_vec.at(1).get_fp32();
                    const auto arg_z = api_command_ptr->api_arg_vec.at(2).get_fp32();
                    const auto arg_w = api_command_ptr->api_arg_vec.at(3).get_fp32();

                    log_sstream << "glColor4f("
                                << arg_x
                                << ", "
                                << arg_y
                                << ", "
                                << arg_z
                                << ", "
                                << arg_w
                                << ")\n";

                    break;
                }

                case APIInterceptor::APIFUNCTION_GL_GLCULLFACE:
                {
                    const auto arg_mode = api_command_ptr->api_arg_vec.at(0).get_u32();

                    log_sstream << "glCullFace("
                                << OpenGL::Utils::get_raw_string_for_gl_enum(arg_mode)
                                << ")\n";

                    break;
                }

                case APIInterceptor::APIFUNCTION_GL_GLDEPTHFUNC:
                {
                    const auto arg_func = api_command_ptr->api_arg_vec.at(0).get_u32();

                    log_sstream << "glDepthFunc("
                                << OpenGL::Utils::get_raw_string_for_gl_enum(arg_func)
                                << ")\n";

                    break;
                }

                case APIInterceptor::APIFUNCTION_GL_GLDEPTHMASK:
                {
                    const auto arg_flag = api_command_ptr->api_arg_vec.at(0).get_u32();

                    log_sstream << "glDepthMask("
                                << arg_flag
                                << ")\n";

                    break;
                }

                case APIInterceptor::APIFUNCTION_GL_GLDEPTHRANGE:
                {
                    const auto arg_n = api_command_ptr->api_arg_vec.at(0).get_fp64();
                    const auto arg_f = api_command_ptr->api_arg_vec.at(1).get_fp64();

                    log_sstream << "glDepthRange("
                                << arg_n
                                << ", "
                                << arg_f
                                << ")\n";

                    break;
                }

                case APIInterceptor::APIFUNCTION_GL_GLDISABLE:
                {
                    const auto arg_cap = api_command_ptr->api_arg_vec.at(0).get_u32();

                    log_sstream << "glDisable("
                                << OpenGL::Utils::get_raw_string_for_gl_enum(arg_cap)
                                << ")\n";

                    break;
                }

                case APIInterceptor::APIFUNCTION_GL_GLDRAWBUFFER:
                {
                    const auto arg_mode = api_command_ptr->api_arg_vec.at(0).get_u32();

                    log_sstream << "glDrawBuffer("
                                << OpenGL::Utils::get_raw_string_for_gl_enum(arg_mode)
                                << ")\n";

                    break;
                }

                case APIInterceptor::APIFUNCTION_GL_GLENABLE:
                {
                    const auto arg_cap = api_command_ptr->api_arg_vec.at(0).get_u32();

                    log_sstream << "glEnable("
                                << OpenGL::Utils::get_raw_string_for_gl_enum(arg_cap)
                                << ")\n";

                    break;
                }

                case APIInterceptor::APIFUNCTION_GL_GLEND:
                {
                    log_sstream << "glEnd()\n";

                    break;
                }

                case APIInterceptor::APIFUNCTION_GL_GLFINISH:
                {
                    log_sstream << "glFinish()\n";

                    break;
                }

                case APIInterceptor::APIFUNCTION_GL_GLFLUSH:
                {
                    log_sstream << "glFlush()\n";

                    break;
                }

                case APIInterceptor::APIFUNCTION_GL_GLFRONTFACE:
                {
                    const auto arg_mode = api_command_ptr->api_arg_vec.at(0).get_u32();

                    log_sstream << "glFrontFace("
                                << OpenGL::Utils::get_raw_string_for_gl_enum(arg_mode)
                                << ")\n";

                    break;
                }

                case APIInterceptor::APIFUNCTION_GL_GLFRUSTUM:
                {
                    const auto arg_left     = api_command_ptr->api_arg_vec.at(0).get_fp64();
                    const auto arg_right    = api_command_ptr->api_arg_vec.at(1).get_fp64();
                    const auto arg_bottom   = api_command_ptr->api_arg_vec.at(2).get_fp64();
                    const auto arg_top      = api_command_ptr->api_arg_vec.at(3).get_fp64();
                    const auto arg_near_val = api_command_ptr->api_arg_vec.at(4).get_fp64();
                    const auto arg_far_val  = api_command_ptr->api_arg_vec.at(5).get_fp64();

                    log_sstream << "glFrustum("
                                << arg_left
                                << ", "
                                << arg_right
                                << ", "
                                << arg_bottom
                                << ", "
                                << arg_top
                                << ", "
                                << arg_near_val
                                << ", "
                                << arg_far_val
                                << ")\n";

                    break;
                }

                case APIInterceptor::APIFUNCTION_GL_GLLOADIDENTITY:
                {
                    log_sstream << "glLoadIdentity()\n";

                    break;
                }

                case APIInterceptor::APIFUNCTION_GL_GLMATRIXMODE:
                {
                    const auto arg_mode = api_command_ptr->api_arg_vec.at(0).get_u32();

                    log_sstream << "glMatrixMode("
                                << OpenGL::Utils::get_raw_string_for_gl_enum(arg_mode)
                                << ")\n";

                    break;
                }

                case APIInterceptor::APIFUNCTION_GL_GLORTHO:
                {
                    const auto arg_left     = api_command_ptr->api_arg_vec.at(0).get_fp64();
                    const auto arg_right    = api_command_ptr->api_arg_vec.at(1).get_fp64();
                    const auto arg_bottom   = api_command_ptr->api_arg_vec.at(2).get_fp64();
                    const auto arg_top      = api_command_ptr->api_arg_vec.at(3).get_fp64();
                    const auto arg_near_val = api_command_ptr->api_arg_vec.at(4).get_fp64();
                    const auto arg_far_val  = api_command_ptr->api_arg_vec.at(5).get_fp64();

                    log_sstream << "glOrtho("
                                << arg_left
                                << ", "
                                << arg_right
                                << ", "
                                << arg_bottom
                                << ", "
                                << arg_top
                                << ", "
                                << arg_near_val
                                << ", "
                                << arg_far_val
                                << ")\n";

                    break;
                }

                case APIInterceptor::APIFUNCTION_GL_GLPOPMATRIX:
                {
                    log_sstream << "glPopMatrix()\n";

                    break;
                }

                case APIInterceptor::APIFUNCTION_GL_GLPUSHMATRIX:
                {
                    log_sstream << "glPushMatrix()\n";

                    break;
                }

                case APIInterceptor::APIFUNCTION_GL_GLROTATEF:
                {
                    const auto arg_angle = api_command_ptr->api_arg_vec.at(0).get_fp32();
                    const auto arg_x     = api_command_ptr->api_arg_vec.at(1).get_fp32();
                    const auto arg_y     = api_command_ptr->api_arg_vec.at(2).get_fp32();
                    const auto arg_z     = api_command_ptr->api_arg_vec.at(3).get_fp32();

                    log_sstream << "glRotatef("
                                << arg_angle
                                << ", "
                                << arg_x
                                << ", "
                                << arg_y
                                << ", "
                                << arg_z
                                << ")\n";

                    break;
                }

                case APIInterceptor::APIFUNCTION_GL_GLSCALEF:
                {
                    const auto arg_x = api_command_ptr->api_arg_vec.at(0).get_fp32();
                    const auto arg_y = api_command_ptr->api_arg_vec.at(1).get_fp32();
                    const auto arg_z = api_command_ptr->api_arg_vec.at(2).get_fp32();

                    log_sstream << "glScalef("
                                << arg_x
                                << ", "
                                << arg_y
                                << ", "
                                << arg_z
                                << ")\n";

                    break;
                }

                case APIInterceptor::APIFUNCTION_GL_GLSHADEMODEL:
                {
                    const auto arg_mode = api_command_ptr->api_arg_vec.at(0).get_u32();

                    log_sstream << "glShaderModel("
                                << OpenGL::Utils::get_raw_string_for_gl_enum(arg_mode)
                                << ")\n";

                    break;
                }

                case APIInterceptor::APIFUNCTION_GL_GLTEXCOORD2F:
                {
                    const auto arg_s = api_command_ptr->api_arg_vec.at(0).get_fp32();
                    const auto arg_t = api_command_ptr->api_arg_vec.at(1).get_fp32();

                    log_sstream << "glTexCoord2f("
                                << arg_s
                                << ", "
                                << arg_t
                                << ")\n";

                    break;
                }

                case APIInterceptor::APIFUNCTION_GL_GLTEXENVF:
                {
                    const auto arg_target = api_command_ptr->api_arg_vec.at(0).get_u32 ();
                    const auto arg_pname  = api_command_ptr->api_arg_vec.at(1).get_u32 ();
                    const auto arg_param  = api_command_ptr->api_arg_vec.at(2).get_fp32();

                    log_sstream << "glTexEnvf("
                                << OpenGL::Utils::get_raw_string_for_gl_enum(arg_target)
                                << ", "
                                << OpenGL::Utils::get_raw_string_for_gl_enum(arg_pname)
                                << ", "
                                << arg_param
                                << ")\n";

                    break;
                }

                case APIInterceptor::APIFUNCTION_GL_GLTEXIMAGE2D:
                {
                    const auto arg_target          = api_command_ptr->api_arg_vec.at(0).get_u32();
                    const auto arg_level           = api_command_ptr->api_arg_vec.at(1).get_i32();
                    const auto arg_internal_format = api_command_ptr->api_arg_vec.at(2).get_i32();
                    const auto arg_width           = api_command_ptr->api_arg_vec.at(3).get_i32();
                    const auto arg_height          = api_command_ptr->api_arg_vec.at(4).get_i32();
                    const auto arg_border          = api_command_ptr->api_arg_vec.at(5).get_i32();
                    const auto arg_format          = api_command_ptr->api_arg_vec.at(6).get_u32();
                    const auto arg_type            = api_command_ptr->api_arg_vec.at(7).get_u32();
                    const auto arg_data            = api_command_ptr->api_arg_vec.at(8).get_ptr();

                    log_sstream << "glTexImage2D("
                                << OpenGL::Utils::get_raw_string_for_gl_enum(arg_target)
                                << ", "
                                << arg_level
                                << ", "
                                << OpenGL::Utils::get_raw_string_for_gl_enum(arg_internal_format)
                                << ", "
                                << arg_width
                                << ", "
                                << arg_height
                                << ", "
                                << arg_border
                                << ", "
                                << OpenGL::Utils::get_raw_string_for_gl_enum(arg_format)
                                << ", "
                                << OpenGL::Utils::get_raw_string_for_gl_enum(arg_type)
                                << ", "
                                << arg_data
                                << ")\n";

                    break;
                }

                case APIInterceptor::APIFUNCTION_GL_GLTEXPARAMETERF:
                {
                    const auto arg_target = api_command_ptr->api_arg_vec.at(0).get_u32 ();
                    const auto arg_pname  = api_command_ptr->api_arg_vec.at(1).get_u32 ();
                    const auto arg_param  = api_command_ptr->api_arg_vec.at(2).get_fp32();

                    log_sstream << "glTexParameterf("
                                << OpenGL::Utils::get_raw_string_for_gl_enum(arg_target)
                                << ", "
                                << OpenGL::Utils::get_raw_string_for_gl_enum(arg_pname)
                                << ", "
                                << arg_param
                                << ")\n";

                    break;
                }

                case APIInterceptor::APIFUNCTION_GL_GLTRANSLATEF:
                {
                    const auto arg_x = api_command_ptr->api_arg_vec.at(0).get_fp32();
                    const auto arg_y = api_command_ptr->api_arg_vec.at(1).get_fp32();
                    const auto arg_z = api_command_ptr->api_arg_vec.at(2).get_fp32();

                    log_sstream << "glTranslatef("
                                << arg_x
                                << ", "
                                << arg_y
                                << ", "
                                << arg_z
                                << ")\n";

                    break;
                }

                case APIInterceptor::APIFUNCTION_GL_GLVERTEX2F:
                {
                    const auto arg_x = api_command_ptr->api_arg_vec.at(0).get_fp32();
                    const auto arg_y = api_command_ptr->api_arg_vec.at(1).get_fp32();

                    log_sstream << "glVertex2f("
                                << arg_x
                                << ", "
                                << arg_y
                                << ")\n";

                    break;
                }

                case APIInterceptor::APIFUNCTION_GL_GLVERTEX3F:
                {
                    const auto arg_x = api_command_ptr->api_arg_vec.at(0).get_fp32();
                    const auto arg_y = api_command_ptr->api_arg_vec.at(1).get_fp32();
                    const auto arg_z = api_command_ptr->api_arg_vec.at(2).get_fp32();

                    log_sstream << "glVertex3f("
                                << arg_x
                                << ", "
                                << arg_y
                                << ", "
                                << arg_z
                                << ")\n";

                    break;
                }

                case APIInterceptor::APIFUNCTION_GL_GLVERTEX4F:
                {
                    const auto arg_x = api_command_ptr->api_arg_vec.at(0).get_fp32();
                    const auto arg_y = api_command_ptr->api_arg_vec.at(1).get_fp32();
                    const auto arg_z = api_command_ptr->api_arg_vec.at(2).get_fp32();
                    const auto arg_w = api_command_ptr->api_arg_vec.at(3).get_fp32();

                    log_sstream << "glVertex3f("
                                << arg_x
                                << ", "
                                << arg_y
                                << ", "
                                << arg_z
                                << ", "
                                << arg_w
                                << ")\n";

                    break;
                }

                case APIInterceptor::APIFUNCTION_GL_GLVIEWPORT:
                {
                    const auto arg_x      = api_command_ptr->api_arg_vec.at(0).get_i32();
                    const auto arg_y      = api_command_ptr->api_arg_vec.at(1).get_i32();
                    const auto arg_width  = api_command_ptr->api_arg_vec.at(2).get_i32();
                    const auto arg_height = api_command_ptr->api_arg_vec.at(3).get_i32();

                    log_sstream << "glViewport("
                                << arg_x
                                << ", "
                                << arg_y
                                << ", "
                                << arg_width
                                << ", "
                                << arg_height
                                << ")\n";

                    break;
                }

                default:
                {
                    assert(false);
                }
            }
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
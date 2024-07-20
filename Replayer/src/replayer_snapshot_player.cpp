/* API Interceptor (c) 2024 Dominik Witczak
 *
 * This code is licensed under MIT license (see LICENSE.txt for details)
 */
#include "OpenGL/globals.h"
#include "replayer_snapshot_player.h"

ReplayerSnapshotPlayer::ReplayerSnapshotPlayer()
    :m_snapshot_initialized(false)
{
    /* Stub */
}

ReplayerSnapshotPlayer::~ReplayerSnapshotPlayer()
{
    /* Stub */
}

ReplayerSnapshotPlayerUniquePtr ReplayerSnapshotPlayer::create()
{
    ReplayerSnapshotPlayerUniquePtr result_ptr(new ReplayerSnapshotPlayer() );

    assert(result_ptr != nullptr);
    return result_ptr;
}

bool ReplayerSnapshotPlayer::is_snapshot_loaded() const
{
    return (m_snapshot_ptr != nullptr);
}

void ReplayerSnapshotPlayer::load_snapshot(GLContextStateUniquePtr        in_start_context_state_ptr,
                                           ReplayerSnapshotUniquePtr      in_snapshot_ptr,
                                           GLIDToTexturePropsMapUniquePtr in_snapshot_gl_id_to_texture_props_map_ptr)
{
    m_snapshot_gl_id_to_texture_props_map_ptr = std::move(in_snapshot_gl_id_to_texture_props_map_ptr);
    m_snapshot_initialized                    = false;
    m_snapshot_ptr                            = std::move(in_snapshot_ptr);
    m_snapshot_start_gl_context_state_ptr     = std::move(in_start_context_state_ptr);

    {
        std::vector<uint32_t> gl_texture_id_vec;

        gl_texture_id_vec.reserve(m_snapshot_texture_gl_id_to_texture_gl_id_map.size() );

        for (const auto& iterator : m_snapshot_texture_gl_id_to_texture_gl_id_map)
        {
            gl_texture_id_vec.push_back(iterator.second);
        }

        reinterpret_cast<PFNGLDELETETEXTURESPROC>(OpenGL::g_cached_gl_delete_textures)(static_cast<GLsizei>  (gl_texture_id_vec.size() ),
                                                                                       gl_texture_id_vec.data() );

        m_snapshot_texture_gl_id_to_texture_gl_id_map.clear();
    }
}

void ReplayerSnapshotPlayer::play_snapshot()
{
    assert(m_snapshot_ptr != nullptr);

    const auto n_api_commands = m_snapshot_ptr->get_n_api_commands();

    if (!m_snapshot_initialized)
    {
        assert(m_snapshot_gl_id_to_texture_props_map_ptr != nullptr);

        /* Set up textures.. */
        for (const auto& iterator : *m_snapshot_gl_id_to_texture_props_map_ptr)
        {
            uint32_t    new_texture_id       =  0;
            const auto& reference_texture_id =  iterator.first;
            const auto  texture_props_ptr    = &iterator.second;
            const auto  texture_n_mips       =  static_cast<uint32_t>(texture_props_ptr->mip_props_vec.size() );

            assert(reference_texture_id    != 0);
            assert(texture_props_ptr->type == TextureType::_2D);

            /* Initialize the textures used by snapshot */
            reinterpret_cast<PFNGLGENTEXTURESPROC>(OpenGL::g_cached_gl_gen_textures)(1,
                                                                                    &new_texture_id);

            assert(m_snapshot_texture_gl_id_to_texture_gl_id_map.find(reference_texture_id) == m_snapshot_texture_gl_id_to_texture_gl_id_map.end() );
            assert(new_texture_id                                                           != 0);

            m_snapshot_texture_gl_id_to_texture_gl_id_map[reference_texture_id] = new_texture_id;

            reinterpret_cast<PFNGLBINDTEXTUREPROC>(OpenGL::g_cached_gl_bind_texture)(GL_TEXTURE_2D,
                                                                                     new_texture_id);

            for (uint32_t n_mip = 0;
                          n_mip < texture_n_mips;
                        ++n_mip)
            {
                const auto texture_mip_props_ptr = &texture_props_ptr->mip_props_vec.at(n_mip);

                reinterpret_cast<PFNGLTEXIMAGE2DPROC>(OpenGL::g_cached_gl_tex_image_2d)(GL_TEXTURE_2D,
                                                                                        n_mip,
                                                                                        texture_props_ptr->internal_format,
                                                                                        texture_mip_props_ptr->mip_size_u32vec3.at(0),
                                                                                        texture_mip_props_ptr->mip_size_u32vec3.at(1),
                                                                                        texture_props_ptr->border,
                                                                                        texture_mip_props_ptr->format,
                                                                                        texture_mip_props_ptr->type,
                                                                                        texture_mip_props_ptr->data_u8_vec.data() );
            }
        }

        m_snapshot_initialized = true;
    }

    /* Set up global state. */
    {
        assert(m_snapshot_start_gl_context_state_ptr != nullptr);

        struct
        {
            GLenum capability;
            bool   state;
        } states_config[] =
        {
            {GL_ALPHA_TEST,   m_snapshot_start_gl_context_state_ptr->alpha_test_enabled},
            {GL_BLEND,        m_snapshot_start_gl_context_state_ptr->blend_enabled},
            {GL_CULL_FACE,    m_snapshot_start_gl_context_state_ptr->cull_face_enabled},
            {GL_DEPTH_TEST,   m_snapshot_start_gl_context_state_ptr->depth_test_enabled},
            {GL_SCISSOR_TEST, m_snapshot_start_gl_context_state_ptr->scissor_test_enabled},
            {GL_TEXTURE_2D,   m_snapshot_start_gl_context_state_ptr->texture_2d_enabled},
        };

        for (const auto& current_state_config : states_config)
        {
            if (current_state_config.state)
            {
                reinterpret_cast<PFNGLENABLEPROC>(OpenGL::g_cached_gl_enable)(current_state_config.capability);
            }
            else
            {
                reinterpret_cast<PFNGLDISABLEPROC>(OpenGL::g_cached_gl_disable)(current_state_config.capability);
            }
        }

        reinterpret_cast<PFNGLALPHAFUNCPROC> (OpenGL::g_cached_gl_alpha_func) (m_snapshot_start_gl_context_state_ptr->alpha_func_func,
                                                                               m_snapshot_start_gl_context_state_ptr->alpha_func_ref);
        reinterpret_cast<PFNGLSHADEMODELPROC>(OpenGL::g_cached_gl_shade_model)(m_snapshot_start_gl_context_state_ptr->shade_model);

        {
            const auto pfn_gl_bind_texture   = reinterpret_cast<PFNGLBINDTEXTUREPROC>  (OpenGL::g_cached_gl_bind_texture);
            const auto pfn_gl_tex_parameterf = reinterpret_cast<PFNGLTEXPARAMETERFPROC>(OpenGL::g_cached_gl_tex_parameterf);

            for (const auto& iterator : m_snapshot_start_gl_context_state_ptr->gl_texture_id_to_texture_state_map)
            {
                const auto& snapshot_gl_texture_id = iterator.first;
                const auto& texture_state          = iterator.second;
                const auto  gl_texture_id          = m_snapshot_texture_gl_id_to_texture_gl_id_map.at(snapshot_gl_texture_id);

                pfn_gl_bind_texture(GL_TEXTURE_2D,
                                    gl_texture_id);

                pfn_gl_tex_parameterf(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, static_cast<float>(texture_state.base_level) );
                pfn_gl_tex_parameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, static_cast<float>(texture_state.mag_filter) );
                pfn_gl_tex_parameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL,  static_cast<float>(texture_state.max_level)  );
                pfn_gl_tex_parameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD,    static_cast<float>(texture_state.max_lod)    );
                pfn_gl_tex_parameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, static_cast<float>(texture_state.min_filter) );
                pfn_gl_tex_parameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_LOD,    static_cast<float>(texture_state.min_lod)    );
                pfn_gl_tex_parameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R,     static_cast<float>(texture_state.wrap_r)     );
                pfn_gl_tex_parameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     static_cast<float>(texture_state.wrap_s)     );
                pfn_gl_tex_parameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     static_cast<float>(texture_state.wrap_t)     );
            }
        }
    }

    reinterpret_cast<PFNGLCLEARCOLORPROC>(OpenGL::g_cached_gl_clear_color)(m_snapshot_start_gl_context_state_ptr->clear_color[0],
                                                                           m_snapshot_start_gl_context_state_ptr->clear_color[1],
                                                                           m_snapshot_start_gl_context_state_ptr->clear_color[2],
                                                                           m_snapshot_start_gl_context_state_ptr->clear_color[3]);

    for (uint32_t n_api_command = 0;
                  n_api_command < n_api_commands;
                ++n_api_command)
    {
        const auto api_command_ptr = m_snapshot_ptr->get_api_command_ptr(n_api_command);

        switch (api_command_ptr->api_func)
        {
            case APIInterceptor::APIFUNCTION_GL_GLACCUM:
            {
                reinterpret_cast<PFNGLACCUMPROC>(OpenGL::g_cached_gl_accum)(api_command_ptr->api_arg_vec.at(0).value.value_u32,
                                                                            api_command_ptr->api_arg_vec.at(1).value.value_fp32);

                break;
            }

            case APIInterceptor::APIFUNCTION_GL_GLACTIVETEXTURE:
            {
                reinterpret_cast<PFNGLACTIVETEXTUREPROC>(OpenGL::g_cached_gl_active_texture)(api_command_ptr->api_arg_vec.at(0).value.value_u32);

                break;
            }

            case APIInterceptor::APIFUNCTION_GL_GLALPHAFUNC:
            {
                reinterpret_cast<PFNGLALPHAFUNCPROC>(OpenGL::g_cached_gl_alpha_func)(api_command_ptr->api_arg_vec.at(0).value.value_u32,
                                                                                     api_command_ptr->api_arg_vec.at(1).value.value_fp32);

                break;
            }

            case APIInterceptor::APIFUNCTION_GL_GLBEGIN:
            {
                reinterpret_cast<PFNGLBEGINPROC>(OpenGL::g_cached_gl_begin)(api_command_ptr->api_arg_vec.at(0).value.value_u32);

                break;
            }

            case APIInterceptor::APIFUNCTION_GL_GLBINDTEXTURE:
            {
                const auto snapshot_texture_id = api_command_ptr->api_arg_vec.at(1).value.value_u32;
                const auto this_texture_id     = m_snapshot_texture_gl_id_to_texture_gl_id_map.at(snapshot_texture_id);

                reinterpret_cast<PFNGLBINDTEXTUREPROC>(OpenGL::g_cached_gl_bind_texture)(api_command_ptr->api_arg_vec.at(0).value.value_u32,
                                                                                         this_texture_id);

                break;
            }

            case APIInterceptor::APIFUNCTION_GL_GLCLEAR:
            {
                reinterpret_cast<PFNGLCLEARPROC>(OpenGL::g_cached_gl_clear)(api_command_ptr->api_arg_vec.at(0).value.value_u32);

                break;
            }

            case APIInterceptor::APIFUNCTION_GL_GLCLEARCOLOR:
            {
                reinterpret_cast<PFNGLCLEARCOLORPROC>(OpenGL::g_cached_gl_clear_color)(api_command_ptr->api_arg_vec.at(0).value.value_fp32,
                                                                                       api_command_ptr->api_arg_vec.at(1).value.value_fp32,
                                                                                       api_command_ptr->api_arg_vec.at(2).value.value_fp32,
                                                                                       api_command_ptr->api_arg_vec.at(3).value.value_fp32);

                break;
            }

            case APIInterceptor::APIFUNCTION_GL_GLCLEARDEPTH:
            {
                reinterpret_cast<PFNGLCLEARDEPTHPROC>(OpenGL::g_cached_gl_clear_depth)(api_command_ptr->api_arg_vec.at(0).value.value_fp64);

                break;
            }

            case APIInterceptor::APIFUNCTION_GL_GLCLEARSTENCIL:
            {
                reinterpret_cast<PFNGLCLEARSTENCILPROC>(OpenGL::g_cached_gl_clear_stencil)(api_command_ptr->api_arg_vec.at(0).value.value_i32);

                break;
            }

            case APIInterceptor::APIFUNCTION_GL_GLCOLOR4F:
            {
                reinterpret_cast<PFNGLCOLOR4FPROC>(OpenGL::g_cached_gl_color_4f)(api_command_ptr->api_arg_vec.at(0).value.value_fp32,
                                                                                 api_command_ptr->api_arg_vec.at(1).value.value_fp32,
                                                                                 api_command_ptr->api_arg_vec.at(2).value.value_fp32,
                                                                                 api_command_ptr->api_arg_vec.at(3).value.value_fp32);

                break;
            }

            case APIInterceptor::APIFUNCTION_GL_GLCULLFACE:
            {
                reinterpret_cast<PFNGLCULLFACEPROC>(OpenGL::g_cached_gl_cull_face)(api_command_ptr->api_arg_vec.at(0).value.value_u32);

                break;
            }

            case APIInterceptor::APIFUNCTION_GL_GLDEPTHFUNC:
            {
                reinterpret_cast<PFNGLDEPTHFUNCPROC>(OpenGL::g_cached_gl_depth_func)(api_command_ptr->api_arg_vec.at(0).value.value_u32);

                break;
            }

            case APIInterceptor::APIFUNCTION_GL_GLDISABLE:
            {
                reinterpret_cast<PFNGLDISABLEPROC>(OpenGL::g_cached_gl_disable)(api_command_ptr->api_arg_vec.at(0).value.value_u32);

                break;
            }

            case APIInterceptor::APIFUNCTION_GL_GLENABLE:
            {
                reinterpret_cast<PFNGLENABLEPROC>(OpenGL::g_cached_gl_enable)(api_command_ptr->api_arg_vec.at(0).value.value_u32);

                break;
            }

            case APIInterceptor::APIFUNCTION_GL_GLEND:
            {
                reinterpret_cast<PFNGLENDPROC>(OpenGL::g_cached_gl_end)();

                break;
            }

            case APIInterceptor::APIFUNCTION_GL_GLFINISH:
            {
                // No need to replay this.

                break;
            }

            case APIInterceptor::APIFUNCTION_GL_GLFLUSH:
            {
                // No need to replay this.

                break;
            }

            case APIInterceptor::APIFUNCTION_GL_GLFRONTFACE:
            {
                reinterpret_cast<PFNGLFRONTFACEPROC>(OpenGL::g_cached_gl_front_face)(api_command_ptr->api_arg_vec.at(0).value.value_u32);

                break;
            }

            case APIInterceptor::APIFUNCTION_GL_GLFRUSTUM:
            {
                reinterpret_cast<PFNGLFRUSTUMPROC>(OpenGL::g_cached_gl_frustum)(api_command_ptr->api_arg_vec.at(0).value.value_fp64,
                                                                                api_command_ptr->api_arg_vec.at(1).value.value_fp64,
                                                                                api_command_ptr->api_arg_vec.at(2).value.value_fp64,
                                                                                api_command_ptr->api_arg_vec.at(3).value.value_fp64,
                                                                                api_command_ptr->api_arg_vec.at(4).value.value_fp64,
                                                                                api_command_ptr->api_arg_vec.at(5).value.value_fp64);

                break;
            }

            case APIInterceptor::APIFUNCTION_GL_GLLOADIDENTITY:
            {
                reinterpret_cast<PFNGLLOADIDENTITYPROC>(OpenGL::g_cached_gl_load_identity)();

                break;
            }

            case APIInterceptor::APIFUNCTION_GL_GLLOADMATRIXF:
            {
                assert(false); // todo

                //reinterpret_cast<PFNGLLOADMATRIXFPROC>(OpenGL::g_cached_gl_load_matrix_f)(api_command_ptr->api_arg_vec.at(0).value.value_fp32_ptr);

                break;
            }

            case APIInterceptor::APIFUNCTION_GL_GLMATRIXMODE:
            {
                reinterpret_cast<PFNGLMATRIXMODEPROC>(OpenGL::g_cached_gl_matrix_mode)(api_command_ptr->api_arg_vec.at(0).value.value_u32);

                break;
            }

            case APIInterceptor::APIFUNCTION_GL_GLMULTMATRIXF:
            {
                assert(false); // todo

                //reinterpret_cast<PFNGLMULTMATRIXFPROC>(OpenGL::g_cached_gl_mult_matrix_f)(api_command_ptr->api_arg_vec.at(0).value.value_fp32_ptr);

                break;
            }

            case APIInterceptor::APIFUNCTION_GL_GLORTHO:
            {
                reinterpret_cast<PFNGLORTHOPROC>(OpenGL::g_cached_gl_ortho)(api_command_ptr->api_arg_vec.at(0).value.value_fp64,
                                                                            api_command_ptr->api_arg_vec.at(1).value.value_fp64,
                                                                            api_command_ptr->api_arg_vec.at(2).value.value_fp64,
                                                                            api_command_ptr->api_arg_vec.at(3).value.value_fp64,
                                                                            api_command_ptr->api_arg_vec.at(4).value.value_fp64,
                                                                            api_command_ptr->api_arg_vec.at(5).value.value_fp64);

                break;
            }

            case APIInterceptor::APIFUNCTION_GL_GLPOLYGONMODE:
            {
                reinterpret_cast<PFNGLPOLYGONMODEPROC>(OpenGL::g_cached_gl_polygon_mode)(api_command_ptr->api_arg_vec.at(0).value.value_u32,
                                                                                         api_command_ptr->api_arg_vec.at(1).value.value_u32);

                break;
            }

            case APIInterceptor::APIFUNCTION_GL_GLREADPIXELS:
            {
                // No need to replay this

                break;
            }

            case APIInterceptor::APIFUNCTION_GL_GLROTATEF:
            {
                reinterpret_cast<PFNGLROTATEFPROC>(OpenGL::g_cached_gl_rotate_f)(api_command_ptr->api_arg_vec.at(0).value.value_fp32,
                                                                                 api_command_ptr->api_arg_vec.at(1).value.value_fp32,
                                                                                 api_command_ptr->api_arg_vec.at(2).value.value_fp32,
                                                                                 api_command_ptr->api_arg_vec.at(3).value.value_fp32);

                break;
            }

            case APIInterceptor::APIFUNCTION_GL_GLSCALEF:
            {
                reinterpret_cast<PFNGLSCALEFPROC>(OpenGL::g_cached_gl_scale_f)(api_command_ptr->api_arg_vec.at(0).value.value_fp32,
                                                                               api_command_ptr->api_arg_vec.at(1).value.value_fp32,
                                                                               api_command_ptr->api_arg_vec.at(2).value.value_fp32);

                break;
            }

            case APIInterceptor::APIFUNCTION_GL_GLSHADEMODEL:
            {
                reinterpret_cast<PFNGLSHADEMODELPROC>(OpenGL::g_cached_gl_shade_model)(api_command_ptr->api_arg_vec.at(0).value.value_u32);

                break;
            }

            case APIInterceptor::APIFUNCTION_GL_GLTEXCOORD2F:
            {
                reinterpret_cast<PFNGLTEXCOORD2FPROC>(OpenGL::g_cached_gl_tex_coord_2f)(api_command_ptr->api_arg_vec.at(0).value.value_fp32,
                                                                                        api_command_ptr->api_arg_vec.at(1).value.value_fp32);

                break;
            }

            case APIInterceptor::APIFUNCTION_GL_GLTEXIMAGE2D:
            {
                reinterpret_cast<PFNGLTEXIMAGE2DPROC>(OpenGL::g_cached_gl_tex_image_2d)(api_command_ptr->api_arg_vec.at(0).value.value_u32,
                                                                                        api_command_ptr->api_arg_vec.at(1).value.value_i32,
                                                                                        api_command_ptr->api_arg_vec.at(2).value.value_i32,
                                                                                        api_command_ptr->api_arg_vec.at(3).value.value_i32,
                                                                                        api_command_ptr->api_arg_vec.at(4).value.value_i32,
                                                                                        api_command_ptr->api_arg_vec.at(5).value.value_i32,
                                                                                        api_command_ptr->api_arg_vec.at(6).value.value_u32,
                                                                                        api_command_ptr->api_arg_vec.at(7).value.value_u32,
                                                                                        api_command_ptr->api_arg_vec.at(8).value.value_ptr);

                break;
            }

            case APIInterceptor::APIFUNCTION_GL_GLTEXPARAMETERF:
            {
                reinterpret_cast<PFNGLTEXPARAMETERFPROC>(OpenGL::g_cached_gl_tex_parameterf)(api_command_ptr->api_arg_vec.at(0).value.value_u32,
                                                                                             api_command_ptr->api_arg_vec.at(1).value.value_u32,
                                                                                             api_command_ptr->api_arg_vec.at(2).value.value_fp32);

                break;
            }

            case APIInterceptor::APIFUNCTION_GL_GLTRANSLATEF:
            {
                reinterpret_cast<PFNGLTRANSLATEFPROC>(OpenGL::g_cached_gl_translate_f)(api_command_ptr->api_arg_vec.at(0).value.value_fp32,
                                                                                       api_command_ptr->api_arg_vec.at(1).value.value_fp32,
                                                                                       api_command_ptr->api_arg_vec.at(2).value.value_fp32);

                break;
            }

            case APIInterceptor::APIFUNCTION_GL_GLVERTEX2F:
            {
                reinterpret_cast<PFNGLVERTEX2FPROC>(OpenGL::g_cached_gl_vertex_2f)(api_command_ptr->api_arg_vec.at(0).value.value_fp32,
                                                                                   api_command_ptr->api_arg_vec.at(1).value.value_fp32);

                break;
            }

            case APIInterceptor::APIFUNCTION_GL_GLVERTEX3F:
            {
                reinterpret_cast<PFNGLVERTEX3FPROC>(OpenGL::g_cached_gl_vertex_3f)(api_command_ptr->api_arg_vec.at(0).value.value_fp32,
                                                                                   api_command_ptr->api_arg_vec.at(1).value.value_fp32,
                                                                                   api_command_ptr->api_arg_vec.at(2).value.value_fp32);

                break;
            }

            case APIInterceptor::APIFUNCTION_GL_GLVERTEX4F:
            {
                reinterpret_cast<PFNGLVERTEX4FPROC>(OpenGL::g_cached_gl_vertex_4f)(api_command_ptr->api_arg_vec.at(0).value.value_fp32,
                                                                                   api_command_ptr->api_arg_vec.at(1).value.value_fp32,
                                                                                   api_command_ptr->api_arg_vec.at(2).value.value_fp32,
                                                                                   api_command_ptr->api_arg_vec.at(3).value.value_fp32);

                break;
            }

            case APIInterceptor::APIFUNCTION_GL_GLVIEWPORT:
            {
                reinterpret_cast<PFNGLVIEWPORTPROC>(OpenGL::g_cached_gl_viewport)(api_command_ptr->api_arg_vec.at(0).value.value_i32,
                                                                                  api_command_ptr->api_arg_vec.at(1).value.value_i32,
                                                                                  api_command_ptr->api_arg_vec.at(2).value.value_i32,
                                                                                  api_command_ptr->api_arg_vec.at(3).value.value_i32);

                break;
            }

            default:
            {
                assert(false);
            }
        }
    }
}
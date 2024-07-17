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

void ReplayerSnapshotPlayer::load_snapshot(ReplayerSnapshotUniquePtr      in_snapshot_ptr,
                                           GLIDToTexturePropsMapUniquePtr in_snapshot_gl_id_to_texture_props_map_ptr)
{
    m_snapshot_gl_id_to_texture_props_map_ptr = std::move(in_snapshot_gl_id_to_texture_props_map_ptr);
    m_snapshot_initialized                    = false;
    m_snapshot_ptr                            = std::move(in_snapshot_ptr);

    m_snapshot_texture_gl_id_to_texture_gl_id_map.clear();
}

void ReplayerSnapshotPlayer::play_snapshot()
{
    assert(m_snapshot_ptr != nullptr);

    const auto n_api_commands = m_snapshot_ptr->get_n_api_commands();

    if (!m_snapshot_initialized)
    {
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

            case APIInterceptor::APIFUNCTION_GL_GLCOLOR3FV:
            {
                assert(false); // todo

                break;
            }

            case APIInterceptor::APIFUNCTION_GL_GLCOLOR4FV:
            {
                assert(false); // todo

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

            case APIInterceptor::APIFUNCTION_GL_GLNORMAL3FV:
            {
                assert(false); // todo

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

            case APIInterceptor::APIFUNCTION_GL_GLTEXCOORD2FV:
            {
                assert(false); // todo

                break;
            }

            case APIInterceptor::APIFUNCTION_GL_GLTEXCOORD3FV:
            {
                assert(false); // todo

                break;
            }

            case APIInterceptor::APIFUNCTION_GL_GLTEXCOORD4FV:
            {
                assert(false); // todo

                break;
            }

            case APIInterceptor::APIFUNCTION_GL_GLTEXPARAMETERF:
            {
                reinterpret_cast<PFNGLTEXPARAMETERFPROC>(OpenGL::g_cached_gl_tex_parameterf)(api_command_ptr->api_arg_vec.at(0).value.value_u32,
                                                                                             api_command_ptr->api_arg_vec.at(1).value.value_u32,
                                                                                             api_command_ptr->api_arg_vec.at(2).value.value_fp32);

                break;
            }

            case APIInterceptor::APIFUNCTION_GL_GLTEXPARAMETERFV:
            {
                assert(false); // todo

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

            case APIInterceptor::APIFUNCTION_GL_GLVERTEX2FV:
            {
                assert(false); // todo

                break;
            }

            case APIInterceptor::APIFUNCTION_GL_GLVERTEX3F:
            {
                reinterpret_cast<PFNGLVERTEX3FPROC>(OpenGL::g_cached_gl_vertex_3f)(api_command_ptr->api_arg_vec.at(0).value.value_fp32,
                                                                                   api_command_ptr->api_arg_vec.at(1).value.value_fp32,
                                                                                   api_command_ptr->api_arg_vec.at(2).value.value_fp32);

                break;
            }

            case APIInterceptor::APIFUNCTION_GL_GLVERTEX3FV:
            {
                assert(false); // todo

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

            case APIInterceptor::APIFUNCTION_GL_GLVERTEX4FV:
            {
                assert(false); // todo

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
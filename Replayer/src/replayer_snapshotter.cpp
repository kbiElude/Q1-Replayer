/* API Interceptor (c) 2024 Dominik Witczak
 *
 * This code is licensed under MIT license (see LICENSE.txt for details)
 */
#include "Common/callbacks.h"
#include "Common/logger.h"
#include "OpenGL/globals.h"
#include <cassert>
#include <functional>
#include "replayer_snapshotter.h"
#include "replayer.h"


ReplayerSnapshotter::ReplayerSnapshotter(const Replayer* in_replayer_ptr)
    :m_is_glbegin_active (false),
     m_replayer_ptr      (in_replayer_ptr),
     m_snapshot_requested(false)
{
    /* Stub */
}

ReplayerSnapshotter::~ReplayerSnapshotter()
{
    /* Stub */
}

void ReplayerSnapshotter::cache_snapshot()
{
    m_snapshot_requested = true;
}

ReplayerSnapshotterUniquePtr ReplayerSnapshotter::create(const Replayer* in_replayer_ptr)
{
    ReplayerSnapshotterUniquePtr result_ptr(new ReplayerSnapshotter(in_replayer_ptr) );

    if (result_ptr != nullptr)
    {
        if (!result_ptr->init() )
        {
            result_ptr.reset();
        }
    }

    return result_ptr;
}

bool ReplayerSnapshotter::init()
{
    /* Initialize object instances. */
    const auto q1_window_extents = m_replayer_ptr->get_q1_window_extents();

    m_current_context_state_ptr.reset         (new GLContextState       (q1_window_extents.at(0),
                                                                         q1_window_extents.at(1) ) );
    m_gl_id_to_texture_props_map_ptr.reset    (new GLIDToTexturePropsMap() );
    m_prev_frame_depth_buffer_u8_vec_ptr.reset(new std::vector<uint8_t> (q1_window_extents.at(0) * q1_window_extents.at(1) * sizeof(float) ));

    /* Initialize callback handlers for all GL entrypoints used by Q1. */
    for (uint32_t current_api_func =  static_cast<uint32_t>(APIInterceptor::APIFUNCTION_GL_FIRST);
                  current_api_func <= static_cast<uint32_t>(APIInterceptor::APIFUNCTION_GL_LAST);
                ++current_api_func)
    {
        APIInterceptor::register_for_callback(static_cast<APIInterceptor::APIFunction>(current_api_func),
                                              ReplayerSnapshotter::on_api_func_callback,
                                              this);
    }

    /* We also need to know when we're done with a snapshot. */
    APIInterceptor::register_for_callback(APIInterceptor::APIFUNCTION_GDI32_SWAPBUFFERS,
                                          ReplayerSnapshotter::on_api_func_callback,
                                          this);

    return true;
}

void ReplayerSnapshotter::on_api_func_callback(APIInterceptor::APIFunction                in_api_func,
                                               uint32_t                                   in_n_args,
                                               const APIInterceptor::APIFunctionArgument* in_args_ptr,
                                               void*                                      in_user_arg_ptr)
{
    auto this_ptr(reinterpret_cast<ReplayerSnapshotter*>(in_user_arg_ptr) );

    if (this_ptr->m_start_gl_context_state_ptr == nullptr)
    {
        this_ptr->m_start_gl_context_state_ptr.reset(new GLContextState(*this_ptr->m_current_context_state_ptr) );

        assert(this_ptr->m_start_gl_context_state_ptr != nullptr);

        /* Query & store matrix state */
        reinterpret_cast<PFNGLGETDOUBLEVPROC>(OpenGL::g_cached_gl_get_doublev)(GL_MODELVIEW_MATRIX,
                                                                               this_ptr->m_start_gl_context_state_ptr->modelview_matrix);
        reinterpret_cast<PFNGLGETDOUBLEVPROC>(OpenGL::g_cached_gl_get_doublev)(GL_PROJECTION_MATRIX,
                                                                               this_ptr->m_start_gl_context_state_ptr->projection_matrix);
    }

    if (this_ptr->m_recording_snapshot_ptr == nullptr)
    {
        this_ptr->m_recording_snapshot_ptr = ReplayerSnapshot::create();

        assert(this_ptr->m_recording_snapshot_ptr != nullptr);
    }

    if (in_api_func != APIInterceptor::APIFUNCTION_GDI32_SWAPBUFFERS)
    {
        bool should_record_api_call = true;

        /* IF this is a GL call which disables/enables a GL capability, cache this information */
        if ( (in_api_func == APIInterceptor::APIFUNCTION_GL_GLDISABLE) || 
             (in_api_func == APIInterceptor::APIFUNCTION_GL_GLENABLE)   )
        {
            const bool should_enable = (in_api_func == APIInterceptor::APIFUNCTION_GL_GLENABLE);

            switch (in_args_ptr[0].get_u32() )
            {
                case GL_ALPHA_TEST:   this_ptr->m_current_context_state_ptr->alpha_test_enabled   = should_enable; break;
                case GL_BLEND:        this_ptr->m_current_context_state_ptr->blend_enabled        = should_enable; break;
                case GL_CULL_FACE:    this_ptr->m_current_context_state_ptr->cull_face_enabled    = should_enable; break;
                case GL_DEPTH_TEST:   this_ptr->m_current_context_state_ptr->depth_test_enabled   = should_enable; break;
                case GL_SCISSOR_TEST: this_ptr->m_current_context_state_ptr->scissor_test_enabled = should_enable; break;
                case GL_TEXTURE_2D:   this_ptr->m_current_context_state_ptr->texture_2d_enabled   = should_enable; break;

                default:
                {
                    assert(false);
                }
            }
        }
        else
        /* If this is a GL call which updates global state we care about, cache it too */
        if (in_api_func == APIInterceptor::APIFUNCTION_GL_GLALPHAFUNC)
        {
            const auto func = in_args_ptr[0].get_u32 ();
            const auto ref  = in_args_ptr[1].get_fp32();

            this_ptr->m_current_context_state_ptr->alpha_func_func = func;
            this_ptr->m_current_context_state_ptr->alpha_func_ref  = ref;
        }
        else
        if (in_api_func == APIInterceptor::APIFUNCTION_GL_GLBLENDFUNC)
        {
            const auto sfactor = in_args_ptr[0].get_u32();
            const auto dfactor = in_args_ptr[1].get_u32();

            this_ptr->m_current_context_state_ptr->blend_func_sfactor = sfactor;
            this_ptr->m_current_context_state_ptr->blend_func_dfactor = dfactor;
        }
        else
        if (in_api_func == APIInterceptor::APIFUNCTION_GL_GLCLEARCOLOR)
        {
            const auto red   = in_args_ptr[0].get_fp32();
            const auto green = in_args_ptr[1].get_fp32();
            const auto blue  = in_args_ptr[2].get_fp32();
            const auto alpha = in_args_ptr[3].get_fp32();

            this_ptr->m_current_context_state_ptr->clear_color[0] = red;
            this_ptr->m_current_context_state_ptr->clear_color[1] = green;
            this_ptr->m_current_context_state_ptr->clear_color[2] = blue;
            this_ptr->m_current_context_state_ptr->clear_color[3] = alpha;
        }
        else
        if (in_api_func == APIInterceptor::APIFUNCTION_GL_GLCLEARDEPTH)
        {
            const auto depth = in_args_ptr[0].get_fp64();

            this_ptr->m_current_context_state_ptr->clear_depth = depth;
        }
        else
        if (in_api_func == APIInterceptor::APIFUNCTION_GL_GLCULLFACE)
        {
            const auto mode = in_args_ptr[0].get_u32();

            this_ptr->m_current_context_state_ptr->cull_face_mode = mode;
        }
        else
        if (in_api_func == APIInterceptor::APIFUNCTION_GL_GLDEPTHFUNC)
        {
            const auto func = in_args_ptr[0].get_u32();

            this_ptr->m_current_context_state_ptr->depth_func = func;
        }
        else
        if (in_api_func == APIInterceptor::APIFUNCTION_GL_GLDEPTHMASK)
        {
            const auto flag = in_args_ptr[0].get_u32();

            this_ptr->m_current_context_state_ptr->depth_mask = (flag != 0);
        }
        else
        if (in_api_func == APIInterceptor::APIFUNCTION_GL_GLDEPTHRANGE)
        {
            const auto n = in_args_ptr[0].get_fp64();
            const auto f = in_args_ptr[1].get_fp64();

            this_ptr->m_current_context_state_ptr->depth_range[0] = n;
            this_ptr->m_current_context_state_ptr->depth_range[1] = f;
        }
        else
        if (in_api_func == APIInterceptor::APIFUNCTION_GL_GLDRAWBUFFER)
        {
            const auto mode = in_args_ptr[0].get_u32();

            this_ptr->m_current_context_state_ptr->draw_buffer_mode = mode;
        }
        else
        if (in_api_func == APIInterceptor::APIFUNCTION_GL_GLFRONTFACE)
        {
            const auto mode = in_args_ptr[0].get_u32();

            this_ptr->m_current_context_state_ptr->front_face_mode = mode;
        }
        else
        if (in_api_func == APIInterceptor::APIFUNCTION_GL_GLMATRIXMODE)
        {
            const auto mode = in_args_ptr[0].get_u32();

            this_ptr->m_current_context_state_ptr->matrix_mode = mode;
        }
        else
        if (in_api_func == APIInterceptor::APIFUNCTION_GL_GLSHADEMODEL)
        {
            const auto mode = in_args_ptr[0].get_u32();

            this_ptr->m_current_context_state_ptr->shade_model = mode;
        }
        else
        if (in_api_func == APIInterceptor::APIFUNCTION_GL_GLTEXENVF)
        {
            AI_ASSERT(in_n_args == 3);

            const auto target = in_args_ptr[0].get_u32 ();
            const auto pname  = in_args_ptr[1].get_u32 ();
            const auto param  = in_args_ptr[2].get_fp32();

            AI_ASSERT(target == GL_TEXTURE_ENV);
            AI_ASSERT(pname  == GL_TEXTURE_ENV_MODE);

            this_ptr->m_current_context_state_ptr->texture_env_mode = static_cast<uint32_t>(param);
        }
        else
        if (in_api_func == APIInterceptor::APIFUNCTION_GL_GLVIEWPORT)
        {
            AI_ASSERT(in_n_args == 4);

            const auto x      = in_args_ptr[0].get_i32();
            const auto y      = in_args_ptr[1].get_i32();
            const auto width  = in_args_ptr[2].get_i32();
            const auto height = in_args_ptr[3].get_i32();

            this_ptr->m_current_context_state_ptr->viewport_x1y1   [0] = x;
            this_ptr->m_current_context_state_ptr->viewport_x1y1   [1] = y;
            this_ptr->m_current_context_state_ptr->viewport_extents[0] = width;
            this_ptr->m_current_context_state_ptr->viewport_extents[1] = height;
        }
        else
        /* If this is a GL call which sets up a texture or updates existing texture's mip's contents,
         * treat this as a state that potentially spans across multiple frames.
         */
        if (in_api_func == APIInterceptor::APIFUNCTION_GL_GLBINDTEXTURE)
        {
            AI_ASSERT(in_n_args == 2);

            const auto target     = in_args_ptr[0].get_u32();
            const auto texture_id = in_args_ptr[1].get_u32();

            AI_ASSERT(target == GL_TEXTURE_2D);

            this_ptr->m_current_context_state_ptr->bound_2d_texture_gl_id = texture_id;
            this_ptr->m_texture_target_to_bound_texture_id_map[target]    = texture_id;
        }
        else
        if (in_api_func == APIInterceptor::APIFUNCTION_GL_GLDELETETEXTURES)
        {
            AI_ASSERT(in_n_args == 2);

            const auto n_texture_ids   = in_args_ptr[0].get_i32    ();
            const auto texture_ids_ptr = in_args_ptr[1].get_u32_ptr();

            for (uint32_t n_texture_id = 0;
                          n_texture_id < static_cast<uint32_t>(n_texture_ids);
                        ++n_texture_id)
            {
                this_ptr->m_current_context_state_ptr->gl_texture_id_to_texture_state_map.erase(texture_ids_ptr[n_texture_id]);
                this_ptr->m_gl_id_to_texture_props_map_ptr->erase                              (texture_ids_ptr[n_texture_id]);
            }

            should_record_api_call = false;
        }
        else
        if (in_api_func == APIInterceptor::APIFUNCTION_GL_GLTEXIMAGE2D)
        {
            uint32_t    call_arg_target         = in_args_ptr[0].get_u32();
            int32_t     call_arg_level          = in_args_ptr[1].get_i32();
            int32_t     call_arg_internalformat = in_args_ptr[2].get_i32();
            int32_t     call_arg_width          = in_args_ptr[3].get_i32();
            int32_t     call_arg_height         = in_args_ptr[4].get_i32();
            int32_t     call_arg_border         = in_args_ptr[5].get_i32();
            uint32_t    call_arg_format         = in_args_ptr[6].get_u32();
            uint32_t    call_arg_type           = in_args_ptr[7].get_u32();
            const void* call_arg_pixels_ptr     = in_args_ptr[8].get_ptr();

            AI_ASSERT(in_n_args                                                                == 9);
            AI_ASSERT(this_ptr->m_texture_target_to_bound_texture_id_map.find(call_arg_target) != this_ptr->m_texture_target_to_bound_texture_id_map.end() );

            const auto bound_texture_id     = this_ptr->m_texture_target_to_bound_texture_id_map.at(call_arg_target);
            auto       texture_map_iterator = this_ptr->m_gl_id_to_texture_props_map_ptr->find     (bound_texture_id);

            AI_ASSERT(bound_texture_id != 0);
            AI_ASSERT(call_arg_format  == GL_LUMINANCE      || call_arg_format  == GL_RGBA);
            AI_ASSERT(call_arg_type    == GL_UNSIGNED_BYTE);

            const auto n_components = (call_arg_format == GL_RGBA) ? 4u
                                                                   : 1u;

            /* Create new map entries for the texture, if necessary. */
            if (texture_map_iterator == this_ptr->m_gl_id_to_texture_props_map_ptr->end() )
            {
                (*this_ptr->m_gl_id_to_texture_props_map_ptr)[bound_texture_id] = TextureProps(call_arg_border,
                                                                                               TextureType::_2D);

                texture_map_iterator = this_ptr->m_gl_id_to_texture_props_map_ptr->find(bound_texture_id);
            }

            {
                auto texture_state_map_iterator = this_ptr->m_current_context_state_ptr->gl_texture_id_to_texture_state_map.find(bound_texture_id);

                if (texture_state_map_iterator == this_ptr->m_current_context_state_ptr->gl_texture_id_to_texture_state_map.end())
                {
                    this_ptr->m_current_context_state_ptr->gl_texture_id_to_texture_state_map[bound_texture_id] = GLContextTextureState();
                }
            }

            /* Cache the specified mip data */
            texture_map_iterator->second.mip_props_vec.resize(call_arg_level + 1);

            {
                auto       mip_props_ptr            = &texture_map_iterator->second.mip_props_vec.at(call_arg_level);
                const auto n_bytes_under_pixels_ptr = call_arg_width * call_arg_height * n_components;
                auto       data_u8_vec_ptr          = &mip_props_ptr->data_u8_vec;

                should_record_api_call = (data_u8_vec_ptr->size() != 0);

                mip_props_ptr->format           = call_arg_format;
                mip_props_ptr->internal_format  = call_arg_internalformat;
                mip_props_ptr->mip_size_u32vec3 = std::array<uint32_t, 3>{static_cast<uint32_t>(call_arg_width), static_cast<uint32_t>(call_arg_height), 1};
                mip_props_ptr->type             = call_arg_type;

                data_u8_vec_ptr->resize(n_bytes_under_pixels_ptr);

                memcpy(data_u8_vec_ptr->data(),
                       call_arg_pixels_ptr,
                       n_bytes_under_pixels_ptr);
            }
        }
        else
        if (in_api_func == APIInterceptor::APIFUNCTION_GL_GLTEXPARAMETERF)
        {
            const auto call_arg_target            = in_args_ptr[0].get_u32 ();
            const auto call_arg_pname             = in_args_ptr[1].get_u32 ();
            const auto call_arg_value             = in_args_ptr[2].get_fp32();
            const auto bound_texture_id_iterator  = this_ptr->m_texture_target_to_bound_texture_id_map.find(call_arg_target);

            if (bound_texture_id_iterator != this_ptr->m_texture_target_to_bound_texture_id_map.end() )
            {
                const auto bound_texture_id           = bound_texture_id_iterator->second;
                auto       texture_state_map_iterator = this_ptr->m_current_context_state_ptr->gl_texture_id_to_texture_state_map.find(bound_texture_id);

                if (texture_state_map_iterator == this_ptr->m_current_context_state_ptr->gl_texture_id_to_texture_state_map.end() )
                {
                    this_ptr->m_current_context_state_ptr->gl_texture_id_to_texture_state_map[bound_texture_id] = GLContextTextureState();
                    texture_state_map_iterator                                                                  = this_ptr->m_current_context_state_ptr->gl_texture_id_to_texture_state_map.find(bound_texture_id);

                    assert(texture_state_map_iterator != this_ptr->m_current_context_state_ptr->gl_texture_id_to_texture_state_map.end() );
                }

                switch (call_arg_pname)
                {
                    case GL_TEXTURE_BASE_LEVEL: texture_state_map_iterator->second.base_level = static_cast<int32_t>(call_arg_value); break;
                    case GL_TEXTURE_MAG_FILTER: texture_state_map_iterator->second.mag_filter = static_cast<GLenum> (call_arg_value); break;
                    case GL_TEXTURE_MAX_LEVEL:  texture_state_map_iterator->second.max_level  = static_cast<int32_t>(call_arg_value); break;
                    case GL_TEXTURE_MAX_LOD:    texture_state_map_iterator->second.max_lod    = call_arg_value;                       break;
                    case GL_TEXTURE_MIN_LOD:    texture_state_map_iterator->second.min_lod    = call_arg_value;                       break;
                    case GL_TEXTURE_MIN_FILTER: texture_state_map_iterator->second.min_filter = static_cast<GLenum> (call_arg_value); break;
                    case GL_TEXTURE_WRAP_S:     texture_state_map_iterator->second.wrap_s     = static_cast<GLenum> (call_arg_value); break;
                    case GL_TEXTURE_WRAP_T:     texture_state_map_iterator->second.wrap_t     = static_cast<GLenum> (call_arg_value); break;
                    case GL_TEXTURE_WRAP_R:     texture_state_map_iterator->second.wrap_r     = static_cast<GLenum> (call_arg_value); break;
                }

                should_record_api_call = false;
            }
        }

        if (should_record_api_call)
        {
            /* This is a generic GL call. Record it. */
            if (in_api_func == APIInterceptor::APIFUNCTION_GL_GLCOLOR3UBV)
            {
                /* Convert to non-ptr representation.. */
                const auto                                               color_data_ptr = reinterpret_cast<const unsigned char*>(in_args_ptr[0].get_u8_ptr() );
                const std::array<APIInterceptor::APIFunctionArgument, 3> api_arg_vec    =
                {
                    APIInterceptor::APIFunctionArgument::create_u8(color_data_ptr[0]),
                    APIInterceptor::APIFunctionArgument::create_u8(color_data_ptr[1]),
                    APIInterceptor::APIFunctionArgument::create_u8(color_data_ptr[2]),
                };

                this_ptr->m_recording_snapshot_ptr->record_api_call(APIInterceptor::APIFUNCTION_GL_GLCOLOR3UB,
                                                                    static_cast<uint32_t>(api_arg_vec.size() ),
                                                                    api_arg_vec.data     () );
            }
            else
            if (in_api_func == APIInterceptor::APIFUNCTION_GL_GLCOLOR4FV)
            {
                /* Convert to non-ptr representation.. */
                const auto                                               color_data_ptr = in_args_ptr[0].get_fp32_ptr();
                const std::array<APIInterceptor::APIFunctionArgument, 4> api_arg_vec    =
                {
                    APIInterceptor::APIFunctionArgument::create_fp32(color_data_ptr[0]),
                    APIInterceptor::APIFunctionArgument::create_fp32(color_data_ptr[1]),
                    APIInterceptor::APIFunctionArgument::create_fp32(color_data_ptr[2]),
                    APIInterceptor::APIFunctionArgument::create_fp32(color_data_ptr[3])
                };

                this_ptr->m_recording_snapshot_ptr->record_api_call(APIInterceptor::APIFUNCTION_GL_GLCOLOR4F,
                                                                    static_cast<uint32_t>(api_arg_vec.size() ),
                                                                    api_arg_vec.data     () );
            }
            else
            if (in_api_func == APIInterceptor::APIFUNCTION_GL_GLVERTEX3FV)
            {
                /* Convert to non-ptr representation.. */
                const auto                                               vertex_data_ptr = in_args_ptr[0].get_fp32_ptr();
                const std::array<APIInterceptor::APIFunctionArgument, 3> api_arg_vec     =
                {
                    APIInterceptor::APIFunctionArgument::create_fp32(vertex_data_ptr[0]),
                    APIInterceptor::APIFunctionArgument::create_fp32(vertex_data_ptr[1]),
                    APIInterceptor::APIFunctionArgument::create_fp32(vertex_data_ptr[2])
                };

                this_ptr->m_recording_snapshot_ptr->record_api_call(APIInterceptor::APIFUNCTION_GL_GLVERTEX3F,
                                                                    static_cast<uint32_t>(api_arg_vec.size() ),
                                                                    api_arg_vec.data     () );
            }
            else
            {
                this_ptr->m_recording_snapshot_ptr->record_api_call(in_api_func,
                                                                    in_n_args,
                                                                    in_args_ptr);
            }
        }
    }
    else
    {
        /* This snapshot is complete. Stash if needed. */
        if (this_ptr->m_snapshot_requested)
        {
            /* Make sure glReadPixels() completes before we cache the contents. */
            reinterpret_cast<PFNGLFINISHPROC>(OpenGL::g_cached_gl_finish)();

            assert(this_ptr->m_gl_id_to_texture_props_map_ptr != nullptr);

            this_ptr->m_cached_gl_id_to_texture_props_map_ptr.reset    (new GLIDToTexturePropsMap(*this_ptr->m_gl_id_to_texture_props_map_ptr) );
            this_ptr->m_cached_prev_frame_depth_buffer_u8_vec_ptr.reset(new std::vector<uint8_t> ( this_ptr->m_prev_frame_depth_buffer_u8_vec_ptr->size() ));

            memcpy(this_ptr->m_cached_prev_frame_depth_buffer_u8_vec_ptr->data(),
                   this_ptr->m_prev_frame_depth_buffer_u8_vec_ptr->data       (),
                   this_ptr->m_prev_frame_depth_buffer_u8_vec_ptr->size       () );

            this_ptr->m_cached_snapshot_ptr               = std::move(this_ptr->m_recording_snapshot_ptr);
            this_ptr->m_cached_start_gl_context_state_ptr = std::move(this_ptr->m_start_gl_context_state_ptr);
            this_ptr->m_snapshot_requested                = false;

            this_ptr->m_replayer_ptr->on_snapshot_available();
        }
        else
        {
            this_ptr->m_recording_snapshot_ptr->reset   ();
            this_ptr->m_start_gl_context_state_ptr.reset();
        }

        /* Cache depth buffer's contents for next frame's reuse. */
        reinterpret_cast<PFNGLPIXELSTOREIPROC>(OpenGL::g_cached_gl_pixel_storei)(GL_PACK_ALIGNMENT,   1);
        reinterpret_cast<PFNGLPIXELSTOREIPROC>(OpenGL::g_cached_gl_pixel_storei)(GL_UNPACK_ALIGNMENT, 1);

        reinterpret_cast<PFNGLREADPIXELSPROC>(OpenGL::g_cached_gl_read_pixels)(0,
                                                                               0,
                                                                               this_ptr->m_current_context_state_ptr->viewport_extents[0],
                                                                               this_ptr->m_current_context_state_ptr->viewport_extents[1],
                                                                               GL_DEPTH_COMPONENT,
                                                                               GL_FLOAT,
                                                                               this_ptr->m_prev_frame_depth_buffer_u8_vec_ptr->data() );
    }

    if (in_api_func == APIInterceptor::APIFUNCTION_GL_GLBEGIN)
    {
        this_ptr->m_is_glbegin_active = true;
    }
    else
    if (in_api_func == APIInterceptor::APIFUNCTION_GL_GLEND)
    {
        AI_ASSERT(this_ptr->m_is_glbegin_active);

        this_ptr->m_is_glbegin_active = false;
    }
    else
    {
        if (!this_ptr->m_is_glbegin_active)
        {
            AI_ASSERT(reinterpret_cast<PFNGLGETERRORPROC>(OpenGL::g_cached_gl_get_error)() == GL_NO_ERROR);
        }
    }
}

bool ReplayerSnapshotter::pop_snapshot(GLContextStateUniquePtr*        out_start_gl_context_state_ptr_ptr,
                                       ReplayerSnapshotUniquePtr*      out_snapshot_ptr_ptr,
                                       GLIDToTexturePropsMapUniquePtr* out_gl_id_to_texture_props_map_ptr_ptr,
                                       U8VecUniquePtr*                 out_prev_frame_depth_data_u8_vec_ptr_ptr)
{
    std::lock_guard<std::mutex> lock  (m_mutex);
    bool                        result(false);

    if (m_snapshot_requested == false)
    {
        if (m_cached_gl_id_to_texture_props_map_ptr != nullptr)
        {
            assert(m_cached_snapshot_ptr != nullptr);

            *out_gl_id_to_texture_props_map_ptr_ptr   = std::move(m_cached_gl_id_to_texture_props_map_ptr);
            *out_prev_frame_depth_data_u8_vec_ptr_ptr = std::move(m_cached_prev_frame_depth_buffer_u8_vec_ptr);
            *out_snapshot_ptr_ptr                     = std::move(m_cached_snapshot_ptr);
            *out_start_gl_context_state_ptr_ptr       = std::move(m_cached_start_gl_context_state_ptr);

            result = true;
        }
        else
        {
            assert(m_cached_snapshot_ptr == nullptr);
        }
    }

    return result;
}
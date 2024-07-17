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


ReplayerSnapshotter::ReplayerSnapshotter()
    :m_snapshot_requested(false)
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

ReplayerSnapshotterUniquePtr ReplayerSnapshotter::create()
{
    ReplayerSnapshotterUniquePtr result_ptr(new ReplayerSnapshotter() );

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
    m_gl_id_to_texture_props_map_ptr.reset(new GLIDToTexturePropsMap() );

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
    auto this_ptr = reinterpret_cast<ReplayerSnapshotter*>(in_user_arg_ptr);

    if (this_ptr->m_recording_snapshot_ptr == nullptr)
    {
        this_ptr->m_recording_snapshot_ptr = ReplayerSnapshot::create();

        assert(this_ptr->m_recording_snapshot_ptr != nullptr);
    }

    if (in_api_func != APIInterceptor::APIFUNCTION_GDI32_SWAPBUFFERS)
    {
        /* If this is a GL call which sets up a texture or updates existing texture's mip's contents,
         * treat this as a state that potentially spans across multiple frames.
         */
        if (in_api_func == APIInterceptor::APIFUNCTION_GL_GLBINDTEXTURE)
        {
            AI_ASSERT(in_n_args == 2);

            const auto target     = in_args_ptr[0].value.value_u32;
            const auto texture_id = in_args_ptr[1].value.value_u32;

            this_ptr->m_texture_target_to_bound_texture_id_map[target] = texture_id;

            /* We still need to record this API call! */
            this_ptr->m_recording_snapshot_ptr->record_api_call(in_api_func,
                                                                in_n_args,
                                                                in_args_ptr);
        }
        else
        if (in_api_func == APIInterceptor::APIFUNCTION_GL_GLDELETETEXTURES)
        {
            AI_ASSERT(in_n_args == 2);

            const auto n_texture_ids   = in_args_ptr[0].value.value_u32;
            const auto texture_ids_ptr = in_args_ptr[1].value.value_u32_ptr;

            for (uint32_t n_texture_id = 0;
                          n_texture_id < n_texture_ids;
                        ++n_texture_id)
            {
                this_ptr->m_gl_id_to_texture_props_map_ptr->erase(texture_ids_ptr[n_texture_id]);
            }
        }
        else
        if (in_api_func == APIInterceptor::APIFUNCTION_GL_GLTEXIMAGE2D)
        {
            uint32_t    call_arg_target         = in_args_ptr[0].value.value_u32;
            int32_t     call_arg_level          = in_args_ptr[1].value.value_i32;
            int32_t     call_arg_internalformat = in_args_ptr[2].value.value_i32;
            int32_t     call_arg_width          = in_args_ptr[3].value.value_i32;
            int32_t     call_arg_height         = in_args_ptr[4].value.value_i32;
            int32_t     call_arg_border         = in_args_ptr[5].value.value_i32;
            uint32_t    call_arg_format         = in_args_ptr[6].value.value_u32;
            uint32_t    call_arg_type           = in_args_ptr[7].value.value_u32;
            const void* call_arg_pixels_ptr     = in_args_ptr[8].value.value_ptr;

            AI_ASSERT(in_n_args                                                                == 9);
            AI_ASSERT(this_ptr->m_texture_target_to_bound_texture_id_map.find(call_arg_target) != this_ptr->m_texture_target_to_bound_texture_id_map.end() );

            const auto bound_texture_id     = this_ptr->m_texture_target_to_bound_texture_id_map.at(call_arg_target);
            auto       texture_map_iterator = this_ptr->m_gl_id_to_texture_props_map_ptr->find     (bound_texture_id);

            AI_ASSERT(bound_texture_id != 0);
            AI_ASSERT(call_arg_format  == GL_LUMINANCE      || call_arg_format  == GL_RGBA);
            AI_ASSERT(call_arg_type    == GL_UNSIGNED_BYTE);

            const auto n_components = (call_arg_format == GL_RGBA) ? 4u
                                                                   : 1u;

            if (texture_map_iterator == this_ptr->m_gl_id_to_texture_props_map_ptr->end() )
            {
                (*this_ptr->m_gl_id_to_texture_props_map_ptr)[bound_texture_id] = TextureProps(call_arg_border,
                                                                                               call_arg_internalformat,
                                                                                               TextureType::_2D);

                texture_map_iterator = this_ptr->m_gl_id_to_texture_props_map_ptr->find(bound_texture_id);
            }

            texture_map_iterator->second.mip_props_vec.resize(call_arg_level + 1);

            {
                auto       mip_props_ptr            = &texture_map_iterator->second.mip_props_vec.at(call_arg_level);
                const auto n_bytes_under_pixels_ptr = call_arg_width * call_arg_height * n_components;
                auto       data_u8_vec_ptr          = &mip_props_ptr->data_u8_vec;

                mip_props_ptr->format           = call_arg_format;
                mip_props_ptr->mip_size_u32vec3 = std::array<uint32_t, 3>{static_cast<uint32_t>(call_arg_width), static_cast<uint32_t>(call_arg_height), 1};
                mip_props_ptr->type             = call_arg_type;

                data_u8_vec_ptr->resize(n_bytes_under_pixels_ptr);

                memcpy(data_u8_vec_ptr->data(),
                       call_arg_pixels_ptr,
                       n_bytes_under_pixels_ptr);
            }
        }
        else
        {
            /* This is a generic GL call. Record it. */
            this_ptr->m_recording_snapshot_ptr->record_api_call(in_api_func,
                                                                in_n_args,
                                                                in_args_ptr);
        }
    }
    else
    {
        /* This snapshot is complete. */
        if (this_ptr->m_snapshot_requested)
        {
            this_ptr->m_cached_gl_id_to_texture_props_map_ptr.reset(new GLIDToTexturePropsMap(*this_ptr->m_gl_id_to_texture_props_map_ptr) );

            this_ptr->m_cached_snapshot_ptr = std::move(this_ptr->m_recording_snapshot_ptr);
            this_ptr->m_snapshot_requested  = false;
        }
        else
        {
            this_ptr->m_recording_snapshot_ptr->reset();
        }
    }
}

bool ReplayerSnapshotter::pop_snapshot(ReplayerSnapshotUniquePtr*      out_snapshot_ptr_ptr,
                                       GLIDToTexturePropsMapUniquePtr* out_gl_id_to_texture_props_map_ptr_ptr)
{
    bool result = false;

    if (m_cached_gl_id_to_texture_props_map_ptr != nullptr)
    {
        assert(m_cached_snapshot_ptr != nullptr);

        *out_gl_id_to_texture_props_map_ptr_ptr = std::move(m_cached_gl_id_to_texture_props_map_ptr);
        *out_snapshot_ptr_ptr                   = std::move(m_cached_snapshot_ptr);

        result = true;
    }
    else
    {
        assert(m_cached_snapshot_ptr == nullptr);
    }

    return result;
}
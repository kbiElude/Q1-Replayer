/* API Interceptor (c) 2024 Dominik Witczak
 *
 * This code is licensed under MIT license (see LICENSE.txt for details)
 */
#include "OpenGL/globals.h"
#include "APIInterceptor/include/Common/types.h"
#include "replayer_types.h"

GLContextState::GLContextState(const uint32_t& in_q1_window_width,
                               const uint32_t& in_q1_window_height)
{
    alpha_func_func        = GL_ALWAYS;
    alpha_func_ref         = 0.0f;
    blend_func_dfactor     = GL_ZERO;
    blend_func_sfactor     = GL_ONE;
    bound_2d_texture_gl_id = 0;
    clear_color[0]         = 0.0f;
    clear_color[1]         = 0.0f;
    clear_color[2]         = 0.0f;
    clear_color[3]         = 0.0f;
    clear_depth            = 1.0;
    cull_face_mode         = GL_BACK;
    depth_func             = GL_LESS;
    depth_mask             = true;
    depth_range[0]         = 0.0f;
    depth_range[1]         = 1.0f;
    draw_buffer_mode       = GL_BACK;
    front_face_mode        = GL_CCW;
    matrix_mode            = GL_MODELVIEW;
    shade_model            = GL_SMOOTH;
    texture_env_mode       = GL_MODULATE;
    viewport_extents[0]    = in_q1_window_width;
    viewport_extents[1]    = in_q1_window_height;
    viewport_x1y1   [0]    = 0;
    viewport_x1y1   [1]    = 0;

    modelview_matrix[0]  = 1; modelview_matrix[1]  = 0; modelview_matrix[2]  = 0; modelview_matrix[3]  = 0;
    modelview_matrix[4]  = 0; modelview_matrix[5]  = 1; modelview_matrix[6]  = 0; modelview_matrix[7]  = 0;
    modelview_matrix[8]  = 0; modelview_matrix[9]  = 0; modelview_matrix[10] = 1; modelview_matrix[11] = 0;
    modelview_matrix[12] = 0; modelview_matrix[13] = 0; modelview_matrix[14] = 0; modelview_matrix[15] = 1;

    memcpy(projection_matrix,
           modelview_matrix,
           sizeof(double) * 16);
}

GLContextTextureState::GLContextTextureState()
{
    base_level = static_cast<int32_t> (0);
    mag_filter = static_cast<uint32_t>(GL_LINEAR);
    max_level  = static_cast<int32_t> (1000.0f);
    max_lod    = static_cast<float>   (1000.0f);
    min_filter = static_cast<uint32_t>(GL_NEAREST_MIPMAP_LINEAR);
    min_lod    = static_cast<float>   (-1000.0f);
    wrap_s     = static_cast<uint32_t>(GL_REPEAT);
    wrap_t     = static_cast<uint32_t>(GL_REPEAT);
    wrap_r     = static_cast<uint32_t>(GL_REPEAT);
}
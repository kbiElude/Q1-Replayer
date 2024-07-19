/* API Interceptor (c) 2024 Dominik Witczak
 *
 * This code is licensed under MIT license (see LICENSE.txt for details)
 */
#include "OpenGL/globals.h"
#include "APIInterceptor/include/Common/types.h"
#include "replayer_types.h"

GLContextState::GLContextState()
{
    alpha_func_func = GL_ALWAYS;
    alpha_func_ref  = 0.0f;
    clear_color[0]  = 0.0f;
    clear_color[1]  = 0.0f;
    clear_color[2]  = 0.0f;
    clear_color[3]  = 0.0f;
    shade_model     = GL_SMOOTH;
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
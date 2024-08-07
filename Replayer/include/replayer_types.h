/* API Interceptor (c) 2024 Dominik Witczak
 *
 * This code is licensed under MIT license (see LICENSE.txt for details)
 */
 #if !defined(REPLAYER_TYPES_H)
 #define REPLAYER_TYPES_H

#include <array>
#include <cassert>
#include <memory>
#include <thread>
#include <map>
#include <unordered_map>
#include <vector>
#include <Windows.h>

#include "Common/types.h"


struct GLContextTextureState
{
    int32_t  base_level;
    uint32_t mag_filter;
    int32_t  max_level;
    float    max_lod;
    uint32_t min_filter;
    float    min_lod;
    uint32_t wrap_s;
    uint32_t wrap_t;
    uint32_t wrap_r;

    GLContextTextureState();
};

struct GLContextState
{
    bool alpha_test_enabled   = false;
    bool blend_enabled        = false;
    bool cull_face_enabled    = false;
    bool depth_test_enabled   = false;
    bool scissor_test_enabled = false;
    bool texture_2d_enabled   = false;

    uint32_t alpha_func_func;
    float    alpha_func_ref;
    uint32_t blend_func_dfactor;
    uint32_t blend_func_sfactor;
    float    clear_color[4];
    double   clear_depth;
    uint32_t cull_face_mode;
    uint32_t depth_func;
    bool     depth_mask;
    double   depth_range[2]; // near, far
    uint32_t draw_buffer_mode;
    uint32_t front_face_mode;
    uint32_t matrix_mode;
    uint32_t shade_model;
    uint32_t texture_env_mode;
    int32_t  viewport_extents[2];
    int32_t  viewport_x1y1   [2];

    // NOTE: Matrix state is only stored for snapshots!.
    double   modelview_matrix [16];
    double   projection_matrix[16];

    uint32_t                                            bound_2d_texture_gl_id;
    std::unordered_map<uint32_t, GLContextTextureState> gl_texture_id_to_texture_state_map;

    GLContextState(const uint32_t& in_q1_window_width,
                   const uint32_t& in_q1_window_height);
};

enum class TextureType : uint8_t
{
    _1D,
    _2D,
    _3D,

    UNKNOWN
};

struct MipProps
{
    uint32_t                format           = 0; // GLenum
    uint32_t                internal_format  = 0; // GLenum
    std::array<uint32_t, 3> mip_size_u32vec3 = {};
    uint32_t                type             = 0; // GLenum

    std::vector<uint8_t> data_u8_vec;

    MipProps()
    {
        /* Stub */
    
    }

    MipProps(const std::array<uint32_t, 3>& in_mip_size_u32vec3,
             const uint32_t&                in_internal_format,
             const uint32_t&                in_format,
             const uint32_t&                in_type,
             const std::vector<uint8_t>&    in_data_u8_vec)
        :data_u8_vec     (in_data_u8_vec),
         format          (in_format),
         internal_format (in_internal_format),
         mip_size_u32vec3(in_mip_size_u32vec3),
         type            (in_type)
    {
        /* Stub */
    }
};

struct TextureProps
{
    int32_t     border          = 0; // GLint
    TextureType type            = TextureType::UNKNOWN;

    std::vector<MipProps> mip_props_vec;

    TextureProps()
    {
        /* Stub */
    }

    TextureProps(const int32_t&     in_border,
                 const TextureType& in_type)
        :border(in_border),
         type  (in_type)
    {
        /* Stub */
    }
};

typedef std::unique_ptr<GLContextState>        GLContextStateUniquePtr;
typedef std::map<uint32_t, TextureProps>       GLIDToTexturePropsMap;
typedef std::unique_ptr<GLIDToTexturePropsMap> GLIDToTexturePropsMapUniquePtr;
typedef std::unique_ptr<std::vector<uint8_t> > U8VecUniquePtr;

#endif /* REPLAYER_TYPES_H */
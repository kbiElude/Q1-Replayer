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
#include <unordered_map>
#include <vector>
#include <Windows.h>

struct APICommand
{
    APIInterceptor::APIFunction                      api_func;
    std::vector<APIInterceptor::APIFunctionArgument> api_arg_vec;

    APICommand()
        :api_func(APIInterceptor::APIFUNCTION_UNKNOWN)
    {
        /* Stub */
    }
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
    std::array<uint32_t, 3> mip_size_u32vec3 = {};
    uint32_t                type             = 0; // GLenum

    std::vector<uint8_t> data_u8_vec;

    MipProps()
    {
        /* Stub */
    
    }
    MipProps(const std::array<uint32_t, 3>& in_mip_size_u32vec3,
             const uint32_t&                in_format,
             const uint32_t&                in_type,
             const std::vector<uint8_t>&    in_data_u8_vec)
        :data_u8_vec     (in_data_u8_vec),
         format          (in_format),
         mip_size_u32vec3(in_mip_size_u32vec3),
         type            (in_type)
    {
        /* Stub */
    }
};

struct TextureProps
{
    int32_t     border          = 0; // GLint
    uint32_t    internal_format = 0; // GLenum
    TextureType type            = TextureType::UNKNOWN;

    std::vector<MipProps> mip_props_vec;

    TextureProps()
    {
        /* Stub */
    }

    TextureProps(const int32_t&     in_border,
                 const uint32_t&    in_internal_format,
                 const TextureType& in_type)
        :border         (in_border),
         internal_format(in_internal_format),
         type           (in_type)
    {
        /* Stub */
    }
};

typedef std::unordered_map<uint32_t, TextureProps> GLIDToTexturePropsMap;
typedef std::unique_ptr<GLIDToTexturePropsMap>     GLIDToTexturePropsMapUniquePtr;

#endif /* REPLAYER_TYPES_H */
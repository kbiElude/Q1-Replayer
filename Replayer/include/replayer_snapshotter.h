/* API Interceptor (c) 2024 Dominik Witczak
 *
 * This code is licensed under MIT license (see LICENSE.txt for details)
 */
 #if !defined(REPLAYER_SNAPSHOTTER_H)
 #define REPLAYER_SNAPSHOTTER_H

#include "replayer_types.h"
#include "replayer_snapshot.h"

/* Forward decls */
class                                        ReplayerSnapshotter;
typedef std::unique_ptr<ReplayerSnapshotter> ReplayerSnapshotterUniquePtr;


class ReplayerSnapshotter
{
public:
    /* Public funcs */
    static ReplayerSnapshotterUniquePtr create();

    ~ReplayerSnapshotter();

    void cache_snapshot();
    bool pop_snapshot  (ReplayerSnapshotUniquePtr*      out_snapshot_ptr_ptr,
                        GLIDToTexturePropsMapUniquePtr* out_gl_id_to_texture_props_map_ptr_ptr);

private:
    /* Private funcs */
    ReplayerSnapshotter();

    bool init();

    static void on_api_func_callback(APIInterceptor::APIFunction                in_api_func,
                                     uint32_t                                   in_n_args,
                                     const APIInterceptor::APIFunctionArgument* in_args_ptr,
                                     void*                                      in_user_arg_ptr);

    /* Private vars */
    GLIDToTexturePropsMapUniquePtr                      m_gl_id_to_texture_props_map_ptr;
    std::unordered_map<uint32_t /* GLenum */, uint32_t> m_texture_target_to_bound_texture_id_map;

    GLIDToTexturePropsMapUniquePtr m_cached_gl_id_to_texture_props_map_ptr;
    ReplayerSnapshotUniquePtr      m_cached_snapshot_ptr;

    ReplayerSnapshotUniquePtr m_recording_snapshot_ptr;
    bool                      m_snapshot_requested;

};

#endif /* REPLAYER_SNAPSHOTTER_H */
/* API Interceptor (c) 2024 Dominik Witczak
 *
 * This code is licensed under MIT license (see LICENSE.txt for details)
 */
 #if !defined(REPLAYER_SNAPSHOTTER_H)
 #define REPLAYER_SNAPSHOTTER_H

#include "replayer_types.h"
#include "replayer_snapshot.h"

/* Forward decls */
class                                        Replayer;
class                                        ReplayerSnapshotter;
typedef std::unique_ptr<ReplayerSnapshotter> ReplayerSnapshotterUniquePtr;


class ReplayerSnapshotter
{
public:
    /* Public funcs */
    static ReplayerSnapshotterUniquePtr create(const Replayer* in_replayer_ptr);

    ~ReplayerSnapshotter();

    void cache_snapshot();
    bool pop_snapshot  (GLContextStateUniquePtr*        out_start_gl_context_state_ptr_ptr,
                        ReplayerSnapshotUniquePtr*      out_snapshot_ptr_ptr,
                        GLIDToTexturePropsMapUniquePtr* out_gl_id_to_texture_props_map_ptr_ptr);

private:
    /* Private funcs */
    ReplayerSnapshotter(const Replayer* in_replayer_ptr);

    bool init();

    static void on_api_func_callback(APIInterceptor::APIFunction                in_api_func,
                                     uint32_t                                   in_n_args,
                                     const APIInterceptor::APIFunctionArgument* in_args_ptr,
                                     void*                                      in_user_arg_ptr);

    /* Private vars */
    const Replayer* m_replayer_ptr;

    GLContextStateUniquePtr                             m_current_context_state_ptr;
    GLIDToTexturePropsMapUniquePtr                      m_gl_id_to_texture_props_map_ptr;
    GLContextStateUniquePtr                             m_start_gl_context_state_ptr;
    std::unordered_map<uint32_t /* GLenum */, uint32_t> m_texture_target_to_bound_texture_id_map;

    GLIDToTexturePropsMapUniquePtr m_cached_gl_id_to_texture_props_map_ptr;
    ReplayerSnapshotUniquePtr      m_cached_snapshot_ptr;
    GLContextStateUniquePtr        m_cached_start_gl_context_state_ptr;

    bool                      m_is_glbegin_active;
    std::mutex                m_mutex;
    ReplayerSnapshotUniquePtr m_recording_snapshot_ptr;
    bool                      m_snapshot_requested;

};

#endif /* REPLAYER_SNAPSHOTTER_H */
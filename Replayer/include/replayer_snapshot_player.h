/* API Interceptor (c) 2024 Dominik Witczak
 *
 * This code is licensed under MIT license (see LICENSE.txt for details)
 */
#if !defined(REPLAYER_SNAPSHOT_PLAYER_H)
#define REPLAYER_SNAPSHOT_PLAYER_H

#include "APIInterceptor/include/Common/types.h"
#include "replayer_snapshot.h"

/* Forward decls */
class                                           Replayer;
class                                           ReplayerSnapshotLogger;
class                                           ReplayerSnapshotPlayer;
typedef std::unique_ptr<ReplayerSnapshotPlayer> ReplayerSnapshotPlayerUniquePtr;


class ReplayerSnapshotPlayer
{
public:
    /* Public funcs */
    static ReplayerSnapshotPlayerUniquePtr create(const Replayer* in_replayer_ptr);

    ~ReplayerSnapshotPlayer();

    void load_snapshot(const GLContextState*        in_start_context_state_ptr,
                       const ReplayerSnapshot*      in_snapshot_ptr,
                       const GLIDToTexturePropsMap* in_snapshot_gl_id_to_texture_props_map_ptr,
                       const std::vector<uint8_t>*  in_snapshot_prev_frame_depth_data_u8_vec_ptr);
    void play_snapshot();

    bool is_snapshot_available     ();
    void lock_for_snapshot_access  ();
    void unlock_for_snapshot_access();

private:
    /* Private type defs */

    /* Private funcs */
    ReplayerSnapshotPlayer(const Replayer* in_replayer_ptr);

    /* Private vars */
    std::mutex m_mutex;

    const Replayer*         m_replayer_ptr;
    bool                    m_snapshot_initialized;

    const GLIDToTexturePropsMap* m_snapshot_gl_id_to_texture_props_map_ptr;
    const std::vector<uint8_t>*  m_snapshot_prev_frame_depth_data_u8_vec_ptr;
    const ReplayerSnapshot*      m_snapshot_ptr;
    const GLContextState*        m_snapshot_start_gl_context_state_ptr;

    std::unordered_map<uint32_t, uint32_t> m_snapshot_texture_gl_id_to_texture_gl_id_map;
};

#endif /* REPLAYER_SNAPSHOT_PLAYER_H */
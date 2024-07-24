/* API Interceptor (c) 2024 Dominik Witczak
 *
 * This code is licensed under MIT license (see LICENSE.txt for details)
 */
#if !defined(REPLAYER_SNAPSHOT_PLAYER_H)
#define REPLAYER_SNAPSHOT_PLAYER_H

#include "APIInterceptor/include/Common/types.h"
#include "replayer_snapshot.h"

/* Forward decls */
class                                           ReplayerSnapshotPlayer;
typedef std::unique_ptr<ReplayerSnapshotPlayer> ReplayerSnapshotPlayerUniquePtr;


class ReplayerSnapshotPlayer
{
public:
    /* Public funcs */
    static ReplayerSnapshotPlayerUniquePtr create();

    ~ReplayerSnapshotPlayer();

    bool is_snapshot_loaded()                                                                          const;
    void load_snapshot     (GLContextStateUniquePtr        in_start_context_state_ptr,
                            ReplayerSnapshotUniquePtr      in_snapshot_ptr,
                            GLIDToTexturePropsMapUniquePtr in_snapshot_gl_id_to_texture_props_map_ptr);
    void play_snapshot     (const float&                   in_playback_segment_end_normalized);

private:
    /* Private type defs */

    /* Private funcs */
    ReplayerSnapshotPlayer();

    /* Private vars */
    GLIDToTexturePropsMapUniquePtr         m_snapshot_gl_id_to_texture_props_map_ptr;
    bool                                   m_snapshot_initialized;
    ReplayerSnapshotUniquePtr              m_snapshot_ptr;
    GLContextStateUniquePtr                m_snapshot_start_gl_context_state_ptr;
    std::unordered_map<uint32_t, uint32_t> m_snapshot_texture_gl_id_to_texture_gl_id_map;
};

#endif /* REPLAYER_SNAPSHOT_PLAYER_H */
/* API Interceptor (c) 2024 Dominik Witczak
 *
 * This code is licensed under MIT license (see LICENSE.txt for details)
 */
#if !defined(REPLAYER_WINDOW_H)
#define REPLAYER_WINDOW_H

#include "replayer_types.h"


/* Forward decls */
struct                                  GLFWwindow;
class                                   ReplayerSnapshotPlayer;
class                                   ReplayerSnapshotter;
class                                   ReplayerWindow;
typedef std::unique_ptr<ReplayerWindow> ReplayerWindowUniquePtr;


class ReplayerWindow
{
public:
    /* Public consts */
    static const uint32_t SCROLLBAR_HEIGHT = 32;

    /* Public funcs */
    ~ReplayerWindow();

    void set_position(const std::array<uint32_t, 2>& in_x1y1);

    static ReplayerWindowUniquePtr create(const std::array<uint32_t, 2>& in_extents,
                                          ReplayerSnapshotter*           in_snapshotter_ptr,
                                          ReplayerSnapshotPlayer*        in_snapshot_player_ptr);

private:
    /* Private funcs */
    ReplayerWindow(const std::array<uint32_t, 2>& in_extents,
                   ReplayerSnapshotter*           in_snapshotter_ptr,
                   ReplayerSnapshotPlayer*        in_snapshot_player_ptr);

    void execute();
    bool init   ();

    /* Private vars */
    const std::array<uint32_t, 2> m_extents;
    float                         m_replay_segment_end_normalized;

    ReplayerSnapshotPlayer* m_snapshot_player_ptr;
    ReplayerSnapshotter*    m_snapshotter_ptr;
    GLFWwindow*             m_window_ptr;
    std::thread             m_worker_thread;
    volatile bool           m_worker_thread_must_die;
};

#endif /* REPLAYER_WINDOW_H */
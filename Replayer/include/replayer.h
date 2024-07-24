/* API Interceptor (c) 2024 Dominik Witczak
 *
 * This code is licensed under MIT license (see LICENSE.txt for details)
 */
#if !defined(REPLAYER_H)
#define REPLAYER_H

#include "replayer_types.h"
#include "replayer_snapshot_player.h"
#include "replayer_snapshotter.h"
#include "replayer_window.h"

/* Forward decls */
class                             Replayer;
typedef std::unique_ptr<Replayer> ReplayerUniquePtr;


class Replayer
{
public:
    /* Public funcs */
    static ReplayerUniquePtr create();

    ~Replayer();

    std::array<uint32_t, 2> get_q1_window_extents() const;
    void                    on_snapshot_requested();

private:
    /* Private funcs */
    Replayer();

    bool init              ();
    void reposition_windows();

    // Q1 API call interceptors -->
    static void on_q1_wglmakecurrent(APIInterceptor::APIFunction                in_api_func,
                                     uint32_t                                   in_n_args,
                                     const APIInterceptor::APIFunctionArgument* in_args_ptr,
                                     void*                                      in_user_arg_ptr);
    // <--

    /* Private vars */
    ReplayerSnapshotPlayerUniquePtr m_replayer_snapshot_player_ptr;
    ReplayerSnapshotterUniquePtr    m_replayer_snapshotter_ptr;
    ReplayerWindowUniquePtr         m_replayer_window_ptr;

    HWND m_q1_hwnd;
};

#endif /* REPLAYER_H */
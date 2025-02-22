/* API Interceptor (c) 2024 Dominik Witczak
 *
 * This code is licensed under MIT license (see LICENSE.txt for details)
 */
#if !defined(REPLAYER_H)
#define REPLAYER_H

#include "replayer_types.h"
#include "replayer_apicall_window.h"
#include "replayer_snapshot_logger.h"
#include "replayer_snapshot_player.h"
#include "replayer_snapshotter.h"
#include "replayer_window.h"

/* Global mutexes */
extern std::mutex g_imgui_mutex;

/* Forward decls */
class                             Replayer;
typedef std::unique_ptr<Replayer> ReplayerUniquePtr;

class Replayer
{
public:
    /* Public funcs */
    static ReplayerUniquePtr create();

    ~Replayer();

    void get_current_snapshot(const GLIDToTexturePropsMap** out_snapshot_gl_id_to_texture_props_map_ptr_ptr,
                              const ReplayerSnapshot**      out_snapshot_ptr_ptr,
                              const GLContextState**        out_snapshot_start_gl_context_state_ptr_ptr) const;

    std::vector<uint8_t>* get_current_snapshot_command_enabled_bool_as_u8_vec_ptr() const;
    const uint32_t&       get_n_current_snapshot                                 () const;

    std::array<uint32_t, 2> get_q1_window_extents () const;
    void                    on_snapshot_available () const;
    void                    on_snapshot_requested ();
    void                    refresh_windows       ();

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
    uint32_t                       m_n_snapshot;
    std::vector<uint8_t>           m_snapshot_command_enabled_bool_as_u8_vec;
    GLIDToTexturePropsMapUniquePtr m_snapshot_gl_id_to_texture_props_map_ptr;
    ReplayerSnapshotUniquePtr      m_snapshot_ptr;
    GLContextStateUniquePtr        m_snapshot_start_gl_context_state_ptr;

    ReplayerAPICallWindowUniquePtr  m_replayer_apicall_window_ptr;
    ReplayerSnapshotLoggerUniquePtr m_replayer_snapshot_logger_ptr;
    ReplayerSnapshotPlayerUniquePtr m_replayer_snapshot_player_ptr;
    ReplayerSnapshotterUniquePtr    m_replayer_snapshotter_ptr;
    ReplayerWindowUniquePtr         m_replayer_window_ptr;

    HWND m_q1_hwnd;
};

#endif /* REPLAYER_H */
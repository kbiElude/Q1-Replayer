/* API Interceptor (c) 2024 Dominik Witczak
 *
 * This code is licensed under MIT license (see LICENSE.txt for details)
 */
#if !defined(REPLAYER_APICALL_WINDOW_H)
#define REPLAYER_APICALL_WINDOW_H

#include "replayer_types.h"


/* Forward decls */
struct                                         GLFWwindow;
class                                          Replayer;
class                                          ReplayerAPICallWindow;
typedef std::unique_ptr<ReplayerAPICallWindow> ReplayerAPICallWindowUniquePtr;
class                                          ReplayerSnapshot;

class ReplayerAPICallWindow : public IUISettings
{
public:
    /* Public consts */

    /* Public funcs */
    ~ReplayerAPICallWindow();

    void load_snapshot             (ReplayerSnapshot* in_snapshot_ptr);
    void lock_for_snapshot_access  ();
    void unlock_for_snapshot_access();

    void set_position(const std::array<uint32_t, 2>& in_x1y1,
                      const std::array<uint32_t, 2>& in_extents);

    static ReplayerAPICallWindowUniquePtr create(Replayer* in_replayer_ptr);

private:
    /* IUISettings funcs */
    bool should_disable_lightmaps() const final
    {
        return m_should_disable_lightmaps;
    }

    bool should_draw_screenspace_geometry() const final
    {
        return m_should_draw_screenspace_geometry;
    }

    bool should_draw_weapon() const final
    {
        return m_should_draw_weapon;
    }

    bool should_shade_3d_models() const final
    {
        return m_should_shade_3d_models;
    }

    /* Private funcs */
    ReplayerAPICallWindow(Replayer* in_replayer_ptr);

    void analyze_snapshot           ();
    void execute                    ();
    void update_api_command_list_vec();

    /* Private vars */
    std::array<uint32_t, 2> m_window_extents;
    std::array<uint32_t, 2> m_window_x1y1;

    bool m_should_disable_lightmaps;
    bool m_should_draw_screenspace_geometry;
    bool m_should_draw_weapon;
    bool m_should_shade_3d_models;

    std::vector<std::string>   m_api_command_vec;
    std::mutex                 m_mutex;
    Replayer*                  m_replayer_ptr;
    ReplayerSnapshot*          m_snapshot_ptr;

    GLFWwindow*     m_window_ptr;
    std::thread     m_worker_thread;
    volatile bool   m_worker_thread_must_die;
};

#endif /* REPLAYER_APICALL_WINDOW_H */
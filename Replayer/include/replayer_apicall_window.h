/* API Interceptor (c) 2024 Dominik Witczak
 *
 * This code is licensed under MIT license (see LICENSE.txt for details)
 */
#if !defined(REPLAYER_APICALL_WINDOW_H)
#define REPLAYER_APICALL_WINDOW_H

#include "replayer_types.h"


/* Forward decls */
struct                                         GLFWwindow;
class                                          ReplayerAPICallWindow;
typedef std::unique_ptr<ReplayerAPICallWindow> ReplayerAPICallWindowUniquePtr;
class                                          ReplayerSnapshot;


class ReplayerAPICallWindow
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

    static ReplayerAPICallWindowUniquePtr create();

private:
    /* Private funcs */
    ReplayerAPICallWindow();

    void execute                    ();
    void update_api_command_list_vec();

    /* Private vars */
    std::array<uint32_t, 2> m_window_extents;
    std::array<uint32_t, 2> m_window_x1y1;

    std::vector<std::string> m_api_command_vec;
    std::mutex               m_mutex;
    ReplayerSnapshot*        m_snapshot_ptr;

    GLFWwindow*   m_window_ptr;
    std::thread   m_worker_thread;
    volatile bool m_worker_thread_must_die;
};

#endif /* REPLAYER_APICALL_WINDOW_H */
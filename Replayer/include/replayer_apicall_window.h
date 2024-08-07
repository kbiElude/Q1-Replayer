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


class ReplayerAPICallWindow
{
public:
    /* Public consts */

    /* Public funcs */
    ~ReplayerAPICallWindow();

    void set_position(const std::array<uint32_t, 2>& in_x1y1,
                      const std::array<uint32_t, 2>& in_extents);

    static ReplayerAPICallWindowUniquePtr create();

private:
    /* Private funcs */
    ReplayerAPICallWindow();

    void execute();

    /* Private vars */
    std::array<uint32_t, 2> m_window_extents;
    std::array<uint32_t, 2> m_window_x1y1;

    GLFWwindow*   m_window_ptr;
    std::thread   m_worker_thread;
    volatile bool m_worker_thread_must_die;
};

#endif /* REPLAYER_APICALL_WINDOW_H */
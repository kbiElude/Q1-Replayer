/* API Interceptor (c) 2024 Dominik Witczak
 *
 * This code is licensed under MIT license (see LICENSE.txt for details)
 */
#include <array>
#include <memory>
#include <thread>

/* Forward decls */
struct                                  GLFWwindow;
class                                   ReplayerWindow;
typedef std::unique_ptr<ReplayerWindow> ReplayerWindowUniquePtr;


class ReplayerWindow
{
public:
    /* Public funcs */
    ~ReplayerWindow();

    void set_position(const std::array<uint32_t, 2>& in_x1y1);

    static ReplayerWindowUniquePtr create(const std::array<uint32_t, 2>& in_extents);

private:
    /* Private funcs */
    ReplayerWindow(const std::array<uint32_t, 2>& in_extents);

    void execute();
    bool init   ();

    /* Private vars */
    const std::array<uint32_t, 2> m_extents;

    GLFWwindow*   m_window_ptr;
    std::thread   m_worker_thread;
    volatile bool m_worker_thread_must_die;
};
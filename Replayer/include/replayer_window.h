/* API Interceptor (c) 2024 Dominik Witczak
 *
 * This code is licensed under MIT license (see LICENSE.txt for details)
 */
#include <memory>
#include <thread>

/* Forward decls */
class                                   ReplayerWindow;
typedef std::unique_ptr<ReplayerWindow> ReplayerWindowUniquePtr;


class ReplayerWindow
{
public:
    /* Public funcs */
    static ReplayerWindowUniquePtr create();

    ~ReplayerWindow();

private:
    /* Private funcs */
    ReplayerWindow();

    void execute();
    bool init   ();

    /* Private vars */
    std::thread   m_worker_thread;
    volatile bool m_worker_thread_must_die;
};
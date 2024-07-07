/* API Interceptor (c) 2024 Dominik Witczak
 *
 * This code is licensed under MIT license (see LICENSE.txt for details)
 */
#include <memory>
#include <thread>

/* Forward decls */
class                             Replayer;
typedef std::unique_ptr<Replayer> ReplayerUniquePtr;


class Replayer
{
public:
    /* Public funcs */
    static ReplayerUniquePtr create();

    ~Replayer();

private:
    /* Private funcs */
    Replayer();

    void execute();
    bool init   ();

    /* Private vars */
    std::thread   m_worker_thread;
    volatile bool m_worker_thread_must_die;
};
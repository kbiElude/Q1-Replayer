/* API Interceptor (c) 2024 Dominik Witczak
 *
 * This code is licensed under MIT license (see LICENSE.txt for details)
 */
#include <memory>
#include <thread>
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

private:
    /* Private funcs */
    Replayer();

    bool init();

    /* Private vars */
    ReplayerWindowUniquePtr m_replayer_window_ptr;
};
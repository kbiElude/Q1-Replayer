/* API Interceptor (c) 2024 Dominik Witczak
 *
 * This code is licensed under MIT license (see LICENSE.txt for details)
 */
#include <memory>
#include <thread>
#include <Windows.h>
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

    bool init              ();
    void reposition_windows();

    // Q1 API call interceptors -->
    static void on_q1_wglmakecurrent(uint32_t                                   in_n_args,
                                     const APIInterceptor::APIFunctionArgument* in_args_ptr,
                                     void*                                      in_user_arg_ptr);
    // <--

    /* Private vars */
    ReplayerWindowUniquePtr m_replayer_window_ptr;

    HWND m_q1_hwnd;
};
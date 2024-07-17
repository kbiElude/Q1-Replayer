/* API Interceptor (c) 2024 Dominik Witczak
 *
 * This code is licensed under MIT license (see LICENSE.txt for details)
 */
#include "APIInterceptor/include/Common/callbacks.h"
#include "replayer.h"
#include "replayer_snapshotter.h"
#include <assert.h>

HHOOK     g_keyboard_hook = 0;
Replayer* g_replayer_ptr  = nullptr;


LRESULT CALLBACK on_keyboard_event(int    code,
                                   WPARAM wParam,
                                   LPARAM lParam)
{
    if (code >= 0)
    {
        if (wParam == VK_F11)
        {
            g_replayer_ptr->on_snapshot_requested();
        }
    }

    return CallNextHookEx(g_keyboard_hook,
                          code,
                          wParam,
                          lParam);
}


Replayer::Replayer()
    :m_q1_hwnd(0)
{
    /* Stub */
}

Replayer::~Replayer()
{
    g_replayer_ptr = nullptr;

    if (g_keyboard_hook != 0)
    {
        ::UnhookWindowsHookEx(g_keyboard_hook);

        g_keyboard_hook = 0;
    }

    m_replayer_window_ptr.reset();
}

ReplayerUniquePtr Replayer::create()
{
    ReplayerUniquePtr result_ptr(new Replayer() );

    if (result_ptr != nullptr)
    {
        if (!result_ptr->init() )
        {
            result_ptr.reset();
        }
        else
        {
            assert(g_replayer_ptr == nullptr);

            g_replayer_ptr = result_ptr.get();
        }
    }

    return result_ptr;
}

bool Replayer::init()
{
    m_replayer_snapshot_player_ptr = ReplayerSnapshotPlayer::create();
    m_replayer_snapshotter_ptr     = ReplayerSnapshotter::create   ();
    m_replayer_window_ptr          = ReplayerWindow::create        ({640, 480},
                                                                    m_replayer_snapshotter_ptr.get    (),
                                                                    m_replayer_snapshot_player_ptr.get() );

    assert(m_replayer_snapshotter_ptr != nullptr);
    assert(m_replayer_window_ptr      != nullptr);

    /* Register for callbacks */
    APIInterceptor::register_for_callback(APIInterceptor::APIFUNCTION_WGL_WGLMAKECURRENT,
                                         &on_q1_wglmakecurrent,
                                          this);

    return true;
}

void Replayer::on_q1_wglmakecurrent(APIInterceptor::APIFunction                in_api_func,
                                    uint32_t                                   in_n_args,
                                    const APIInterceptor::APIFunctionArgument* in_args_ptr,
                                    void*                                      in_user_arg_ptr)
{
    Replayer* this_ptr = reinterpret_cast<Replayer*>(in_user_arg_ptr);

    if (in_args_ptr[1].value.value_ptr != nullptr)
    {
        HDC  q1_hdc  = reinterpret_cast<HDC>(reinterpret_cast<intptr_t>(in_args_ptr[0].value.value_ptr) );
        HWND q1_hwnd = ::WindowFromDC       (q1_hdc);

        if (q1_hwnd != nullptr)
        {
            if (this_ptr->m_q1_hwnd == nullptr)
            {
                this_ptr->m_q1_hwnd = q1_hwnd;

                /* Now that we know which window the game uses, reposition the windows accordingly */
                this_ptr->reposition_windows();

                /* Register for keyboard events */
                if (g_keyboard_hook == 0)
                {
                    g_keyboard_hook = ::SetWindowsHookEx(WH_KEYBOARD,
                                                        &on_keyboard_event,
                                                         ::GetModuleHandleA("Replayer.dll"),
                                                         ::GetThreadId     (::GetCurrentThread() ));
                }
            }
            else
            {
                /* Wait this should never happen */
                assert(false);
            }
        }
    }
}

void Replayer::on_snapshot_requested()
{
    m_replayer_snapshotter_ptr->cache_snapshot();
}

void Replayer::reposition_windows()
{
    RECT q1_window_rect = {};

    ::GetWindowRect(m_q1_hwnd,
                   &q1_window_rect);

    /* Q1 window goes on top */
    const auto desktop_width    = ::GetSystemMetrics(SM_CXSCREEN);
    const auto desktop_height   = ::GetSystemMetrics(SM_CYSCREEN);
    const auto q1_window_width  = q1_window_rect.right - q1_window_rect.left;
    const auto q1_window_height = q1_window_rect.bottom - q1_window_rect.top;

    ::SetWindowPos(m_q1_hwnd,
                   HWND_DESKTOP,
                   (desktop_width - q1_window_width)  / 2,
                   0,
                   q1_window_width,
                   q1_window_height,
                   SWP_SHOWWINDOW);

    /* Replayer windows goes to the bottom */
    {
        const std::array<uint32_t, 2> new_x1y1 =
        {
            static_cast<uint32_t>( (desktop_width  - q1_window_width)  / 2),
            static_cast<uint32_t>( (desktop_height - q1_window_height)    )
        };

        m_replayer_window_ptr->set_position(new_x1y1);
    }
}
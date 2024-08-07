/* API Interceptor (c) 2024 Dominik Witczak
 *
 * This code is licensed under MIT license (see LICENSE.txt for details)
 */
#include "APIInterceptor/include/Common/callbacks.h"
#include "replayer.h"
#include "replayer_apicall_window.h"
#include "replayer_snapshotter.h"
#include "replayer_window.h"
#include <cassert>

std::mutex g_imgui_mutex;
HHOOK      g_keyboard_hook = 0;
Replayer*  g_replayer_ptr  = nullptr;

#ifdef max
    #undef max
#endif
#ifdef min
    #undef min
#endif

LRESULT CALLBACK on_keyboard_event(int    code,
                                   WPARAM wParam,
                                   LPARAM lParam)
{
    if (code >= 0)
    {
        if (wParam == VK_F11)
        {
            /* Only react when the key is being released */
            if (lParam & (1 << 31) )
            {
                g_replayer_ptr->on_snapshot_requested();
            }
        }
    }

    return CallNextHookEx(g_keyboard_hook,
                          code,
                          wParam,
                          lParam);
}


Replayer::Replayer()
    :m_n_snapshot(UINT32_MAX),
     m_q1_hwnd   (0)
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

void Replayer::get_current_snapshot(const GLIDToTexturePropsMap** out_snapshot_gl_id_to_texture_props_map_ptr_ptr,
                                    const ReplayerSnapshot**      out_snapshot_ptr_ptr,
                                    const std::vector<uint8_t>**  out_snapshot_prev_frame_depth_data_u8_vec_ptr_ptr,
                                    const GLContextState**        out_snapshot_start_gl_context_state_ptr_ptr) const
{
    *out_snapshot_gl_id_to_texture_props_map_ptr_ptr   = m_snapshot_gl_id_to_texture_props_map_ptr.get  ();
    *out_snapshot_ptr_ptr                              = m_snapshot_ptr.get                             ();
    *out_snapshot_prev_frame_depth_data_u8_vec_ptr_ptr = m_snapshot_prev_frame_depth_data_u8_vec_ptr.get();
    *out_snapshot_start_gl_context_state_ptr_ptr       = m_snapshot_start_gl_context_state_ptr.get      ();
}

std::vector<uint8_t>* Replayer::get_current_snapshot_command_enabled_bool_as_u8_vec_ptr() const
{
    return &const_cast<Replayer*>(this)->m_snapshot_command_enabled_bool_as_u8_vec;
}

const uint32_t& Replayer::get_n_current_snapshot() const
{
    return m_n_snapshot;
}

std::array<uint32_t, 2> Replayer::get_q1_window_extents() const
{
    return {640, 480};
}

bool Replayer::init()
{
    m_replayer_apicall_window_ptr  = ReplayerAPICallWindow::create (this);
    m_replayer_snapshot_logger_ptr = ReplayerSnapshotLogger::create();
    m_replayer_snapshot_player_ptr = ReplayerSnapshotPlayer::create(this);
    m_replayer_snapshotter_ptr     = ReplayerSnapshotter::create   (this);
    m_replayer_window_ptr          = ReplayerWindow::create        (get_q1_window_extents             (),
                                                                    this,
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
    auto      arg0_ptr = in_args_ptr[0].get_ptr();
    auto      arg1_ptr = in_args_ptr[1].get_ptr();
    Replayer* this_ptr = reinterpret_cast<Replayer*>(in_user_arg_ptr);

    if (arg1_ptr != nullptr)
    {
        HDC  q1_hdc  = reinterpret_cast<HDC>(reinterpret_cast<intptr_t>(arg0_ptr) );
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

void Replayer::on_snapshot_available() const
{
    /* If a snapshot has been captured, cache it and wake up the replayer window, so that it can consume it next. */
    {
        Replayer* this_ptr = const_cast<Replayer*>(this);

        m_replayer_snapshot_player_ptr->lock_for_snapshot_access();
        {
            /* NOTE: Since this function is likely called from within apllication's rendering thread, we need to remember
             *       that the replayer's rendering thread lives elsewhere, possibly consuming the snapshot in parallel.
             *       Make sure this is not the case by locking the access.
             */
            m_replayer_snapshotter_ptr->pop_snapshot(&this_ptr->m_snapshot_start_gl_context_state_ptr,
                                                     &this_ptr->m_snapshot_ptr,
                                                     &this_ptr->m_snapshot_gl_id_to_texture_props_map_ptr,
                                                     &this_ptr->m_snapshot_prev_frame_depth_data_u8_vec_ptr);

            /* Refresh the "command enabled" vector. Assume all commands are enabled by default.
             *
             * NOTE: We can't do a bare memset here because bool is a compiler-specific type (sic) and imgui explicitly
             *       requires bool-typed input
             */
            {
                bool*      bool_ptr       = nullptr;
                const auto n_api_commands = this_ptr->m_snapshot_ptr->get_n_api_commands();

                this_ptr->m_snapshot_command_enabled_bool_as_u8_vec.resize(sizeof(bool) * n_api_commands);

                bool_ptr = reinterpret_cast<bool*>(this_ptr->m_snapshot_command_enabled_bool_as_u8_vec.data() );

                for (uint32_t n_api_command = 0;
                              n_api_command < n_api_commands;
                            ++n_api_command, bool_ptr++)
                {
                    *bool_ptr = true;
                }
            }

            /* Reinitialize API call window with the new snapshot */
            m_replayer_apicall_window_ptr->load_snapshot(this_ptr->m_snapshot_ptr.get() );

            /* While we're at it, log the snapshot's contents to a dump file.. */
            m_replayer_snapshot_logger_ptr->log_snapshot(this_ptr->m_snapshot_start_gl_context_state_ptr.get    (),
                                                         this_ptr->m_snapshot_ptr.get                           (),
                                                         this_ptr->m_snapshot_gl_id_to_texture_props_map_ptr.get() );

            ++this_ptr->m_n_snapshot;
        }
        m_replayer_snapshot_player_ptr->unlock_for_snapshot_access();
    }

    m_replayer_window_ptr->refresh();
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
    static const uint32_t APICALL_WINDOW_WIDTH = 600;

    const auto caption_height         = ::GetSystemMetrics(SM_CYCAPTION);
    const auto desktop_width          = ::GetSystemMetrics(SM_CXSCREEN);
    const auto desktop_height         = ::GetSystemMetrics(SM_CYSCREEN);
    const auto frame_height           = ::GetSystemMetrics(SM_CYFRAME);
    const auto q1_window_width        = q1_window_rect.right - q1_window_rect.left;
    const auto q1_window_height       = q1_window_rect.bottom - q1_window_rect.top;
    const auto replayer_window_width  = q1_window_width;
    const auto replayer_window_height = q1_window_height;
    const auto q1_window_x            = (desktop_width  - q1_window_width - APICALL_WINDOW_WIDTH) / 2;
    const auto q1_window_y            = (desktop_height - replayer_window_height - q1_window_height)  / 2;

    {
        ::SetWindowPos(m_q1_hwnd,
                       HWND_DESKTOP,
                       q1_window_x,
                       q1_window_y,
                       q1_window_width,
                       q1_window_height,
                       SWP_SHOWWINDOW);

        /* Glue replayer window right underneath Q1 window */
        {
            const std::array<uint32_t, 2> new_x1y1 =
            {
                static_cast<uint32_t>(q1_window_x),
                static_cast<uint32_t>(q1_window_y + q1_window_height + caption_height + frame_height),
            };

            m_replayer_window_ptr->set_position(new_x1y1);
        }

        /* API call window goes to the right */
        {
            const std::array<uint32_t, 2> new_x1y1 =
            {
                static_cast<uint32_t>(q1_window_x + q1_window_width),
                static_cast<uint32_t>(q1_window_y + caption_height + frame_height),
            };
            const std::array<uint32_t, 2> new_extents =
            {
                APICALL_WINDOW_WIDTH,
                static_cast<uint32_t>(q1_window_height + replayer_window_height),
            };

            m_replayer_apicall_window_ptr->set_position(new_x1y1,
                                                        new_extents);
        }
    }
}
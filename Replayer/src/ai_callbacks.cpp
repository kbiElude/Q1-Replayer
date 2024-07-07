/* API Interceptor (c) 2024 Dominik Witczak
 *
 * This code is licensed under MIT license (see LICENSE.txt for details)
 */
 #include "Common/callbacks.h"
 #include "Replayer.h"

static ReplayerUniquePtr g_replayer_ptr;


void callback_test(uint32_t                                   in_n_args,
                   const APIInterceptor::APIFunctionArgument* in_args_ptr,
                   void*                                      in_user_arg_ptr)
{
    in_user_arg_ptr = nullptr;
}

void on_api_interceptor_removed()
{
    g_replayer_ptr.reset();
}

void on_api_interceptor_injected()
{
    g_replayer_ptr = Replayer::create();
}
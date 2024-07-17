/* API Interceptor (c) 2024 Dominik Witczak
 *
 * This code is licensed under MIT license (see LICENSE.txt for details)
 */
#include <cassert>
#include <functional>
#include "replayer_snapshot.h"

ReplayerSnapshot::ReplayerSnapshot()
{
    /* Stub */
}

ReplayerSnapshot::~ReplayerSnapshot()
{
    /* Stub */
}

ReplayerSnapshotUniquePtr ReplayerSnapshot::create()
{
    ReplayerSnapshotUniquePtr result_ptr(new ReplayerSnapshot() );

    assert(result_ptr != nullptr);
    return result_ptr;
}

uint32_t ReplayerSnapshot::get_n_api_commands() const
{
    return static_cast<uint32_t>(m_api_command_vec.size() );
}

const APICommand* ReplayerSnapshot::get_api_command_ptr(const uint32_t& in_n_api_command) const
{
    return &m_api_command_vec.at(in_n_api_command);
}


void ReplayerSnapshot::record_api_call(const APIInterceptor::APIFunction&         in_api_func,
                                       const uint32_t&                            in_n_args,
                                       const APIInterceptor::APIFunctionArgument* in_args_ptr)
{
    APICommand* cached_api_command_ptr = nullptr;

    m_api_command_vec.push_back(APICommand() );

    cached_api_command_ptr           = &m_api_command_vec.back();
    cached_api_command_ptr->api_func = in_api_func;

    cached_api_command_ptr->api_arg_vec.reserve(in_n_args);

    for (uint32_t n_current_arg = 0;
                  n_current_arg < in_n_args;
                ++n_current_arg)
    {
        cached_api_command_ptr->api_arg_vec.emplace_back(in_args_ptr[n_current_arg]);
    }
}

void ReplayerSnapshot::reset()
{
    m_api_command_vec.clear();
}
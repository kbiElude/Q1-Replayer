/* API Interceptor (c) 2024 Dominik Witczak
 *
 * This code is licensed under MIT license (see LICENSE.txt for details)
 */
#if !defined(REPLAYER_SNAPSHOT_H)
#define REPLAYER_SNAPSHOT_H

#include "APIInterceptor/include/Common/types.h"
#include "replayer_types.h"

/* Forward decls */
class                                     ReplayerSnapshot;
typedef std::unique_ptr<ReplayerSnapshot> ReplayerSnapshotUniquePtr;


class ReplayerSnapshot
{
public:
    /* Public funcs */
    ~ReplayerSnapshot();

    uint32_t          get_n_api_commands ()                                 const;
    const APICommand* get_api_command_ptr(const uint32_t& in_n_api_command) const;

    void record_api_call(const APIInterceptor::APIFunction&         in_api_func,
                         const uint32_t&                            in_n_args,
                         const APIInterceptor::APIFunctionArgument* in_args_ptr);
    void reset          ();

    static ReplayerSnapshotUniquePtr create();

private:
    /* Private funcs */
    ReplayerSnapshot();

    /* Private vars */
    std::vector<APICommand> m_api_command_vec;
};

#endif /* REPLAYER_SNAPSHOT_H */
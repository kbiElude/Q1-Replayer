/* API Interceptor (c) 2024 Dominik Witczak
 *
 * This code is licensed under MIT license (see LICENSE.txt for details)
 */
#if !defined(REPLAYER_SNAPSHOT_LOGGER_H)
#define REPLAYER_SNAPSHOT_LOGGER_H

#include "APIInterceptor/include/Common/types.h"
#include "replayer_snapshot.h"

/* Forward decls */
class                                           ReplayerSnapshotLogger;
typedef std::unique_ptr<ReplayerSnapshotLogger> ReplayerSnapshotLoggerUniquePtr;


class ReplayerSnapshotLogger
{
public:
    /* Public funcs */
    static ReplayerSnapshotLoggerUniquePtr create();

    ~ReplayerSnapshotLogger();

    void log_snapshot(const GLContextState*        in_start_context_state_ptr,
                      const ReplayerSnapshot*      in_snapshot_ptr,
                      const GLIDToTexturePropsMap* in_snapshot_gl_id_to_texture_props_map_ptr);

private:
    /* Private type defs */

    /* Private funcs */
    ReplayerSnapshotLogger();

    /* Private vars */
    uint32_t m_n_snapshots_dumped;
};

#endif /* REPLAYER_SNAPSHOT_LOGGER_H */
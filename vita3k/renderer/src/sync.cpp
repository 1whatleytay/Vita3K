#include <chrono>
#include <gxm/types.h>
#include <renderer/commands.h>
#include <renderer/renderer.h>
#include <renderer/types.h>

#include "driver_functions.h"
#include <renderer/gl/functions.h>

#include <renderer/functions.h>
#include <util/log.h>

namespace renderer {
COMMAND(handle_nop) {
    // Signal back to client
    int code_to_finish = command.pop<int>();
    renderer.complete(command, code_to_finish);
}

COMMAND(handle_signal_sync_object) {
    SceGxmSyncObject *sync = command.pop<Ptr<SceGxmSyncObject>>().get(mem);
    renderer::subject_done(sync, renderer::SyncObjectSubject::Fragment);
}

void wishlist(SceGxmSyncObject *sync_object, const SyncObjectSubject subjects) {
    {
        const std::lock_guard<std::mutex> mutex_guard(sync_object->lock);

        if (sync_object->done & subjects) {
            return;
        }
    }

    std::unique_lock<std::mutex> finish_mutex(sync_object->lock);
    sync_object->cond.wait(finish_mutex, [&]() { return sync_object->done & subjects; });
}

void subject_done(SceGxmSyncObject *sync_object, const SyncObjectSubject subjects) {
    {
        const std::lock_guard<std::mutex> mutex_guard(sync_object->lock);
        sync_object->done |= subjects;
    }

    sync_object->cond.notify_all();
}

void subject_in_progress(SceGxmSyncObject *sync_object, const SyncObjectSubject subjects) {
    const std::lock_guard<std::mutex> mutex_guard(sync_object->lock);
    sync_object->done &= ~subjects;
}
} // namespace renderer

#include <renderer/commands.h>
#include <renderer/functions.h>
#include <renderer/types.h>

#include "driver_functions.h"

#include <functional>
#include <util/log.h>

struct FeatureState;

namespace renderer {
void Command::complete(int code) const {
    *status = code;
}
} // namespace renderer
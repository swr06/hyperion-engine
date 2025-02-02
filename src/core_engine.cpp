#include "core_engine.h"
#include <exception>

namespace hyperion {

CoreEngine *CoreEngine::instance = nullptr;

CoreEngine *CoreEngine::GetInstance()
{
    if (instance == nullptr) {
        throw "no engine instance created";
    }
    return instance;
}

void CoreEngine::SetInstance(CoreEngine *ptr)
{
    instance = ptr;
}

} // namespace hyperion

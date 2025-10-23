#pragma once

namespace cheat
{
    void init();
    void shutdown();
    void hookMonoBehaviour(uintptr_t unity, int versionMajor);
}

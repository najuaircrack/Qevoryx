#include "app/thread_affinity.hpp"
#include "common/platform.hpp"
#include <thread>

#if QEVORYX_PLATFORM_LINUX
#include <pthread.h>
#endif

namespace app {

bool pin_current_thread(std::uint32_t cpu_index) noexcept {
#if QEVORYX_PLATFORM_LINUX
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu_index % std::thread::hardware_concurrency(), &cpuset);
    return pthread_setaffinity_np(pthread_self(), sizeof(cpuset), &cpuset) == 0;
#else
    // Windows: SetThreadAffinityMask would go here
    // For now, just return true (no pinning on Windows)
    (void)cpu_index;
    return true;
#endif
}

} // namespace app

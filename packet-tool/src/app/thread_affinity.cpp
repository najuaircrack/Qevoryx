#include "app/thread_affinity.hpp"
#include <pthread.h>
#include <thread>

namespace app {

bool pin_current_thread(std::uint32_t cpu_index) noexcept {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu_index % std::thread::hardware_concurrency(), &cpuset);
    return pthread_setaffinity_np(pthread_self(), sizeof(cpuset), &cpuset) == 0;
}

} // namespace app

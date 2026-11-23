#include "core/main_thread.hpp"

#include <mutex>
#include <vector>

namespace il2cpp_base::main_thread {
namespace {

std::mutex g_mutex;
std::vector<std::function<void()>> g_queue;

} // namespace

void post(std::function<void()> task) {
    if (!task) {
        return;
    }
    std::lock_guard lock(g_mutex);
    g_queue.push_back(std::move(task));
}

void run_pending() {
    std::vector<std::function<void()>> tasks;
    {
        std::lock_guard lock(g_mutex);
        tasks.swap(g_queue);
    }
    for (auto& task : tasks) {
        task();
    }
}

} // namespace il2cpp_base::main_thread

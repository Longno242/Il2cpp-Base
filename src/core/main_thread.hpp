#pragma once

#include <functional>

namespace il2cpp_base::main_thread {

// Queues `task` for Unity's main thread, the only place it is safe to call game code.
// Queued tasks run once per frame from the game's window procedure.
void post(std::function<void()> task);

void run_pending();

} // namespace il2cpp_base::main_thread

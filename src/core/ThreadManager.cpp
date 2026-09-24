#include "core/ThreadManager.h"

uint32_t
Cel::ThreadManager::total_threads()
{
    return 1;
}

uint32_t
Cel::ThreadManager::get_thread_id()
{
    return 0;
}

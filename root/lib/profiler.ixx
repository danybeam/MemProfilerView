module;
#include "profiler.h"

uint8_t ProfileLock::semaphore_ = 1; // Necessary to define saveProfiling
bool ProfileLock::forceLock_ = false;

export module ProfilerModule;

// TODO(danybeam) this is to enable proper compilation. After texture issue is fixed try to migrate header here.
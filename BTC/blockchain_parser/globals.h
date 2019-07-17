#pragma once
#include <atomic>
#include <common/misc.h>

extern std::atomic<bool> continue_running;
extern OutputSequencer mstdout;
extern OutputSequencer mstderr;

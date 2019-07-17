#include "globals.h"
#include <iostream>

std::atomic<bool> continue_running;
std::mutex output_mutex;
OutputSequencer mstdout(std::cout, output_mutex);
OutputSequencer mstderr(std::cerr, output_mutex);

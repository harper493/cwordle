#include "random.h"

std::random_device random::rd;
std::mt19937 random::engine(rd());

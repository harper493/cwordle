#ifndef __RANDOM__
#define __RANDOM__

#include "types.h"
#include <random>

class random
{
private:
    static std::random_device rd;
    static std::mt19937 engine;
public:
    static void initialize()
    {
    }
    
    static float get_random(float range=1.0)
    {
         std::uniform_real_distribution<> dis(0, range);
         return dis(engine);
    }
    static float get_poisson(float range=1.0)
    {
         std::poisson_distribution<> dis(0.5);
         return dis(engine) * 0.1;
    }
    static size_t get_int(size_t max)
    {
        std::uniform_int_distribution<> dis(0, max);
        return dis(engine);
    }
};

#endif

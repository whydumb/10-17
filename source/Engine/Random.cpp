/**
 * \file
 * \author Rudy Castan
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */
#include "Random.hpp"

#include <cassert>
#include <chrono>
#include <random>
#include <thread>

namespace
{
    using namespace std;
    using namespace chrono;

    mt19937_64& Engine()
    {
        thread_local static mt19937_64 engine{ static_cast<mt19937_64::result_type>(system_clock::now().time_since_epoch().count()) + hash<thread::id>{}(this_thread::get_id()) };
        return engine;
    }
}

namespace util
{
    double random(double min_inclusive, double max_exclusive) noexcept
    {
        assert(min_inclusive < max_exclusive);
        return std::uniform_real_distribution<double>(min_inclusive, max_exclusive)(Engine());
    }

    int random(int min_inclusive, int max_exclusive) noexcept
    {
        assert(min_inclusive < max_exclusive - 1);
        return std::uniform_int_distribution<int>(min_inclusive, max_exclusive - 1)(Engine());
    }

    int random(int max_exclusive) noexcept
    {
        return random(0, max_exclusive);
    }
}

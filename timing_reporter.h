#ifndef __TIMING_REPORTER
#define __TIMING_REPORTER

#include "types.h"
#include "formatted.h"

typedef std::chrono::time_point<std::chrono::high_resolution_clock> hrclock;

class timing_reporter
{
private:
    hrclock start;
    hrclock stop;
    float total_duration = 0;
    bool paused = false;
    size_t start_count = 0;
public:
    timing_reporter(bool p=false)
        : start(high_resolution_clock::now()), paused(p) {};
    void pause()
    {
        if (!paused) {
            stop = high_resolution_clock::now();
            auto duration = duration_cast<nanoseconds>(stop - start);
            total_duration += duration.count();
            paused = true;
        }
    }
    void restart()
    {
        if (paused) {
            start = high_resolution_clock::now();
            paused = false;
            ++start_count;
        }
    }
    void reset()
    {
        paused = true;
        total_duration = 0;
        start_count = 0;
    }
    timing_reporter operator+=(const timing_reporter &other)
    {
        total_duration += other.total_duration;
        start_count += other.start_count;
        return *this;
    }
    string show_time()
    {
        pause();
        return show_time(total_duration);
    }
    string show_time(float ns) const
    {
        if (ns > 1e9) {
            return formatted("%.3f S", ns / 1e9);
        } else if (ns > 1e6) {
            return formatted("%.3f mS", ns/ 1e6);
        } else if (ns > 1e3) {
            return formatted("%.3f uS", ns / 1e3);
        } else {
            return formatted("%.3f nS", ns);
        }
    }
    string report(const string &what, const string &prefix="")
    {
        return report(start_count, what, prefix);
    }
    string report(size_t count, const string &what, const string &prefix="")
    {
        pause();
        string w = what.empty() ? "" : what + " ";
        return formatted("%s%d %sin %s, %s each\n",
                         prefix, count, w, show_time(total_duration),
                         show_time(total_duration / count));
    }
    void show(const string &what, const string &prefix="")
    {
        show(start_count, what, prefix);
    }
    void show(size_t count, const string &what, const string &prefix="")
    {
        cout << report(count, what, prefix);
    }
};

#endif

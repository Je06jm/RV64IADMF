#ifndef DELTA_TIME_HPP
#define DELTA_TIME_HPP

class _DeltaTime {
    double last = 0.0;
    double delta_time = 0.0;

public:
    inline double operator()() { return delta_time; }

    void Update();
    double GetCurrentTime();
};

inline _DeltaTime delta_time;

#endif
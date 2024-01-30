#include <chrono>
using namespace std::chrono;
class Timer {
  public:
    Timer() { start_time = steady_clock::now(); }

    ~Timer() { stop_time = steady_clock::now(); }

    double get_elapsed_time_ms() {
        auto current_time = std::chrono::steady_clock::now();
        return duration_cast<milliseconds>(current_time - start_time).count();
    }

  private:
    time_point<steady_clock> start_time;
    time_point<steady_clock> stop_time;
};
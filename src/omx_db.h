
#include <chrono>
#include <ratio>
#include <vector>
#include <unordered_map>
#include <mutex>


class OMXDatabase {
  struct DataPoint {
    std::chrono::time_point<std::chrono::high_resolution_clock> time;
    std::string data;
  };
  using InstrumentDataStore = std::unordered_map<std::string, std::vector<DataPoint>>;

public:
  bool add_new_datapoint(
    std::string instrument_id,
    std::chrono::time_point<std::chrono::high_resolution_clock> time,
    std::string data) {

    std::scoped_lock<std::mutex> lock(m);

    auto handle = instrument_data[instrument_id];
    handle.push_back({
      time,
      data
      }
    );

    return true;
  }

private:
  InstrumentDataStore instrument_data;
  std::mutex m;
};

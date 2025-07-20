
#include <string>


struct OrderBookMessage {
  uint64_t timestamp;
  std::string Symbol;
  double price;
};

struct BasicDataMessage {
  uint64_t timestamp;
  std::string Symbol;
  double price;
};

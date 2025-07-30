
#include <array>
#include <new>
#include <string_view>

enum class MessageTag : uint8_t {
  BasicDataTradable = 0,
  BasicDataBond = 1,
  OrderBook = 20,
};

class MessageSlot {
public:
  template<typename T>
  auto get() {
    return std::launder(reinterpret_cast<T>(payload));
  }

private:
  MessageTag tag;
  std::array<std::byte, 2048> payload;
};

class RingBuffer {
public:
  bool emplace(const std::array<std::byte, 2048>&& payload) {
    auto tag = get_message_type(payload);

    switch(tag) {
      case MessageTag::BasicDataBond: break;
    }
    return true;
  }
    

private:
  MessageTag get_message_type(const std::array<std::byte, 2048>& payload) {

   
  }

  std::array<std::array<std::byte, 2048>, 4096> messages;
  uint64_t head;
  uint64_t tail;

  static constexpr std::array<std::pair<std::string_view, MessageTag>, static_cast<uint64_t>(MessageTag::OrderBook)> translation;
};

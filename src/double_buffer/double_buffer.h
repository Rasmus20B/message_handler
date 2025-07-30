#pragma once

#include <array>
#include <atomic>
#include <bit>
#include <chrono>
#include <cstddef>
#include <memory>
#include <mutex>
#include <print>
#include <ratio>
#include <span>
#include <thread>
#include <tuple>
#include <utility>

#include <ranges>

#include "../messages.h"

namespace message_handler {


static inline std::atomic<int> counter = 0;
// This is a container of a given type of decoded message that get processed
// similar to a std::vector<OrderBookMessage>
template <typename MessageType, uint64_t FrameSize>
  requires(std::has_single_bit(FrameSize) && FrameSize % 8 == 0)
struct MessageLine {
	using value_type = MessageType;

  void append(MessageType m) {
    if (this->head >= FrameSize) {
  		std::println("tried to push into full buffer ({} / {}).", head, FrameSize);
      return;
    }

    buffer[this->head] = m;
    head++;
  }

  void emplace(MessageType&& m) {
  	if(this->head >= FrameSize) {
  		std::println("tried to emplace into full buffer ({} / {}).", head, FrameSize);
  		return;
  	}

  	buffer[this->head] = m;
  	head++;
  }

  std::array<MessageType, FrameSize> buffer;
  uint64_t head{0};
};

template <typename MessageType> struct Processor;

template <> struct Processor<OrderBookMessage> {
  static void process(std::span<OrderBookMessage> messages) {
  	for(auto m: messages) {
  		counter++;
  	}
    // std::println("messages in o buffer: {}", messages.size());
  }
};

template <> struct Processor<BasicDataMessage> {
  static void process(std::span<BasicDataMessage> messages) {
  	for(auto m: messages) {
  		counter++;
  	}
    // std::println("messages in bdm buffer: {}", messages.size());
  }
};

template <uint64_t FrameSize, typename CTX, typename... MessageTypes>
  requires(std::has_single_bit(FrameSize))
struct MessageHandler {
	using MessageFrame = std::tuple<MessageLine<MessageTypes, FrameSize>...>;

public:
  template <typename MessageType> bool emplace(MessageType&& message) {
  	while(true) {
	  	uint8_t old_active = active_frame_handle.load(std::memory_order_relaxed);
	  	uint8_t pending = !old_active;

	  	writer_counts[pending].fetch_add(1, std::memory_order_acq_rel);

	  	if(active_frame_handle.load(std::memory_order_acquire) != old_active) {
	  		writer_counts[pending].fetch_sub(1, std::memory_order_relaxed);
	  		continue;
	  	}

	  	auto& line = std::get<MessageLine<MessageType, FrameSize>>(message_frames[pending]);
	  	line.emplace(std::move(message));

	  	writer_counts[pending].fetch_sub(1, std::memory_order_acq_rel);
	  	return true;
  	}
  }

	void try_flush() {
		auto new_active = !active_frame_handle.load(std::memory_order_relaxed);
		active_frame_handle.store(new_active, std::memory_order_release);

		// if there are any threads still writing to the old pending frame
		// then we wait.
		while (writer_counts[new_active].load(std::memory_order_seq_cst) != 0) {
			std::this_thread::yield();
		}

	  std::apply([&]<typename... MessageLines>(MessageLines&... lines) {
	    (Processor<typename MessageLines::value_type>::process(std::span(lines.buffer.data(), lines.head)), ...);
	    ((lines.head = 0),...);
	  }, message_frames[new_active]);
	}

	void start() {
		shutdown.store(false);
		process_thread = std::thread(
			[&]() {
				while(!shutdown.load(std::memory_order_relaxed)) {
					try_flush();
				}
				// Flush the final buffer
				try_flush();
			}
		);
	}

	void stop() {

		while(writer_counts[!active_frame_handle.load()].load() != 0) {
			std::this_thread::yield();
		}
		shutdown.store(true);

		process_thread.join();
		try_flush();
	}

private:
  std::array<MessageFrame, 2>
      message_frames;
  std::atomic<uint8_t> active_frame_handle{0};
  std::array<std::atomic<uint32_t>, 2> writer_counts{0, 0};
  std::thread process_thread;
  std::atomic<bool> shutdown;
  std::shared_ptr<CTX> context;
};

} // namespace message_handler

#pragma once

#include <array>
#include <atomic>
#include <bit>
#include <chrono>
#include <cstddef>
#include <latch>
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
#include "../thread_pool/thread_pool.h"

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
      return;
    }

    buffer[this->head] = m;
    head++;
  }

  bool emplace(MessageType&& m) {
  	if(this->head >= FrameSize) {
  		return false;
  	}

  	buffer[this->head] = m;
  	head++;
  	return true;
  }

  std::array<MessageType, FrameSize> buffer;
  uint64_t head{0};
};

template <typename MessageType, typename CTX> struct Processor;

template <typename CTX> struct Processor<OrderBookMessage, CTX> {
  static void process(std::span<OrderBookMessage> messages, std::shared_ptr<CTX>& context) {
  	for(auto m: messages) {
  		counter.fetch_add(1);
  	}
  }
};

template <typename CTX> struct Processor<BasicDataMessage, CTX> {
  static void process(std::span<BasicDataMessage> messages, std::shared_ptr<CTX>& context) {
  	for(auto m: messages) {
  		counter.fetch_add(1);
  	}
  }
};

template <uint64_t FrameSize, typename CTX, typename... MessageTypes>
  requires(std::has_single_bit(FrameSize))
struct MessageHandler {
	using MessageFrame = std::tuple<MessageLine<MessageTypes, FrameSize>...>;

  template <typename MessageType> bool emplace(MessageType&& message) {
  	while(true) {
	  	uint8_t active = active_frame_handle.load(std::memory_order_relaxed);
	  	uint8_t pending = !active;

	  	writer_counts[pending].fetch_add(1, std::memory_order_acq_rel);

	  	if(active_frame_handle.load(std::memory_order_acquire) != active) {
	  		writer_counts[pending].fetch_sub(1, std::memory_order_relaxed);
	  		continue;
	  	}

	  	auto& line = std::get<MessageLine<MessageType, FrameSize>>(message_frames[pending]);
	  	if(!line.emplace(std::move(message))) {
	  		writer_counts[pending].fetch_sub(1, std::memory_order_relaxed);
	  		return false;
	  	}

	  	writer_counts[pending].fetch_sub(1, std::memory_order_acq_rel);
	  	return true;
  	}
  }

	void try_flush() {
		auto new_active = static_cast<uint32_t>(!active_frame_handle.load(std::memory_order_acquire));
		active_frame_handle.store(new_active, std::memory_order_release);

		// if there are any threads still writing to the old pending frame
		// then we wait.
		while (writer_counts[new_active].load(std::memory_order_seq_cst) != 0) {
			std::this_thread::yield();
		}

		std::latch done_latch(sizeof...(MessageTypes));

		ThreadPool pool(std::thread::hardware_concurrency());

	  std::apply([&]<typename... MessageLines>(MessageLines&... lines) {
	    (pool.submit(([this, &done_latch, &lines] {
		    Processor<typename MessageLines::value_type, CTX>::process(
		    	std::span(lines.buffer.data(), lines.head),
		    	this->context
		    );
		    lines.head = 0;
		    done_latch.count_down();
	    })), ...);
	  }, message_frames[new_active]);

		done_latch.wait();

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

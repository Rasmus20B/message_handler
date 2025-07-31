#include "../src/double_buffer/double_buffer_tp.h"
#include "../src/single_buffer/single_buffer.h"
#include <chrono>
#include <gtest/gtest.h>

#include <string>
#include <thread>

using namespace message_handler;

TEST(add, basic) {

	MessageHandler<2048, void,
	 		OrderBookMessage,
	 		BasicDataMessage> message_handler;

	message_handler.start();

	for(uint64_t i = 0; i < 500000; i++) {
		while(!message_handler.emplace (
		  OrderBookMessage {
				.timestamp = 1238947389,
				.Symbol = "MSFT",
				.price = 12.43,
     	}));
		while(!message_handler.emplace (
		  BasicDataMessage {
				.timestamp = 1238947134,
				.Symbol = "NVDA",
				.price = 67.43,
     	}));
	}
	message_handler.stop();

	std::println("{}", counter.load());
}

TEST(single_arena, basic) {
	
}

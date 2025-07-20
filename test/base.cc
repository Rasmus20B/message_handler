#include "../src/message_handler.h"
#include <chrono>
#include <gtest/gtest.h>

#include <string>
#include <thread>

using namespace message_handler;

TEST(add, basic) {

	MessageHandler<32768,
	 		OrderBookMessage,
	 		BasicDataMessage> message_handler;

	message_handler.start();

	for(int i = 0; i < 300000000; i++) {
		message_handler.emplace (
		  OrderBookMessage {
				.timestamp = 1238947389,
				.Symbol = "MSFT",
				.price = 12.43,
     	});
		message_handler.emplace (
		  BasicDataMessage {
				.timestamp = 1238947134,
				.Symbol = "NVDA",
				.price = 67.43,
     	});
	}
	message_handler.stop();

	std::println("{}", counter.load());
}

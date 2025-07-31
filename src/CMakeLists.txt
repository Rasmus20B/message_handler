
add_library(${PROJECT_NAME} STATIC)

target_link_libraries(${PROJECT_NAME})

target_sources(${PROJECT_NAME}
	PUBLIC
		./message_handler.cc
		./messages.h
		./double_buffer/double_buffer.h
		./double_buffer/double_buffer_tp.h
		./single_buffer/single_buffer.h
		./output_buffer/output_buffer.h
		./thread_pool/thread_pool.h
)


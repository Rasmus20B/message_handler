set(SOURCE_FILES
	${CMAKE_SOURCE_DIR}/src/message_handler.cc
	${CMAKE_SOURCE_DIR}/src/message_handler.h
)
include_directories(${CMAKE_SOURCE_DIR}/src)
add_library(${PROJECT_NAME} ${SOURCE_FILES})
target_link_libraries(${PROJECT_NAME})

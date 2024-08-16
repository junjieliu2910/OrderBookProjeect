set(ORDERBOOK_SRC_DIR ${PROJECT_SOURCE_DIR}/src)
set(ORDERBOOK_SRC_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/src/include)

file(GLOB ORDERBOOK_SOURCE RELATIVE ${ORDERBOOK_SRC_DIR} "*.cpp")
add_library(orderBook)

target_sources(
  orderBook
  PRIVATE ${ORDERBOOK_SOURCE}
)

target_include_directories(
  orderBook
  PUBLIC ${ORDERBOOK_SRC_INCLUDE_DIR}
)

target_compile_options(
  orderBook
  PRIVATE ${CMAKE_COMPILER_FLAG}
)
include(GoogleTest)

file(GLOB TEST_FILES "test_*.cpp")
foreach (FILE_NAME ${TEST_FILES})
  get_filename_component(TEST_NAME ${FILE_NAME} NAME_WE)
  add_executable(${TEST_NAME} ${FILE_NAME})
  target_include_directories(
    ${TEST_NAME}
    PRIVATE ${ORDERBOOK_SRC_INCLUDE_DIR}
  )
  target_link_libraries(
    ${TEST_NAME}
    gtest
    gtest_main
    orderBook
  )
  gtest_discover_tests(${TEST_NAME})
endforeach (FILE_NAME ${TEST_FILES})
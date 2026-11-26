#pragma once
#include <cstdio>
#include <cstring>
#include <cstdint>

inline int ava_unity_failures = 0;
inline int ava_unity_tests = 0;

#define UNITY_BEGIN() do { ava_unity_failures = 0; ava_unity_tests = 0; std::puts("==== AVA OTA V3 native Unity tests ===="); } while (0)
#define RUN_TEST(fn) do { ++ava_unity_tests; std::printf("[ RUN      ] %s\n", #fn); fn(); } while (0)
#define UNITY_END() do { std::printf("[==========] %d tests, %d failures\n", ava_unity_tests, ava_unity_failures); } while (0)
#define AVA_UNITY_ASSERT(condition, message) do { if (!(condition)) { ++ava_unity_failures; std::printf("[  FAILED  ] %s:%d: %s\n", __FILE__, __LINE__, message); } } while (0)
#define TEST_ASSERT_TRUE(value) AVA_UNITY_ASSERT((value), "expected true")
#define TEST_ASSERT_FALSE(value) AVA_UNITY_ASSERT(!(value), "expected false")
#define TEST_ASSERT_EQUAL(expected, actual) AVA_UNITY_ASSERT((expected) == (actual), "values differ")
#define TEST_ASSERT_EQUAL_UINT32(expected, actual) AVA_UNITY_ASSERT(static_cast<uint32_t>(expected) == static_cast<uint32_t>(actual), "uint32 values differ")
#define TEST_ASSERT_EQUAL_STRING(expected, actual) AVA_UNITY_ASSERT(std::strcmp((expected), (actual)) == 0, "strings differ")

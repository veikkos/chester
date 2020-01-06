#include "test-runner.hpp"

#include "gtest/gtest.h"

extern "C" {
#include "chester.h"
}

#include <string>

static std::string* outputPtr = nullptr;

// Can't get function pointer with capture from lambda, so have
// to use static function
static void serialListener(uint8_t b) {
  *outputPtr += b;
}

enum Result {
  UNINITIALIZED,
  ONGOING,
  PASSED,
  FAILED
};

std::function<Result()> getValidator(RomType romType, const std::string& testOutput) {
  const auto validate = [](const std::string& output, const char* passCondition, const char* failCondition) {
    return output.find(passCondition) != std::string::npos ? PASSED :
      output.find(failCondition) != std::string::npos ? FAILED : ONGOING;
  };

  switch (romType) {
  case BLARGG:
    return [&]() {
      return validate(testOutput, "Passed", "Failed");
    };
  case GEKKIO:
    return [&]() {
      // Magic numbers as can be found in "mooneye-gb/tests/common/lib/quit.s"
      return validate(testOutput, "\x03\x05\x08\x0d\x15\x22", "\x42\x42\x42\x42\x42\x42");
    };
  default:
    return nullptr;
  }
}

void runTest(RomType romType, const char* romPath) {
  chester chester;
  std::string serialOutput;
  outputPtr = &serialOutput;

  register_keys_callback(&chester, [](keys*) { return 0; });
  register_get_ticks_callback(&chester, []() { return static_cast<uint32_t>(0); });
  register_delay_callback(&chester, [](uint32_t) {});
  register_gpu_init_callback(&chester, [](gpu*) { return true; });
  register_gpu_uninit_callback(&chester, [](gpu*) {});
  register_gpu_render_callback(&chester, [](gpu*) {});
  register_gpu_alloc_image_buffer_callback(&chester, NULL);
  register_serial_callback(&chester, serialListener);

  const std::string romRoot(romType == BLARGG ?
    BLARGGS_ROMS_DIR : GEKKIOS_ROMS_DIR);
  ASSERT_TRUE(init(&chester, (romRoot + '/' + romPath).c_str(), NULL, NULL));

  const auto validator = getValidator(romType, serialOutput);
  ASSERT_TRUE(validator);

  int i = 4 * 60;
  Result result = UNINITIALIZED;
  do {
    run(&chester);
    result = validator();
  } while(--i && result == ONGOING);

  EXPECT_EQ(PASSED, result);

  uninit(&chester);
}

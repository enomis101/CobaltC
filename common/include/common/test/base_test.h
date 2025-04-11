#include "common/build_options.h"

#ifdef ENABLE_TESTING
#    include "common/decoder/data_decoder.h"
#    include "common/log/log.h"
#    include "common/test_config.h"
#    include <gtest/gtest.h>

class BaseTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        logging::LogManager::init();
    }
};
#endif

#include "common/build_options.h"

#ifdef ENABLE_TESTING
#    include "common/log/log.h"
#    include <gtest/gtest.h>

class BaseTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        logging::LogManager::init();
    }
};
#endif

#include "Helpers.hpp"
#include <pocolog_cpp/LogFile.hpp>

using namespace pocolog_cpp;
using namespace std;

struct LogFileTest : public helpers::Test {
};

TEST_F(LogFileTest, it_returns_the_description_of_the_streams) {
    auto& logfile = openFixtureLogfile("plain.0.log");

    auto descriptions = logfile.getStreamDescriptions();
    ASSERT_EQ(2, descriptions.size());
    ASSERT_EQ("a", descriptions[0].getName());
    ASSERT_EQ("/int32_t", descriptions[0].getTypeName());
    ASSERT_EQ("b", descriptions[1].getName());
    ASSERT_EQ("/float", descriptions[1].getTypeName());
}

TEST_F(LogFileTest, it_reads_samples_sequentially) {
    auto& logfile = openFixtureLogfile("plain.0.log");

    auto descriptions = logfile.getStreamDescriptions();

    {
        auto [index, time, value] = logfile.readNextSample().value();
        ASSERT_EQ(0, index);
        ASSERT_EQ(10, value.get<int32_t>());
    }

    {
        auto [index, time, value] = logfile.readNextSample().value();
        ASSERT_EQ(0, index);
        ASSERT_EQ(20, value.get<int32_t>());
    }

    {
        auto [index, time, value] = logfile.readNextSample().value();
        ASSERT_EQ(0, index);
        ASSERT_EQ(30, value.get<int32_t>());
    }

    {
        auto [index, time, value] = logfile.readNextSample().value();
        ASSERT_EQ(1, index);
        ASSERT_FLOAT_EQ(0.1, value.get<float>());
    }

    {
        auto [index, time, value] = logfile.readNextSample().value();
        ASSERT_EQ(1, index);
        ASSERT_FLOAT_EQ(0.2, value.get<float>());
    }

    {
        auto [index, time, value] = logfile.readNextSample().value();
        ASSERT_EQ(1, index);
        ASSERT_FLOAT_EQ(0.3, value.get<float>());
    }

    ASSERT_FALSE(logfile.readNextSample().has_value());
    ASSERT_FALSE(logfile.readNextSample().has_value());
}

TEST_F(LogFileTest, it_rewinds) {
    auto& logfile = openFixtureLogfile("plain.0.log");
    auto descriptions = logfile.getStreamDescriptions();

    {
        auto [index, time, value] = logfile.readNextSample().value();
        ASSERT_EQ(0, index);
        ASSERT_EQ(10, value.get<int32_t>());
    }

    logfile.rewind();

    {
        auto [index, time, value] = logfile.readNextSample().value();
        ASSERT_EQ(0, index);
        ASSERT_EQ(10, value.get<int32_t>());
    }
}
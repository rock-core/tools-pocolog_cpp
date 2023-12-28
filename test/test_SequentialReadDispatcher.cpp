#include <base/Eigen.hpp>
#include <filesystem>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <pocolog_cpp/SequentialReadDispatcher.hpp>

using namespace pocolog_cpp;
using namespace std;
using namespace testing;

struct LogFileIndexCleaner {
    LogFile& logFile;
    LogFileIndexCleaner(LogFile& logFile)
        : logFile(logFile)
    {
    }
    ~LogFileIndexCleaner()
    {
        logFile.removeAllIndexes();
    }
};

struct SequentialReadDispatcherTest : public ::testing::Test {};

TEST_F(SequentialReadDispatcherTest, it_dispatches_samples_to_the_relevant_callbacks)
{
    auto fixture = filesystem::path(__FILE__).parent_path() / "fixtures" / "plain.0.log";
    LogFile logfile(fixture.string());
    LogFileIndexCleaner delete_index(logfile);
    SequentialReadDispatcher dispatcher(logfile);

    dispatcher.importTypesFrom("std");
    std::vector<int32_t> a_values;
    std::vector<float> b_values;
    dispatcher.add<int32_t>("a", [&a_values](auto value) { a_values.push_back(value); });
    dispatcher.add<float>("b", [&b_values](auto value) { b_values.push_back(value); });
    dispatcher.run();

    EXPECT_THAT(a_values, ElementsAre(10, 20, 30));
}

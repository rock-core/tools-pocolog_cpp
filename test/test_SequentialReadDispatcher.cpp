#include "Helpers.hpp"
#include <gmock/gmock.h>

#include <base/Eigen.hpp>
#include <pocolog_cpp/SequentialReadDispatcher.hpp>

using namespace pocolog_cpp;
using namespace std;
using namespace testing;

struct SequentialReadDispatcherTest : public helpers::Test {};

TEST_F(SequentialReadDispatcherTest, it_dispatches_samples_to_the_relevant_callbacks)
{
    auto& logfile = openFixtureLogfile("plain.0.log");
    SequentialReadDispatcher dispatcher(logfile);

    dispatcher.importTypesFrom("std");
    std::vector<int32_t> a_values;
    std::vector<float> b_values;
    dispatcher.add<int32_t>("a", [&a_values](auto value) { a_values.push_back(value); });
    dispatcher.add<float>("b", [&b_values](auto value) { b_values.push_back(value); });
    dispatcher.run();

    EXPECT_THAT(a_values, ElementsAre(10, 20, 30));
}

TEST_F(SequentialReadDispatcherTest, it_handles_types_that_require_initialization)
{
    auto& logfile = openFixtureLogfile("vector.0.log");
    SequentialReadDispatcher dispatcher(logfile);

    dispatcher.importTypesFrom("std");
    std::vector<std::vector<double>> a_values;
    dispatcher.add<std::vector<double>>("vector",
        [&a_values](auto value) { a_values.push_back(value); });
    dispatcher.run();

    EXPECT_THAT(a_values,
        ElementsAre(std::vector{0.0, 1.0, 2.0, 3.0},
            std::vector{4.0, 5.0, 6.0, 7.0},
            std::vector{0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0}));
}

TEST_F(SequentialReadDispatcherTest, it_handles_opaques)
{
    auto& logfile = openFixtureLogfile("opaques.0.log");
    SequentialReadDispatcher dispatcher(logfile);

    dispatcher.importTypesFrom("base");
    std::vector<base::Vector3d> a_values;
    dispatcher.add<Eigen::Vector3d>("a",
        [&a_values](auto value) { a_values.push_back(value); });
    dispatcher.run();

    EXPECT_THAT(a_values,
        ElementsAre(Eigen::Vector3d(1, 2, 3), Eigen::Vector3d(4, 5, 6)));
}

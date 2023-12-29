#include "Helpers.hpp"
#include <gmock/gmock.h>

#include <pocolog_cpp/StreamDescription.hpp>

using namespace pocolog_cpp;
using namespace testing;

struct StreamDescriptionTest : public helpers::Test {
};

TEST_F(StreamDescriptionTest, it_parses_the_stream_metadata) {
    auto& logfile = openFixtureLogfile("metadata.0.log");
    auto descriptions = logfile.getStreamDescriptions();
    auto actual = descriptions[0].getMetadataMap();
    EXPECT_THAT(actual, ElementsAre(Pair("rock_cxx_type_name", "/base/Vector3d")));
}
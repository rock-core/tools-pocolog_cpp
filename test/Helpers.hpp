#ifndef POCOLOG_CPP_TEST_HELPERS_HPP
#define POCOLOG_CPP_TEST_HELPERS_HPP

#include <gtest/gtest.h>
#include <filesystem>

#include <pocolog_cpp/LogFile.hpp>

namespace pocolog_cpp {
    namespace helpers {
        inline std::filesystem::path fixturePath(
            std::string const& fixture_name
        ) {
            return std::filesystem::path(__FILE__).parent_path() / "fixtures" / fixture_name;
        }

        class Test : public ::testing::Test {
            std::vector<LogFile*> logfiles;
        public:
            ~Test() {
                for (auto l : logfiles) {
                    l->removeAllIndexes();
                    delete l;
                }
            }

            /** Open a logfile in test/fixtures/ and delete the built index on teardown */
            LogFile& openFixtureLogfile(std::string const& fixtureName) {
                auto path = fixturePath(fixtureName);
                LogFile* logfile = new LogFile(path.string());
                logfiles.push_back(logfile);
                return *logfile;
            }

        };
    }
}

#endif
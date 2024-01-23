#include <iostream>
#include <fstream>
#include "pocolog_cpp/LogFile.hpp"
#include "pocolog_cpp/InputDataStream.hpp"
#include "laser_readings.h"

using namespace std;
using namespace pocolog_cpp;

int main(int argc, char** argv)
{
    std::string file(argv[1]);
    LogFile logfile(file);
    const std::vector<Stream*>& streams = logfile.getStreams();
    cerr << argv[1] << " has " << streams.size() << " streams" << endl;
    for (size_t i = 0; i < streams.size(); ++i)
    {
        InputDataStream& stream = *dynamic_cast<InputDataStream*>(streams[i]);
        cerr << "  stream " << i << " is " << stream.getName() << "[" << stream.getStreamType() << "]" << endl;

        for (size_t t = 0; t < stream.getSize(); t++)
        {
            DFKI::LaserReadings readings;
            stream.getSample<DFKI::LaserReadings>(readings, t);
            cout << readings.stamp << endl;
        }
    }
}


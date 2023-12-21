#include "LogFile.hpp"
#include "Index.hpp"
#include "IndexFile.hpp"
#include "InputDataStream.hpp"
#include <iostream>
#include <boost/foreach.hpp>
#include <base/samples/Joints.hpp>

using namespace pocolog_cpp;

int main(int argc, char **argv)
{
    if(argc != 2)
    {
        std::cout << "Usage indexer <Logfile>" << std::endl;
        exit(0);
    }
    std::string file(argv[1]);

    try
    {
        pocolog_cpp::LogFile logfile(file);



        Stream *stream = &(logfile.getStream("/simple_controller.command"));
        InputDataStream *dataStream = dynamic_cast<InputDataStream *>(stream);

        std::cout << "Stream size is " << dataStream->getSize() << std::endl;

        for(size_t i = 0; i < dataStream->getSize(); i++)
        {
            base::samples::Joints joints;
            if(!dataStream->getSample<base::samples::Joints>(joints, i))
            {
                std::cout << "Error could not load sample  " << i << std::endl;
                return 0;
            }

//             std::cout << "Found Names :" << std::endl;
//
//             BOOST_FOREACH( std::string name, joints.names )
//             {
//                 std::cout << name<< std::endl;;
//             }

        }

    }
    catch (std::runtime_error& e)
    {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}
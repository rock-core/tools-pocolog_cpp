#include "LogFile.hpp"
#include "Index.hpp"
#include "IndexFile.hpp"
#include "InputDataStream.hpp"
#include <iostream>
#include <boost/foreach.hpp>
#include <typelib/csvoutput.hh>
#include <sstream>
#include <boost/program_options.hpp>


using namespace pocolog_cpp;

/*!
 * \brief fast_forward_to
 * \param dataStream
 * \param time
 * \return return numSamples if given time if no sample older than time is in stream
 */
size_t fast_forward_to(InputDataStream *dataStream, const base::Time& time)
{
    size_t idx = 0;
    while(true)
    {
        if(idx >= dataStream->getFileIndex().getNumSamples()){
            return dataStream->getFileIndex().getNumSamples();
        }

        if(dataStream->getFileIndex().getSampleTime(idx) > time){
            return idx;
        }
        idx++;
    }
}

void extract(InputDataStream *stream,
             const base::Time& start_time, const base::Time& stop_time)
{
    std::string sep = ",";
    std::string sdelim = "\"";


    // Fast forward to start time
    size_t idx = 0;
    if(!start_time.isNull()){
        idx = fast_forward_to(stream, start_time);
    }

    size_t stop_idx = stream->getSize();
    if(!stop_time.isNull()){
        stop_idx = fast_forward_to(stream, stop_time);
    }

    // Write CSV header
    std::cout << Typelib::csv_header(*stream->getType(), stream->getName(), sep, sdelim) << std::endl;

    // Initialize buffer
    std::vector<uint8_t> buffer;
    buffer.resize(stream->getTypeMemorySize());

    // Iterate samples
    size_t start_idx = idx;
    std::clog << "Iterating samples\n" << "    start index: " << start_idx
              << " ("<<stream->getFileIndex().getSampleTime(start_idx).toString(base::Time::Resolution::Seconds)
              <<")\n    stop index: " <<stop_idx
              << " ("<<stream->getFileIndex().getSampleTime(stop_idx-1).toString(base::Time::Resolution::Seconds) << std::endl;

    while(idx < stop_idx)
    {
        Typelib::Value v = stream->getTyplibValue(buffer.data(), stream->getTypeMemorySize(), idx);
        std::cout << Typelib::csv(*stream->getType(), buffer.data(), sep, false, sdelim) << "\n";
        idx++;
    }

    std::cout << std::endl;
    std::clog << "Finished at sample index " << idx <<". Processed " << idx -start_idx<<std::endl;
}

struct Args{
    base::Time start_time;
    base::Time stop_time;
    std::string filepath;
    std::string stream_name;
};


void usage(boost::program_options::options_description& desc){
    std::cout << R"(Extract values from Pocolog files as CSV

USAGE:
     pocolog-extract LOGFILE
)"<< std::endl;
    std::cout << desc << std::endl;
}

Args parse_args(int argc, char** argv)
{
    Args ret;
    std::string start_time_str;
    std::string stop_time_str;
    std::string time_format =  base::Time::DEFAULT_FORMAT;

    namespace po = boost::program_options;
    // Declare the supported options.
    po::options_description desc("Allowed option");
    desc.add_options()
        ("help,h",          "Produce help message")

        ("from",          po::value<std::string>(&start_time_str),     "Start time for extraction. To be given in the format specified by 'time_format'.")
        ("to",            po::value<std::string>(&stop_time_str),      "End time for extraction. To be given in the format specified by 'time_format'.")
        ("time_format,f", po::value<std::string>(&time_format),        "Format string that explains hwo timestamps are passed (from/to). Default: '%Y%m%d-%H:%M:%S'")

        ("streamname,s",    po::value<std::string>(&(ret.stream_name)),    "Save list of unresoved transform to this file")
        ;
    //These arguments are positional and thuis should not be show to the user
    po::options_description hidden("Hidden");
    hidden.add_options()
        ("logfile", po::value<std::string>(&(ret.filepath)), "Input log file")
    ;

    //Collection of all arguments
    po::options_description all("");
    all.add(desc).add(hidden);

    //Collection of only the visible arguments
    po::options_description visible("");
    visible.add(desc);

    //Define the positional arguments (they refer to the hidden arguments)
    po::positional_options_description p;
    p.add("logfile", 1);

    //Parse all arguments, but ....
    po::variables_map vm;
    try{
        po::store(po::command_line_parser(argc, argv).options(all).positional(p).run(), vm);
    }catch(boost::program_options::error_with_option_name& ex){
        std::cerr << std::string("Error parsing command line arguments: ")+ex.what()+"\n";
        exit(EXIT_FAILURE);
    }
    po::notify(vm);

    if (vm.count("help"))
    {
        //... but display only the visible arguments
        usage(visible);
        exit(EXIT_FAILURE);
    }

    if (vm.count("from"))
    {
        ret.start_time = ret.start_time.fromString(start_time_str, base::Time::Resolution::Seconds, time_format);
    }

    if (vm.count("to"))
    {
        ret.stop_time = ret.stop_time.fromString(stop_time_str, base::Time::Resolution::Seconds, time_format);
    }

    if (ret.filepath == "")
    {
        std::cerr << "No input logfile was given" << std::endl;
        //... but display only the visible arguments
        usage(visible);
        exit(EXIT_FAILURE);
    }

    po::notify(vm);

    return ret;
}

int main(int argc, char **argv)
{
    Args args = parse_args(argc, argv);



    // Open log file
    std::clog << "Reading Log file... " << std::endl;
    pocolog_cpp::LogFile logfile(args.filepath);
    std::clog << "Log file read" << std::endl;


    // Initialize stream
    Stream *stream_base = &(logfile.getStream(args.stream_name));
    InputDataStream *stream = dynamic_cast<InputDataStream *>(stream_base);
    if(!stream){
        throw std::runtime_error("Could not cast stream");
    }

    // Extract
    extract(stream, args.start_time, args.stop_time);

    return 0;
}

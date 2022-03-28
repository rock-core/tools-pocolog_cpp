#include "LogFile.hpp"
#include "Index.hpp"
#include "IndexFile.hpp"
#include "InputDataStream.hpp"
#include <iostream>
#include <boost/foreach.hpp>
#include <typelib/csvoutput.hh>
#include <sstream>
#include <boost/program_options.hpp>
#include <yaml-cpp/yaml.h>


#define FMT_BOLD "\033[1m"
#define FMT_UNDERLINE "\033[4m"
#define FMT_END "\033[0m"


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
        stream->getTyplibValue(buffer.data(), stream->getTypeMemorySize(), idx);
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
    std::string mode = "extract";
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

        ("streamname,s",  po::value<std::string>(&(ret.stream_name)),  "Save list of unresoved transform to this file")

        ("info,i",         "Extract information about streams in YAMl format")
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

    if (vm.count("info"))
    {
        ret.mode = "info";
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


struct StreamSummary{
    base::Time firstSampleTime;
    base::Time lastSampleTime;
    std::string dataTypeName;
    std::string name;
    size_t nSamples;
};


struct FileSummary{
    std::string fileName;
    std::vector<StreamSummary> streams;
    base::Time firstSampleTime;
    base::Time lastSampleTime;
};

StreamSummary extract_summary(const Stream& stream)
{
    StreamSummary ret;
    ret.name = stream.getName();
    ret.firstSampleTime = stream.getFistSampleTime();
    ret.lastSampleTime = stream.getLastSampleTime();
    ret.nSamples = stream.getSize();
    ret.dataTypeName = stream.getDescription().getTypeName();
    return ret;
}

void pretty_print(StreamSummary const &m){
    std::cout << FMT_BOLD << m.name << FMT_END << std::endl;
    std::cout << "  " << m.nSamples << " samples from "
              <<  m.firstSampleTime.toString() << " to " << m.lastSampleTime.toString()
              << " [" <<m.lastSampleTime - m.firstSampleTime << "]" << std::endl;
}

FileSummary extract_summary(pocolog_cpp::LogFile& logfile)
{
    FileSummary ret;

    ret.fileName = logfile.getFileName();

    for(const Stream* stream : logfile.getStreams())
    {
        ret.streams.push_back(extract_summary(*stream));
    }

    // Extract earlied and latest sample of all streams
    if(ret.streams.size() > 0) {
        ret.firstSampleTime = base::Time::fromMicroseconds(INT64_MAX);
        ret.lastSampleTime = base::Time::fromMicroseconds(0);

        for(StreamSummary& summary : ret.streams){
            if(summary.nSamples > 0){
                if(summary.firstSampleTime < ret.firstSampleTime){
                    ret.firstSampleTime = summary.firstSampleTime;
                }
                if(summary.lastSampleTime > ret.lastSampleTime){
                    ret.lastSampleTime = summary.lastSampleTime;
                }
            }
        }
    }

    return ret;
}

void pretty_print(FileSummary const &m)
{
    std::cout << std::endl;
    std::cout << "Logfile " << FMT_BOLD << FMT_UNDERLINE << m.fileName << FMT_END << " has the following streams: " << std::endl;
    for(const StreamSummary& ssummary : m.streams)
    {
        pretty_print(ssummary);
    }
}

std::string to_yaml(FileSummary const &m)
{
    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << "file_name";
    out << YAML::Value << m.fileName;
    out << YAML::Key << "first_sample_time";
    out << YAML::Value << m.firstSampleTime.toString();
    out << YAML::Key << "last_sample_time";
    out << YAML::Value << m.lastSampleTime.toString();
    out << YAML::Key << "streams";
    out << YAML::Value << YAML::BeginSeq;

    for(const StreamSummary& summary : m.streams){
        out << YAML::BeginMap;
        out << YAML::Key << "name" << YAML::Value << summary.name;
        out << YAML::Key << "n_samples" << YAML::Value << summary.nSamples;
        if(summary.nSamples){
            out << YAML::Key << "first_sample_time" << YAML::Value << summary.firstSampleTime.toString();
            out << YAML::Key << "last_sample_time" << YAML::Value << summary.lastSampleTime.toString();
        }
        out << YAML::Key << "data_type_name" << YAML::Value << summary.dataTypeName;
        out << YAML::EndMap;
    }
    out << YAML::EndSeq;
    return out.c_str();
}


int do_main(Args& args){
    // Open log file
    std::clog << "Reading logfile '" << args.filepath << "'..." << std::flush;
    pocolog_cpp::LogFile logfile(args.filepath);
    std::clog << " OK" << std::endl;

    if(args.mode == "info")
    {
        FileSummary summary = extract_summary(logfile);
        std::cout << to_yaml(summary) <<std::endl;
        exit(EXIT_SUCCESS);
    }

    if(args.stream_name.empty())
    {
        std::clog << "\nNo Stream name was specified!" <<std::endl;
        FileSummary summary = extract_summary(logfile);
        pretty_print(summary);
        exit(EXIT_FAILURE);
    }

    // Initialize stream
    try{
        Stream *stream_base = &(logfile.getStream(args.stream_name));
        InputDataStream *stream = dynamic_cast<InputDataStream *>(stream_base);
        if(!stream){
            throw std::runtime_error("Could not cast stream");
        }

        // Extract
        extract(stream, args.start_time, args.stop_time);
    }
    catch(std::runtime_error& err)
    {
        std::cerr << "Error: " << err.what();
        exit(EXIT_FAILURE);
    }

    return EXIT_SUCCESS;
}


int main(int argc, char **argv)
{
    Args args;
    try{
        args = parse_args(argc, argv);
    }
    catch(std::exception& err){
        std::cout << "Error parsing arguments: " << err.what() << std::endl;
        return EXIT_FAILURE;
    }

    try{
        return do_main(args);
    }catch(std::exception& err){
        std::cerr << std::endl;
        std::cerr << "Error: " << err.what() << std::endl;
        return EXIT_FAILURE;
    }
}

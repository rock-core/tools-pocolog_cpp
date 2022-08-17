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
#include <iostream>
#include <sstream>
#include <iomanip>

#include "named_vector_helpers.hpp"


#define FMT_BOLD "\033[1m"
#define FMT_UNDERLINE "\033[4m"
#define FMT_END "\033[0m"


using namespace pocolog_cpp;

struct Args{
    base::Time start_time;
    base::Time stop_time;
    size_t start_idx = 0;
    size_t stop_idx = 0;
    std::string filepath;
    std::string stream_name;
    std::vector<std::string> fields;
    std::string mode = "extract";
    bool write_header = true;
    std::string output_folder="";
    std::string out_file_suffix=".dat";
    bool add_idx = false;
    bool add_time = true;
    std::string sep = ",";
    std::string sdelim = "\"";
    std::string timestamp_field = "";
    std::string info_format = "pretty"; // yaml, csv, pretty
};


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

bool guess_field_with_timestamps(InputDataStream *stream, std::string& field_name)
{
    // Initialize buffer
    std::vector<uint8_t> buffer;
    buffer.resize(stream->getTypeMemorySize());

    Typelib::Value v(buffer.data(), *stream->getType());
    Typelib::Type::Category category = v.getType().getCategory();

    if(category == Typelib::Type::Category::Compound)
    {
        const Typelib::Compound* compound = dynamic_cast<const Typelib::Compound*>(&v.getType());
        std::vector<const Typelib::Field*> candidates;
        for(const Typelib::Field& field: compound->getFields()){
            if(field.getType().getName() == "/base/Time"){
                candidates.push_back(&field);
            }
        }

        if(candidates.size() > 1){
            std::clog << "Multiple field have been identified that could possibly contain a sample time: " << std::endl;
            for(size_t idx=0; idx < candidates.size(); idx++)
            {
                std::cout << candidates[idx]->getName() << " ";
            }
            return false;
        }

        if(candidates.size() < 1){
            std::clog << "No field has been found that contains a sample time" << std::endl;
            return false;
        }

        field_name = candidates[0]->getName();
        std::clog << "Using field '" << field_name << "' as timestamps" << std::endl;
        return true;
    }
    else{
        std::clog << "Not a compound type" << std::endl;
        return false;
    }
}


void write_header(InputDataStream *stream,
                  Args& args)
{
    // Initialize buffer
    std::vector<uint8_t> buffer;
    buffer.resize(stream->getTypeMemorySize());

    Typelib::Value v(buffer.data(), *stream->getType());
    Typelib::Type::Category category = v.getType().getCategory();

    // Retrieve first sample so that there is data in dynamically sized strauctures (needed for named vector treatment)
    stream->getTyplibValue(buffer.data(), stream->getTypeMemorySize(), 0);

    // Determine the element order if data type is a named vector
    bool _is_named_vector = is_named_vector(v);
    std::vector<std::string> named_vector_order;
    if(_is_named_vector){
        named_vector_order = extract_names(stream);
    }

    // Write index and log time column headers
    if(args.add_idx){
        std::cout << "log_idx" << args.sep;
    }
    if(args.add_time){
        std::cout << "log_time" << args.sep;
    }

    //
    // Write type field headers
    //
    // For non-compound types we use the standard csv function
    if(category != Typelib::Type::Category::Compound){
        std::cout << Typelib::csv_header(*stream->getType(), stream->getName(), args.sep, args.sdelim) << std::endl;
    }
    // For compound types we call the csv header extraction for each field individually so that we can account for
    //    a. extracting only a subset of all fields (field names where explicitly given)
    //    b. special treatment of named vector
    else{
        size_t i_fields = 0;
        for(const std::string& field_name : args.fields){
            Typelib::Value v(buffer.data(), *stream->getType());
            Typelib::Value fv = Typelib::FieldGetter().apply(v, field_name);

            // Apply special treatment of named vector elements: Prefix each subfield with the name of
            // the element according to the fixed element order
            if(_is_named_vector && field_name == "elements")
            {
                const Typelib::Container* fc = dynamic_cast<const Typelib::Container*>(&fv.getType());
                size_t n_elems = fc->getElementCount(fv.getData());
                size_t i_elems = 0;
                for( size_t name_idx=0; name_idx<n_elems; name_idx++ )
                {
                    Typelib::Value fcv = fc->getElement(fv.getData(), name_idx);
                    std::cout << Typelib::csv_header(fcv.getType(),
                                                     named_vector_order[name_idx],
                                                     args.sep,
                                                     args.sdelim);
                    if(i_elems < n_elems-1){
                        std::cout << args.sep;
                    }
                    i_elems++;
                }
            }
            else{
                std::cout << Typelib::csv_header(fv.getType(), field_name, args.sep, args.sdelim);
            }

            if(i_fields < args.fields.size()-1){
                std::cout << args.sep;
            }
            i_fields++;
        }
    }

    std::cout << std::endl;
}


void extract(InputDataStream *stream,
             Args& args)
{



    // Fast forward to start time or index
    size_t idx = 0;
    size_t stop_idx = 0;
    if( !args.start_time.isNull() ){
        idx = fast_forward_to(stream, args.start_time);
    }else{
        idx = args.start_idx;
    }

    if( !args.stop_time.isNull() ){
        stop_idx = fast_forward_to(stream, args.stop_time);
    }
    else if( args.stop_idx == 0 ){
        stop_idx = stream->getSize();
    }else{
        stop_idx = args.stop_idx;
    }


    // Initialize buffer
    std::vector<uint8_t> buffer;
    buffer.resize(stream->getTypeMemorySize());


    std::clog << "Extracting "<<args.stream_name << " from " << args.filepath << "\n"
              << "    start index: " << idx
              << " (" << stream->getFileIndex().getSampleTime(idx).toString(base::Time::Resolution::Seconds)
              <<")\n    stop index: " << stop_idx
              << " (" << stream->getFileIndex().getSampleTime(stop_idx-1).toString(base::Time::Resolution::Seconds)
              << ")" << std::endl;

    Typelib::Value v(buffer.data(), *stream->getType());
    Typelib::Type::Category category = v.getType().getCategory();

    // Determine a fixed order of names for named vector types
    bool _is_named_vector = is_named_vector(stream, (char*)buffer.data());
    std::map<std::string, size_t> named_vector_sorting_map;
    if( _is_named_vector )
    {
        std::vector <std::string> order = extract_names(stream);
        for(size_t idx = 0; idx < order.size(); idx++)
        {
            named_vector_sorting_map[order[idx]] = idx;
        }
    }


    // Set field names if empty
    if(category == Typelib::Type::Category::Compound && args.fields.empty()){
        std::list<Typelib::Field> fields = dynamic_cast<const Typelib::Compound*>(&v.getType())->getFields();
        for(const Typelib::Field f : fields){
            // Don't extract names field of named vector
            if( _is_named_vector && f.getName() == "names"){
                continue;
            }

            args.fields.push_back(f.getName());
        }
    }

    // Write CSV header
    if(args.write_header){
        write_header(stream, args);
    }

    // Iterate samples
    size_t start_idx = idx;
    while(idx < stop_idx)
    {
        stream->getTyplibValue(buffer.data(), stream->getTypeMemorySize(), idx);

        // Write index
        if(args.add_idx){
            std::cout << idx << args.sep;
        }

        // Write log time column
        if(args.add_time){
            std::cout << stream->getFileIndex().getSampleTime(idx).microseconds << args.sep;
        }

        // For non-compound types we use the standard csv function
        if(category != Typelib::Type::Category::Compound){
            std::cout << Typelib::csv(*stream->getType(), buffer.data(), args.sep, false, args.sdelim) << "\n";
        }
        // For compound types we call the csv extraction for each field individually so that we can account for
        //    a. extracting only a subset of all fields (field names where explicitly given)
        //    b. special treatment of named vector
        else{
            size_t i_fields = 0;
            Typelib::Value v(buffer.data(), *stream->getType());
            for(const std::string& field_name : args.fields){
                Typelib::Value fv = Typelib::FieldGetter().apply(v, field_name);

                if(args.output_folder.empty()){
                    //TO CSV
                    if(_is_named_vector && field_name == "names")
                    {
                        continue;
                    }
                    if(_is_named_vector && field_name == "elements")
                    {
                        Typelib::Value names_v = Typelib::FieldGetter().apply(v, "names");
                        std::vector<Typelib::Value> reordered = sort_named_vector_values(names_v, fv, named_vector_sorting_map);
                        size_t i_elems=0;
                        for(Typelib::Value& elv : reordered){
                            std::cout << Typelib::csv(elv.getType(), elv.getData(), args.sep, false, args.sdelim);
                            if(i_elems < reordered.size()-1){
                                std::cout << args.sep;
                            }
                            i_elems++;
                        }
                    }
                    else{
                        std::cout << Typelib::csv(fv.getType(), fv.getData(), args.sep, false, args.sdelim);
                    }
                    if(i_fields < args.fields.size()-1){
                         std::cout << args.sep;
                    }else{
                        std::cout << "\n";
                    }
                }else{
                    //TO FILES

                    // Filename will be the index in log file
                    std::stringstream ss;
                    ss << std::setfill('0') << std::setw(8) << idx;
                    std::string filepath = args.output_folder+"/"+ss.str()+"-"+field_name+args.out_file_suffix;

                    std::ofstream ostream(filepath, std::fstream::binary);
                    if(!ostream.is_open()){
                        throw(std::runtime_error("Can't create output file " + filepath));
                    }

                    const Typelib::Type& g = fv.getType();
                    const Typelib::Container* arr = dynamic_cast<const Typelib::Container*>(&g);
                    if(!arr){
                        throw(std::runtime_error("writing to files is currently only implemented for Container Typed fields"));
                    }
                    size_t n_elems = arr->getElementCount(fv.getData());
                    for(size_t i = 0; i < n_elems; i++)
                    {
                        Typelib::Value elem = arr->getElement(fv.getData(), i);
                        ostream.write((char*)elem.getData(), elem.getType().getSize());
                    }
                    ostream.close();
                }

                i_fields++;
            }
            Typelib::destroy(v);
        }

        idx++;
    }

    std::cout << std::endl;
    std::clog << "Finished at sample index " << idx <<". Processed " << idx -start_idx<<std::endl;
}


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

        ("from",          po::value<std::string>(&start_time_str),
         "Start time for extraction. To be given in the format specified by 'time_format' or as int index.")

        ("to",            po::value<std::string>(&stop_time_str),
         "End time for extraction. To be given in the format specified by 'time_format' or as int index.")

        ("time_format,tf", po::value<std::string>(&time_format),
         "Format string that explains hwo timestamps are passed (from/to). Default: '%Y%m%d-%H:%M:%S'")

        ("streamname,s",  po::value<std::string>(&(ret.stream_name)),
         "Name of the stream to extract")

        ("fields,f",      po::value<std::vector<std::string>>(&(ret.fields))->multitoken(),
         "Blank separated list of file names to extract. If not given, all filed of the stream are extracted.")

        ("out_folder,of",      po::value<std::string>(&(ret.output_folder)),
             "Write individual files for each sample/file instead of CSV to the output specified by this argument folder.")
        ("out_file_suffix,ofs",      po::value<std::string>(&(ret.out_file_suffix)),
             "Suffix of files writte to 'out_folder'. Default is .dat")

        ("no_header",
             "Don't write CSV header when extracting data")

        ("no_time",
                 "Don't write column with timestamp from stream")

        ("write_index",
                 "Write column with index from stream")

        ("info,i",
         "Print information about streams")
        ("infofmt",      po::value<std::string>(&(ret.info_format)),
             "Format to print stream/file information. Possible values are 'pretty' (default), 'yaml'")
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
        try{
            ret.start_time = ret.start_time.fromString(start_time_str, base::Time::Resolution::Seconds, time_format);
        }catch(std::exception& ex){
            try{
                ret.start_idx = std::stol(start_time_str);
            }catch(std::exception& err){
                std::cerr << "The value given for --from does not seem to be a vald time stamp in the format " << time_format << ", and not a int index" << std::endl;
                exit(EXIT_FAILURE);
            }
        }
    }

    if (vm.count("to"))
    {
        try{
            ret.stop_time = ret.start_time.fromString(stop_time_str, base::Time::Resolution::Seconds, time_format);
        }catch(std::exception& ex){
            try{
                ret.stop_idx = std::stol(stop_time_str);
            }catch(std::exception& err){
                std::cerr << "The value given for --to does not seem to be a vald time stamp in the format " << time_format << ", and not a int index" << std::endl;
                exit(EXIT_FAILURE);
            }
        }
    }

    if (vm.count("info"))
    {
        ret.mode = "info";
    }

    if (vm.count("no_header"))
    {
        ret.write_header = false;
    }

    if (vm.count("no_time"))
    {
        ret.add_time = false;
    }

    if (vm.count("write_index"))
    {
        ret.add_idx = true;
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

struct FieldSummary{
    std::string fieldName;
    std::string typeName;
};


struct StreamSummary{
    base::Time firstSampleTime;
    base::Time lastSampleTime;
    std::string dataTypeName;
    std::vector<FieldSummary> fields;
    std::string name;
    size_t nSamples;
};


struct FileSummary{
    std::string fileName;
    std::vector<StreamSummary> streams;
    base::Time firstSampleTime;
    base::Time lastSampleTime;
};

std::vector<FieldSummary> extract_fields(const Stream& stream){
    std::vector<FieldSummary> ret;

    const Stream *stream_base = &(stream);
    const InputDataStream *istream = dynamic_cast<const InputDataStream *>(stream_base);
    if( istream->getType()->getCategory() == Typelib::Type::Category::Compound ){
        const Typelib::Compound* compound = dynamic_cast<const Typelib::Compound*>(istream->getType());
        if(compound){
            for(const Typelib::Field& field : compound->getFields()){
                FieldSummary summary;
                summary.fieldName = field.getName();
                summary.typeName = field.getType().getName();
                ret.push_back(summary);
            }
        }
    }

    return ret;
}

StreamSummary extract_summary(const Stream& stream)
{
    StreamSummary ret;
    ret.name = stream.getName();
    ret.firstSampleTime = stream.getFistSampleTime();
    ret.lastSampleTime = stream.getLastSampleTime();
    ret.nSamples = stream.getSize();
    ret.dataTypeName = stream.getDescription().getTypeName();
    ret.fields = extract_fields(stream);

    return ret;
}

void pretty_print(StreamSummary const &m){
    std::cout << FMT_BOLD << m.name << FMT_END << std::endl;
    std::cout << "  " << m.nSamples << " samples from "
              <<  m.firstSampleTime.toString() << " to " << m.lastSampleTime.toString()
              << " [" <<m.lastSampleTime - m.firstSampleTime << "]" << std::endl;
}


FileSummary extract_summary(pocolog_cpp::LogFile& logfile, const std::string& stream_name="")
{
    FileSummary ret;

    ret.fileName = logfile.getFileName();



    if(stream_name.empty()){
        for(const Stream* stream : logfile.getStreams())
        {
            ret.streams.push_back(extract_summary(*stream));
        }
    }
    else{
        ret.streams.push_back(extract_summary(logfile.getStream(stream_name)));
    }

    if(ret.streams.size() > 0) {
        // Extract earlied and latest sample of all streams
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

            // Extract fields for all stream types
            Stream *stream_base = &(logfile.getStream(summary.name));
            InputDataStream *istream = dynamic_cast<InputDataStream *>(stream_base);
            if( istream->getType()->getCategory() == Typelib::Type::Category::Compound ){
                const Typelib::Compound* compound = dynamic_cast<const Typelib::Compound*>(istream->getType());
                if(compound){
                    for(const Typelib::Field& field : compound->getFields()){
                        field.getName();
                    }
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

        if(summary.fields.size() > 0){
            out << YAML::Key << "fields";
            out << YAML::BeginSeq;
            for(const FieldSummary& field : summary.fields){
                out << YAML::BeginMap;
                out << YAML::Key << "name" << field.fieldName;
                out << YAML::Key << "data_type_name" << field.typeName;
                out << YAML::EndMap;
            }
            out << YAML::EndSeq;
        }
        out << YAML::EndMap;
    }

    out << YAML::EndSeq;
    return out.c_str();
}


void print_summary(FileSummary const &summary, Args& args)
{
    if(args.info_format == "pretty")
    {
        pretty_print(summary);
    }
    else if(args.info_format == "yaml")
    {
        std::cout << to_yaml(summary) << std::endl;
    }
    else{
        throw(std::runtime_error("Unexpected value '"+args.info_format+"' was given for argument info format"));
    }
}


int do_main(Args& args){
    // Open log file
    std::clog << "Reading logfile '" << args.filepath << "'..." << std::flush;
    pocolog_cpp::LogFile logfile(args.filepath);
    std::clog << " OK" << std::endl;

    // Fall back to info mode when no stream was given for extraction
    if(args.stream_name.empty()){
        if (args.mode != "info"){
            std::clog << "\nNo Stream name was specified!" <<std::endl;
            args.mode = "info";
        }
    }

    if( args.mode == "info" )
    {
        FileSummary summary = extract_summary(logfile, args.stream_name);
        print_summary(summary, args);

        return(EXIT_SUCCESS);
    }
    else{
        // If mode is not 'info' it must be 'extract'
        args.mode = "extract";
    }


    if(args.mode == "extract"){
        // Initialize stream
        try{
            Stream *stream_base = &(logfile.getStream(args.stream_name));
            InputDataStream *stream = dynamic_cast<InputDataStream *>(stream_base);
            if(!stream){
                throw std::runtime_error("Could not cast stream");
            }

            // Extract
            extract(stream, args);
        }
        catch(std::runtime_error& err)
        {
            std::cerr << "Error: " << err.what();
            exit(EXIT_FAILURE);
        }
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

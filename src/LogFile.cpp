#include "LogFile.hpp"
#include "StreamDescription.hpp"
#include "InputDataStream.hpp"
#include "IndexFile.hpp"
#include <base-logging/Logging.hpp>
#include <iostream>

namespace pocolog_cpp
{


LogFile::LogFile(const std::string& fileName, bool verbose) : filename(fileName)
{

    logFile.open(fileName.c_str(), std::ifstream::binary | std::ifstream::in);

    readBuffer.resize(8096 * 1024);
//     logFile.rdbuf()->pubsetbuf(readBuffer.data(), readBuffer.size());

    if (!logFile.good()){
        std::cerr << "\ncould not load " << fileName.c_str() << std::endl;
        perror("stat");
        throw std::runtime_error("Error, empty File");
    }

    // Load the prologue
    Prologue prologue;
    logFile.read(reinterpret_cast<char*>(&prologue), sizeof(prologue));
    if (! logFile.good() || std::string(prologue.magic, 7) != std::string(FORMAT_MAGIC))
        throw std::runtime_error("Error, Bad Magic Block, not a Pocolog file ?");;

    firstBlockHeaderPos = logFile.tellg();
    nextBlockHeaderPos = firstBlockHeaderPos;
    gotBlockHeader = false;

    LOG_DEBUG_S << "Found " << descriptions.size() << " stream in logfile " << getFileName();

    //load Index
    IndexFile *indexFile = new IndexFile(*this);
    indexFiles.push_back(indexFile);

    //we need to start from the start, as Stream declarations may be any where
    //inside the logfile
    nextBlockHeaderPos = firstBlockHeaderPos;
    gotBlockHeader = false;

    descriptions = indexFile->getStreamDescriptions();

    for(std::vector<StreamDescription>::const_iterator it = descriptions.begin(); it != descriptions.end();it++)
    {
        switch(it->getType())
        {
            case DataStreamType:
                    LOG_DEBUG_S << "Creating InputDataStream " << it->getName();
                    try
                    {
                        streams.push_back(new InputDataStream(*it, indexFile->getIndexForStream(*it)));
                    }
                    catch(...)
                    {
                        std::cerr << "WARNING, skipping corrupted stream " << it->getName() << " of type " << it->getTypeName();
                    }

                break;
            default:
                LOG_INFO_S << "Ignoring stream " << it->getName();
                break;
        }
    }
}

LogFile::~LogFile()
{
    for (size_t i = 0; i < streams.size(); i++) {
        delete streams[i];
        streams[i] = NULL;
    }
    streams.clear();

    for (size_t i = 0; i < indexFiles.size(); i++) {
        delete indexFiles[i];
    }
    indexFiles.clear();
}


const std::vector< Stream* >& LogFile::getStreams() const
{
    return streams;
}

Stream& LogFile::getStream(const std::string streamName) const
{
    for(std::vector<Stream *>::const_iterator it = streams.begin(); it != streams.end(); it++)
    {
        if(streamName == (*it)->getName())
            return **it;
    }
    throw std::runtime_error("Error stream " + streamName + " not Found");
}

bool LogFile::loadStreamDescription(StreamDescription &result, std::streampos descPos)
{
    nextBlockHeaderPos = descPos;
    if(!readNextBlockHeader())
    {
        LOG_ERROR_S << "Failed to read block header ";
        return false;
    }

    std::vector<uint8_t> descriptionData;
    if(!readCurBlock(descriptionData))
    {
        if(eof())
        {
            LOG_ERROR_S << "IndexFile: Warning, log file seems to be truncated";
            return false;
        }
        throw std::runtime_error("IndexFile: Error building index, log file seems corrupted");
    }

    result = StreamDescription(getFileName(), descriptionData, curBlockHeader.stream_idx);

    return true;
}


const std::vector< StreamDescription >& LogFile::getStreamDescriptions() const
{
    return descriptions;
}

std::string LogFile::getFileName() const
{
    return filename;
}

std::string LogFile::getFileBaseName() const
{
    std::string baseName(filename);
    //easy implemenatiton, filename should end as '.log'
    baseName.resize(filename.size() - 4);

    return baseName;
}

bool LogFile::readNextBlockHeader(BlockHeader& curBlockHeade)
{
    bool ret = readNextBlockHeader();
    curBlockHeade = curBlockHeader;
    return ret;
}

bool LogFile::readNextBlockHeader()
{
    logFile.seekg(nextBlockHeaderPos);
    if(logFile.eof())
    {
        return false;
    }

    curBlockHeaderPos = nextBlockHeaderPos;

    logFile.read((char *) &curBlockHeader, sizeof(BlockHeader));
    if(!logFile.good())
    {
        LOG_ERROR_S << "Reading Block Header failedSample Pos is " << curBlockHeaderPos;
//
        return false;
    }

    nextBlockHeaderPos += sizeof(BlockHeader) + curBlockHeader.data_size;
    curSampleHeaderPos = curBlockHeaderPos;
    curSampleHeaderPos += sizeof(BlockHeader);

    gotBlockHeader = true;
    gotSampleHeader = false;

    return true;
}

const BlockHeader& LogFile::getCurBlockHeader() const
{
    return curBlockHeader;
}

bool LogFile::readCurBlock(std::vector< uint8_t >& blockData)
{
    blockData.resize(curBlockHeader.data_size);
    logFile.seekg(curSampleHeaderPos);
    logFile.read(reinterpret_cast<char *>(blockData.data()), curBlockHeader.data_size);
    return logFile.good();
}

bool LogFile::readSampleHeader()
{
    if(!gotBlockHeader)
    {
        throw std::runtime_error("Internal Error: Called readSampleHeader without reading Block header first");
    }

    logFile.seekg(curSampleHeaderPos);

    if(logFile.eof() || logFile.fail())
    {
        return false;
    }

    logFile.read((char *) &curSampleHeader, sizeof(SampleHeaderData));
    if(!logFile.good())
    {
        return false;
    }

    gotSampleHeader = true;

    return true;
}

bool LogFile::checkSampleComplete()
{
    return (logFile.size() >= nextBlockHeaderPos);
}

std::streampos LogFile::getSamplePos() const
{
    if(!gotBlockHeader)
    {
        throw std::runtime_error("Internal Error: Called getSamplePos without reading Block header first");
    }
    std::streampos ret(curSampleHeaderPos);
    ret += sizeof(SampleHeaderData);
    return ret;
}

std::streampos LogFile::getBlockDataPos() const
{
    if(!gotBlockHeader)
    {
        throw std::runtime_error("Internal Error: Called getBlockDataPos without reading Block header first");
    }
    return curSampleHeaderPos;
}

std::streampos LogFile::getBlockHeaderPos() const
{
    if(!gotBlockHeader)
    {
        throw std::runtime_error("Internal Error: Called getBlockHeaderPos without reading Block header first");
    }
    return curBlockHeaderPos;
}

size_t LogFile::getSampleStreamIdx() const
{
    if(!gotBlockHeader)
    {
        throw std::runtime_error("Internal Error: Called getSampleStreamIdx without reading Block header first");
    }
    return curBlockHeader.stream_idx;
}

const base::Time LogFile::getSampleTime() const
{
    if(!gotSampleHeader)
    {
        throw std::runtime_error("Internal Error: Called getSampleTime without reading Sample header first");
    }
    return base::Time::fromSeconds(curSampleHeader.realtime_tv_sec, curSampleHeader.realtime_tv_usec);
}

bool LogFile::getSampleData(std::vector<uint8_t>& buffer)
{
    if(!gotSampleHeader)
    {
        throw std::runtime_error("Internal Error: Called getSampleData without reading Sample header first");
    }

    logFile.read(reinterpret_cast<char*>(buffer.data()), curSampleHeader.data_size);
    return logFile.good();
}

OwnedValue LogFile::getSample() {
    std::vector<uint8_t> buffer;
    return getSample(buffer);
}

OwnedValue LogFile::getSample(std::vector<uint8_t>& buffer) {
    if (!getSampleData(buffer)) {
        throw std::logic_error("reading sample data failed");
    }

    Typelib::Type const& type = getStreamDescriptions()[getSampleStreamIdx()].getTypelibType();
    OwnedValue sample(type);
    sample.load(buffer);
    return sample;
}

bool LogFile::eof() const
{
    return logFile.eof();
}


    
}

    


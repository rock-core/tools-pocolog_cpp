#include "LogFile.hpp"
#include "StreamDescription.hpp"
#include "InputDataStream.hpp"
#include "IndexFile.hpp"
#include <iostream>
         
namespace pocolog_cpp
{

LogFile::LogFile(const std::string& fileName, bool verbose) : filename(fileName)
{

    logFile.open(fileName.c_str(), std::ifstream::binary | std::ifstream::in);
    
    readBuffer.resize(8096 * 1024);
//     logFile.rdbuf()->pubsetbuf(readBuffer.data(), readBuffer.size());
    
    if (! logFile.good())
        throw std::runtime_error("Error, empty File");

    // Load the prologue
    Prologue prologue;
    logFile.read(reinterpret_cast<char*>(&prologue), sizeof(prologue));
    if (! logFile.good() || std::string(prologue.magic, 7) != std::string(FORMAT_MAGIC))
        throw std::runtime_error("Error, Bad Magic Block, not a Pocolog file ?");;

    firstBlockHeaderPos = logFile.tellg();
    nextBlockHeaderPos = firstBlockHeaderPos;
    gotBlockHeader = false;

//     std::cout << "Found " << descriptions.size() << " stream in logfile " << getFileName() << std::endl;
    
    //load Index
    IndexFile *indexFile = new IndexFile(*this, verbose);
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
//                 std::cout << "Creating InputDataStream " << it->getName() << std::endl;
                streams.push_back(new InputDataStream(*it, indexFile->getIndexForStream(*it)));
                
                break;
            default:
                std::cout << "Ignoring stream " << it->getName() << std::endl;
                break;
        }
    }
}

LogFile::~LogFile()
{
    for(Stream *s : streams)
        delete s;
    
    for(IndexFile *index : indexFiles)
        delete index;
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
        std::cout << "Failed to read block header " << std::endl;
        return false;
    }
    
    std::vector<uint8_t> descriptionData;
    if(!readCurBlock(descriptionData))
    {
        if(eof())
        {
            std::cout << "IndexFile: Warning, log file seems to be truncated" << std::endl;
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
//         std::cout << "Reading Block Header failedSample Pos is " << curBlockHeaderPos << std::endl;
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

bool LogFile::eof() const
{
    return logFile.eof();
}


    
}

    


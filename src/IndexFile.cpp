#include "IndexFile.hpp"
#include "Stream.hpp"
#include "Index.hpp"
#include "LogFile.hpp"
#include <base-logging/Logging.hpp>
#include <string.h>
#include <iostream>
#include <cassert>
#include <boost/lexical_cast.hpp>
#include <stdexcept>

namespace pocolog_cpp
{
    
IndexFile::IndexFileHeader::IndexFileHeader() : numStreams(0)
{
    memcpy(magic, getMagic().c_str(), sizeof(magic));
}

std::string IndexFile::IndexFileHeader::getMagic()
{
    return std::string("IndexV2");
}

    
IndexFile::IndexFile(std::string indexFileName, LogFile &logFile, bool verbose)
{    
    if(!loadIndexFile(indexFileName, logFile))
        throw std::runtime_error("Error, index is corrupted");
    
}

IndexFile::IndexFile(LogFile &logFile, bool verbose)
{
    std::string indexFileName(logFile.getFileBaseName() + ".id2");
    if(!loadIndexFile(indexFileName, logFile))
    {
        if(!createIndexFile(indexFileName, logFile))
        {
            throw std::runtime_error("Error building Index");
        }
        if(!loadIndexFile(indexFileName, logFile))
            throw std::runtime_error("Internal Error, created index is corrupted");
    }
}

IndexFile::~IndexFile()
{
    for (size_t i = 0; i < indices.size(); i++) {
        delete indices[i];
        indices[i] = NULL;
    }
    indices.clear();
}


bool IndexFile::loadIndexFile(std::string indexFileName, pocolog_cpp::LogFile& logFile)
{
    LOG_DEBUG_S << "Loading Index File ";
    std::ifstream indexFile(indexFileName.c_str(), std::fstream::in | std::fstream::binary );
    
    if(!indexFile.good())
        return false;
    
    IndexFileHeader header;
    
    indexFile.read((char *) &header, sizeof(IndexFileHeader));
    if(!indexFile.good())
        return false;

    if(header.magic != header.getMagic())
    {
        LOG_ERROR_S << "Magic is " << header.magic;;
        LOG_ERROR_S << "Magic should be " << header.getMagic();;
        LOG_ERROR_S << "Error, index magic does not match";
        return false;
    }
    
    indexFile.close();
    
    for(uint32_t i = 0; i < header.numStreams; i++)
    {
        Index *idx = new Index(indexFileName, i);
        //load streams
        indices.push_back(idx);
        

        
        StreamDescription newStream;
        if(!logFile.loadStreamDescription(newStream, idx->getDescriptionPos()))
        {
            throw std::runtime_error("IndexFile: Internal error, could not load stream description");
        }
        
        streams.push_back(newStream);
        
        LOG_INFO_S << "Stream " << newStream.getName() << " [" << newStream.getTypeName() << "]" <<std::endl;
        LOG_INFO_S << "  " << idx->getNumSamples() << " Samples from " << idx->getFirstSampleTime().toString(base::Time::Seconds) 
                    << " to " << idx->getLastSampleTime().toString(base::Time::Seconds) << " [" 
                    << (idx->getLastSampleTime() - idx->getFirstSampleTime()).toString(base::Time::Milliseconds , "%H:%M:%S") << "]" <<std::endl;
    }
    
    return true;
    
}

Index& IndexFile::getIndexForStream(const StreamDescription& desc)
{
    for(std::vector<Index *>::const_iterator it = indices.begin(); it != indices.end(); it++ )
    {
        if((*it)->matches(desc))
            return **it;
    }
    
    throw std::runtime_error("IndexFile does not contain valid index for Stream ");
}

bool IndexFile::createIndexFile(std::string indexFileName, LogFile& logFile)
{
    LOG_DEBUG_S << "IndexFile: Creating Index File for logfile " << logFile.getFileName();
    std::vector<char> writeBuffer;
    writeBuffer.resize(8096 * 1024);
    std::fstream indexFile;
    indexFile.rdbuf()->pubsetbuf(writeBuffer.data(), writeBuffer.size());

    indexFile.open(indexFileName.c_str(), std::fstream::out | std::fstream::binary | std::fstream::trunc);
    
    std::vector<Index> foundIndices;
    std::vector<StreamDescription> foundStreams;
    
    
    while(logFile.readNextBlockHeader())
    {
        const BlockHeader &curBlockHeader(logFile.getCurBlockHeader());
        switch(curBlockHeader.type)
        {
            case UnknownBlockType:
                throw std::runtime_error("IndexFile: Error, encountered unknown block type");
                break;
            case StreamBlockType:
            {
                off_t descPos = logFile.getBlockHeaderPos();
                StreamDescription newStream;
                if(!logFile.loadStreamDescription(newStream, descPos))
                {
                    break;
                }
                
                foundStreams.push_back(newStream);
                
                size_t index = newStream.getIndex();
                if(index != foundIndices.size() )
                {
                    throw std::runtime_error("IndexFile: Error building Index, Unexpected stream index");
                }
                foundIndices.emplace_back(newStream, descPos);
                break;
            }
            case DataBlockType:
            {
                if(!logFile.readSampleHeader())
                {
                    if(logFile.eof())
                    {
                        LOG_WARN_S << "IndexFile: Warning, log file seems to be truncated";
                        break;
                    }
                    throw std::runtime_error("IndexFile: Error building index, log file seems corrupted");
                }
                
                size_t idx = logFile.getSampleStreamIdx();
                if(idx >= foundIndices.size())
                    throw std::runtime_error("Error: Corrupt log file " + logFile.getFileName() + ", got sample for nonexisting stream " + boost::lexical_cast<std::string>(idx) );
                
                if(logFile.checkSampleComplete())
                {
                    foundIndices[idx].addSample(logFile.getSamplePos(), logFile.getSampleTime());
                }
                else
                {
                    LOG_WARN_S << "IndexFile: Warning, ignoring truncated sample for stream " << foundIndices[idx].getName();
                }
            }
                break;
            case ControlBlockType:
                break;
                
        }
        
    }
    LOG_DEBUG_S << "IndexFile: Found " << foundStreams.size() << " datastreams " << std::flush;

    IndexFileHeader header;
    header.numStreams = foundStreams.size();
    
    assert(header.magic == header.getMagic());
    
    indexFile.write((char *) &header, sizeof(header));
    if(!indexFile.good())
        throw std::runtime_error("IndexFile: Error writing index header");
    
    off_t curProloguePos = sizeof(IndexFileHeader);
    off_t curDataPos = foundIndices.size() * Index::getPrologueSize() + sizeof(IndexFileHeader);
    for ( auto curIdx : foundIndices )
    {
        LOG_INFO_S << "Writing index for stream " << curIdx.getName() << " , num samples " << curIdx.getNumSamples();
    
        //Write index prologue
        curDataPos = curIdx.writeIndexToFile(indexFile, curProloguePos, curDataPos);
        curProloguePos += Index::getPrologueSize();
        
        if(!indexFile.good())
            throw std::runtime_error("IndexFile: Error writing index File");
        
    }
    
    indexFile.close();
    LOG_DEBUG_S << "done ";
    
    return true;
}

const std::vector< StreamDescription >& IndexFile::getStreamDescriptions() const
{
    return streams;
}


}

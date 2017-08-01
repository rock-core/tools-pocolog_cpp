#include "IndexFile.hpp"
#include "Stream.hpp"
#include "Index.hpp"
#include "LogFile.hpp"
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

    
IndexFile::IndexFile(std::string indexFileName, LogFile &logFile, bool verbose) : verbose(verbose)
{    
    if(!loadIndexFile(indexFileName, logFile))
        throw std::runtime_error("Error, index is corrupted");
    
}

IndexFile::IndexFile(LogFile &logFile, bool verbose) : verbose(verbose)
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
    for(Index *index : indices)
        delete index;
    
}


bool IndexFile::loadIndexFile(std::string indexFileName, pocolog_cpp::LogFile& logFile)
{
//     std::cout << "Loading Index File " << std::endl;
    std::ifstream indexFile(indexFileName.c_str(), std::fstream::in | std::fstream::binary );
    
    if(!indexFile.good())
        return false;
    
    IndexFileHeader header;
    
    indexFile.read((char *) &header, sizeof(IndexFileHeader));
    if(!indexFile.good())
        return false;

    if(header.magic != header.getMagic())
    {
        std::cout << "Magic is " << header.magic << std::endl;;
        std::cout << "Magic should be " << header.getMagic() << std::endl;;
        std::cout << "Error, index magic does not match" << std::endl;
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
        
        if(verbose)
        {
            std::cout << "Stream " << newStream.getName() << " [" << newStream.getTypeName() << "]" <<std::endl;
            std::cout << "  " << idx->getNumSamples() << " Samples from " << idx->getFirstSampleTime().toString(base::Time::Seconds) 
                        << " to " << idx->getLastSampleTime().toString(base::Time::Seconds) << " [" 
                        << (idx->getLastSampleTime() - idx->getFirstSampleTime()).toString(base::Time::Milliseconds , "%H:%M:%S") << "]" <<std::endl;
        }
        
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
//     std::cout << "IndexFile: Creating Index File for logfile " << logFile.getFileName() << std::endl;
    std::vector<char> writeBuffer;
    writeBuffer.resize(8096 * 1024);
    std::fstream indexFile;
    indexFile.rdbuf()->pubsetbuf(writeBuffer.data(), writeBuffer.size());

    indexFile.open(indexFileName.c_str(), std::fstream::out | std::fstream::binary | std::fstream::trunc);
    
    
    
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
                
                streams.push_back(newStream);
                
                Index *newIndex = new Index(newStream, descPos);
                size_t index = newStream.getIndex();
                if(index != indices.size() )
                {
                    throw std::runtime_error("IndexFile: Error building Index, Unexpected stream index");
                }
                indices.push_back(newIndex);
                break;
            }
            case DataBlockType:
            {
                if(!logFile.readSampleHeader())
                {
                    if(logFile.eof())
                    {
                        std::cout << "IndexFile: Warning, log file seems to be truncated" << std::endl;
                        break;
                    }
                    throw std::runtime_error("IndexFile: Error building index, log file seems corrupted");
                }
                
                size_t idx = logFile.getSampleStreamIdx();
                if(idx >= indices.size())
                    throw std::runtime_error("Error: Corrupt log file " + logFile.getFileName() + ", got sample for nonexisting stream " + boost::lexical_cast<std::string>(idx) );
                
                if(logFile.checkSampleComplete())
                {
                    indices[idx]->addSample(logFile.getSamplePos(), logFile.getSampleTime());
                }
                else
                {
                    std::cout << "IndexFile: Warning, ignoring truncated sample for stream " << indices[idx]->getName() << std::endl;
                }
            }
                break;
            case ControlBlockType:
                break;
                
        }
        
    }
//     std::cout << "IndexFile: Found " << streams.size() << " datastreams " << std::endl << std::flush;

    IndexFileHeader header;
    header.numStreams = streams.size();
    
    assert(header.magic == header.getMagic());
    
    indexFile.write((char *) &header, sizeof(header));
    if(!indexFile.good())
        throw std::runtime_error("IndexFile: Error writing index header");
    
    off_t curProloguePos = sizeof(IndexFileHeader);
    off_t curDataPos = indices.size() * Index::getPrologueSize() + sizeof(IndexFileHeader);
    for(std::vector<Index* >::iterator it = indices.begin(); it != indices.end(); it++)
    {
        std::cout << "Writing index for stream " << (*it)->getName() << " , num samples " << (*it)->getNumSamples() << std::endl;
    
        //Write index prologue
        curDataPos = (*it)->writeIndexToFile(indexFile, curProloguePos, curDataPos);
        curProloguePos += Index::getPrologueSize();
        
        if(!indexFile.good())
            throw std::runtime_error("IndexFile: Error writing index File");
        
        delete *it;
    }
    
    indexFile.close();
//     std::cout << "done " << std::endl;
    
    indices.clear();;
    
    return true;
}

const std::vector< StreamDescription >& IndexFile::getStreamDescriptions() const
{
    return streams;
}


}

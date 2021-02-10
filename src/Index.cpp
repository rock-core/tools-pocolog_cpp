#include "Index.hpp"
#include "LogFile.hpp"
#include "IndexFile.hpp"
#include <base-logging/Logging.hpp>
#include <stdint.h>
#include <unistd.h>
#include <iostream>
#include <stdexcept>

namespace pocolog_cpp
{
    
Index::Index(std::string indexFileName, size_t streamIdx):  firstAdd(true), curSampleNr(-1), indexFile(indexFileName.c_str(), std::ifstream::binary | std::ifstream::in)
{
    indexFile.seekg(streamIdx * sizeof(IndexPrologue) + sizeof(IndexFile::IndexFileHeader));
    
    indexFile.read((char *) & prologue, sizeof(IndexPrologue));
    
    firstSampleTime = base::Time::fromMicroseconds(prologue.firstSampleTime);
    lastSampleTime = base::Time::fromMicroseconds(prologue.lastSampleTime);
}

Index::Index(const pocolog_cpp::StreamDescription& desc, off_t posOfStreamDesc) : firstAdd(true), curSampleNr(-1)
{
    prologue.streamIdx = desc.getIndex();
    prologue.nameCrc = 0;
    prologue.numSamples = 0;
    prologue.streamDescPos = posOfStreamDesc;
    prologue.firstSampleTime = 0;
    prologue.lastSampleTime = 0;
    name = desc.getName();
}

bool Index::matches(const StreamDescription& odesc) const
{
    return prologue.streamIdx == odesc.getIndex();
//    return (desc.getName() == odesc.getName() && desc.getTypeName() == odesc.getTypeName());
}


void Index::addSample(off_t filePosition, const base::Time& sampleTime)
{
    if(firstAdd)
    {
        prologue.firstSampleTime = sampleTime.microseconds;
        firstAdd = false;
    }
    
    IndexInfo info;
    info.samplePosInLogFile = filePosition;
    info.sampleTime = sampleTime.microseconds;
    buildBuffer.push_back(info);
    
    prologue.lastSampleTime = sampleTime.microseconds;
    prologue.numSamples++;
}

off_t Index::getPrologueSize()
{
    return sizeof(IndexPrologue);
}

off_t Index::writeIndexToFile(std::fstream& indexFile, off_t prologPos, off_t indexDataPos)
{
    indexFile.seekp(prologPos, std::fstream::beg);
    if(!indexFile.good())
        throw std::runtime_error("Error writing index file");
    
    prologue.dataPos = indexDataPos;
    LOG_DEBUG_S << "Found " << prologue.numSamples << " in stream " << name; 
    
    indexFile.write((char *) &prologue, sizeof(IndexPrologue));
    if(!indexFile.good())
        throw std::runtime_error("Error writing index file");
    
    indexFile.seekp(indexDataPos, std::fstream::beg);
    if(!indexFile.good())
        throw std::runtime_error("Error writing index file");

    LOG_DEBUG_S << "Wrinting " << buildBuffer.size() * sizeof(IndexInfo) / 1024 << " KBytes to index File ";
    
    indexFile.write((char *) buildBuffer.data(), buildBuffer.size() * sizeof(IndexInfo));
    if(!indexFile.good())
        throw std::runtime_error("Error writing index file");
    
    LOG_DEBUG_S << "Done new pos " << indexFile.tellp(); 
    
    return indexFile.tellp();
}

void Index::loadIndex(size_t sampleNr)
{
    if(sampleNr >= prologue.numSamples)
        throw std::runtime_error("Index::loadIndex : Error sample out of index requested");
        
    if(sampleNr != curSampleNr)
    {
        std::streampos pos(prologue.dataPos + sampleNr * sizeof(IndexInfo));
        LOG_DEBUG_S << "Seeking to " << pos << " start of index Data " << prologue.dataPos << " pos in data " << sampleNr * sizeof(IndexInfo);
        indexFile.seekg(std::streampos(prologue.dataPos + sampleNr * sizeof(IndexInfo)));
        if(!indexFile.good())
            throw std::runtime_error("Internal Error, index file is corrupted");
        indexFile.read((char *) &curIndexInfo, sizeof(IndexInfo));
        if(!indexFile.good())
            throw std::runtime_error("Internal Error, index file is corrupted");
        
        curSampleNr = sampleNr;
    }
}


std::streampos Index::getSamplePos(size_t sampleNr)
{

    loadIndex(sampleNr);
    
    return std::streampos(curIndexInfo.samplePosInLogFile);
    
}

base::Time Index::getSampleTime(size_t sampleNr)
{
    loadIndex(sampleNr);

    return base::Time::fromMicroseconds(curIndexInfo.sampleTime);
}

Index::~Index()
{
    indexFile.close();
}
}

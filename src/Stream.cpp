#include "Stream.hpp"
#include <base-logging/Logging.hpp>
#include <iostream>
#include <stdexcept>

pocolog_cpp::Stream::Stream(const pocolog_cpp::StreamDescription& desc, pocolog_cpp::Index& index) : desc(desc), index(index)
{
    fileStream.open(desc.getFileName().c_str(), std::ifstream::binary | std::ifstream::in);
    if(!fileStream.good())
        throw std::runtime_error("Error, could not open logfile for stream " + desc.getName());
}

pocolog_cpp::Stream::~Stream()
{

}

bool pocolog_cpp::Stream::loadSampleHeader(std::streampos pos, SampleHeaderData &header)
{
    fileStream.seekg(pos);
    fileStream.read((char *) &header, sizeof(SampleHeaderData));
    return fileStream.good();
}


bool pocolog_cpp::Stream::getSampleData(std::vector< uint8_t >& result, size_t sampleNr)
{
    std::streampos samplePos = index.getSamplePos(sampleNr);
    std::streampos sampleHeaderPos = samplePos;
    sampleHeaderPos -= sizeof(SampleHeaderData);
    
    SampleHeaderData header;
    if(!loadSampleHeader(sampleHeaderPos, header))
    {
        LOG_ERROR_S << "Could not load sample header of sample " << sampleNr << " samplePos " << samplePos << " headerPos " << sampleHeaderPos;
        return false;
    }
    result.resize(header.data_size);

    fileStream.seekg(samplePos);
    fileStream.read((char *) result.data(), header.data_size);
    if(!fileStream.good())
    {
        LOG_ERROR_S << "Could not load sample data of sample " << sampleNr;
    }
    return fileStream.good();
}

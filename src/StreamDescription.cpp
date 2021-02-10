#include "StreamDescription.hpp"
#include <base-logging/Logging.hpp>
#include <fstream>
#include <vector>
#include <iostream>
#include <stdexcept>
#include <sstream>


namespace pocolog_cpp
{
StreamDescription::StreamDescription() : m_index(-1)
{

}

    
std::string StreamDescription::readString(const std::vector<uint8_t> data, size_t &pos)
{
    if(pos + sizeof(uint32_t) > data.size())
    {
        throw std::runtime_error("StreamDescription: Error, data is truncated");
    }
    uint32_t stringSize = *(reinterpret_cast<const uint32_t *>(data.data() + pos));
    pos += sizeof(uint32_t);
    
    if(pos + stringSize > data.size())
    {
        throw std::runtime_error("StreamDescription: Error, data is truncated");
    }
    std::string ret = std::string(reinterpret_cast<const char *>(data.data() + pos), stringSize);
    pos += stringSize;
    
    return ret;
}

    
StreamDescription::StreamDescription(const std::string& fileName, const std::vector<uint8_t> data, size_t stream_idx)
{
    m_fileName = fileName;
    m_index = stream_idx;
    
    size_t pos = 0;
    
    uint8_t stream_type;
    stream_type = data[pos];
    pos+= sizeof(uint8_t);
    
    m_type = static_cast<enum StreamType>(stream_type);

    switch(stream_type) 
    {
        case ControlStreamType:
        {
        }
        break;
        
        case DataStreamType:
        {
            m_streamName = readString(data, pos);
            m_typeName = readString(data, pos);

            LOG_DEBUG_S << "StreamDescription: Found Stream, Name " << m_streamName << " of type " << m_typeName;
            
            m_typeDescription = readString(data, pos);
            LOG_DEBUG_S << "Found stream description : " << m_typeDescription;
            m_metadata = readString(data, pos);

            if(pos != data.size())
            {
                LOG_ERROR_S << "StreamDescription: Stream Declaration size is " << pos << " expected size " << data.size();
                
                throw std::runtime_error("StreamDescription: Error, stream declaration has wrong size");
            }
        }
        break;
        
        default:
            throw std::runtime_error("StreamDescription: Error, unexpected Stream description");
            break;
    };  
    
    std::string key, val;
    std::istringstream inputData(m_metadata);

    while(std::getline(std::getline(inputData, key, ':') >> std::ws, val))
        m_metadataMap[key] = val;
}

StreamDescription::~StreamDescription()
{

}


}

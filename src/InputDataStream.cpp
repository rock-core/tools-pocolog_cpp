#include "InputDataStream.hpp"
#include <base-logging/Logging.hpp>
#include <typelib/pluginmanager.hh>
#include <typelib/registry.hh>

namespace pocolog_cpp
{

// InputDataStream::InputDataStream()
// {
// 
// }
// 
// InputDataStream::~InputDataStream()
// {
// 
// }

InputDataStream::InputDataStream(const StreamDescription& desc, Index& index): Stream(desc, index)
{
    loadTypeLib();
}

InputDataStream::~InputDataStream()
{

}


void InputDataStream::loadTypeLib()
{
    utilmm::config_set empty;
    m_registry = new Typelib::Registry;
    
    std::istringstream stream(desc.getTypeDescription());

    // Load the data_types registry from pocosim
    Typelib::PluginManager::load("tlb", stream, empty, *m_registry);
    
    m_type = m_registry->build(desc.getTypeName());
}

const Typelib::Type* InputDataStream::getType() const
{
    return m_type; 
}

Typelib::Value InputDataStream::getTyplibValue(void *memoryOfType, size_t memorySize, size_t sampleNr)
{
    std::vector<uint8_t> buffer;
    if(!getSampleData(buffer, sampleNr))
        throw std::runtime_error("Error, sample for stream " + desc.getName() + " could not be loaded");

    if(memorySize < m_type->getSize())
    {
        throw std::runtime_error("Error, given memory area is to small for type " + m_type->getName() + " at stream " + desc.getName());
    }

    Typelib::Value v(memoryOfType, *m_type);
    //init memory area
    Typelib::init(v);
    Typelib::load(v, buffer);
    return v;
}

const std::string InputDataStream::getCXXType() const
{
    std::map<std::string, std::string>::const_iterator it = desc.getMetadataMap().find("rock_cxx_type_name");
    if(it == desc.getMetadataMap().end())
    {
        std::cout << "Error: Logfile does not contain metadata CXXType. Maybe old logfile?" << std::endl << "MetaData : " << std::endl;
        for(auto e : desc.getMetadataMap())
        {
            std::cout << e.first << " : " << e.second << std::endl;
        }
        throw std::runtime_error("Error: Logfile does not contain metadata CXXType. Maybe old logfile?");
    }
        
    return it->second;
}


}

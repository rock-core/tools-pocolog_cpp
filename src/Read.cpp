/* 
 * Copyright (c) 2005-2006 LAAS/CNRS <openrobots@laas.fr>
 *	Sylvain Joyeux <sylvain.joyeux@m4x.org>
 *
 * All rights reserved.
 *
 * Redistribution and use  in source  and binary  forms,  with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *   1. Redistributions of  source  code must retain the  above copyright
 *      notice, this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright
 *      notice,  this list of  conditions and the following disclaimer in
 *      the  documentation  and/or  other   materials provided  with  the
 *      distribution.
 *
 * THIS  SOFTWARE IS PROVIDED BY  THE  COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND  ANY  EXPRESS OR IMPLIED  WARRANTIES,  INCLUDING,  BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES  OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR  PURPOSE ARE DISCLAIMED. IN  NO EVENT SHALL THE COPYRIGHT
 * HOLDERS OR      CONTRIBUTORS  BE LIABLE FOR   ANY    DIRECT, INDIRECT,
 * INCIDENTAL,  SPECIAL,  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF  SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN  CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE   OF THIS SOFTWARE, EVEN   IF ADVISED OF   THE POSSIBILITY OF SUCH
 * DAMAGE.
 */


#include "pocolog_cpp/Read.hpp"

#include <typelib/pluginmanager.hh>
#include <typelib/registry.hh>

#include <iostream>
#include <sstream>

using namespace std;
using namespace Typelib;
using base::Time;

namespace
{
    template<typename T>
    bool read(std::istream& input, T& data, size_t size = sizeof(T))
    {
        input.read(reinterpret_cast<char*>(&data), size);
        return input.good();
    }

    template<typename T>
    struct as
    {
        const T* data;
        
        as(const uint8_t* ptr)
            : data(reinterpret_cast<const T*>(ptr)) {}

        operator T()  const { return *data; }
    };

    template<typename T>
    struct as<T*>
    {
        const T* data;

        as(const uint8_t* ptr)
            : data(reinterpret_cast<const T*>(ptr)) {}
        operator T*() const { return data; }
    };
}

namespace pocolog_cpp
{
    Input::Input() 
        : m_input(0) {}

    Input::~Input()
    {
        for (Streams::iterator it = m_streams.begin(); it != m_streams.end(); ++it)
            delete *it;
    }

    void Input::init(std::istream& input)
    {
        m_input = &input;
        if (! input.good())
            throw Truncated();

        // Load the prologue
        Prologue prologue;
        input.read(reinterpret_cast<char*>(&prologue), sizeof(prologue));
        if (! input || string(prologue.magic, 7) != string(FORMAT_MAGIC))
            throw BadMagic();

        BlockHeader       header;
        vector<uint8_t>   data;

        // Read first header
        read(input, header);
        while (input.good())
        {
            size_t pos       = input.tellg();
            size_t data_size = header.data_size;
            size_t stream_index   = header.stream_idx;
            if (header.type == StreamBlockType)
            {
                readBlockData(data, data_size);
                readStreamDeclaration(data, stream_index, pos, data_size);
                read(input, header);
            }
            else if (header.type == DataBlockType)
            {
                // Check that there is a valid stream declared here
                if (m_streams.size() <= stream_index || ! m_streams[stream_index] -> isValid())
                    throw BadStream(stream_index, pos);
                header = m_streams[stream_index] -> newDataBlock(header);
            }
            else if (header.type == ControlBlockType)
            {
                header = skip(header);
            }
            else
                throw BadBlockType(header.type, pos);
        }    

        input.clear();
    }

    size_t  Input::size() const { return m_streams.size(); }
    Stream& Input::operator[] (size_t index) const
    { return *m_streams[index]; }

    DataStream& Input::getDataStream( const std::string& name ) const
    {
	for(size_t i=0;i<m_streams.size();i++)
	{
	    if( m_streams[i]->getType() == DataStreamType )
	    {
		DataStream& stream = dynamic_cast<DataStream&>(*m_streams[i]);
		if( stream.getName() == name )
		    return stream;
	    }
	}

	throw NoSuchStream(name);
    }

    void Input::readBlockData(std::vector<uint8_t>& buffer, size_t size_)
    { readBlockData(*m_input, buffer, size_); }
    void Input::readBlockData(std::istream& input, std::vector<uint8_t>& buffer, size_t size_)
    {
        buffer.resize(size_);

        input.read(reinterpret_cast<char*>(&buffer[0]), size_);
        if (! input.good())
            throw Truncated();
    }

    struct buffer_reader
    {
        vector<uint8_t> const& m_buffer;
        uint32_t m_cursor;
        size_t   m_pos;
        size_t   m_size;

        buffer_reader(vector<uint8_t> const& buffer, size_t pos, size_t size)
            : m_buffer(buffer), m_cursor(0), m_pos(pos), m_size(size) {}
        
        void advance(size_t offset) 
        { 
            if (m_cursor + offset > m_size)
                throw BadDataSize(StreamBlockType, m_pos);
            m_cursor += offset;
        }

        size_t cursor() const { return m_cursor; }

        template<typename T>
        T as() const { return ::as<T>(&m_buffer[m_cursor]); }

        template<typename T>
        T read()
        { 
            size_t cursor = m_cursor;
            advance(sizeof(T));
            return ::as<T>(&m_buffer[cursor]);
        }
    };

    bool Input::readStreamDeclaration(const std::vector<uint8_t>& buffer, size_t stream_idx, size_t pos_, size_t size_)
    {
        buffer_reader reader(buffer, pos_, size_);
        uint8_t stream_type = reader.read<uint8_t>();

        unique_ptr<Stream> new_stream;
        if (stream_type == ControlStreamType)
        {
            new_stream.reset(new ControlStream(*m_input, stream_idx));
        }
        else if (stream_type == DataStreamType)
        {
            uint32_t name_length = reader.read<uint32_t>();
            const char* name = reader.as<const char*>();
            reader.advance(name_length);

            uint32_t type_name_length = reader.read<uint32_t>();
            const char* type_name = reader.as<const char*>();
            reader.advance(type_name_length);

            uint32_t tlb_length = 0;
            char const* tlb = "";

	    utilmm::config_set empty;
            auto_ptr<Registry> registry(new Registry);
            if (reader.cursor() < size_)
            {
                tlb_length = reader.read<uint32_t>();
                tlb = reader.as<const char*>();
                reader.advance(tlb_length);

                istringstream stream(string(tlb, tlb_length));

		// Load the data_types registry from pocosim
                Typelib::PluginManager::load("tlb", stream, empty, *registry);
            }
            
            if (reader.cursor() != size_)
                throw DataSizeMismatch(StreamBlockType, pos_);

            new_stream.reset(
                    new DataStream(*m_input, stream_idx, 
                        string(name, name_length), 
                        string(type_name, type_name_length),
                        registry.release()));
        }
        else
            throw BadStreamType(stream_type);

        if (m_streams.size() < stream_idx + 1)
            m_streams.resize(stream_idx + 1);
        m_streams[stream_idx] = new_stream.release();

        return true;
    }

    BlockHeader Input::skip(const BlockHeader& header)
    { return skip(*m_input, header); }

    BlockHeader Input::skip(std::istream& input, const BlockHeader& header)
    {
        size_t data_size(header.data_size);
        input.seekg( data_size, ios_base::cur );
        if (! input)
            throw Truncated();

        BlockHeader new_header;
        read(input, new_header);
        return new_header;
    }




    DataStream::DataStream(std::istream& input, size_t stream_index, const std::string& name, const std::string& type_name, Typelib::Registry* registry)
        : Stream(input, DataStreamType, stream_index)
        , m_name(name), m_type_name(type_name), m_registry(registry) {}

    DataStream::~DataStream() { delete m_registry; }


    DataInputIterator DataStream::begin()   
    { return DataInputIterator(getType(), getInputStream(), getIndex(), getBeginPos(), m_firstheader); }
    DataInputIterator DataStream::end()     
    { return DataInputIterator(getType(), getInputStream(), getIndex(), Input::npos, m_firstheader); }

    string DataStream::getName() const { return m_name; }
    string DataStream::getTypeName() const { return m_type_name; }
    Type const*       DataStream::getType() const { 
        if (m_registry)
            return m_registry->build(m_type_name); 
        else
            return 0;
    }
    Registry const*   DataStream::getTypeRegistry() const { return m_registry; }

    Time   DataStream::getBeginTime() const { return m_begin; }
    Time   DataStream::getEndTime() const   { return m_end; }

    BlockHeader DataStream::newDataBlock(BlockHeader header)
    {
        istream& input(getInputStream());
        size_t   pos = input.tellg();

        SampleHeader sample_header;
        if (! read(input, sample_header, SAMPLE_HEADER_SIZE))
            throw Truncated();

        size_t size (sample_header.data_size);
        input.seekg(size, ios_base::cur);
        if (! input)
            throw Truncated();

        const Time timestamp = sample_header.timestamp;
        if (m_begin.isNull())
        {
            // First block
            m_begin = m_end = timestamp;
            m_firstheader = header;
        }
        else if (m_end < timestamp)
            m_end = timestamp;

        newBlockAt(pos, size);

        BlockHeader new_header;
        read(input, new_header);
        return new_header;
    }

    ControlStream::ControlStream(std::istream& input, size_t stream_index)
        : Stream(input, ControlStreamType, stream_index) {}

    BlockHeader ControlStream::newDataBlock(BlockHeader header)
    { 
        size_t pos = getInputStream().tellg();
        newBlockAt(pos, 0);
        return Input::skip(getInputStream(), header);
    }





    DataInputIterator::DataInputIterator()
    {}

    DataInputIterator::DataInputIterator
            ( Typelib::Type const* type
            , std::istream& input, size_t stream_idx
            , size_t first_block, const BlockHeader& first_header)
        : m_sample_type(type), m_input(&input), m_index(stream_idx)
        , m_pos(first_block), m_block_header(first_header)
    { 
        if (first_block != Input::npos)
        {
            input.clear();
            input.seekg( first_block, ios_base::beg );
            readCurBlock(first_header.data_size);
        }
    }
    DataInputIterator::~DataInputIterator() {}

    bool   DataInputIterator::isValid() const      { return Input::npos != m_pos; }
    void   DataInputIterator::failed()             
    { 
        m_pos = Input::npos; 
        m_input->clear();
    }
    Time   DataInputIterator::getRealtime() const  { return m_sample_header.realtime; }
    Time   DataInputIterator::getTimestamp() const { return m_sample_header.timestamp; }
    size_t DataInputIterator::getDataSize() const  { return m_sample_header.data_size; }
    const uint8_t* DataInputIterator::getData() const  { return as<const uint8_t*>(&m_buffer[SAMPLE_HEADER_SIZE]); }

    bool DataInputIterator::operator == (const DataInputIterator& with) const
    { 
        return (m_index == with.m_index) && (m_pos == with.m_pos); 
    }

    DataInputIterator& DataInputIterator::operator ++()
    { return (*this) += 1; }
    DataInputIterator  DataInputIterator::operator + (size_t offset) const
    { return (DataInputIterator(*this) += offset); }
    DataInputIterator& DataInputIterator::operator += (size_t offset)
    {
        if (! offset)    return *this;
        if (! isValid()) return *this;

        m_input->clear();
        m_input->seekg(m_pos, ios_base::beg);

        BlockHeader header(m_block_header);
        while(true)
        {
            header = Input::skip(*m_input, header);
            if (! *m_input)
            {
                failed();
                return *this;
            }

            if (header.type == DataBlockType && header.stream_idx == m_index)
                --offset;
            if (! offset)
                break;
        }

        m_block_header = header;
        m_pos = m_input->tellg();
        readCurBlock(header.data_size);
        return *this;
    }

    void DataInputIterator::readCurBlock(size_t data_size)
    {
        Input::readBlockData(*m_input, m_buffer, data_size);
        if (! m_input)
        {
            failed();
            return;
        }
        
        m_sample_header = as<SampleHeader>(&m_buffer[0]);
    }








    void FileException::output(std::ostream& display) throw()
    { if (m_pos != Input::npos)
        display << "at byte " << m_pos; }
    void Truncated::output(std::ostream& display) throw()
    { display << "file truncated "; }
    void BadMagic::output(std::ostream& display) throw()
    { display << "bad magic code "; }
    void NoSuchStream::output(std::ostream& display) throw()
    { display << "stream '" << name << "' not found "; }
    void BadStream::output(std::ostream& display) throw()
    { display << "stream index " << m_index << " unknown ";
        FileException::output(display); }
    void BadBlockType::output(std::ostream& display) throw()
    { display << "block type " << m_type << " unknown ";
        FileException::output(display); }
    void DataBlockException::output(std::ostream& display) throw()
    { display << "in data block ";
        FileException::output(display); }
    void BadDataSize::output(std::ostream& display) throw()
    { display << "bad data size ";
        DataBlockException::output(display); }
    void DataSizeMismatch::output(std::ostream& display) throw()
    { display << "data size mismatch between declared size and found size ";
        DataBlockException::output(display); }
    void BadStreamType::output(std::ostream& display) throw()
    {
        display << "bad stream type ";
        FileException::output(display);
    }

}


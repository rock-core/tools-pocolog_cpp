#ifndef STREAM_H
#define STREAM_H

#include <fstream>
#include "Format.hpp"
#include "StreamDescription.hpp"
#include "Index.hpp"
#include "FileStream.hpp"

namespace pocolog_cpp
{

class Stream
{

protected:
    const StreamDescription &desc;
    Index &index;

    FileStream fileStream;
    Stream(const StreamDescription &desc, Index &index);

    bool loadSampleHeader(std::streampos pos, pocolog_cpp::SampleHeaderData& header);

public:
    virtual ~Stream();

    bool isValid() const
    {
        return desc.getType() != UnknownStreamType;
    }

    const StreamDescription& getDescription() const
    {
        return desc;
    }

    std::string getName() const
    {
        return desc.getName();
    }

    std::string getTypeName() const
    {
        return desc.getTypeName();
    }

    StreamType getStreamType() const
    {
        return desc.getType();
    }

    const base::Time &getFistSampleTime() const
    {
        return index.getFirstSampleTime();
    }

    const base::Time &getLastSampleTime() const
    {
        return index.getLastSampleTime();
    }

    size_t getIndex() const
    {
        return desc.getIndex();
    }

    Index& getFileIndex() const
    {
        return index;
    }

    size_t getSize() const
    {
        return index.getNumSamples();
    }

    const FileStream& getFileStream() const
    {
        return fileStream;
    }

    bool getSampleData(std::vector<uint8_t> &result, size_t sampleNr);

    template<typename T>
    bool readSample(T &sample, size_t sampleNr)
    {
        fileStream.seekg(index.getSamplePos(sampleNr));
        fileStream.read((char *) &sample, sizeof(T));

        return fileStream.good();
    }
    
};

}
#endif // STREAM_H

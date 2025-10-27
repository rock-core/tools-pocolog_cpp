#ifndef INDEX_H
#define INDEX_H
#include <stddef.h>
#include <string>
#include <vector>
#include <stdint.h>
#include <fstream>
#include <base/Time.hpp>
#include "StreamDescription.hpp"
// #include "FileStream.hpp"
#include <iostream>

namespace pocolog_cpp
{

class Index
{
    struct IndexInfo {
        int64_t samplePosInLogFile;
        int64_t sampleTime;
    } __attribute__((packed));

    struct IndexPrologue {
        size_t numSamples;
        int16_t streamIdx;
        int32_t nameCrc;
        int64_t firstSampleTime;
        int64_t lastSampleTime;
        int64_t dataPos;
        int64_t streamDescPos;
    } __attribute__ ((packed));
    
    void loadIndex(size_t sampleNr);
    
public:
    Index(std::string indexFileName, size_t streamIdx);

    Index(const StreamDescription &desc, off_t posOfStreamDesc);
    
    bool matches(const StreamDescription &desc) const;
    
    /**
     * Writes the index to the Index File.
     * @param prologPos position, from the start of the file, where to write the prologue
     * @param indexDataPos position, from the start of the file, were to write the index data
     * @return the offset from the start of the file, were the index Data ENDS + 1
     * */
    off_t writeIndexToFile(std::fstream &indexFile, off_t prologPos, off_t indexDataPos);
    
    /**
     * Returns the size of the prologue written to the file
     * */
    static off_t getPrologueSize();
    
    void addSample(off_t filePosition, const base::Time &sampleTime);
    
    std::streampos getSamplePos(size_t sampleNr);
    base::Time getSampleTime(size_t sampleNr);
    
    const base::Time &getFirstSampleTime() const
    {
        return firstSampleTime;// base::Time::fromMicroseconds(prologue.firstSampleTime);
    }

    const base::Time &getLastSampleTime() const
    {
        return lastSampleTime; //base::Time::fromMicroseconds(prologue.lastSampleTime);
    }
    
    size_t getNumSamples() const
    {
        return prologue.numSamples;
    }
    
    std::streampos getDescriptionPos() const
    {
        return prologue.streamDescPos;
    };
    
    std::string getName()
    {
        return name;
    }
    
    ~Index();
private:
    std::string name;
    bool firstAdd;
    size_t curSampleNr;
    IndexInfo curIndexInfo;
    std::ifstream indexFile;
    std::vector<IndexInfo> buildBuffer;
    IndexPrologue prologue;
    base::Time firstSampleTime;
    base::Time lastSampleTime;
};
}
#endif // INDEX_H

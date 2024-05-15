#ifndef LOGFILE_H
#define LOGFILE_H
#include <string>
#include <vector>
#include <optional>
#include "Stream.hpp"
#include "Format.hpp"
#include "FileStream.hpp"
#include "OwnedValue.hpp"

namespace pocolog_cpp
{

class IndexFile;

class LogFile
{
    std::string filename;
    std::streampos firstBlockHeaderPos;
    std::streampos nextBlockHeaderPos;
    std::streampos curBlockHeaderPos;
    std::streampos curSampleHeaderPos;
    FileStream logFile;
    std::vector<IndexFile *> indexFiles;

    std::vector<Stream *> streams;
    std::vector<StreamDescription> descriptions;

    bool gotBlockHeader = false;
    struct BlockHeader curBlockHeader;
    bool gotSampleHeader = false;
    struct SampleHeaderData curSampleHeader;
    void readPrologue();

    std::vector<Stream*> createStreamsFromDescriptions(
        std::vector<StreamDescription> const& descriptions, IndexFile* indexFile
    );

public:
    LogFile(const std::string &fileName, bool verbose = true);
    ~LogFile();

    /** Move the read pointer at the beginning of the file, ready to read blocks */
    void rewind();

    /** Remove all built indexes from disk */
    void removeAllIndexes();

    std::string getFileName() const;
    std::string getFileBaseName() const;

    const std::vector<Stream *> &getStreams() const;
    const std::vector<StreamDescription> &getStreamDescriptions() const;

    bool loadStreamDescription(StreamDescription &result, std::streampos descPos);

    const BlockHeader &getCurBlockHeader() const;

    bool readNextBlockHeader(struct BlockHeader &curBlockHeade);
    bool readNextBlockHeader();
    bool readSampleHeader();
    bool checkSampleComplete();

    bool readCurBlock(std::vector<uint8_t> &blockData);

    std::streampos getSamplePos() const;
    std::streampos getBlockDataPos() const;
    std::streampos getBlockHeaderPos() const;
    const base::Time getSampleTime() const;
    size_t getSampleStreamIdx() const;
    bool getSampleData(std::vector<uint8_t>& buffer);

    OwnedValue getSample();
    OwnedValue getSample(std::vector<uint8_t>& buffer);

    using Sample = std::tuple<uint16_t, base::Time, OwnedValue>;
    std::optional<Sample> readNextSample();

    bool eof() const;

    Stream &getStream(const std::string streamName) const;

};
}
#endif // LOGFILE_H

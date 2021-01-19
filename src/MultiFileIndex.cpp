#include "MultiFileIndex.hpp"
#include "Index.hpp"
#include "IndexFile.hpp"
#include <map>
#include <iostream>
#include "InputDataStream.hpp"

namespace pocolog_cpp
{
    
MultiFileIndex::MultiFileIndex(const std::vector< std::string >& fileNames, bool verbose) : verbose(verbose)
{
    createIndex(fileNames);
}

MultiFileIndex::MultiFileIndex(bool verbose) : verbose(verbose)
{
    
}

MultiFileIndex::~MultiFileIndex()
{
    for(LogFile *file : logFiles)
        delete file;
}


bool MultiFileIndex::createIndex(const std::vector< LogFile* >& logfiles)
{
    //order all streams by time
    std::multimap<base::Time, IndexEntry> streamMap;

    globalSampleCount = 0;
    
    size_t globalStreamIdx = 0;
    
    
    for(std::vector< LogFile* >::const_iterator it = logfiles.begin(); it != logfiles.end(); it++)
    {
        LogFile *curLogfile = *it;
        for(std::vector< Stream* >::const_iterator it2 = curLogfile->getStreams().begin(); it2 != curLogfile->getStreams().end(); it2++)
        {
            Stream *stream = *it2;
            
            if(streamCheck)
            {
                if(!streamCheck(stream))
                    continue;
            }
            
            globalSampleCount += stream->getSize();
            
            IndexEntry entry;
            entry.stream = stream;
            entry.globalStreamIdx = globalStreamIdx;
            
            streamToGlobalIdx.insert(std::make_pair(stream, globalStreamIdx));
            
            globalStreamIdx++;
            streamMap.insert(std::make_pair(stream->getFistSampleTime(), entry));

            InputDataStream *dataStream = dynamic_cast<InputDataStream *>(entry.stream);
            if(dataStream)
            {
                combinedRegistry.merge(dataStream->getStreamRegistry());
            }
            streams.push_back(stream);
        }
        if(verbose)
            std::cout << "Loading logfile Done " << curLogfile->getFileName() << std::endl;
    }

    index.resize(globalSampleCount);
    
    int64_t globalSampleNr = 0;
    
    int lastPercentage = 0;
    
    if(verbose)
        std::cout << "Building multi file index " << std::endl;
    
    while(!streamMap.empty())
    {
        //remove stream from map
        IndexEntry curEntry = streamMap.begin()->second;
        streamMap.erase(streamMap.begin());
        
        //ignore empty stream here, no data to play back
        if(!curEntry.stream->getSize())
            continue;

        //add index sample
        index[globalSampleNr] = curEntry;

        curEntry.sampleNrInStream++;

        //check if we reached the end of the stream
        if(curEntry.sampleNrInStream < curEntry.stream->getSize())
        {
            //get time of next sample
            base::Time sampleTime = curEntry.stream->getFileIndex().getSampleTime(curEntry.sampleNrInStream);

            //reenter stream
            streamMap.insert(std::make_pair(sampleTime, curEntry));
        }        

        globalSampleNr++;
        int curPercentag = (globalSampleNr * 100 / globalSampleCount);
        if(lastPercentage != curPercentag)
        {
            lastPercentage = curPercentag;
            
	    if(verbose)
	    {
	    	std::cout << "\r" << lastPercentage << "% Done" << std::flush;
	    }
        }
    }
    
    if(verbose)
    {
        std::cout << "\r 100% Done";
        std::cout << "Processed " << globalSampleNr << " of " << globalSampleCount << " samples " << std::endl;
    }
    
    std::cout << std::endl;
    
    return true;

}

size_t MultiFileIndex::getGlobalStreamIdx(Stream* stream) const
{
    std::map<Stream *, size_t>::const_iterator it = streamToGlobalIdx.find(stream);
    if(it != streamToGlobalIdx.end())
        return it->second;
    
    throw std::runtime_error("Error, got unknown stream");
}


bool MultiFileIndex::createIndex(const std::vector< std::string >& fileNames)
{
    for(std::vector< std::string>::const_iterator it = fileNames.begin(); it != fileNames.end(); it++ )
    {
        if(verbose)
            std::cout << "Loading logfile " << *it << std::endl;
       
        LogFile *curLogfile = new LogFile(*it, verbose);
        logFiles.push_back(curLogfile);
        
        if(verbose)
            std::cout << "Loading logfile Done " << *it << std::endl;
    }

    return createIndex(logFiles);
}

void MultiFileIndex::registerStreamCheck(boost::function<bool (Stream *stream)> test)
{
    streamCheck = test;
}


   
    
}

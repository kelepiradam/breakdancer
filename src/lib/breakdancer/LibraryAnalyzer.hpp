#pragma once

#include "LibraryInfo.hpp"
#include "IBamReader.hpp"
#include <vector>

class LibraryAnalyzer {
    public:
        void analyze_bam(IBamReader& reads, Options const& opts, int number = -1);

    private:
        std::vector<LibraryInfo> _library_info;
        void _analyze_read(Read const& aln);
};

class LibraryInsertSizeAnalyzer : LibraryAnalyzer {
    private:
        void _analyze_read(Read const& aln);
};

//what we want is to do two things
//1. Accumulate information about both the counts of the flag types and the total number of types in the libraries
//2. Accumulate information about the insert sizes
//
//Both access the same alignments, but retain different information
//Both operate on, up to, the entire BAM
//Both store information about each library
//
//For example, insert size histogram would be generated by:
//1. Recording count information
//2. Recording insert size information
//
//If a config file has been provided, then that would have insert size metrics and those would need to be provided and used to set flags
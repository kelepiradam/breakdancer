#pragma once

#include "ReadCountsByLib.hpp"
#include "BasicRegion.hpp"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <functional>
#include <iterator>
#include <map>
#include <numeric>
#include <stdint.h>
#include <string>
#include <vector>

#include <boost/unordered_map.hpp>

class Options;
class BamConfig;
class IBamReader;

class BreakDancer {
public:
    typedef breakdancer::Read ReadType;
    typedef BasicRegion::ReadVector ReadVector;
    typedef std::vector<BasicRegion*> RegionData;
    typedef std::vector<ReadCountsByLib> RoiReadCounts;
    typedef std::map<std::string, std::vector<int> > ReadsToRegionsMap;

    BreakDancer(
        Options const& opts,
        BamConfig const& cfg,
        IBamReader& merged_reader,
        int max_read_window_size
        );

    ~BreakDancer();

    void push_read(ReadType& aln, bam_header_t const* bam_header);
    void build_connection(bam_header_t const* bam_header);
    int sum_of_region_sizes(std::vector<int> const& region_ids) const;

    void add_per_lib_read_counts_to_last_region(ReadCountsByLib const& counts) {
        assert(num_regions() > 0);

        size_t region_idx = num_regions()-1;
        if (region_idx >= _read_count_ROI_map.size())
            _read_count_ROI_map.resize(2*(region_idx+1));

        _read_count_ROI_map[region_idx] +=  counts;
    }

    void add_current_read_counts_to_last_region() {
        assert(num_regions() > 0);

        size_t region_idx = num_regions() - 1;
        if (region_idx >= _read_count_ROI_map.size())
            _read_count_ROI_map.resize(2*(region_idx+1));

        _read_count_ROI_map[region_idx] = nread_ROI;

        if (region_idx >= _read_count_FR_map.size())
            _read_count_FR_map.resize(2*(region_idx+1));

        _read_count_FR_map[region_idx] = nread_FR - nread_ROI;
    }

    void accumulate_reads_between_regions(ReadCountsByLib& acc, size_t begin, size_t end) {
        for(size_t i = begin; i < std::min(end, _read_count_ROI_map.size()); i++){
            acc += _read_count_ROI_map[i];

            // flanking region doesn't contain the first node
            if(i > begin && i < _read_count_FR_map.size())
                acc += _read_count_FR_map[i];
        }
    }

    uint32_t region_lib_read_count(size_t region_idx, std::string const& lib) const {
        return _region_lib_counts(region_idx, lib, _read_count_ROI_map);
    }

    void swap_reads_in_region(size_t region_idx, ReadVector& reads) {
        _regions[region_idx]->swap_reads(reads);
    }

    ReadVector const& reads_in_region(size_t region_idx) const {
        return _regions[region_idx]->reads();
    }

    bool region_exists(size_t region_idx) const {
        return (region_idx < _regions.size()) && _regions[region_idx];
    }

    void clear_region(size_t region_idx) {
        delete _regions[region_idx];
        _regions[region_idx] = 0;
    }

    size_t num_regions() const {
        return _regions.size();
    }

    size_t add_region(BasicRegion* r) {
        _regions.push_back(r);
        return _regions.size() - 1;
    }

    BasicRegion const& get_region_data(size_t region_idx) const {
        return *_regions[region_idx];
    }

    void set_max_read_window_size(int val) {
        _max_read_window_size = val;
    }

    void process_breakpoint(bam_header_t const* bam_header);
    void process_final_region(bam_header_t const* bam_header);

    void run();

private:
    uint32_t _region_lib_counts(size_t region_idx, std::string const& lib, RoiReadCounts const& x) const {
        if (region_idx >= x.size())
            return 0;
        RoiReadCounts::value_type::const_iterator found = x[region_idx].find(lib);
        if (found != x[region_idx].end())
            return found->second;
        return 0;
    }

private:
    Options const& _opts;
    BamConfig const& _cfg;
    IBamReader& _merged_reader;
    int _max_read_window_size;

    RoiReadCounts _read_count_ROI_map;
    RoiReadCounts _read_count_FR_map;
    RegionData _regions;

    ReadCountsByLib nread_ROI;
    ReadCountsByLib nread_FR;
    ReadVector reads_in_current_region;

    bool _collecting_normal_reads;
    int _nnormal_reads;
    int _ntotal_nucleotides;
    int _max_readlen;
    int _buffer_size;

    int _region_start_tid;
    int _region_start_pos;
    int _region_end_tid; // global (chr, should be int in samtools)
    int _region_end_pos; // global

public:
    ReadsToRegionsMap _read_regions;
    std::map<std::string, float> read_density;

    void process_sv(std::vector<int> const& snodes, std::set<int>& free_nodes, bam_header_t const* bam_header);
};

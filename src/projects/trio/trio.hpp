#pragma once


#include <array>
#include <vector>
#include <unordered_map>
#include <sequences/seqio.hpp>
#include "common/logging.hpp"
#include <common/string_utils.hpp>
#include "lja/multi_graph.hpp"
#include "lja/subdataset_processing.hpp"
#include "haplo_stats.hpp"

namespace trio {

struct HaplotypeRemover {
    multigraph::MultiGraph &mg;
    haplo_map_type haplotypes;
    logging::Logger &logger_;
    Haplotype haplotype_;
    static const size_t MAX_TIP_LENGTH = 1000000;
    static constexpr double BULGE_MULTIPLICATIVE_CUTOFF = 1.2;
//Bridges of wrong haplotype longer that this cutoff are deleted, shorter are saved;
    const size_t saved_bridge_cutoff;
    std::experimental::filesystem::path out_dir;
    std::unordered_map<std::string, std::string> bulges;

    HaplotypeRemover(logging::Logger &logger, multigraph::MultiGraph &mg,
                     const std::experimental::filesystem::path &haployak, const Haplotype haplotype,
                     const std::experimental::filesystem::path &out_dir, const size_t saved_bridge_cutoff);

    void process();

    void deleteEdgeHaplo(int eid);

    void compressAllVertices();

    void cleanGraph();

    std::unordered_map<std::string, std::string> getBulgeLabels();

    void updateFixableHaplotypes();

    void updateAmbiguousHaplotypes();

    void removeHaplotype();
};

std::experimental::filesystem::path simplifyHaplo(logging::Logger &logger, size_t threads,
                                                  const std::experimental::filesystem::path &output_file,
                                                  const std::experimental::filesystem::path &diplo_graph,
                                                  const std::experimental::filesystem::path &haployak,
                                                  const char haplotype,
                                                  const std::experimental::filesystem::path &corrected_reads,
                                                  const io::Library &reads,
                                                  const std::experimental::filesystem::path &dir,
                                                  const size_t saved_bridge_cutoff);

}
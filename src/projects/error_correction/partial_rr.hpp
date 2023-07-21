#pragma once

#include "dbg/sparse_dbg.hpp"
#include "dbg/graph_alignment_storage.hpp"
#include "diploidy_analysis.hpp"

std::vector<DBGGraphPath> ResolveBulgePath(const BulgePath &bulgePath, const RecordStorage &reads) {
    VERIFY(bulgePath.size() > 0);
    if(bulgePath.size() == 1)
        return {DBGGraphPath() + *bulgePath[0].first};
    std::vector<DBGGraphPath> res;
    size_t left = 0;
    while(left < bulgePath.size() && bulgePath[left].first == bulgePath[left].second)
        left++;
    if(left == bulgePath.size()) {
        return {DBGGraphPath(bulgePath.randomPath())};
    }
    DBGGraphPath path1, path2;
    {
        dbg::Vertex &sv = bulgePath.getVertex(left + 1);
        const VertexRecord &vrec = reads.getRecord(sv.rc());
        path1 = vrec.getFullUniqueExtension(bulgePath[left].first->rc().truncSeq().Subseq(0, 1), 2,
                                            1).RC().getAlignment();
        path2 = vrec.getFullUniqueExtension(bulgePath[left].second->rc().truncSeq().Subseq(0, 1), 2,
                                            1).RC().getAlignment();
    }
    Sequence s;
    for(size_t i = left + 1; i < bulgePath.size(); i++) {
        if(!bulgePath.isBulge(i))
            s = s + bulgePath[i].first->truncSeq().Subseq(0, 1);
        else {
            Sequence s11 =
                    path1.back().contig().truncSeq().Subseq(0, 1) + s + bulgePath[i].first->truncSeq().Subseq(0, 1);
            Sequence s12 =
                    path1.back().contig().truncSeq().Subseq(0, 1) + s + bulgePath[i].second->truncSeq().Subseq(0, 1);
            Sequence s21 =
                    path2.back().contig().truncSeq().Subseq(0, 1) + s + bulgePath[i].first->truncSeq().Subseq(0, 1);
            Sequence s22 =
                    path2.back().contig().truncSeq().Subseq(0, 1) + s + bulgePath[i].second->truncSeq().Subseq(0, 1);
            const VertexRecord &vrec = reads.getRecord(path1.back().contig().getStart());
            size_t n11 = vrec.countStartsWith(s11);
            size_t n12 = vrec.countStartsWith(s12);
            size_t n21 = vrec.countStartsWith(s21);
            size_t n22 = vrec.countStartsWith(s22);
            DBGGraphPath repeat = dbg::CompactPath(path1.finish(), s).getAlignment();
            s = {};
            path1 += repeat;
            path2 += repeat;
            if((n11 + n22 != 0 && n12 + n21 != 0) || (n11 + n22 == 0 && n12 + n21 == 0)) {
                res.emplace_back(std::move(path1));
                res.emplace_back(std::move(path2));
                path1 = repeat + *bulgePath[i].first;
                path2 = repeat + *bulgePath[i].second;
            } else {
                if(n11 + n22 > 0) {
                    path1 += *bulgePath[i].first;
                    path2 += *bulgePath[i].second;
                } else {
                    VERIFY(n12 + n21 > 0);
                    path1 += *bulgePath[i].second;
                    path2 += *bulgePath[i].first;
                }
            }
        }
    }
    {
        dbg::Vertex &sv = path1.back().contig().getStart();
        VERIFY(sv == path2.back().contig().getStart());
        const VertexRecord &vrec = reads.getRecord(sv);
        path1 += vrec.getFullUniqueExtension(path1.back().contig().truncSeq().Subseq(0, 1), 2,
                                             1).getAlignment().subPath(1);
        path2 += vrec.getFullUniqueExtension(path2.back().contig().truncSeq().Subseq(0, 1), 2,
                                             1).getAlignment().subPath(1);
        res.emplace_back(std::move(path1));
        res.emplace_back(std::move(path2));
    }
    return std::move(res);
}

std::vector<DBGGraphPath> PartialRR(logging::Logger &logger, size_t threads, dbg::SparseDBG &dbg, const RecordStorage &reads) {
    logger.info() << "Performing partial repeat resolution" << std::endl;
    BulgePathFinder bulges(dbg, 1);
    std::vector<DBGGraphPath> res;
    for(BulgePath &bulgePath : bulges.paths) {
        std::vector<DBGGraphPath> resolved = ResolveBulgePath(bulgePath, reads);
        res.insert(res.end(), resolved.begin(), resolved.end());
    }
    logger.info() << "Finished partial repeat resolution. Generated " << res.size() << " pseudoreads" << std::endl;
    return std::move(res);
}
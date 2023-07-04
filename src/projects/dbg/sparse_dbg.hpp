//
// Created by anton on 7/22/20.
//

#pragma once
#include "sequences/sequence.hpp"
#include "sequences/seqio.hpp"
#include "common/omp_utils.hpp"
#include "common/logging.hpp"
#include "common/rolling_hash.hpp"
#include "common/hash_utils.hpp"
#include <common/oneline_utils.hpp>
#include <common/iterator_utils.hpp>
#include <utility>
#include <vector>
#include <numeric>
#include <unordered_map>
#include <unordered_set>
#include <forward_list>
#include <list>

namespace dbg {
    enum EdgeMarker {
        incorrect,
        suspicious,
        common,
        possible_break,
        correct,
        unique,
        repeat
    };
    bool IsMarkerCorrect(EdgeMarker marker);
    class Vertex;

    class SparseDBG;

    class Edge {
    private:
        Vertex *start;
        Vertex *finish;
        mutable size_t cov;
        static Edge _fake;
        mutable EdgeMarker marker = EdgeMarker::common;
        Sequence seq;
        Edge(Vertex *_start, Vertex *_end, Sequence _seq) :
                start(_start), finish(_end), cov(0), extraInfo(-1), seq(std::move(_seq)) {
        }
        friend class dbg::Vertex;
    public:
        typedef int id_type;
        mutable size_t extraInfo;//TODO remove
        const Sequence &getSeq() const {return seq;}
        friend class Vertex;
        bool is_reliable = false;
        Edge(Vertex &_start, Vertex &_end, Sequence _seq) :
                start(&_start), finish(&_end), cov(0), extraInfo(-1), seq(std::move(_seq)) {
        }
        Edge() : start(nullptr), finish(nullptr), cov(0), extraInfo(-1), seq() {}

        static Edge &fake() {return _fake;}
        std::string getId() const;
        std::string oldId() const;
        std::string getShortId() const;
        EdgeMarker getMarker() const {return marker;};
        bool checkCorrect() const {return IsMarkerCorrect(marker);}
        Vertex *getFinish() const;
        Vertex *getStart() const;
        size_t getTipSize() const;
        void  setTipSize(size_t val) const;
        size_t updateTipSize() const;
        size_t size() const;
        double getCoverage() const;
        size_t intCov() const;
        Edge &rc() const;
        Edge &sparseRcEdge() const;
        Sequence firstNucl() const;
        Sequence kmerSeq(size_t pos) const;
        Sequence suffix(size_t pos) const;
        std::string str() const;

        bool operator==(const Edge &other) const;
        bool operator!=(const Edge &other) const;
        bool operator<(const Edge &other) const;
        bool operator>(const Edge &other) const;
        bool operator<=(const Edge &other) const;

        void bindTip(Vertex &start, Vertex &end);
        void incCov(size_t val) const;
        void mark(EdgeMarker _marker) const { marker = _marker;};
    };

//    std::ostream& operator<<(std::ostream& os, const Edge& edge);



    class Vertex {
    private:
        friend class SparseDBG;
        mutable std::list<Edge> outgoing_{};
        Vertex *rc_;
        hashing::htype hash_;
        omp_lock_t writelock = {};
        size_t coverage_ = 0;
        bool canonical = false;
        bool mark_ = false;
        explicit Vertex(hashing::htype hash, Vertex *_rc);
        Sequence seq;
    public:

        explicit Vertex(hashing::htype hash = 0);
        Vertex(const Vertex &) = delete;
        ~Vertex();

        void mark() {mark_ = true;}
        void unmark() {mark_ = false;}
        bool marked() const {return mark_;}
        hashing::htype hash() const {return hash_;}
        Vertex &rc() {return *rc_;}
        const Vertex &rc() const {return *rc_;}
        void setSeq(Sequence _seq);
        const Sequence &getSeq() const {return seq;}
//        void clearSequence();
        void lock() {omp_set_lock(&writelock);}
        void unlock() {omp_unset_lock(&writelock);}
        std::list<Edge>::iterator begin() const {return outgoing_.begin();}
        std::list<Edge>::iterator end() const {return outgoing_.end();}
        size_t outDeg() const {return outgoing_.size();}
        size_t inDeg() const {return rc_->outgoing_.size();}
        Edge &front() const {return outgoing_.front();}
        Edge &back() const {return outgoing_.back();}
//        Edge &operator[](size_t ind) const {
//            if(ind == outgoing_.size() - 1)
//                return outgoing_.back();
//            auto it = outgoing_.begin();
//            for(size_t i = 0; i < ind; i++){
//                ++it;
//            }
//            return *it;
//        }


        size_t coverage() const;
        bool isCanonical() const;
        bool isCanonical(const Edge &edge) const;
        void clear();
        void clearOutgoing();
        void sortOutgoing();
        void checkConsistency() const;
        std::string getId() const;
        std::string oldId() const;
        std::string getShortId() const;
        void incCoverage();
        Edge &addEdgeLockFree(const Edge &edge);
        void addEdge(const Edge &e);
        void addSequence(const Sequence &edge_seq) {addEdge(Edge(this, nullptr, edge_seq));}
        Edge &getOutgoing(unsigned char c) const;
        bool hasOutgoing(unsigned char c) const;
        bool isJunction() const;

        bool operator==(const Vertex &other) const;
        bool operator!=(const Vertex &other) const;
        bool operator<(const Vertex &other) const;
        bool operator>(const Vertex &other) const;
    };

    struct EdgePosition {
        Edge *edge;
        size_t pos;

        EdgePosition(Edge &_edge, size_t _pos) : edge(&_edge), pos(_pos) {VERIFY(pos >= 0 && pos <= edge->size());}
        EdgePosition() : edge(nullptr), pos(0) {}

        Sequence kmerSeq() const {return edge->kmerSeq(pos);}
        unsigned char lastNucl() const {return edge->getSeq()[pos - 1];}
        bool isBorder() const {return pos == 0 || pos == edge->size();}

        std::vector<EdgePosition> step() const;
        EdgePosition RC() const {return {edge->rc(), edge->size() - pos};}
    };

    class SparseDBG {
    public:
        typedef std::unordered_map<hashing::htype , Vertex, hashing::alt_hasher<hashing::htype>> vertex_map_type;
        typedef std::unordered_map<hashing::htype, Vertex, hashing::alt_hasher<hashing::htype>>::iterator vertex_iterator_type;
        typedef std::unordered_map<hashing::htype, EdgePosition, hashing::alt_hasher<hashing::htype>> anchor_map_type;
    private:
//    TODO: replace with perfect hash map? It is parallel, maybe faster and compact.
        vertex_map_type v;
        anchor_map_type anchors;
        hashing::RollingHash hasher_;
        bool anchors_filled = false;

//    Be careful since hash does not define vertex. Rc vertices share the same hash
        Vertex &innerAddVertex(hashing::htype h) {
            return v.emplace(std::piecewise_construct, std::forward_as_tuple(h),
                             std::forward_as_tuple(h)).first->second;
        }

    public:

        template<class Iterator>
        SparseDBG(Iterator begin, Iterator end, hashing::RollingHash _hasher) : hasher_(_hasher) {
            while (begin != end) {
                hashing::htype hash = *begin;
                if (v.find(hash) == v.end())
                    addVertex(hash);
                ++begin;
            }
        }
        explicit SparseDBG(hashing::RollingHash _hasher) : hasher_(_hasher) {}
        SparseDBG(SparseDBG &&other) = default;
        SparseDBG &operator=(SparseDBG &&other) = default;
        SparseDBG(const SparseDBG &other) noexcept = delete;

        SparseDBG Subgraph(std::vector<Segment<Edge>> &pieces);
        SparseDBG SplitGraph(const std::vector<EdgePosition> &breaks);
        SparseDBG AddNewSequences(logging::Logger &logger, size_t threads, const std::vector<Sequence> &new_seqs);

        const hashing::RollingHash &hasher() const {return hasher_;}
        bool containsVertex(const hashing::htype &hash) const {return v.find(hash) != v.end();}
        Vertex &getVertex(const hashing::KWH &kwh);
        Vertex &getVertex(const Sequence &seq);
        Vertex &getVertex(hashing::htype hash, bool canonical = true) {return canonical ? v.find(hash)->second : v.find(hash)->second.rc();}
        Vertex &getVertex(const Vertex &other_graph_vertex);
        std::array<Vertex *, 2> getVertices(hashing::htype hash);
//        const Vertex &getVertex(const hashing::KWH &kwh) const;
        bool isAnchor(hashing::htype hash) const {return anchors.find(hash) != anchors.end();}
        EdgePosition getAnchor(const hashing::KWH &kwh);
        size_t size() const {return v.size();}
        bool alignmentReady() const {return anchors_filled;}

        void checkConsistency(size_t threads, logging::Logger &logger);
        void checkDBGConsistency(size_t threads, logging::Logger &logger);
        void checkSeqFilled(size_t threads, logging::Logger &logger);
        void fillAnchors(size_t w, logging::Logger &logger, size_t threads);
        void fillAnchors(size_t w, logging::Logger &logger, size_t threads, const std::unordered_set<hashing::htype, hashing::alt_hasher<hashing::htype>> &to_add);
        void processRead(const Sequence &seq);
        void processEdge(Vertex &vertex, Sequence old_seq);
        void processEdge(Edge &other_graph_edge);
        Vertex &bindTip(Vertex &start, Edge &tip);
        void removeIsolated();
        void removeMarked();

        void resetMarkers();

        void addVertex(hashing::htype h) {innerAddVertex(h);}
        Vertex &addVertex(const hashing::KWH &kwh);
        Vertex &addVertex(const Sequence &seq);
        Vertex &addVertex(const Vertex &other_graph_vertex);
//        Edge &addEdge(Vertex &from, Vertex &to, Sequence seq, int id = 0) {
//            Edge edge(from, to, seq, id);
//        }


        std::vector<hashing::KWH> extractVertexPositions(const Sequence &seq, size_t max = size_t(-1)) const;
        void printFastaOld(const std::experimental::filesystem::path &out);

        IterableStorage<ApplyingIterator<vertex_iterator_type, Vertex, 2>> vertices(bool unique = false);
        IterableStorage<ApplyingIterator<vertex_iterator_type, Vertex, 2>> verticesUnique();
        IterableStorage<ApplyingIterator<vertex_iterator_type, Edge, 8>> edges(bool unique = false);
        IterableStorage<ApplyingIterator<vertex_iterator_type, Edge, 8>> edgesUnique();
        typename vertex_map_type::iterator begin() {return v.begin();}
        typename vertex_map_type::iterator end() {return v.end();}
        typename vertex_map_type::const_iterator begin() const {return v.begin();}
        typename vertex_map_type::const_iterator end() const {return v.end();}
    };

}

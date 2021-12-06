//
// Created by Andrey Bzikadze on 11/10/21.
//

#include "mdbg_topology.hpp"
using namespace repeat_resolution;

void RRVertexProperty::IncLeft(std::list<char> prefix) {
  seq.splice(seq.begin(), std::move(prefix));
}
void RRVertexProperty::IncRight(std::list<char> suffix) {
  seq.splice(seq.end(), std::move(suffix));
}
void RRVertexProperty::DecLeft(const uint64_t inc) {
  for (uint64_t i = 0; i < inc; ++i) {
    seq.pop_front();
  }
}
void RRVertexProperty::DecRight(const uint64_t inc) {
  for (uint64_t i = 0; i < inc; ++i) {
    seq.pop_back();
  }
}

std::list<char> RRVertexProperty::GetSeqPrefix(size_t len,
                                               int64_t shift) const {
  VERIFY(seq.size() >= len);
  auto it = seq.cbegin();
  if (shift > 0) {
    std::advance(it, shift);
  }
  std::list<char> prefix;
  for (auto i = 0; i < len; ++i, ++it) {
    prefix.emplace_back(*it);
  }
  return prefix;
}

std::list<char> RRVertexProperty::GetSeqSuffix(size_t len,
                                               int64_t shift) const {
  VERIFY(seq.size() >= len);
  auto it = seq.crbegin();
  if (shift > 0) {
    std::advance(it, shift);
  }
  std::list<char> suffix;
  for (auto i = 0; i < len; ++i, ++it) {
    suffix.emplace_front(*it);
  }
  return suffix;
}

std::ostream &repeat_resolution::operator<<(std::ostream &os,
                                            const RRVertexProperty &vertex) {
  os << vertex.size();
  return os;
}

std::ostream &
repeat_resolution::operator<<(std::ostream &os,
                              const RREdgeProperty &edge_property) {
  os << edge_property.size() << "\\n" << edge_property.IsUnique();
  return os;
}

bool repeat_resolution::operator==(const RRVertexProperty &lhs,
                                   const RRVertexProperty &rhs) {
  return lhs.Seq() == rhs.Seq() and lhs.IsFrozen() == rhs.IsFrozen();
}

[[nodiscard]] int64_t RREdgeProperty::size() const {
  if (not seq.empty()) {
    VERIFY(size_ == seq.size())
  }
  return size_;
}

void RREdgeProperty::Merge(RRVertexProperty vertex, RREdgeProperty rhs) {
  // in case current edge has negative length, there is an overlap b/w vertices
  int64_t vertex_size = vertex.size();
  if (size_ < 0) {
    vertex.DecLeft(std::min((int64_t)vertex.size(), -size_));
  }
  size_ += (int64_t)vertex_size + rhs.size_;
  if (rhs.size_ < 0) {
    vertex.DecRight(std::min((int64_t)vertex.size(), -rhs.size_));
  }
  seq.splice(seq.end(), std::move(vertex.seq));
  seq.splice(seq.end(), std::move(rhs.seq));
  if (size_ > 0) {
    VERIFY(seq.size() == size_);
  }
  if (rhs.unique) {
    unique = true;
  }
}

std::list<char> RREdgeProperty::ExtractSeqPrefix(const size_t len) {
  VERIFY(seq.size() >= len);
  std::list<char> prefix;
  for (int i = 0; i < len; ++i) {
    prefix.emplace_back(seq.front());
    seq.pop_front();
  }
  size_ -= len;
  return prefix;
}

std::list<char> RREdgeProperty::ExtractSeqSuffix(const size_t len) {
  VERIFY(seq.size() >= len);
  std::list<char> suffix;
  for (int i = 0; i < len; ++i) {
    suffix.emplace_back(seq.back());
    seq.pop_back();
  }
  size_ -= len;
  return suffix;
}

void RREdgeProperty::ShortenWithEmptySeq(size_t len) {
  VERIFY(seq.empty());
  size_ -= len;
}

bool repeat_resolution::operator==(const RREdgeProperty &lhs,
                                   const RREdgeProperty &rhs) {
  return lhs.Index() == rhs.Index();
}

bool repeat_resolution::operator!=(const RREdgeProperty &lhs,
                                   const RREdgeProperty &rhs) {
  return not(lhs == rhs);
}

RREdgeProperty repeat_resolution::Add(const RRVertexProperty &lhs,
                                      const RRVertexProperty &rhs,
                                      const uint64_t index) {
  VERIFY(lhs.size() == rhs.size());
  // can assign uniqueness more carefully if we pass left&right edges
  return RREdgeProperty(/*index=*/index, /*(inner)seq=*/{},
                        /*size=*/-((int64_t)lhs.size()) + 1, /*unique=*/false);
}

bool repeat_resolution::operator==(const SuccinctEdgeInfo &lhs,
                                   const SuccinctEdgeInfo &rhs) {
  return lhs.start_ind == rhs.start_ind and lhs.start_prop == rhs.start_prop and
         lhs.end_ind == rhs.end_ind and lhs.end_prop == rhs.end_prop and
         lhs.infix_size == rhs.infix_size and lhs.seq == rhs.seq and
         lhs.unique == rhs.unique;
}

bool repeat_resolution::operator!=(const SuccinctEdgeInfo &lhs,
                                   const SuccinctEdgeInfo &rhs) {
  return !repeat_resolution::operator==(lhs, rhs);
}
/*!
 ******************************************************************************
 *
 * \file
 *
 * \brief   Header file containing RAJA reduction templates for CUDA execution.
 *
 *          These methods should work on any platform that supports
 *          CUDA devices.
 *
 ******************************************************************************
 */

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
// Copyright (c) 2016-21, Lawrence Livermore National Security, LLC
// and RAJA project contributors. See the RAJA/COPYRIGHT file for details.
//
// SPDX-License-Identifier: (BSD-3-Clause)
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

#ifndef RAJA_util_GPUReducerTally_HPP
#define RAJA_util_GPUReducerTally_HPP

#include "RAJA/config.hpp"

#include <type_traits>

#include "RAJA/util/macros.hpp"
#include "RAJA/util/basic_mempool.hpp"
#include "RAJA/util/mutex.hpp"
#include "RAJA/util/types.hpp"

#if defined(RAJA_ENABLE_CUDA)
#include "RAJA/policy/cuda/MemUtils_CUDA.hpp"
#include "RAJA/policy/cuda/raja_cudaerrchk.hpp"
#endif

#if defined(RAJA_ENABLE_HIP)
#include "RAJA/policy/hip/MemUtils_HIP.hpp"
#include "RAJA/policy/hip/raja_hiperrchk.hpp"
#endif

namespace RAJA
{

namespace detail
{

template < typename Resource >
struct ResourceInfo;

#if defined(RAJA_ENABLE_CUDA)

template < >
struct ResourceInfo<resources::Cuda>
{
  using pinned_mempool_type = cuda::pinned_mempool_type;
  using identifier = cudaStream_t;
  static cudaStream_t get_identifier(resources::Cuda& r)
  {
    return r.get_stream();
  }
  static void synchronize(cudaStream_t s)
  {
    cuda::synchronize(s);
  }
};

#endif


#if defined(RAJA_ENABLE_HIP)

template < >
struct ResourceInfo<resources::Hip>
{
  using pinned_mempool_type = hip::pinned_mempool_type;
  using identifier = hipStream_t;
  static hipStream_t get_identifier(resources::Hip& r)
  {
    return r.get_stream();
  }
  static void synchronize(hipStream_t s)
  {
    hip::synchronize(s);
  }
};

#endif


//! Object that manages pinned memory buffers for reduction results
//  use one per reducer object
template <typename T, typename Resource>
class GPUReducerTally
{
  using resource_info = ResourceInfo<Resource>;
  using pinned_mempool_type = typename resource_info::pinned_mempool_type;
  using identifier = typename resource_info::identifier;
public:
  //! Object put in Pinned memory with value and pointer to next Node
  struct Node {
    Node* next;
    T value;
  };
  //! Object per id to keep track of pinned memory nodes
  struct ResourceNode {
    ResourceNode* next;
    identifier id;
    Node* node_list;
  };

  //! Iterator over streams used by reducer
  class StreamIterator
  {
  public:
    StreamIterator() = delete;

    StreamIterator(ResourceNode* rn) : m_rn(rn) {}

    const StreamIterator& operator++()
    {
      m_rn = m_rn->next;
      return *this;
    }

    StreamIterator operator++(int)
    {
      StreamIterator ret = *this;
      this->operator++();
      return ret;
    }

    identifier& operator*() { return m_rn->id; }

    bool operator==(const StreamIterator& rhs) const
    {
      return m_rn == rhs.m_rn;
    }

    bool operator!=(const StreamIterator& rhs) const
    {
      return !this->operator==(rhs);
    }

  private:
    ResourceNode* m_rn;
  };

  //! Iterator over all values generated by reducer
  class ResourceNodeIterator
  {
  public:
    ResourceNodeIterator() = delete;

    ResourceNodeIterator(ResourceNode* rn, Node* n) : m_rn(rn), m_n(n) {}

    const ResourceNodeIterator& operator++()
    {
      if (m_n->next) {
        m_n = m_n->next;
      } else if (m_rn->next) {
        m_rn = m_rn->next;
        m_n = m_rn->node_list;
      } else {
        m_rn = nullptr;
        m_n = nullptr;
      }
      return *this;
    }

    ResourceNodeIterator operator++(int)
    {
      ResourceNodeIterator ret = *this;
      this->operator++();
      return ret;
    }

    T& operator*() { return m_n->value; }

    bool operator==(const ResourceNodeIterator& rhs) const
    {
      return m_n == rhs.m_n;
    }

    bool operator!=(const ResourceNodeIterator& rhs) const
    {
      return !this->operator==(rhs);
    }

  private:
    ResourceNode* m_rn;
    Node* m_n;
  };

  GPUReducerTally() : stream_list(nullptr) {}

  GPUReducerTally(const GPUReducerTally&) = delete;

  //! get begin iterator over streams
  StreamIterator streamBegin() { return {stream_list}; }

  //! get end iterator over streams
  StreamIterator streamEnd() { return {nullptr}; }

  //! get begin iterator over values
  ResourceNodeIterator begin()
  {
    return {stream_list, stream_list ? stream_list->node_list : nullptr};
  }

  //! get end iterator over values
  ResourceNodeIterator end() { return {nullptr, nullptr}; }

  //! get new value for use in id
  T* new_value(identifier id)
  {
#if defined(RAJA_ENABLE_OPENMP) && defined(_OPENMP)
    lock_guard<omp::mutex> lock(m_mutex);
#endif
    ResourceNode* rn = stream_list;
    while (rn) {
      if (rn->id == id) break;
      rn = rn->next;
    }
    if (!rn) {
      rn = (ResourceNode*)malloc(sizeof(ResourceNode));
      rn->next = stream_list;
      rn->id = id;
      rn->node_list = nullptr;
      stream_list = rn;
    }
    Node* n = pinned_mempool_type::getInstance().template malloc<Node>(1);
    n->next = rn->node_list;
    rn->node_list = n;
    return &n->value;
  }

  //! synchronize all streams used
  void synchronize_streams()
  {
    auto end = streamEnd();
    for (auto s = streamBegin(); s != end; ++s) {
      resource_info::synchronize(*s);
    }
  }

  //! all values used in all streams
  void free_list()
  {
    while (stream_list) {
      ResourceNode* rn = stream_list;
      while (rn->node_list) {
        Node* n = rn->node_list;
        rn->node_list = n->next;
        pinned_mempool_type::getInstance().free(n);
      }
      stream_list = rn->next;
      free(rn);
    }
  }

  ~GPUReducerTally() { free_list(); }

#if defined(RAJA_ENABLE_OPENMP) && defined(_OPENMP)
  omp::mutex m_mutex;
#endif

private:
  ResourceNode* stream_list;
};




}  // end namespace detail

}  // namespace RAJA

#endif  // closing endif for header file include guard

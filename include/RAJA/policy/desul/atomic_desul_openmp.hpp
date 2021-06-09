//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
// Copyright (c) 2016-21, Lawrence Livermore National Security, LLC
// and RAJA project contributors. See the RAJA/COPYRIGHT file for details.
//
// SPDX-License-Identifier: (BSD-3-Clause)
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

#ifndef RAJA_policy_atomic_desul_HPP
#define RAJA_policy_atomic_desul_HPP

#include "RAJA/config.hpp"

#if defined(RAJA_ENABLE_OPENMP)

#include "RAJA/util/macros.hpp"

#include "RAJA/policy/atomic_builtin.hpp"

#include "desul/atomics.hpp"

// Default desul options for RAJA
using raja_default_desul_order = desul::MemoryOrderRelaxed;
using raja_default_desul_scope = desul::MemoryScopeDevice;

namespace RAJA
{

#if defined(RAJA_COMPILER_MSVC)

// For MS Visual C, just default to builtin_atomic for everything
using omp_atomic = builtin_atomic;

#else  // not defined RAJA_COMPILER_MSVC

struct omp_atomic {};

RAJA_SUPPRESS_HD_WARN
template <typename T>
RAJA_HOST_DEVICE
RAJA_INLINE T
atomicAdd(omp_atomic, T volatile *acc, T value) {
  return desul::atomic_fetch_add(const_cast<T*>(acc)
                                , value
                                , raja_default_desul_order{}
                                , raja_default_desul_scope{});
}

RAJA_SUPPRESS_HD_WARN
template <typename T>
RAJA_HOST_DEVICE
RAJA_INLINE T
atomicSub(omp_atomic, T volatile *acc, T value) {
  return desul::atomic_fetch_sub(const_cast<T*>(acc)
                                , value
                                , raja_default_desul_order{}
                                , raja_default_desul_scope{});
}

RAJA_SUPPRESS_HD_WARN
template <typename T>
RAJA_HOST_DEVICE
RAJA_INLINE T atomicMin(omp_atomic, T volatile *acc, T value)
{
  return desul::atomic_fetch_min(const_cast<T*>(acc)
                                , value
                                , raja_default_desul_order{}
                                , raja_default_desul_scope{});
}

RAJA_SUPPRESS_HD_WARN
template <typename T>
RAJA_HOST_DEVICE
RAJA_INLINE T atomicMax(omp_atomic, T volatile *acc, T value)
{
  return desul::atomic_fetch_max(const_cast<T*>(acc)
                                , value
                                , raja_default_desul_order{}
                                , raja_default_desul_scope{});
}


RAJA_SUPPRESS_HD_WARN
template <typename T>
RAJA_HOST_DEVICE
RAJA_INLINE T atomicInc(omp_atomic, T volatile *acc)
{
  return desul::atomic_fetch_inc(const_cast<T*>(acc)
                                , raja_default_desul_order{}
                                , raja_default_desul_scope{});
}


RAJA_SUPPRESS_HD_WARN
template <typename T>
RAJA_HOST_DEVICE
RAJA_INLINE T atomicInc(omp_atomic, T volatile *acc, T val)
{
  return desul::atomic_fetch_add(const_cast<T*>(acc)
                                , val
                                , raja_default_desul_order{}
                                , raja_default_desul_scope{});
}


RAJA_SUPPRESS_HD_WARN
template <typename T>
RAJA_HOST_DEVICE
RAJA_INLINE T atomicDec(omp_atomic, T volatile *acc)
{
  return desul::atomic_fetch_dec(const_cast<T*>(acc)
                                , raja_default_desul_order{}
                                , raja_default_desul_scope{});
}


RAJA_SUPPRESS_HD_WARN
template <typename T>
RAJA_HOST_DEVICE
RAJA_INLINE T atomicDec(omp_atomic, T volatile *acc, T val)
{
  return desul::atomic_fetch_sub(const_cast<T*>(acc)
                                , val
                                , raja_default_desul_order{}
                                , raja_default_desul_scope{});
}

RAJA_SUPPRESS_HD_WARN
template <typename T>
RAJA_HOST_DEVICE
RAJA_INLINE T atomicAnd(omp_atomic, T volatile *acc, T value)
{
  return desul::atomic_fetch_and(const_cast<T*>(acc)
                                , value
                                , raja_default_desul_order{}
                                , raja_default_desul_scope{});
}

RAJA_SUPPRESS_HD_WARN
template <typename T>
RAJA_HOST_DEVICE
RAJA_INLINE T atomicOr(omp_atomic, T volatile *acc, T value)
{
  return desul::atomic_fetch_or(const_cast<T*>(acc)
                                , value
                                , raja_default_desul_order{}
                                , raja_default_desul_scope{});
}

RAJA_SUPPRESS_HD_WARN
template <typename T>
RAJA_HOST_DEVICE
RAJA_INLINE T atomicXor(omp_atomic, T volatile *acc, T value)
{
  return desul::atomic_fetch_xor(const_cast<T*>(acc)
                                , value
                                , raja_default_desul_order{}
                                , raja_default_desul_scope{});
}

RAJA_SUPPRESS_HD_WARN
template <typename T>
RAJA_HOST_DEVICE
RAJA_INLINE T atomicExchange(omp_atomic, T volatile *acc, T value)
{
  return desul::atomic_exchange(const_cast<T*>(acc)
                                , value
                                , raja_default_desul_order{}
                                , raja_default_desul_scope{});
}

RAJA_SUPPRESS_HD_WARN
template <typename T>
RAJA_HOST_DEVICE
RAJA_INLINE T atomicCAS(omp_atomic, T volatile *acc, T compare, T value)
{
  return desul::atomic_compare_exchange_strong(const_cast<T*>(acc)
                                , compare
                                , value
                                , raja_default_desul_order{} // success order
                                , raja_default_desul_order{} // failure order
                                , raja_default_desul_scope{});
}

#endif  // not defined RAJA_COMPILER_MSVC

}  // namespace RAJA

#endif  // RAJA_ENABLE_OPENMP
#endif // guard

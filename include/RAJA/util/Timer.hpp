/*!
 ******************************************************************************
 *
 * \file
 *
 * \brief   RAJA header file for simple classes for timing code sections.
 *
 * There are multiple timer classes to deal with platform indiosyncracies.
 *
 ******************************************************************************
 */

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
// Copyright (c) 2016-21, Lawrence Livermore National Security, LLC
// and RAJA project contributors. See the RAJA/COPYRIGHT file for details.
//
// SPDX-License-Identifier: (BSD-3-Clause)
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

#ifndef RAJA_Timer_HPP
#define RAJA_Timer_HPP

#include "RAJA/config.hpp"

#if defined(RAJA_USE_CALIPER)
#include <caliper/Annotation.h>
#endif

#if defined(RAJA_ENABLE_OPENMP) && defined(RUN_OPENMP)
#include <omp.h>
#endif


// libstdc++ on BGQ only has gettimeofday for some reason
#if defined(__bgq__) && (!defined(_LIBCPP_VERSION))

#include <sys/time.h>
#include <chrono>

namespace RAJA
{
/*!
 ******************************************************************************
 *
 * \brief  Timer class for BG/Q that uses gettimeofday since full std::chrono
 *         functionality is not supported.
 *
 *         Generates elapsed time in seconds.
 *
 ******************************************************************************
 */
class BGQTimer
{
public:
  using ElapsedType = double;

private:
  using TimeType = timeval;
  using DurationType = std::chrono::duration<ElapsedType>;

public:
  BGQTimer() : tstart(), tstop(), telapsed(0) {}

  void start() { gettimeofday(&tstart, 0); }

  void stop()
  {
    gettimeofday(&tstop, 0);
    auto start = std::chrono::seconds(tstart.tv_sec) +
                 std::chrono::microseconds(tstart.tv_usec);
    auto stop = std::chrono::seconds(tstop.tv_sec) +
                std::chrono::microseconds(tstop.tv_usec);
    telapsed += DurationType(stop - start).count();
  }

  ElapsedType elapsed() const { return telapsed; }

  void reset() { telapsed = 0; }

private:
  TimeType tstart;
  TimeType tstop;
  ElapsedType telapsed;
};

using TimerBase = BGQTimer;
}  // namespace RAJA


#elif defined(RAJA_USE_CHRONO)

#include <chrono>

namespace RAJA
{
/*!
 ******************************************************************************
 *
 * \brief  Timer class that uses std::chrono.
 *
 *         Generates elapsed time in seconds.
 *
 ******************************************************************************
 */
class ChronoTimer
{
public:
  using ElapsedType = double;

private:
  using ClockType = std::chrono::steady_clock;
  using TimeType = ClockType::time_point;
  using DurationType = std::chrono::duration<ElapsedType>;

public:
  ChronoTimer() : tstart(ClockType::now()), tstop(ClockType::now()), telapsed(0), paused(false)
  {
  }

  void start()
  {
    #if defined(RAJA_ENABLE_OPENMP) && defined(RUN_OPENMP)
    int tid = omp_get_thread_num();
    #else
    int tid = 0;
    #endif
    if (tid == 0) {
      if (paused)
      { tstart = tstart + (ClockType::now() - tpause); paused = false; }
      else
      { tstart = ClockType::now(); }
    }
  }

  void stop()
  {
    #if defined(RAJA_ENABLE_OPENMP) && defined(RUN_OPENMP)
    int tid = omp_get_thread_num();
    #else
    int tid = 0;
    #endif
    if (tid == 0) {
      if (paused)
      { tstop = tpause; paused = false; }
      else
      { tstop = ClockType::now(); }
      telapsed +=
          std::chrono::duration_cast<DurationType>(tstop - tstart).count();
    }
  }

  void pause() {
    #if defined(RAJA_ENABLE_OPENMP) && defined(RUN_OPENMP)
    int tid = omp_get_thread_num();
    #else
    int tid = 0;
    #endif
    if (tid == 0) {
      tpause = ClockType::now();
      paused = true;
    }
  }

  ElapsedType elapsed() const { return telapsed; }

  void reset() {
    #if defined(RAJA_ENABLE_OPENMP) && defined(RUN_OPENMP)
    int tid = omp_get_thread_num();
    #else
    int tid = 0;
    #endif
    if (tid == 0) {
      telapsed = 0;
      paused = false;
    }
  }

private:
  TimeType tstart;
  TimeType tstop;
  TimeType tpause;
  ElapsedType telapsed;
  bool paused;
};

using TimerBase = ChronoTimer;
}  // namespace RAJA


#elif defined(RAJA_USE_GETTIME)

#include <time.h>

namespace RAJA
{
/*!
 ******************************************************************************
 *
 * \brief  Timer class that uses std::chrono.
 *
 *         Generates elapsed time in seconds.
 *
 ******************************************************************************
 */
class GettimeTimer
{
public:
  using ElapsedType = double;

private:
  using TimeType = timespec;

public:
  GettimeTimer() : telapsed(0), stime_elapsed(0), nstime_elapsed(0) { ; }

  void start() { clock_gettime(CLOCK_MONOTONIC, &tstart); }

  void stop()
  {
    clock_gettime(CLOCK_MONOTONIC, &tstop);
    set_elapsed();
  }

  ElapsedType elapsed() const { return (stime_elapsed + nstime_elapsed); }

  void reset()
  {
    stime_elapsed = 0;
    nstime_elapsed = 0;
  }

private:
  TimeType tstart;
  TimeType tstop;
  ElasedType telapsed;

  ElapsedType stime_elapsed;
  ElapsedType nstime_elapsed;

  void set_elapsed()
  {
    stime_elapsed += static_cast<ElapsedType>(tstop.tv_sec - tstart.tv_sec);
    nstime_elapsed +=
        static_cast<ElapsedType>(tstop.tv_nsec - tstart.tv_nsec) / 1000000000.0;
  }
};

using TimerBase = GettimeTimer;
}  // namespace RAJA

#elif defined(RAJA_USE_CLOCK)

#include <time.h>
namespace RAJA
{

/*!
 ******************************************************************************
 *
 * \brief  Timer class that uses clock_t.
 *
 *         Generates elapsed time in seconds.
 *
 ******************************************************************************
 */
class ClockTimer
{
public:
  using ElapsedType = double;

private:
  using TimeType = clock_t;

public:
  ClockTimer() : telapsed(0) { ; }

  void start() { tstart = clock(); }

  void stop()
  {
    tstop = clock();
    set_elapsed();
  }

  ElapsedType elapsed() const
  {
    return static_cast<ElapsedType>(telapsed) / CLOCKS_PER_SEC;
  }

  void reset() { telapsed = 0; }

private:
  TimeType tstart;
  TimeType tstop;
  long double telapsed;

  void set_elapsed() { telapsed += (tstop - tstart); }
};

using TimerBase = ClockTimer;
}  // namespace RAJA

#else

#error RAJA_TIMER is undefined!

#endif

namespace RAJA
{

class Timer : public TimerBase
{
public:
  using TimerBase::start;
  using TimerBase::stop;
  using TimerBase::pause;

#if defined(RAJA_USE_CALIPER)
  void start(const char* name) { cali::Annotation(name).begin(); }
  void stop(const char* name) { cali::Annotation(name).end(); }
#else
  void start(const char*) { start(); }
  void stop(const char*) { stop(); }
#endif
};

}  // namespace RAJA

#endif  // closing endif for header file include guard

/*!
 ******************************************************************************
 *
 * \file
 *
 * \brief   RAJA header file for simple classes for PAPI instrumentation of
 * code sections.
 *
 ******************************************************************************
 */

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
// Copyright (c) 2016-20, Lawrence Livermore National Security, LLC
// and RAJA project contributors. See the RAJA/COPYRIGHT file for details.
//
// SPDX-License-Identifier: (BSD-3-Clause)
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

#ifndef RAJA_PerfEvents_HPP
#define RAJA_PerfEvents_HPP

#include "RAJA/config.hpp"


#if defined(RAJA_USE_PAPI)

#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <vector>
#include <papi.h>
#if defined(RAJA_ENABLE_OPENMP) && defined(RUN_OPENMP)
#include <omp.h>
#endif

#if defined(CHECK_PAPI_ERRORS)
#define CHECK_PAPI_ERROR(msg, rval) \
  if (rval != PAPI_OK) \
  { const char* s = PAPI_strerror(rval); \
    std::cerr << msg << " - " << (s ? s : "null") << ", error: " << rval << std::endl; \
    exit(1); }
#define CHECK_PAPI_LIB_ERROR(rval) \
  if (rval != PAPI_VER_CURRENT && rval > 0) \
  { std::cerr << "PAPI library version mismatch, error: " << rval << std::endl; \
    exit(1); }
#define CHECK_PAPI_THREAD_ERROR(rval) \
  if (rval == PAPI_EMISC || rval == -1) \
  { const char* s = PAPI_strerror(rval); \
    std::cerr << "PAPI thread id - " << (s ? s : "null") << ", error: " << rval << std::endl; \
    exit(1); }
#else
#define CHECK_PAPI_ERROR(msg, rval) (void)rval;
#define CHECK_PAPI_LIB_ERROR(rval) (void)rval;
#define CHECK_PAPI_THREAD_ERROR(rval) (void)rval;
#endif

#define EVENTS_ENV_NAME "RAJA_EVENTS"
#define DEFAULT_EVENTS "PAPI_L3_TCM,PAPI_RES_STL,FP_ARITH:SCALAR_SINGLE,FP_ARITH:SCALAR_DOUBLE"
//#define DEFAULT_EVENTS "PAPI_L3_TCM,PAPI_RES_STL,PAPI_TOT_INS,PAPI_BR_INS"

namespace RAJA
{

#if defined(RAJA_ENABLE_OPENMP) && defined(RUN_OPENMP)
unsigned long omp_get_thread_num_helper(void);
#endif

class PerfEvents
{
  public:
    using EventsType = double;
    PerfEvents()
    {
      int rval;
      const char* event_names_str;
      int event_code = 0x0;
      if (PAPI_is_initialized() == PAPI_NOT_INITED)
      {
        rval = PAPI_library_init(PAPI_VER_CURRENT);
        CHECK_PAPI_LIB_ERROR(rval)
        #if defined(RAJA_ENABLE_OPENMP) && defined(RUN_OPENMP)
        rval = PAPI_thread_init((unsigned long (*)(void))omp_get_thread_num);
        CHECK_PAPI_ERROR("PAPI_thread_init", rval)
        #endif
      }
      if ( !(event_names_str = std::getenv(EVENTS_ENV_NAME)) )
      { event_names_str = DEFAULT_EVENTS; }
      std::stringstream ss(event_names_str);
      while (ss.good())
      {
        std::string event;
        std::getline(ss, event, ',');
        rval = PAPI_event_name_to_code(const_cast<char*>(event.c_str()), &event_code);
        CHECK_PAPI_ERROR("PAPI_event_name_to_code", rval)
        event_names.push_back(event);
        event_codes.push_back(event_code);
      }
      #if defined(RAJA_ENABLE_OPENMP) && defined(RUN_OPENMP)
      num_threads = omp_get_max_threads();
      #else
      num_threads = 1;
      #endif
      num_events = event_codes.size();
      events = new events_type[num_threads];
      for (int tid=0; tid<num_threads; tid++)
      {
        events[tid].EventSet = PAPI_NULL;
        events[tid].event_values = new long_long[num_events];
        events[tid].start_values = new long_long[num_events];
        events[tid].pause_values = new long_long[num_events];
        events[tid].paused = false;
        for (int i=0; i<num_events; i++)
        {
          events[tid].event_values[i] = 0;
          events[tid].start_values[i] = 0;
        }
      }
    }
    ~PerfEvents()
    {
      for (int tid=0; tid<num_threads; tid++)
      {
        delete events[tid].event_values;
        delete events[tid].start_values;
        delete events[tid].pause_values;
      }
      delete events;
    }
    void start()
    {
      #if defined(RAJA_ENABLE_OPENMP) && defined(RUN_OPENMP)
      int tid = PAPI_thread_id();
      CHECK_PAPI_THREAD_ERROR(tid)
      #else
      int tid = 0;
      #endif
      int rval;
      if (events[tid].paused)
      {
        rval = PAPI_read(events[tid].EventSet, events[tid].event_values);
        CHECK_PAPI_ERROR("PAPI_read", rval)
        for (int i=0; i<num_events; i++)
        { events[tid].start_values[i] += events[tid].event_values[i] - events[tid].pause_values[i]; }
        events[tid].paused = false;
      }
      else
      {
        rval = PAPI_create_eventset(&events[tid].EventSet);
        CHECK_PAPI_ERROR("PAPI_create_eventset", rval)
        for (int i=0; i<num_events; i++)
        {
          rval = PAPI_add_event(events[tid].EventSet, event_codes[i]);
          CHECK_PAPI_ERROR("PAPI_add_event", rval)
          events[tid].start_values[i] = 0;
        }
        rval = PAPI_start(events[tid].EventSet);
        CHECK_PAPI_ERROR("PAPI_start", rval)
      }
    }
    void stop()
    {
      #if defined(RAJA_ENABLE_OPENMP) && defined(RUN_OPENMP)
      int tid = PAPI_thread_id();
      CHECK_PAPI_THREAD_ERROR(tid)
      #else
      int tid = 0;
      #endif
      int rval = PAPI_stop(events[tid].EventSet, events[tid].event_values);
      CHECK_PAPI_ERROR("PAPI_stop", rval)
      rval = PAPI_cleanup_eventset(events[tid].EventSet);
      CHECK_PAPI_ERROR("PAPI_cleanup_eventset", rval)
      events[tid].EventSet = PAPI_NULL;
      if (events[tid].paused)
      {
        for (int i=0; i<num_events; i++)
        { events[tid].event_values[i] = events[tid].pause_values[i] - events[tid].start_values[i]; }
        events[tid].paused = false;
      }
      else
      {
        for (int i=0; i<num_events; i++)
        { events[tid].event_values[i] -= events[tid].start_values[i]; }
      }
    }
    void pause()
    {
      #if defined(RAJA_ENABLE_OPENMP) && defined(RUN_OPENMP)
      int tid = PAPI_thread_id();
      CHECK_PAPI_THREAD_ERROR(tid)
      #else
      int tid = 0;
      #endif
      int rval = PAPI_read(events[tid].EventSet, events[tid].pause_values);
      CHECK_PAPI_ERROR("PAPI_read", rval)
      events[tid].paused = true;
    }
    std::map<std::string,long_long> getEvents()
    {
      reset();
      std::map<std::string,long_long> events_map;
      for (int tid=0; tid<num_threads; tid++)
      {
        for (int i=0; i<num_events; i++)
        {
          std::ostringstream oss;
          oss << event_names[i];
          if (num_threads > 1)
          { oss << "(" << std::setfill('0') << std::setw(2) << tid << ")"; }
          events_map.insert({oss.str(), events[tid].event_values[i]});
        }
      }
      return events_map;
    }
    void reset()
    {
      // stop and update all paused thread counters
      for (int tid=0; tid<num_threads; tid++)
      {
        if (events[tid].paused)
        {
          int rval = PAPI_stop(events[tid].EventSet, events[tid].event_values);
          CHECK_PAPI_ERROR("PAPI_stop", rval)
          rval = PAPI_cleanup_eventset(events[tid].EventSet);
          CHECK_PAPI_ERROR("PAPI_cleanup_eventset", rval)
          events[tid].EventSet = PAPI_NULL;
          for (int i=0; i<num_events; i++)
          { events[tid].event_values[i] = events[tid].pause_values[i] - events[tid].start_values[i]; }
          events[tid].paused = false;
        }
      }
    }
  private:
    int num_threads;
    int num_events;
    std::vector<std::string> event_names;
    std::vector<int> event_codes;
    struct events_type
    {
      int EventSet;
      long_long* event_values;
      long_long* start_values;
      long_long* pause_values;
      bool paused;
    };
    events_type* events;
};

}  // namespace RAJA

#endif // RAJA_USE_PAPI


#endif  // closing endif for header file include guard

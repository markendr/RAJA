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
      int event_code = 0x0;
      const char* event_names_str;
      if ( !(event_names_str = std::getenv(EVENTS_ENV_NAME)) )
        event_names_str = DEFAULT_EVENTS;
      std::stringstream ss(event_names_str);
      while (ss.good())
      {
        std::string event;
        std::getline(ss, event, ',');
        event_names.push_back(event);
      }
      PAPI_library_init(PAPI_VER_CURRENT);
      #if defined(RAJA_ENABLE_OPENMP) && defined(RUN_OPENMP)
      PAPI_thread_init(omp_get_thread_num_helper);
      #endif
      PAPI_create_eventset(&EventSet);
      for (auto name: event_names)
      {
        PAPI_event_name_to_code(const_cast<char*>(name.c_str()), &event_code);
        PAPI_add_event(EventSet, event_code);
      }
      #if defined(RAJA_ENABLE_OPENMP) && defined(RUN_OPENMP)
      num_threads = omp_get_max_threads();
      #else
      num_threads = 1;
      #endif
      num_events = event_names.size();
      events = new events_type[num_threads];
      for (int tid=0; tid<num_threads; tid++)
      {
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
      #else
      int tid = 0;
      #endif
      if (events[tid].paused)
      {
        PAPI_read(EventSet, events[tid].event_values);
        for (int i=0; i<num_events; i++)
        { events[tid].start_values[i] += events[tid].event_values[i] - events[tid].pause_values[i]; }
        events[tid].paused = false;
      }
      else
      {
        PAPI_start(EventSet);
        for (int i=0; i<num_events; i++)
        { events[tid].start_values[i] = 0; }
      }
    }
    void stop()
    {
      #if defined(RAJA_ENABLE_OPENMP) && defined(RUN_OPENMP)
      int tid = PAPI_thread_id();
      #else
      int tid = 0;
      #endif
      PAPI_stop(EventSet, events[tid].event_values);
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
      #else
      int tid = 0;
      #endif
      PAPI_read(EventSet, events[tid].pause_values);
      events[tid].paused = true;
    }
    std::map<std::string,long_long> getEvents()
    {
      // stop and update all paused thread counters first
      for (int tid=0; tid<num_threads; tid++)
      {
        if (events[tid].paused)
        {
          for (int i=0; i<num_events; i++)
          { events[tid].event_values[i] = events[tid].pause_values[i] - events[tid].start_values[i]; }
          events[tid].paused = false;
        }
      }
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
    void reset() { for (int tid=0; tid<num_threads; tid++) { events[tid].paused = false; } }
  private:
    int EventSet = PAPI_NULL;
    int num_threads;
    int num_events;
    std::vector<std::string> event_names;
    struct events_type
    {
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

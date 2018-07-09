#pragma once
#include <ice/utility.hpp>
#include <benchmark/benchmark.h>

#ifndef ICE_BENCHMARK_SET_THREAD_AFFINITY
#  define ICE_BENCHMARK_SET_THREAD_AFFINITY 1
#endif

#if ICE_BENCHMARK_SET_THREAD_AFFINITY
#  define ice_set_thread_affinity(affinity) ice::set_thread_affinity(affinity)
#else
#  define ice_set_thread_affinity(affinity)
#endif

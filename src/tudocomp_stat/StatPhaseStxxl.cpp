#include <tudocomp_stat/StatPhaseStxxl.hpp>

#ifndef STATS_DISABLED

using tdc::StatPhaseStxxl;
using tdc::StatPhaseStxxlConfig;

stxxl::stats* StatPhaseStxxl::s_stxxl_stats = stxxl::stats::get_instance();
StatPhaseStxxlConfig StatPhaseStxxl::s_config = StatPhaseStxxlConfig(true);

#endif

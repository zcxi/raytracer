#include "TraceStats.h"

namespace {

thread_local TraceStats* activeStats = nullptr;

} // namespace

TraceStats* currentTraceStats() {
    return activeStats;
}

TraceStatsScope::TraceStatsScope(TraceStats* stats)
    : previous(activeStats) {
    activeStats = stats;
}

TraceStatsScope::~TraceStatsScope() {
    activeStats = previous;
}

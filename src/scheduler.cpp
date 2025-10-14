#include <oryx/chron/scheduler.hpp>

#include <oryx/chron/common.hpp>

namespace oryx::chron {

template class ORYX_CHRON_API Scheduler<LocalClock, NullMutex, ExpressionParser>;
template class ORYX_CHRON_API Scheduler<LocalClock, std::mutex, ExpressionParser>;
template class ORYX_CHRON_API Scheduler<LocalClock, NullMutex, CachedExpressionParser<NullMutex>>;
template class ORYX_CHRON_API Scheduler<LocalClock, std::mutex, CachedExpressionParser<std::mutex>>;

template class ORYX_CHRON_API Scheduler<UTCClock, NullMutex, ExpressionParser>;
template class ORYX_CHRON_API Scheduler<UTCClock, std::mutex, ExpressionParser>;
template class ORYX_CHRON_API Scheduler<UTCClock, NullMutex, CachedExpressionParser<NullMutex>>;
template class ORYX_CHRON_API Scheduler<UTCClock, std::mutex, CachedExpressionParser<std::mutex>>;

template class ORYX_CHRON_API Scheduler<TzClock, NullMutex, ExpressionParser>;
template class ORYX_CHRON_API Scheduler<TzClock, std::mutex, ExpressionParser>;
template class ORYX_CHRON_API Scheduler<TzClock, NullMutex, CachedExpressionParser<NullMutex>>;
template class ORYX_CHRON_API Scheduler<TzClock, std::mutex, CachedExpressionParser<std::mutex>>;

}  // namespace oryx::chron
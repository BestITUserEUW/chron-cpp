#pragma once

#include <oryx/chron/traits.hpp>

namespace oryx::chron::details {

class NullMutex {
public:
    void lock() {}
    void unlock() {}
};

static_assert(traits::BasicLockable<NullMutex>);

}  // namespace oryx::chron::details
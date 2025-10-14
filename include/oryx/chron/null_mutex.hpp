#pragma once

#include "traits.hpp"

namespace oryx::chron {

class NullMutex {
public:
    void lock() noexcept {}
    void unlock() noexcept {}
};

static_assert(traits::BasicLockable<NullMutex>);

}  // namespace oryx::chron
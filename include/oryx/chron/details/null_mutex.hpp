#pragma once

namespace oryx::chron::details {

class NullMutex {
public:
    void lock() {}
    void unlock() {}
};

}  // namespace oryx::chron::details
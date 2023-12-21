#include <pocolog_cpp/SequentialReadDispatcher.hpp>

using namespace pocolog_cpp;

SequentialReadDispatcher::SequentialReadDispatcher(LogFile& logfile)
    : logfile(&logfile) {}

SequentialReadDispatcher::~SequentialReadDispatcher() {
    for (auto ptr: dispatches) {
        delete ptr;
    }
}

SequentialReadDispatcher::DispatchBase::DispatchBase(
    std::string const& streamName,
    std::string const& typeName
)
    : streamName(streamName)
    , typeName(typeName) {}

SequentialReadDispatcher::DispatchBase::~DispatchBase() {}
void* SequentialReadDispatcher::DispatchBase::resolveValue(Typelib::Value const& value) {
    return orogen_transports::getOpaqueValue(typeName, value);
}

#include <pocolog_cpp/SequentialReadDispatcher.hpp>
#include <rtt/plugin/PluginLoader.hpp>
#include <utilmm/configfile/pkgconfig.hh>

using namespace pocolog_cpp;
using namespace std;

SequentialReadDispatcher::SequentialReadDispatcher(LogFile& logfile)
    : logfile(logfile)
{
}

SequentialReadDispatcher::~SequentialReadDispatcher()
{
    for (auto ptr : dispatches) {
        delete ptr;
    }
}

void SequentialReadDispatcher::run()
{
    logfile.rewind();

    auto per_index_dispatch = buildPerIndexDispatch();
    while (auto maybe_sample = logfile.readNextSample()) {
        if (!maybe_sample.has_value()) {
            return;
        }

        auto [index, time, value] = *maybe_sample;

        for (auto d : per_index_dispatch[index]) {
            d->dispatch(*value);
        }
    }
}

SequentialReadDispatcher::PerIndexDispatch SequentialReadDispatcher::
    buildPerIndexDispatch()
{
    PerIndexDispatch per_index_dispatch;
    for (auto const& stream : logfile.getStreamDescriptions()) {
        size_t index = stream.getIndex();
        if (per_index_dispatch.size() <= index) {
            per_index_dispatch.resize(index + 1);
        }

        for (auto d : dispatches) {
            if (d->matches(stream)) {
                per_index_dispatch[index].push_back(d);
            }
        }
    }
    return per_index_dispatch;
}

void SequentialReadDispatcher::importTypesFrom(std::string const& typekitName)
{
    utilmm::pkgconfig pkg(typekitName + "-typekit-gnulinux");
    auto base = pkg.get("libdir") + "/lib" + typekitName;
    RTT::plugin::PluginLoader::Instance()->loadLibrary(base + "-typekit-gnulinux.so");
    RTT::plugin::PluginLoader::Instance()->loadLibrary(
        base + "-transport-typelib-gnulinux.so");
}

std::string SequentialReadDispatcher::getCXXTypename(StreamDescription const& description) {
    auto const& map = description.getMetadataMap();
    auto it = map.find("rock_cxx_type_name");
    if (it == map.end()) {
        return description.getTypeName();
    }

    return it->second;
}

SequentialReadDispatcher::DispatchBase::DispatchBase(std::string const& streamName,
    std::string const& typeName)
    : streamName(streamName)
    , typeName(typeName)
    , typelibMarshaller(orogen_transports::getMarshallerFor(typeName))
    , typelibHandle(typelibMarshaller->createHandle())
{
}

SequentialReadDispatcher::DispatchBase::~DispatchBase()
{
    typelibMarshaller->deleteHandle(typelibHandle);
}

bool SequentialReadDispatcher::DispatchBase::matches(
    StreamDescription const& stream) const
{
    return (stream.getName() == streamName);
}
void* SequentialReadDispatcher::DispatchBase::resolveValue(Typelib::Value const& value)
{
    typelibMarshaller->setTypelibSample(typelibHandle,
        reinterpret_cast<uint8_t*>(value.getData()),
        true);
    return typelibMarshaller->releaseOrocosSample(typelibHandle);
}

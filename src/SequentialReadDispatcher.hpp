#ifndef POCOLOG_CPP_SEQUENTIALREADDISPATCHER_HPP
#define POCOLOG_CPP_SEQUENTIALREADDISPATCHER_HPP

#include <string>
#include <typelib/value.hh>
#include <pocolog_cpp/LogFile.hpp>
#include <rtt/typelib/TypelibMarshallerBase.hpp>

namespace pocolog_cpp {

/** Declarative interface to read a pocolog file sequentially, dispatching
 * values in different lambdas while doing so
 *
 * The idea of this class is to allow workflows where pocolog files are
 * carefully constructed as test cases / regression tests, and we build
 * tools in the libraries to play these files automatically
 */
class SequentialReadDispatcher {
    /** Abstract base class for all dispatchers */
    class DispatchBase {
        std::string streamName;
        std::string typeName;
        int streamIndex = -1;

    public:
        DispatchBase(std::string const& stream_name, std::string const& typeName);
        virtual ~DispatchBase();
        void* resolveValue(Typelib::Value const& value);
        virtual void dispatch(Typelib::Value const& value) = 0;
    };

    template<typename T> using Callback = std::function<void(T const&)>;

    /** Actually typed dispatch */
    template<typename T>
    class Dispatch : public DispatchBase {
    private:
        Callback<T> callback;

    public:
        Dispatch(std::string const& streamName, std::string const& typeName,
                 Callback<T> callback)
            : DispatchBase(streamName, typeName)
            , callback(callback) {}

        virtual void dispatch(Typelib::Value const& value) override {
            void* resolvedValue = resolveValue(value);
            std::unique_ptr<T> ptr(reinterpret_cast<T*>(resolvedValue));
            return callback(*ptr);
        }
    };

    LogFile* logfile = nullptr;
    std::vector<DispatchBase*> dispatches;

public:
    SequentialReadDispatcher(LogFile& logfile);
    ~SequentialReadDispatcher();

    template<typename T>
    void add(std::string const& streamName,
             Callback<T> callback) {
        auto const& streamInfo = logfile->getStream(streamName);
        return add(streamName, streamInfo.getTypeName(), callback);
    }

    template<typename T>
    void add(std::string const& streamName,
             std::string const& typeName,
             Callback<T> callback) {

        dispatches.push_back(new Dispatch<T>(streamName));
    }
};

}

#endif

#include "named_vector_helpers.hpp"

namespace pocolog_cpp {


template <typename T>
void container_to_vector(const Typelib::Container *cont, void *data, std::vector<T> &ret)
{
    ret.resize(cont->getElementCount(data));

    for(size_t idx=0; idx<cont->getElementCount(data); idx++){
        void* str_data = cont->getElement(data, idx).getData();
        T* _ptr = reinterpret_cast< T* >(str_data);
        ret[idx] = *_ptr;
    }
}


std::vector<Typelib::Value> sort_named_vector_values(Typelib::Value& names_v,
                                                     Typelib::Value& values_v,
                                                     std::map<std::string, size_t> order)
{
    // Covert names_v to std::vector<std::string>
    const Typelib::Container* names_c = dynamic_cast<const Typelib::Container*>(&names_v.getType());
    if(!names_c){
        throw(std::invalid_argument("names_v is not a container"));
    }
    std::vector<std::string> names;
    container_to_vector(names_c, names_v.getData(), names);


    // Covert values_v to std::vector<Typelib::Value>
    const Typelib::Container* values_c = dynamic_cast<const Typelib::Container*>(&values_v.getType());
    std::vector<Typelib::Value> values(values_c->getElementCount(values_v.getData()));
    if(!values_c){
        throw(std::invalid_argument("values_v is not a container"));
    }
    for(size_t idx=0; idx<values_c->getElementCount(values_v.getData()); idx++){
        values[idx] = values_c->getElement(values_v.getData(), idx);
    }


    if(names.size() != values.size() || names.size() != order.size()){
        throw(std::invalid_argument("names, values and order must be of same size"));
    }

    std::vector<Typelib::Value> resorted(names.size());
    size_t idx = 0;
    for(const std::string& name : names)
    {
        if(order.find(name) == order.end()){
            throw(std::invalid_argument(name+" could not be found the given sorting order"));
        }
        resorted[order[name]] = values[idx++];
    }
    return resorted;
}


std::vector<std::string> extract_names(const Typelib::Value& v)
{
    Typelib::Value names_v = Typelib::FieldGetter().apply(v, "names");

    const Typelib::Container* names_c = dynamic_cast<const Typelib::Container*>(&names_v.getType());
    std::vector<std::string> names;

    // When no names a given, but elements, return string of indices as names
    if(names_c->getElementCount(names_v.getData()) == 0)
    {
        Typelib::Value elems_v = Typelib::FieldGetter().apply(v, "elements");
        const Typelib::Container* elems_c = dynamic_cast<const Typelib::Container*>(&elems_v.getType());
        for(size_t idx = 0; idx < elems_c->getElementCount(elems_v.getData()); idx++)
        {
            names.push_back(std::to_string(idx));
        }
    }

    else
    {
        container_to_vector(names_c, names_v.getData(), names);
    }

    return names;
}


std::vector<std::string> extract_names(InputDataStream *stream){
    // Initialize buffer
    std::vector<uint8_t> buffer;
    buffer.resize(stream->getTypeMemorySize());

    Typelib::Value v((void*)buffer.data(), *stream->getType());
    Typelib::Value names_v = Typelib::FieldGetter().apply(v, "names");

    stream->getTyplibValue(buffer.data(), stream->getTypeMemorySize(), 0);

    return extract_names(v);
}


bool is_named_vector(const Typelib::Value& v)
{
    try{
        Typelib::FieldGetter().apply(v, "names");
        Typelib::FieldGetter().apply(v, "elements");
        return true;
    }catch(...){
        return false;
    }
}


bool is_named_vector(const Typelib::Type& t)
{
    // make sure we look at a struct
    if (t.getCategory() != Typelib::Type::Compound) {
        return false;
    }
    
    // ok, then try to cast it
    auto cmp = dynamic_cast<const Typelib::Compound*>(&t);
    
    if (!cmp)
        return false;
    
    // check if we have the typical named vector fields
    if (cmp->getField("names") && cmp->getField("elements")) {
        return true;
    } 
    
    return false;
}


bool is_named_vector(InputDataStream *stream, char* buffer)
{
    Typelib::Value v( buffer, *stream->getType() );
    return is_named_vector(v);
}

}

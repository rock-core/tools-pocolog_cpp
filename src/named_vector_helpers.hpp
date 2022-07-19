#ifndef NAMED_VECTOR_HELPERS_HPP
#define NAMED_VECTOR_HELPERS_HPP

#include<vector>
#include<typelib/value.hh>
#include "InputDataStream.hpp"

namespace pocolog_cpp{

template <typename T>
void container_to_vector(const Typelib::Container* cont, void* data, std::vector<T>& ret);

std::vector<Typelib::Value> sort_named_vector_values(Typelib::Value& names_v,
                                                     Typelib::Value& values_v,
                                                     std::map<std::string, size_t> order);

std::vector<std::string> extract_names(InputDataStream *stream);

bool is_named_vector(const Typelib::Value& v);

bool is_named_vector(InputDataStream *stream, char *buffer);

}

#endif // NAMED_VECTOR_HELPERS_HPP

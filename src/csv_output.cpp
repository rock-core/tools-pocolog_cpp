#include "csv_output.hpp"
#include <typelib/value.hh>
#include <typelib/typevisitor.hh>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/join.hpp>

using namespace Typelib;
using namespace std;
using namespace pocolog_cpp;

namespace
{
    using namespace Typelib;
    using namespace std;
    using boost::join;
    class HeaderVisitor : public TypeVisitor
    {
    public:
        list<string> m_name;
        vector<string> m_headers;
        vector<string> m_typenames;

    protected:
        void output(const std::string& type_name)
        {
            string name = join(m_name, "");
            m_headers.push_back(name);
            m_typenames.push_back(type_name);
        }

        bool visit_ (OpaqueType const& type) {
            output(type.getName());
            return true;
        }
        bool visit_ (Numeric const& type) {
            output(type.getName());
            return true;
        }
        bool visit_ (Enum const& type) {
            output(type.getName());
            return true;
        }

        bool visit_ (Pointer const& type)
        {
            m_name.push_front("*(");
            m_name.push_back(")");
            TypeVisitor::visit_(type);
            m_name.pop_front();
            m_name.pop_back();
            return true;
        }
        bool visit_ (Array const& type)
        {
            m_name.push_back("[");
            m_name.push_back("");
            m_name.push_back("]");
            list<string>::iterator count = m_name.end();
            --(--count);
            for (size_t i = 0; i < type.getDimension(); ++i)
            {
                *count = boost::lexical_cast<string>(i);
                TypeVisitor::visit_(type);
            }
            m_name.pop_back();
            m_name.pop_back();
            m_name.pop_back();
            return true;
        }

        bool visit_ (Compound const& type)
        {
            m_name.push_back(".");
            TypeVisitor::visit_(type);
            m_name.pop_back();
            return true;
        }
        bool visit_ (Compound const& type, Field const& field)
        {
            m_name.push_back(field.getName());
            TypeVisitor::visit_(type, field);
            m_name.pop_back();
            return true;
        }

        using TypeVisitor::visit_;
    public:
        vector<string> apply(Typelib::Type const& type, std::string const& basename)
        {
            m_headers.clear();
            m_name.clear();
            m_name.push_back(basename);
            TypeVisitor::apply(type);
            return m_headers;
        }
    };

    class LineVisitor : public ValueVisitor
    {
        list<string>  m_output;
        bool m_char_as_numeric;
        std::string m_string_delimeter;

    protected:
        template<typename T>
        bool display(T value)
        {
            m_output.push_back(boost::lexical_cast<string>(value));
            return true;
        }
        using ValueVisitor::visit_;
        bool visit_ (Value value, OpaqueType const& type)
        {
            display("<" + type.getName() + ">");
            return true;
        }
        virtual bool visit_ (Value const& v, Container const& a)
        {
            if (a.getName() == "/std/string"){
                std::string* string_ptr =
                    reinterpret_cast< std::string* >(v.getData());
                m_output.push_back(m_string_delimeter + *string_ptr + m_string_delimeter);

                return true;
            }

           return ValueVisitor::visit_(v,a);
        }
        bool visit_ (int8_t  & value)
        {
            if (m_char_as_numeric)
                return display<int>(value);
            else
                return display(value);
        }
        bool visit_ (uint8_t & value)
        {
            if (m_char_as_numeric)
                return display<int>(value);
            else
                return display(value);
        }
        bool visit_ (int16_t & value) { return display(value); }
        bool visit_ (uint16_t& value) { return display(value); }
        bool visit_ (int32_t & value) { return display(value); }
        bool visit_ (uint32_t& value) { return display(value); }
        bool visit_ (int64_t & value) { return display(value); }
        bool visit_ (uint64_t& value) { return display(value); }
        bool visit_ (float   & value) { return display(value); }
        bool visit_ (double  & value) { return display(value); }
        bool visit_ (Enum::integral_type& v, Enum const& e)
        {
            try { m_output.push_back(e.get(v)); }
            catch(Typelib::Enum::ValueNotFound&)
            { display(v); }
            return true;
        }

    public:
        list<string> apply(Value const& value, bool char_as_numeric, std::string string_delimeter)
        {
            m_output.clear();
            m_char_as_numeric = char_as_numeric;
            m_string_delimeter = string_delimeter;
            ValueVisitor::apply(value);
            return m_output;
        }
    };
}


CSVOutput::CSVOutput(Type const& type, std::string const& sep, bool char_as_numeric = true, const string &string_delim)
    : m_type(type), m_separator(sep), m_char_as_numeric(char_as_numeric), m_string_delimeter(string_delim) {}

/** Displays the header */
void CSVOutput::header(std::ostream& out, std::string const& basename)
{
    HeaderVisitor visitor;
    out << join(visitor.apply(m_type, basename), m_separator);
}

void CSVOutput::column_types(std::ostream& out, std::string const& basename)
{
    HeaderVisitor visitor;
    visitor.apply(m_type, basename);
    for(size_t idx=0; idx<visitor.m_typenames.size(); idx++){
        out << visitor.m_headers[idx] << this->m_separator << visitor.m_typenames[idx] << std::endl;
    }
}

void CSVOutput::display(std::ostream& out, void* value)
{
    LineVisitor visitor;
    out << join(visitor.apply( Value(value, m_type), m_char_as_numeric, m_string_delimeter), m_separator );
}


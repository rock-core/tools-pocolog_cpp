#pragma once
#include <string>
#include <iosfwd>
#include <typelib/value.hh>

namespace pocolog_cpp
{
    class CSVOutput
    {
        Typelib::Type const& m_type;
        std::string m_separator;
        bool m_char_as_numeric;
        std::string m_string_delimeter;

    public:
        CSVOutput(Typelib::Type const& type, std::string const& sep, bool char_as_numeric, std::string const& string_delim = "");

        /** Displays the header */
        void header(std::ostream& out, std::string const& basename);
        void display(std::ostream& out, void* value);
        void column_types(std::ostream& out, std::string const& basename);
    };


    namespace details {
        struct csvheader
        {
            CSVOutput output;
            std::string basename;
            bool output_column_types;

            csvheader(Typelib::Type const& type, std::string const& basename_, std::string const& sep = " ", std::string const& string_delim = "",
                      bool output_column_types = false)
                : output(type, sep, true, string_delim), basename(basename_), output_column_types(output_column_types) {}
        };
        struct csvline
        {
            CSVOutput output;
            void* value;
            bool char_as_numeric;

            csvline(Typelib::Type const& type_, void* value_, std::string const& sep_ = " ", bool char_as_numeric = true, std::string const& string_delim = "")
                : output(type_, sep_, char_as_numeric, string_delim), value(value_), char_as_numeric(char_as_numeric) {}
        };
        inline std::ostream& operator << (std::ostream& stream, csvheader header)
        {
            if(header.output_column_types){
                header.output.column_types(stream, header.basename);
            }
            else{
                header.output.header(stream, header.basename);
            }
                return stream;
        }
        inline std::ostream& operator << (std::ostream& stream, csvline line)
        {
            line.output.display(stream, line.value);
            return stream;
        }
    }

    /** Display a CSV header matching a Type object
     * @arg type        the type to display
     * @arg basename    the basename to use. For simple type, it is the variable name. For compound types, names in the header are &lt;basename&gt;.&lt;fieldname&gt;
     * @arg sep         the separator to use
     * @arg output_column_types output column types for each column of the header instead of the header itself
     */
    inline details::csvheader csv_header(Typelib::Type const& type, std::string const& basename, std::string const& sep = " ", std::string const& string_delim="", bool outout_column_types=false)
    { return details::csvheader(type, basename, sep, string_delim, outout_column_types); }

    /** Display a CSV line for a Type object and some raw data
     * @arg type        the data type
     * @arg value       the data as a void* pointer
     * @arg sep         the separator to use
     */
    inline details::csvline   csv(Typelib::Type const& type, void* value, std::string const& sep = " ", bool char_as_numeric = true, std::string const& string_delim="")
    { return details::csvline(type, value, sep, char_as_numeric, string_delim); }
};


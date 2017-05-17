#pragma once

#include "json/elements.h"
#include "json/reader.h"
#include "json/writer.h"
#include <string>
#include <ostream>
#include <istream>
#include <sstream>

namespace json
{
    inline std::ostream & operator<<(std::ostream& out, const UnknownElement& element)
    {
        Writer::Write(element, out);
        return out;
    }

    inline std::string ToString(const json::UnknownElement& element)
    {
        std::ostringstream out;
        out << element;
        return out.str();
    }

    inline UnknownElement Parse(std::istream & in)
    {
        json::UnknownElement res;
        in >> res;
        return res;
    }

    inline json::UnknownElement Parse(const std::string & json)
    {
        std::istringstream in(json);
        return Parse(in);
    }

    class ConstElement
    {
    public:
        ConstElement() {}
        ConstElement(const UnknownElement& element) : m_element(element) {}

        ConstElement& operator=(const UnknownElement& element)
        {
            m_element = element;
            return *this;
        }

        const UnknownElement& operator[] (const std::string& key) const
        {
            return GetConstElement()[key];
        }

        const UnknownElement& operator[] (size_t index) const
        {
            return GetConstElement()[index];
        }

        operator const UnknownElement& () const
        {
            return m_element;
        }

    private:
        const UnknownElement& GetConstElement() const
        {
            return m_element;
        }

        UnknownElement m_element;
    };

    template <typename Iterator>
    json::Array ToArray(Iterator begin, Iterator end)
    {
        json::Array res;
        for (; begin != end; ++begin)
        {
            res.Insert(json::UnknownElement(*begin));
        }
        return res;
    }
}

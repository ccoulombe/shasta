#ifndef SHASTA_SPAN_HPP
#define SHASTA_SPAN_HPP

// A span class similar to std::span in C++20.
// We currently compile using the C++14 standard, so we cannot use std::span.

#include "algorithm.hpp"
#include "cstddef.hpp"
#include "iostream.hpp"
#include "iterator.hpp"
#include "stdexcept.hpp"
#include "string.hpp"
#include "vector.hpp"

namespace shasta {
    template<class T> class span;

    // Allow converting a span<T> to a span<const T>.
    template<class T> span<const T> makeSpanOfConst(const span<T>&);

    // Output a span<const char> as a string.
    inline ostream& operator<<(ostream&, const span<const char>&);

    // Convert a span<const char> to an integer.
    // This cannot be done using std::atol because the
    // span<const char> is not null terminated.
    uint64_t atoul(const span<const char>&);

    // Convert a span<const char> to an std::string.
    string convertToString(const span<const char>&);
}



template<class T> class shasta::span {
public:

    span(T* begin, T* end) :
        dataBegin(begin),
        dataEnd(end)
    {
    }

    span(vector<T>& v) :
        dataBegin(&v[0]),
        dataEnd(dataBegin + v.size())
    {
    }

    span() : dataBegin(0), dataEnd(0) {}

    size_t size() const
    {
        return dataEnd - dataBegin;
    }
    bool empty() const
    {
        return dataBegin == dataEnd;
    }
    T* begin() const
    {
        return dataBegin;
    }
    T* end() const
    {
        return dataEnd;
    }
    T* rbegin() const
    {
        return dataEnd - 1;
    }
    T* rend() const
    {
        return dataBegin - 1;
    }
    T& operator[](size_t i) const
    {
        return dataBegin[i];
    }

    T& front()
    {
        SHASTA_ASSERT(dataBegin);
        SHASTA_ASSERT(dataEnd);
        SHASTA_ASSERT(!empty());
        return *dataBegin;
    }
    const T& front() const
    {
        SHASTA_ASSERT(dataBegin);
        SHASTA_ASSERT(dataEnd);
        SHASTA_ASSERT(!empty());
        return *dataBegin;
    }

    T& back()
    {
        SHASTA_ASSERT(dataBegin);
        SHASTA_ASSERT(dataEnd);
        SHASTA_ASSERT(!empty());
        return *(dataEnd - 1);
    }

    const T& back() const
    {
        SHASTA_ASSERT(dataBegin);
        SHASTA_ASSERT(dataEnd);
        SHASTA_ASSERT(!empty());
        return *(dataEnd - 1);
    }

    bool operator==(const span<T>& that) const
    {
        if(size() == that.size()) {
            return std::equal(begin(), end(), that.begin());
        } else {
            return false;
        }
    }
    bool operator!=(const span<T>& that) const
    {
        return not((*this) == that);
    }

    bool operator<(const span<T>& that) const
    {
        return std::lexicographical_compare(
            dataBegin, dataEnd,
            that.dataBegin, that.dataEnd);
    }

private:
    T* dataBegin;
    T* dataEnd;
};



// Allow converting a span<T> to a span<const T>.
// I was not able to get an operator conversion to work
template<class T> shasta::span<const T> shasta::makeSpanOfConst(const shasta::span<T>& s)
{
    return span<const T>(s.begin(), s.end());
}



// Output a span<const char> as a string.
inline std::ostream& shasta::operator<<(
    std::ostream& s,
    const shasta::span<const char>&  m)
{
    copy(m.begin(), m.end(), ostream_iterator<char>(s));
    return s;
}


// Convert a span<const char> to an integer.
// This cannot be done using std::atol because the
// span<const char> is not null terminated.
// The string can only contain numeric characters.
inline uint64_t shasta::atoul(const span<const char>& s)
{
    uint64_t n = 0;
    for(uint64_t i=0; ; i++) {
        const char c = s[i];
        if(not std::isdigit(c)) {
            throw runtime_error("Non-digit found in " + convertToString(s));
        }

        n += (c- '0');

        if(i ==  s.size()-1) {
            return n;
        }

        n *= 10;
    }
}



// Convert a span<const char> to an std::string.
inline std::string shasta::convertToString(const span<const char>& m)
{
    return string(m.begin(), m.size());
}



#endif

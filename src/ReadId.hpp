#ifndef SHASTA_READ_ID_HPP
#define SHASTA_READ_ID_HPP

// Shasta.
#include "SHASTA_ASSERT.hpp"

// Standard libraries.
#include <cstdlib>
#include "cstdint.hpp"
#include "iostream.hpp"
#include <limits>
#include "string.hpp"



namespace shasta {

    // Type used to identify a read.
    // This is used as an index into Assembler::reads.
    using ReadId = uint32_t;
    const ReadId invalidReadId = std::numeric_limits<ReadId>::max();

    // Class used to identify an oriented read,
    // that is a read, possibly reverse complemented.
    class OrientedReadId;
    inline ostream& operator<<(ostream&, OrientedReadId);
    using Strand = ReadId;

}


// Class used to identify an oriented read,
// that is a read, possibly reverse complemented.
// The strand stored in the least significant
// bit is 0 if the oriented read is identical
// to the original read and 1 if it is reverse complemented
class shasta::OrientedReadId {
public:
    OrientedReadId() : value(std::numeric_limits<ReadId>::max()) {}
    OrientedReadId(ReadId readId, Strand strand) : value((readId<<1) | strand)
    {
        SHASTA_ASSERT(strand < 2);
    }
    explicit OrientedReadId(ReadId value) : value(value) {}

    // Constructor from a string of the form readId-strand.
    explicit OrientedReadId(const string& s)
    {
        const auto dashPosition = s.find_first_of('-');
        SHASTA_ASSERT(dashPosition != string::npos);
        const ReadId readId = std::atoi(s.substr(0, dashPosition).c_str());
        const Strand strand = std::atoi(s.substr(dashPosition+1, s.size()).c_str());
        value = (readId<<1) | strand;
    }
    ReadId getReadId() const
    {
        return value >> 1;
    }
    Strand getStrand() const
    {
        return value & 1;
    }

    void flipStrand()
    {
        value ^= Int(1);
    }

    // Return the integer value, which can be used as an index intoi Assembler::orientedReadIds.
    ReadId getValue() const
    {
        return value;
    }

    // Return a string representing this OrientedReadId.
    string getString() const
    {
        return to_string(getReadId()) + "-" + to_string(getStrand());
    }

    // Return an invalid OrientedReadId.
    static OrientedReadId invalid()
    {
        return OrientedReadId();
    }

    bool operator==(const OrientedReadId& that) const
    {
        return value == that.value;
    }
    bool operator!=(const OrientedReadId& that) const
    {
        return value != that.value;
    }
    bool operator<(const OrientedReadId& that) const
    {
        return value < that.value;
    }
    bool operator<=(const OrientedReadId& that) const
    {
        return value <= that.value;
    }
    bool operator>(const OrientedReadId& that) const
    {
        return value > that.value;
    }

    using Int = ReadId;
private:
    Int value;
};



inline std::ostream& shasta::operator<<(
    std::ostream& s,
    OrientedReadId orientedReadId)
{
    s << orientedReadId.getString();
    return s;
}


#endif

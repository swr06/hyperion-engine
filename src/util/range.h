#ifndef HYPERION_RANGE_H
#define HYPERION_RANGE_H

#include <math/math_util.h>

namespace hyperion {

template <class T>
class Range {
public:
    using iterator = T;

    Range() : m_start{}, m_end{} {}
    Range(const T &start, const T &end) : m_start(start), m_end(end) {}
    Range(const Range &other) = default;
    Range &operator=(const Range &other) = default;
    Range(Range &&other) = default;
    Range &operator=(Range &&other) = default;
    ~Range() = default;

    inline explicit operator bool() const      { return Distance() > 0; }

    inline const T &GetStart() const           { return m_start; }
    inline void SetStart(const T &start)       { m_start = start; }
    inline const T &GetEnd()   const           { return m_end; }
    inline void SetEnd(const T &end)           { m_end = end; }

    inline int64_t Distance() const            { return static_cast<int64_t>(m_end) - static_cast<int64_t>(m_start); }
    inline int64_t Step() const                { return MathUtil::Sign(Distance()); }
    inline bool Includes(const T &value) const { return value >= m_start && value < m_end; }

    Range operator|(const Range &other) const
    {
        return {
            MathUtil::Min(m_start, other.m_start),
            MathUtil::Max(m_end, other.m_end)
        };
    }

    Range &operator|=(const Range &other)
    {
        m_start = MathUtil::Min(m_start, other.m_start);
        m_end   = MathUtil::Max(m_end, other.m_end);

        return *this;
    }

    Range operator&(const Range &other) const
    {
        return {
            MathUtil::Max(m_start, other.m_start),
            MathUtil::Min(m_end, other.m_end)
        };
    }

    Range &operator&=(const Range &other)
    {
        m_start = MathUtil::Max(m_start, other.m_start);
        m_end   = MathUtil::Min(m_end, other.m_end);

        return *this;
    }

    bool operator<(const Range &other) const { return Distance() < other.Distance(); }
    bool operator>(const Range &other) const { return Distance() > other.Distance(); }

    bool operator==(const Range &other) const
    {
        if (this == &other) {
            return true;
        }

        return m_start == other.m_start
            && m_end == other.m_end;
    }

    bool operator!=(const Range &other) const { return !operator==(other); }

    /* \brief Return a range equal to this range BUT:
     * If value is <= the start value, have the resulting range start at value+step.
     * If the value is >= the end value, have the resulting range end at value.
     */
    Range Excluding(const T &value, T step = 1) const
    {
        return {
            MathUtil::Max(m_start, value + step),
            MathUtil::Min(m_end, value)
        };
    }
    
    iterator begin() const { return m_start; }
    iterator end() const { return m_end; }

private:
    T m_start, m_end;
};

} // namespace hyperion

#endif
/*
 * Copyright 2002-2020 Intel Corporation.
 *
 * This software is provided to you as Sample Source Code as defined in the accompanying
 * End User License Agreement for the Intel(R) Software Development Products ("Agreement")
 * section 1.L.
 *
 * This software and the related documents are provided as is, with no express or implied
 * warranties, other than those that are expressly stated in the License.
 */

/*
 * This file is modified to implement compression and decompression functions
 * in addition to multi-level caches.
 * Authors: Jeffrey Luo, Shivani Prasad, Deanyone Su
 */

/*! @file
 *  This file contains a configurable cache class
 */

#ifndef PIN_CACHE_H
#define PIN_CACHE_H


#define KILO (1024)
#define MEGA (KILO*KILO)
#define GIGA (KILO*MEGA)

typedef UINT64 CACHE_STATS; // type of cache hit/miss counters

#include "compression.h"
#include <sstream>
using std::string;
using std::ostringstream;
/*! RMR (rodric@gmail.com)
 *   - temporary work around because decstr()
 *     casts 64 bit ints to 32 bit ones
 */
static string mydecstr(UINT64 v, UINT32 w) {
    ostringstream o;
    o.width(w);
    o << v;
    string str(o.str());
    return str;
}

/*!
 *  @brief Checks if n is a power of 2.
 *  @returns true if n is power of 2
 */
static inline bool IsPower2(UINT32 n) {
    return ((n & (n - 1)) == 0);
}

/*!
 *  @brief Computes floor(log2(n))
 *  Works by finding position of MSB set.
 *  @returns -1 if n == 0.
 */
static inline INT32 FloorLog2(UINT32 n) {
    INT32 p = 0;

    if (n == 0) return -1;

    if (n & 0xffff0000) { p += 16; n >>= 16; }
    if (n & 0x0000ff00)	{ p +=  8; n >>=  8; }
    if (n & 0x000000f0) { p +=  4; n >>=  4; }
    if (n & 0x0000000c) { p +=  2; n >>=  2; }
    if (n & 0x00000002) { p +=  1; }

    return p;
}

/*!
 *  @brief Computes floor(log2(n))
 *  Works by finding position of MSB set.
 *  @returns -1 if n == 0.
 */
static inline INT32 CeilLog2(UINT32 n) {
    return FloorLog2(n - 1) + 1;
}

/*!
 *  @brief Cache tag - self clearing on creation
 */
class CACHE_TAG {
  private:
    ADDRINT _tag;
  public:
    CACHE_TAG(ADDRINT tag = 0) { _tag = tag; }
    bool operator==(const CACHE_TAG &right) const { return _tag == right._tag; }
    operator ADDRINT() const { return _tag; }
};

/*
 * Return struct for providing more detailed return information. Provides
 * implicit cast to bool for backwards compatibility
 */
struct CACHE_ACCESS_STRUCT {
    bool hit;

    // more useful stuff
    ADDRINT evicted_addr;
    CACHE_TAG evicted_tag;
    bool evicted_dirty;

    operator bool() {return hit;} // allow implicit cast
};


/*!
 * Everything related to cache sets
 */
namespace CACHE_SET {

/*!
 *  @brief Cache set direct mapped
 */
class DIRECT_MAPPED {
  private:
    CACHE_TAG _tag;

  public:
    DIRECT_MAPPED(UINT32 associativity = 1) { ASSERTX(associativity == 1); }

    VOID SetAssociativity(UINT32 associativity) { ASSERTX(associativity == 1); }
    UINT32 GetAssociativity(UINT32 associativity) { return 1; }

    UINT32 Find(CACHE_TAG tag) { return(_tag == tag); }
    VOID Replace(CACHE_TAG tag) { _tag = tag; }
};

/*!
 *  @brief Cache set with round robin replacement
 */
template <UINT32 MAX_ASSOCIATIVITY = 4>
class ROUND_ROBIN {
  private:
    CACHE_TAG _tags[MAX_ASSOCIATIVITY];
    UINT32 _tagsLastIndex;
    UINT32 _nextReplaceIndex;

  public:
    ROUND_ROBIN(UINT32 associativity = MAX_ASSOCIATIVITY)
      : _tagsLastIndex(associativity - 1) {

        ASSERTX(associativity <= MAX_ASSOCIATIVITY);
        _nextReplaceIndex = _tagsLastIndex;

        for (INT32 index = _tagsLastIndex; index >= 0; index--) {
            _tags[index] = CACHE_TAG(0);
        }
    }

    VOID SetAssociativity(UINT32 associativity) {
        ASSERTX(associativity <= MAX_ASSOCIATIVITY);
        _tagsLastIndex = associativity - 1;
        _nextReplaceIndex = _tagsLastIndex;
    }
    UINT32 GetAssociativity(UINT32 associativity) {
        return _tagsLastIndex + 1;
    }

    UINT32 Find(CACHE_TAG tag) {
        bool result = true;

        for (INT32 index = _tagsLastIndex; index >= 0; index--) {
            // this is an ugly micro-optimization, but it does cause a
            // tighter assembly loop for ARM that way ...
            if(_tags[index] == tag) goto end;
        }
        result = false;

        end: return result;
    }

    VOID Replace(CACHE_TAG tag) {
        // g++ -O3 too dumb to do CSE on following lines?!
        const UINT32 index = _nextReplaceIndex;

        _tags[index] = tag;
        // condition typically faster than modulo
        _nextReplaceIndex = (index == 0 ? _tagsLastIndex : index - 1);
    }
};

/*!
 *  @brief Cache set with least recently used replacement
 */
template <UINT32 MAX_ASSOCIATIVITY = 4, UINT32 LINE_SIZE = 64, bool DATA = false>
class LRU {
  private:
    CACHE_TAG _tags[MAX_ASSOCIATIVITY];
    UINT32 _tagsLastIndex;
    UINT64 _timeSinceAccess[MAX_ASSOCIATIVITY];
    UINT32 _dirty[MAX_ASSOCIATIVITY];
    bool _dataTrack;
    char _dataValue[MAX_ASSOCIATIVITY][LINE_SIZE];

  public:
    CACHE_TAG _evicted_tag;
    bool _evicted_dirty;

    LRU(UINT32 associativity = MAX_ASSOCIATIVITY)
        : _tagsLastIndex(associativity - 1) {

        ASSERTX(associativity <= MAX_ASSOCIATIVITY);

        for (INT32 index = _tagsLastIndex; index >= 0; index--) {
            _tags[index] = CACHE_TAG(0);
            _timeSinceAccess[index] = UINT64_MAX;
        }

        _dataTrack = DATA;
    }

    VOID SetAssociativity(UINT32 associativity) {
        ASSERTX(associativity <= MAX_ASSOCIATIVITY);
        _tagsLastIndex = associativity - 1;
    }
    UINT32 GetAssociativity(UINT32 associativity) {
        return _tagsLastIndex + 1;
    }

    UINT32 Find(CACHE_TAG tag) {
        bool result = true;

        for (UINT32 index = 0; index <= _tagsLastIndex; index++) {
            if (_timeSinceAccess[index] < UINT64_MAX) {
                _timeSinceAccess[index]++;
            }
        }

        for (UINT32 index = 0; index <= _tagsLastIndex; index++) {
            if (_tags[index] == tag) {
                // this is now the most recently used
                _timeSinceAccess[index] = 0;
                return result;
            }
        }
        result = false;

        return result;
    }

    VOID Replace(CACHE_TAG tag) {
        UINT32 index = 0;
        for (UINT32 myIdx = 0; myIdx <= _tagsLastIndex; myIdx++) {
            if (_timeSinceAccess[myIdx] > _timeSinceAccess[index]) {
                index = myIdx;
            }
        }
        _evicted_tag = _tags[index];
        _evicted_dirty = _dirty[index];

        _tags[index] = tag;
        _dirty[index] = 0;
    }

    VOID SetDirtyBit(CACHE_TAG tag) {
        UINT32 index = UINT32_MAX;
        for (UINT32 myIdx = 0; myIdx <= _tagsLastIndex; myIdx++) {
            if (tag == _tags[myIdx]) {
                index = myIdx;
            }
        }
        if (index <= _tagsLastIndex) {
            _dirty[index] = 1;
        } else {
            // attempted to set data value of tag not in cache
            // multithreaded?
            printf("SetDirtyBit of tag: %zu\n", ADDRINT(tag));
            printf("Current cache tags: \n");
            for (UINT32 idx = 0; idx <= _tagsLastIndex; idx++) {
                printf("%zu\n", ADDRINT(_tags[idx]));
            }
            assert(false);
        }

    }

    VOID SetDataValue(CACHE_TAG tag, UINT32 size, char* value) {
        if (_dataTrack) {
            UINT32 index = UINT32_MAX;
            for (UINT32 myIdx = 0; myIdx <= _tagsLastIndex; myIdx++) {
                if (tag == _tags[myIdx]) {
                    index = myIdx;
                }
            }
            if (index <= _tagsLastIndex) {
                memset(_dataValue[index], 0, LINE_SIZE*sizeof(char));
                memcpy(_dataValue[index], value, size);
            } else {
                // attempted to set data value of tag not in cache
                // multithreaded?
                printf("SetDataValue of tag: %zu\n", ADDRINT(tag));
                printf("Current cache tags: \n");
                for (UINT32 idx = 0; idx <= _tagsLastIndex; idx++) {
                    printf("%zu\n", ADDRINT(_tags[idx]));
                }
                assert(false);
            }
        }
    }

    VOID GetDataValue(CACHE_TAG tag, UINT32 size, char* value) {
        if (_dataTrack) {
            UINT32 index = UINT32_MAX;
            for (UINT32 myIdx = 0; myIdx <= _tagsLastIndex; myIdx++) {
                if (tag == _tags[myIdx]) {
                    index = myIdx;
                }
            }
            if (index <= _tagsLastIndex) {
                memcpy(value, _dataValue[index], size);
            } else {
                // attempted to set data value of tag not in cache
                // multithreaded?
                printf("SetDataValue of tag: %zu\n", ADDRINT(tag));
                printf("Current cache tags: \n");
                for (UINT32 idx = 0; idx <= _tagsLastIndex; idx++) {
                    printf("%zu\n", ADDRINT(_tags[idx]));
                }
                assert(false);
            }
        }
    }
};

} // namespace CACHE_SET

namespace CACHE_ALLOC {
    typedef enum {
        STORE_ALLOCATE,
        STORE_NO_ALLOCATE
    } STORE_ALLOCATION;
} // namespace CACHE_ALLOC

/*!
 *  @brief Generic cache base class; no allocate specialization, no cache set specialization
 */
class CACHE_BASE {
  public:
    // types, constants
    typedef enum {
        ACCESS_TYPE_LOAD,
        ACCESS_TYPE_STORE,
        ACCESS_TYPE_NUM
    } ACCESS_TYPE;

    typedef enum {
        CACHE_TYPE_ICACHE,
        CACHE_TYPE_DCACHE,
        CACHE_TYPE_NUM
    } CACHE_TYPE;

  protected:
    static const UINT32 HIT_MISS_NUM = 2;
    CACHE_STATS _access[ACCESS_TYPE_NUM][HIT_MISS_NUM];

  private:    // input params
    const std::string _name;
    const UINT32 _cacheSize;
    const UINT32 _lineSize;
    const UINT32 _associativity;

    // computed params
    const UINT32 _lineShift;
    const UINT32 _setIndexMask;

    CACHE_STATS SumAccess(bool hit) const {
        CACHE_STATS sum = 0;

        for (UINT32 accessType = 0; accessType < ACCESS_TYPE_NUM; accessType++) {
            sum += _access[accessType][hit];
        }

        return sum;
    }

  protected:
    UINT32 NumSets() const { return _setIndexMask + 1; }

  public:
    // constructors/destructors
    CACHE_BASE(std::string name, UINT32 cacheSize, UINT32 lineSize, UINT32 associativity);

    // accessors
    UINT32 CacheSize() const { return _cacheSize; }
    UINT32 LineSize() const { return _lineSize; }
    UINT32 Associativity() const { return _associativity; }
    //
    CACHE_STATS Hits(ACCESS_TYPE accessType) const { return _access[accessType][true];}
    CACHE_STATS Misses(ACCESS_TYPE accessType) const { return _access[accessType][false];}
    CACHE_STATS Accesses(ACCESS_TYPE accessType) const { return Hits(accessType) + Misses(accessType);}
    CACHE_STATS Hits() const { return SumAccess(true);}
    CACHE_STATS Misses() const { return SumAccess(false);}
    CACHE_STATS Accesses() const { return Hits() + Misses();}

    VOID SplitAddress(const ADDRINT addr, CACHE_TAG & tag, UINT32 & setIndex) const {
        tag = addr >> _lineShift;
        setIndex = tag & _setIndexMask;
    }

    VOID SplitAddress(const ADDRINT addr, CACHE_TAG & tag, UINT32 & setIndex, UINT32 & lineIndex) const {
        const UINT32 lineMask = _lineSize - 1;
        lineIndex = addr & lineMask;
        SplitAddress(addr, tag, setIndex);
    }

    ADDRINT MergeAddress(CACHE_TAG tag) {
        return ADDRINT(tag) << _lineShift;
    }

    string StatsLong(string prefix = "", CACHE_TYPE = CACHE_TYPE_DCACHE) const;
};

CACHE_BASE::CACHE_BASE(std::string name, UINT32 cacheSize, UINT32 lineSize, UINT32 associativity)
  : _name(name),
    _cacheSize(cacheSize),
    _lineSize(lineSize),
    _associativity(associativity),
    _lineShift(FloorLog2(lineSize)),
    _setIndexMask((cacheSize / (associativity * lineSize)) - 1) {

    ASSERTX(IsPower2(_lineSize));
    ASSERTX(IsPower2(_setIndexMask + 1));

    for (UINT32 accessType = 0; accessType < ACCESS_TYPE_NUM; accessType++) {
        _access[accessType][false] = 0;
        _access[accessType][true] = 0;
    }
}

/*!
 *  @brief Stats output method
 */

string CACHE_BASE::StatsLong(string prefix, CACHE_TYPE cache_type) const {
    const UINT32 headerWidth = 19;
    const UINT32 numberWidth = 12;

    string out;

    out += prefix + _name + ":" + "\n";

    if (cache_type != CACHE_TYPE_ICACHE) {
        for (UINT32 i = 0; i < ACCESS_TYPE_NUM; i++) {
            const ACCESS_TYPE accessType = ACCESS_TYPE(i);

            std::string type(accessType == ACCESS_TYPE_LOAD ? "Load" : "Store");

            out += prefix + ljstr(type + "-Hits:      ", headerWidth)
                + mydecstr(Hits(accessType), numberWidth)  +
                "  " +fltstr(100.0 * Hits(accessType) / Accesses(accessType), 2, 6) + "%\n";

            out += prefix + ljstr(type + "-Misses:    ", headerWidth)
                + mydecstr(Misses(accessType), numberWidth) +
                "  " +fltstr(100.0 * Misses(accessType) / Accesses(accessType), 2, 6) + "%\n";

            out += prefix + ljstr(type + "-Accesses:  ", headerWidth)
                + mydecstr(Accesses(accessType), numberWidth) +
                "  " +fltstr(100.0 * Accesses(accessType) / Accesses(accessType), 2, 6) + "%\n";

            out += prefix + "\n";
        }
    }

    out += prefix + ljstr("Total-Hits:      ", headerWidth)
           + mydecstr(Hits(), numberWidth) +
           "  " +fltstr(100.0 * Hits() / Accesses(), 2, 6) + "%\n";

    out += prefix + ljstr("Total-Misses:    ", headerWidth)
           + mydecstr(Misses(), numberWidth) +
           "  " +fltstr(100.0 * Misses() / Accesses(), 2, 6) + "%\n";

    out += prefix + ljstr("Total-Accesses:  ", headerWidth)
           + mydecstr(Accesses(), numberWidth) +
           "  " +fltstr(100.0 * Accesses() / Accesses(), 2, 6) + "%\n";
    out += "\n";

    return out;
}


/*!
 *  @brief Templated cache class with specific cache set allocation policies
 *
 *  All that remains to be done here is allocate and deallocate the right
 *  type of cache sets.
 */
template <class SET, UINT32 MAX_SETS, UINT32 STORE_ALLOCATION, UINT32 LINE_SIZE>
class CACHE : public CACHE_BASE {
    private:
        SET _sets[MAX_SETS];
        bool _data;

    public:
        // constructors/destructors
        CACHE(std::string name, UINT32 cacheSize, UINT32 associativity)
            : CACHE_BASE(name, cacheSize, LINE_SIZE, associativity) {

            ASSERTX(NumSets() <= MAX_SETS);

            for (UINT32 i = 0; i < NumSets(); i++) {
                _sets[i].SetAssociativity(associativity);
            }
        }

        // modifiers
        /// Cache access from addr to addr+size-1
        CACHE_ACCESS_STRUCT Access(ADDRINT addr, UINT32 size, ACCESS_TYPE accessType, char* value);
        /// Cache access at addr that does not span cache lines
        CACHE_ACCESS_STRUCT AccessSingleLine(ADDRINT addr, UINT32 size, ACCESS_TYPE accessType, char* value);
};

/*!
 *  @return true if all accessed cache lines hit
 */

template <class SET, UINT32 MAX_SETS, UINT32 STORE_ALLOCATION, UINT32 LINE_SIZE>
CACHE_ACCESS_STRUCT CACHE<SET,MAX_SETS,STORE_ALLOCATION,LINE_SIZE>::Access(ADDRINT addr, UINT32 size, ACCESS_TYPE accessType, char* value) {
    CACHE_ACCESS_STRUCT retval;
    retval.evicted_dirty = false;
    const ADDRINT highAddr = addr + size;
    bool allHit = true;

    const ADDRINT lineSize = LineSize();
    assert(lineSize == LINE_SIZE);
    const ADDRINT notLineMask = ~(lineSize - 1);
    UINT32 this_size;

    do {
        if (size > LINE_SIZE) {
            this_size = LINE_SIZE;
            size = size - LINE_SIZE;
        } else {
            this_size = size;
        }

        CACHE_TAG tag;
        UINT32 setIndex;

        SplitAddress(addr, tag, setIndex);

        SET & set = _sets[setIndex];

        bool localHit = set.Find(tag);
        allHit &= localHit;

        // on hit, writes set dirty bit of cache line
        if (localHit && accessType == ACCESS_TYPE_STORE) {
            set.SetDirtyBit(tag);
        }

        // on miss, loads always allocate, stores optionally
        if ((!localHit) &&
            (accessType == ACCESS_TYPE_LOAD ||
                STORE_ALLOCATION == CACHE_ALLOC::STORE_ALLOCATE)) {

            set.Replace(tag);

            if (set._evicted_dirty) {
                retval.evicted_tag = set._evicted_tag;
                retval.evicted_addr = MergeAddress(set._evicted_tag);
                retval.evicted_dirty = true;
            }
        }
        addr = (addr & notLineMask) + lineSize; // start of next cache line
        if (value != NULL) {
            set.SetDataValue(tag, this_size, value);
            if (accessType == ACCESS_TYPE_LOAD) {
                set.GetDataValue(tag, this_size, value);
            }

            value = value + LINE_SIZE;
        }
    } while (addr < highAddr);

    _access[accessType][allHit]++;


    retval.hit = allHit;
    return retval;
}

/*!
 *  @return true if accessed cache line hits
 */
template <class SET, UINT32 MAX_SETS, UINT32 STORE_ALLOCATION, UINT32 LINE_SIZE>
CACHE_ACCESS_STRUCT CACHE<SET,MAX_SETS,STORE_ALLOCATION,LINE_SIZE>::AccessSingleLine(ADDRINT addr, UINT32 size, ACCESS_TYPE accessType, char* value) {
    assert(size <= LINE_SIZE);
    CACHE_ACCESS_STRUCT retval;
    retval.evicted_dirty = false;
    CACHE_TAG tag;
    UINT32 setIndex;

    SplitAddress(addr, tag, setIndex);

    SET & set = _sets[setIndex];

    bool hit = set.Find(tag);

    // on miss, loads always allocate, stores optionally
    if ((!hit) &&
        (accessType == ACCESS_TYPE_LOAD ||
            STORE_ALLOCATION == CACHE_ALLOC::STORE_ALLOCATE)) {

        set.Replace(tag);
        if (set._evicted_dirty) {
            retval.evicted_tag = set._evicted_tag;
            retval.evicted_dirty = true;
        }
    }

    if (value != NULL) {
        set.SetDataValue(tag, size, value);
        if (accessType == ACCESS_TYPE_LOAD) {
            set.GetDataValue(tag, size, value);
        }
    }

    _access[accessType][hit]++;

    retval.hit = hit;
    return retval;
}

// define shortcuts
#define CACHE_DIRECT_MAPPED(MAX_SETS, ALLOCATION, LINE_SIZE) CACHE<CACHE_SET::DIRECT_MAPPED, MAX_SETS, ALLOCATION, LINE_SIZE>
#define CACHE_ROUND_ROBIN(MAX_SETS, MAX_ASSOCIATIVITY, ALLOCATION, LINE_SIZE) CACHE<CACHE_SET::ROUND_ROBIN<MAX_ASSOCIATIVITY>, MAX_SETS, ALLOCATION, LINE_SIZE>
#define CACHE_LRU(MAX_SETS, MAX_ASSOCIATIVITY, ALLOCATION, LINE_SIZE, DATA) CACHE<CACHE_SET::LRU<MAX_ASSOCIATIVITY, LINE_SIZE, DATA>, MAX_SETS, ALLOCATION, LINE_SIZE>

#endif // PIN_CACHE_H

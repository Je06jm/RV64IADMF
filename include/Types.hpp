#ifndef TYPES_HPP
#define TYPES_HPP

#include <cstdint>

using Address = uint64_t;

using Long = uint64_t;
using Word = uint32_t;
using Half = uint16_t;
using Byte = uint8_t;

using SAddress = int64_t;

using SLong = int64_t;
using SWord = int32_t;
using SHalf = int16_t;
using SByte = int8_t;

using Hart = uint64_t;

#undef LONG_MIN
#undef LONG_MAX

constexpr Address ADDRESS_MIN = 0;
constexpr Address ADDRESS_MAX = UINT64_MAX;

constexpr Long LONG_MIN = 0;
constexpr Long LONG_MAX = UINT64_MAX;

constexpr Word WORD_MIN = 0;
constexpr Word WORD_MAX = UINT32_MAX;

constexpr Half HALF_MIN = 0;
constexpr Half HALF_MAX = UINT16_MAX;

constexpr Byte BYTE_MIN = 0;
constexpr Byte BYTE_MAX = UINT8_MAX;

constexpr SAddress SADDRESS_MIN = INT64_MIN;
constexpr SAddress SADDRESS_MAX = INT64_MAX;

constexpr SLong SLONG_MIN = INT64_MIN;
constexpr SLong SLONG_MAX = INT64_MAX;

constexpr SWord SWORD_MIN = INT32_MIN;
constexpr SWord SWORD_MAX = INT32_MAX;

constexpr SHalf SHALF_MIN = INT16_MIN;
constexpr SHalf SHALF_MAX = INT16_MAX;

constexpr SByte SBYTE_MIN = INT8_MIN;
constexpr SByte SBYTE_MAX = INT8_MAX;

#endif
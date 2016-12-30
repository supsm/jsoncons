// Copyright 2013 Daniel Parker
// Distributed under the Boost license, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See https://github.com/danielaparker/jsoncons for latest version

/*
 * Includes Unicode, Inc decomposition code derived from ConvertUTF.h and ConvertUTF.c 
 * http://www.unicode.org/  
 *  
 * Unicode, Inc. hereby grants the right to freely use the information
 * supplied in this file in the creation of products supporting the
 * Unicode Standard, and to make copies of this file in any form
 * for internal or external distribution as long as this notice
 * remains attached.
*/

#ifndef JSONCONS_JSON_TEXT_TRAITS_HPP
#define JSONCONS_JSON_TEXT_TRAITS_HPP

#include <locale>
#include <string>
#include <vector>
#include <cstdlib>
#include <cwchar>
#include <cstdint> 
#include <iostream>
#include <vector>
#include <jsoncons/jsoncons.hpp>
#include <jsoncons/json_error_category.hpp>

namespace jsoncons {

inline
static bool is_continuation_byte(unsigned char ch)
{
    return (ch & 0xC0) == 0x80;
}

/*
 * Magic values subtracted from a buffer value during UTF8 conversion.
 * This table contains as many values as there might be trailing bytes
 * in a UTF-8 sequence.
 */
const uint32_t offsets_from_utf8[6] = { 0x00000000UL, 0x00003080UL, 0x000E2080UL, 
              0x03C82080UL, 0xFA082080UL, 0x82082080UL };

/*
 * Once the bits are split out into bytes of UTF-8, this is a mask OR-ed
 * into the first byte, depending on how many bytes follow.  There are
 * as many entries in this table as there are UTF-8 sequence types.
 * (I.e., one byte sequence, two byte... etc.). Remember that sequencs
 * for *legal* UTF-8 will be 4 or fewer bytes total.
 */
const uint8_t first_byte_mark[7] = { 0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC };

/*
 * Index into the table below with the first byte of a UTF-8 sequence to
 * get the number of trailing bytes that are supposed to follow it.
 * Note that *legal* UTF-8 values can't have 4 or 5-bytes. The table is
 * left as-is for anyone who may want to do such conversion, which was
 * allowed in earlier algorithms.
 */
const uint8_t trailing_bytes_for_utf8[256] = {
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 3,3,3,3,3,3,3,3,4,4,4,4,5,5,5,5
};

// Some fundamental constants 
const uint32_t uni_replacement_char = 0x0000FFFD;
const uint32_t uni_max_bmp = 0x0000FFFF;
const uint32_t uni_max_utf16 = 0x0010FFFF;
const uint32_t uni_max_utf32 = 0x7FFFFFFF;
const uint32_t uni_max_legal_utf32 = 0x0010FFFF;

const int half_shift  = 10; // used for shifting by 10 bits
const uint32_t half_base = 0x0010000UL;
const uint32_t half_mask = 0x3FFUL;

const uint16_t uni_sur_high_start = 0xD800;
const uint16_t uni_sur_high_end = 0xDBFF;
const uint16_t uni_sur_low_start = 0xDC00;
const uint16_t uni_sur_low_end = 0xDFFF;

inline bool is_leading_surrogate(uint16_t c)
{
    return c >= uni_sur_high_start && c <= uni_sur_high_end;
}

inline bool is_trailing_surrogate(uint16_t c)
{
    return c >= uni_sur_low_start && c <= uni_sur_low_end;
}


// json_text_traits

template <class CharT, class Enable=void>
struct json_text_traits
{
};

template <class CharT>
struct Json_text_traits_
{

    static bool is_control_character(CharT c)
    {
        uint32_t u(c >= 0 ? c : 256 + c);
        return u <= 0x1F || u == 0x7f;
    }

    static CharT to_hex_character(unsigned char c)
    {
        JSONCONS_ASSERT(c <= 0xF);

        return (c < 10) ? ('0' + c) : ('A' - 10 + c);
    }

    static bool is_non_ascii_codepoint(uint32_t cp)
    {
        return cp >= 0x80;
    }

    static std::pair<const CharT*,size_t> char_sequence_at(const CharT* it, 
                                                           const CharT* end,
                                                           size_t index)
    {
        const CharT* p = it;
        size_t count = 0;

        while (p < end && count < index)
        {
            size_t length = json_text_traits<CharT>::codepoint_length(p,end);
            p += length;
            ++count;
        }
        size_t len = json_text_traits<CharT>::codepoint_length(p,end);
        return (count == index) ? std::make_pair(p,len) : std::make_pair(it,static_cast<size_t>(0));
    }

    static size_t codepoint_count(const CharT* it, 
                                  const CharT* end)
    {
        size_t count = 0;
        const CharT* p = it;
        while (p < end)
        {
            p += json_text_traits<CharT>::codepoint_length(p,end);
            ++count;
        }
        return count;
    }

    static uint32_t codepoint_at(const CharT* it, 
                                 const CharT* end,
                                 size_t index)
    {
        uint32_t cp = 0;
        const CharT* p = it;
        size_t count = 0;
        while (p < end && count <= index)
        {
            cp = json_text_traits<CharT>::char_sequence_to_codepoint(p,end,&p);
            ++count;
        }
        return cp;
    }
};

enum class uni_conversion_result
{
    ok,                  // conversion successful
    source_exhausted,    // partial character in source, but hit end
    source_illegal       // source sequence is illegal/malformed
};

enum class uni_conversion_flags
{
    strict = 0,
    lenient
};

template <class CharT>
struct json_text_traits<CharT,
                        typename std::enable_if<std::is_integral<CharT>::value &&
                        sizeof(CharT)==sizeof(uint8_t)>::type> : public Json_text_traits_<CharT>
{
    static const std::pair<const CharT*,size_t>& null_literal() 
    {
        static const std::pair<const CharT*,size_t> value = {"null",4};
        return value;
    }

    static const std::pair<const CharT*,size_t>& true_literal() 
    {
        static const std::pair<const CharT*,size_t> value = {"true",4};
        return value;
    }

    static const std::pair<const CharT*,size_t>& false_literal() 
    {
        static const std::pair<const CharT*,size_t> value = {"false",5};
        return value;
    }

    /*
     * Utility routine to tell whether a sequence of bytes is legal UTF-8.
     * This must be called with the length pre-determined by the first byte.
     * If not calling this from ConvertUTF8to*, then the length can be set by:
     *  length = trailing_bytes_for_utf8[*source]+1;
     * and the sequence is illegal right away if there aren't that many bytes
     * available.
     * If presented with a length > 4, this returns false.  The Unicode
     * definition of UTF-8 goes up to 4-byte sequences.
     */

    static bool is_legal(const CharT *source, int length) {
        CharT a;
        const CharT *srcptr = source+length;
        switch (length) {
        default: return false;
            /* Everything else falls through when "true"... */
        case 4: if ((a = (*--srcptr)) < 0x80 || a > 0xBF) return false;
        case 3: if ((a = (*--srcptr)) < 0x80 || a > 0xBF) return false;
        case 2: if ((a = (*--srcptr)) < 0x80 || a > 0xBF) return false;

            switch (*source) {
                /* no fall-through in this inner switch */
                case 0xE0: if (a < 0xA0) return false; break;
                case 0xED: if (a > 0x9F) return false; break;
                case 0xF0: if (a < 0x90) return false; break;
                case 0xF4: if (a > 0x8F) return false; break;
                default:   if (a < 0x80) return false;
            }

        case 1: if (*source >= 0x80 && *source < 0xC2) return false;
        }
        if (*source > 0xF4) return false;
        return true;
    }

    template <class UTF8,class STraits,class SAllocator>
    static typename std::enable_if<std::is_integral<UTF8>::value && sizeof(UTF8) == sizeof(uint8_t),uni_conversion_result>::type 
    to_utf8(const CharT** source_begin, const CharT* source_end, 
            std::basic_string<UTF8,STraits,SAllocator>& target, uni_conversion_flags) 
    {
        target.assign(*source_begin,source_end);
        uni_conversion_result result = uni_conversion_result::ok;
        return result;
    }

    template <class UTF8,class STraits,class SAllocator>
    static typename std::enable_if<std::is_integral<UTF8>::value && sizeof(UTF8) == sizeof(uint8_t),uni_conversion_result>::type 
    from_utf8(const UTF8** source_begin, const UTF8* source_end, 
              std::basic_string<CharT,STraits,SAllocator>& target, 
              uni_conversion_flags) 
    {
        target.assign(*source_begin,source_end);
        uni_conversion_result result = uni_conversion_result::ok;
        return result;
    }

    template <class UTF32>
    static typename std::enable_if<std::is_integral<UTF32>::value && sizeof(UTF32) == sizeof(uint32_t),uni_conversion_result>::type 
    next_codepoint(const CharT** source_begin, const CharT* source_end, 
                                         UTF32* target, 
                                         uni_conversion_flags flags) 
    {
        uni_conversion_result result = uni_conversion_result::ok;

        const CharT* source = *source_begin;

        unsigned short extraBytesToRead = trailing_bytes_for_utf8[*source];
        if (extraBytesToRead >= source_end - source) {
            result = uni_conversion_result::source_exhausted;
            return result;
        }
        /* Do this check whether lenient or strict */
        if (!json_text_traits<CharT>::is_legal(source, extraBytesToRead+1)) {
            result = uni_conversion_result::source_illegal;
            return result;
        }
        /*
         * The cases all fall through. See "Note A" below.
         */
        UTF32 ch = 0;
        switch (extraBytesToRead) {
            case 5: ch += *source++; ch <<= 6;
            case 4: ch += *source++; ch <<= 6;
            case 3: ch += *source++; ch <<= 6;
            case 2: ch += *source++; ch <<= 6;
            case 1: ch += *source++; ch <<= 6;
            case 0: ch += *source++;
        }
        ch -= offsets_from_utf8[extraBytesToRead];

        if (ch <= uni_max_legal_utf32) {
            /*
             * UTF-16 surrogate values are illegal in UTF-32, and anything
             * over Plane 17 (> 0x10FFFF) is illegal.
             */
            if (ch >= uni_sur_high_start && ch <= uni_sur_low_end) {
                if (flags == uni_conversion_flags::strict) {
                    source -= (extraBytesToRead+1); /* return to the illegal value itself */
                    result = uni_conversion_result::source_illegal;
                } else {
                    *target = uni_replacement_char;
                }
            } else {
                *target = ch;
            }
        } else { /* i.e., ch > UNI_MAX_LEGAL_UTF32 */
            result = uni_conversion_result::source_illegal;
            *target = uni_replacement_char;
        }
        *source_begin = source;
        return result;
    }

    static size_t detect_bom(const CharT* it, size_t length)
    {
        size_t count = 0;
        if (length >= 3)
        {
            uint32_t bom = static_cast<uint32_t>(static_cast<uint8_t>(it[0]) |
                                                (static_cast<uint8_t>(it[1]) << 8) |
                                                (static_cast<uint8_t>(it[2]) << 16));
            if ((bom & 0xFFFFFF) == 0xBFBBEF)  
                count += 3;
        }
        return count;
    }

    static uint32_t char_sequence_to_codepoint(const CharT* it, 
                                               const CharT* end,
                                               const CharT** stop)
    {
        CharT c = *it;
        uint32_t u(c >= 0 ? c : 256 + c );
        uint32_t cp = u;
        if (u < 0x80)
        {
            *stop = it + 1;
        }
        else if ((u >> 5) == 0x6 && (end-it) > 1)
        {
            c = *(++it);
            u = (c >= 0 ? c : 256 + c );
            cp = ((cp << 6) & 0x7ff) + (u & 0x3f);
            *stop = it + 1;
        }
        else if ((u >> 4) == 0xe && (end-it) > 2)
        {
            c = *(++it);
            u = (c >= 0 ? c : 256 + c );
            cp = ((cp << 12) & 0xffff) + ((static_cast<uint32_t>(0xff & u) << 6) & 0xfff);
            c = *(++it);
            u = (c >= 0 ? c : 256 + c );
            cp += (u) & 0x3f;
            *stop = it + 1;
        }
        else if ((u >> 3) == 0x1e && (end-it) > 3)
        {
            c = *(++it);
            u = (c >= 0 ? c : 256 + c );
            cp = ((cp << 18) & 0x1fffff) + ((static_cast<uint32_t>(0xff & u) << 12) & 0x3ffff);
            c = *(++it);
            u = (c >= 0 ? c : 256 + c );
            cp += (static_cast<uint32_t>(0xff & u) << 6) & 0xfff;
            c = *(++it);
            u = (c >= 0 ? c : 256 + c );
            cp += (u) & 0x3f;
            *stop = it + 1;
        }
        else
        {
            *stop = it;
        }
        return cp;
    }

    static size_t codepoint_length(const CharT* it, 
                                   const CharT* end)
    {
        size_t length = 0;
        CharT c = *it;
        uint32_t u(c >= 0 ? c : 256 + c );
        if (u < 0x80)
        {
            length = 1;
        }
        else if ((u >> 5) == 0x6 && (end-it) > 1)
        {
            length = 2;
        }
        else if ((u >> 4) == 0xe && (end-it) > 2)
        {
            length = 3;
        }
        else if ((u >> 3) == 0x1e && (end-it) > 3)
        {
            length = 4;
        }
        return length;
    }

    static void append_codepoint_to_string(uint32_t cp, std::string& s)
    {
        if (cp <= 0x7f)
        {
            s.push_back(static_cast<CharT>(cp));
        }
        else if (cp <= 0x7FF)
        {
            s.push_back(static_cast<CharT>(0xC0 | (0x1f & (cp >> 6))));
            s.push_back(static_cast<CharT>(0x80 | (0x3f & cp)));
        }
        else if (cp <= 0xFFFF)
        {
            s.push_back(0xE0 | static_cast<CharT>((0xf & (cp >> 12))));
            s.push_back(0x80 | static_cast<CharT>((0x3f & (cp >> 6))));
            s.push_back(static_cast<CharT>(0x80 | (0x3f & cp)));
        }
        else if (cp <= 0x10FFFF)
        {
            s.push_back(static_cast<CharT>(0xF0 | (0x7 & (cp >> 18))));
            s.push_back(static_cast<CharT>(0x80 | (0x3f & (cp >> 12))));
            s.push_back(static_cast<CharT>(0x80 | (0x3f & (cp >> 6))));
            s.push_back(static_cast<CharT>(0x80 | (0x3f & cp)));
        }
    }

};

template <class CharT>
struct json_text_traits<CharT,
                        typename std::enable_if<std::is_integral<CharT>::value &&
                        sizeof(CharT)==sizeof(uint16_t)>::type> : public Json_text_traits_<CharT>
{
    template <class UTF8,class STraits,class SAllocator>
    static typename std::enable_if<std::is_integral<UTF8>::value && sizeof(UTF8) == sizeof(uint8_t),uni_conversion_result>::type 
    to_utf8(const CharT** source_begin, const CharT* source_end, 
            std::basic_string<UTF8,STraits,SAllocator>& target, uni_conversion_flags flags) {
        uni_conversion_result result = uni_conversion_result::ok;
        const CharT* source = *source_begin;
        while (source < source_end) {
            unsigned short bytes_to_write = 0;
            const uint32_t byteMask = 0xBF;
            const uint32_t byteMark = 0x80; 
            uint32_t ch = *source++;
            /* If we have a surrogate pair, convert to uint32_t first. */
            if (ch >= uni_sur_high_start && ch <= uni_sur_high_end) {
                /* If the 16 bits following the high surrogate are in the source buffer... */
                if (source < source_end) {
                    uint32_t ch2 = *source;
                    /* If it's a low surrogate, convert to uint32_t. */
                    if (ch2 >= uni_sur_low_start && ch2 <= uni_sur_low_end) {
                        ch = ((ch - uni_sur_high_start) << half_shift)
                            + (ch2 - uni_sur_low_start) + half_base;
                        ++source;
                    } else if (flags == uni_conversion_flags::strict) { /* it's an unpaired high surrogate */
                        --source; /* return to the illegal value itself */
                        result = uni_conversion_result::source_illegal;
                        break;
                    }
                } else { /* We don't have the 16 bits following the high surrogate. */
                    --source; /* return to the high surrogate */
                    result = uni_conversion_result::source_exhausted;
                    break;
                }
            } else if (flags == uni_conversion_flags::strict) {
                /* UTF-16 surrogate values are illegal in UTF-32 */
                if (ch >= uni_sur_low_start && ch <= uni_sur_low_end) {
                    --source; /* return to the illegal value itself */
                    result = uni_conversion_result::source_illegal;
                    break;
                }
            }
            /* Figure out how many bytes the result will require */
            if (ch < (uint32_t)0x80) {      
                bytes_to_write = 1;
            } else if (ch < (uint32_t)0x800) {     
                bytes_to_write = 2;
            } else if (ch < (uint32_t)0x10000) {   
                bytes_to_write = 3;
            } else if (ch < (uint32_t)0x110000) {  
                bytes_to_write = 4;
            } else {                            
                bytes_to_write = 3;
                ch = uni_replacement_char;
            }
            target.resize(target.size()+bytes_to_write);
            UTF8* target_ptr = &target[0] + target.length();

            switch (bytes_to_write) { /* note: everything falls through. */
                case 4: *--target_ptr = (uint8_t)((ch | byteMark) & byteMask); ch >>= 6;
                case 3: *--target_ptr = (uint8_t)((ch | byteMark) & byteMask); ch >>= 6;
                case 2: *--target_ptr = (uint8_t)((ch | byteMark) & byteMask); ch >>= 6;
                case 1: *--target_ptr = (uint8_t)(ch | first_byte_mark[bytes_to_write]);
            }
        }
        *source_begin = source;
        return result;
    }

    template <class UTF8,class STraits,class SAllocator>
    static typename std::enable_if<std::is_integral<UTF8>::value && sizeof(UTF8) == sizeof(uint8_t),uni_conversion_result>::type 
    from_utf8(const UTF8** source_begin, const UTF8* source_end, 
              std::basic_string<CharT,STraits,SAllocator>& target, 
              uni_conversion_flags flags) 
    {
        uni_conversion_result result = uni_conversion_result::ok;
        const UTF8* source = *source_begin;
        while (source < source_end) {
            uint32_t ch = 0;
            unsigned short extra_bytes_to_read = trailing_bytes_for_utf8[*source];
            if (extra_bytes_to_read >= source_end - source) {
                result = uni_conversion_result::source_exhausted; break;
            }
            /* Do this check whether lenient or strict */
            if (!json_text_traits<UTF8>::is_legal(source, extra_bytes_to_read+1)) {
                result = uni_conversion_result::source_illegal;
                break;
            }
            /*
             * The cases all fall through. See "Note A" below.
             */
            switch (extra_bytes_to_read) {
                case 5: ch += *source++; ch <<= 6; /* remember, illegal UTF-8 */
                case 4: ch += *source++; ch <<= 6; /* remember, illegal UTF-8 */
                case 3: ch += *source++; ch <<= 6;
                case 2: ch += *source++; ch <<= 6;
                case 1: ch += *source++; ch <<= 6;
                case 0: ch += *source++;
            }
            ch -= offsets_from_utf8[extra_bytes_to_read];

            if (ch <= uni_max_bmp) { /* Target is a character <= 0xFFFF */
                /* UTF-16 surrogate values are illegal in UTF-32 */
                if (ch >= uni_sur_high_start && ch <= uni_sur_low_end) {
                    if (flags == uni_conversion_flags::strict) {
                        source -= (extra_bytes_to_read+1); /* return to the illegal value itself */
                        result = uni_conversion_result::source_illegal;
                        break;
                    } else {
                        target.push_back(uni_replacement_char);
                    }
                } else {
                    target.push_back((uint16_t)ch); /* normal case */
                }
            } else if (ch > uni_max_utf16) {
                if (flags == uni_conversion_flags::strict) {
                    result = uni_conversion_result::source_illegal;
                    source -= (extra_bytes_to_read+1); /* return to the start */
                    break; /* Bail out; shouldn't continue */
                } else {
                    target.push_back( uni_replacement_char);
                }
            } else {
                /* target is a character in range 0xFFFF - 0x10FFFF. */
                ch -= half_base;
                target.push_back((uint16_t)((ch >> half_shift) + uni_sur_high_start));
                target.push_back((uint16_t)((ch & half_mask) + uni_sur_low_start));
            }
        }
        *source_begin = source;
        return result;
    }

    template <class UTF32>
    static typename std::enable_if<std::is_integral<UTF32>::value && sizeof(UTF32) == sizeof(uint32_t),uni_conversion_result>::type 
    next_codepoint(const CharT** source_begin, const CharT* source_end, 
                                         UTF32* target, 
                                         uni_conversion_flags flags) 
    {
        uni_conversion_result result = uni_conversion_result::ok;
        const CharT* source = *source_begin;
        uint32_t ch, ch2;

        ch = *source++;
        /* If we have a surrogate pair, convert to UTF32 first. */
        if (ch >= uni_sur_high_start && ch <= uni_sur_high_end) {
            /* If the 16 bits following the high surrogate are in the source buffer... */
            if (source < source_end) {
                ch2 = *source;
                /* If it's a low surrogate, convert to UTF32. */
                if (ch2 >= uni_sur_low_start && ch2 <= uni_sur_low_end) {
                    ch = ((ch - uni_sur_high_start) << half_shift)
                        + (ch2 - uni_sur_low_start) + half_base;
                    ++source;
                } else if (flags == uni_conversion_flags::strict) { /* it's an unpaired high surrogate */
                    --source; /* return to the illegal value itself */
                    result = uni_conversion_result::source_illegal;
                    return result;
                }
            } else { /* We don't have the 16 bits following the high surrogate. */
                --source; /* return to the high surrogate */
                result = uni_conversion_result::source_exhausted;
            }
        } else if (flags == uni_conversion_flags::strict) {
            /* UTF-16 surrogate values are illegal in UTF-32 */
            if (ch >= uni_sur_low_start && ch <= uni_sur_low_end) {
                --source; /* return to the illegal value itself */
                result = uni_conversion_result::source_illegal;
            }
        }
        *target = ch;

        *source_begin = source;
        return result;
    }

    static const std::pair<const CharT*,size_t>& null_literal() 
    {
        static const std::pair<const CharT*,size_t> value = {L"null",4};
        return value;
    }

    static const std::pair<const CharT*,size_t>& true_literal() 
    {
        static const std::pair<const CharT*,size_t> value = {L"true",4};
        return value;
    }

    static const std::pair<const CharT*,size_t>& false_literal() 
    {
        static const std::pair<const CharT*,size_t> value = {L"false",5};
        return value;
    }

    static size_t detect_bom(const CharT* it, size_t length)
    {
        size_t count = 0;
        if (length >= 1)
        {
            uint32_t bom = static_cast<uint32_t>(it[0]);
            if ((bom & 0xFFFF) == 0xFFFE)      
                ++count;
            else if ((bom & 0xFFFF) == 0xFEFF)      
                ++count;
        }
        return count;
    }

    static void append_codepoint_to_string(uint32_t cp, std::wstring& s)
    {
        if (cp <= 0xFFFF)
        {
            s.push_back(static_cast<CharT>(cp));
        }
        else if (cp <= 0x10FFFF)
        {
            s.push_back(static_cast<CharT>((cp >> 10) + uni_sur_high_start - (0x10000 >> 10)));
            s.push_back(static_cast<CharT>((cp & 0x3ff) + uni_sur_low_start));
        }
    }

    static uint32_t char_sequence_to_codepoint(const CharT* it, const CharT* end, const CharT** stop)
    {
        uint32_t cp = (0xffff & *it);
        if ((cp >= uni_sur_high_start && cp <= uni_sur_high_end) && (end-it) > 1) // surrogate pair
        {
            uint32_t trail_surrogate = 0xffff & *(++it);
            cp = (cp << 10) + trail_surrogate + 0x10000u - (uni_sur_high_start << 10) - uni_sur_low_start;
            *stop = it + 1;
        }
        else if (end > it)
        {
            *stop = it+1;
        }
        else
        {
            *stop = it;
        }
        return cp;
    }

    static size_t codepoint_length(const CharT* it, const CharT* end)
    {
        size_t length = 1;

        uint32_t cp = (0xffff & *it);
        if ((cp >= uni_sur_high_start && cp <= uni_sur_high_end) && (end-it) > 1) // surrogate pair
        {
            length = 2;
        }
        return length;
    }
};

template <class CharT>
struct json_text_traits<CharT,
                        typename std::enable_if<std::is_integral<CharT>::value &&
                        sizeof(CharT)==sizeof(uint32_t)>::type> : public Json_text_traits_<CharT>
{

    static const std::pair<const CharT*,size_t>& null_literal() 
    {
        static const std::pair<const CharT*,size_t> value = {L"null",4};
        return value;
    }

    static const std::pair<const CharT*,size_t>& true_literal() 
    {
        static const std::pair<const CharT*,size_t> value = {L"true",4};
        return value;
    }

    static const std::pair<const CharT*,size_t>& false_literal() 
    {
        static const std::pair<const CharT*,size_t> value = {L"false",5};
        return value;
    }

    template <class UTF8,class STraits,class SAllocator>
    static typename std::enable_if<std::is_integral<UTF8>::value && sizeof(UTF8) == sizeof(uint8_t),uni_conversion_result>::type 
    to_utf8(const CharT** source_begin, const CharT* source_end, 
            std::basic_string<UTF8,STraits,SAllocator>& target, 
            uni_conversion_flags flags) 
    {
        uni_conversion_result result = uni_conversion_result::ok;
        const CharT* source = *source_begin;
        while (source < source_end) {
            uint32_t ch;
            unsigned short bytes_to_write = 0;
            const uint32_t byteMask = 0xBF;
            const uint32_t byteMark = 0x80; 
            ch = *source++;
            if (flags == uni_conversion_flags::strict ) {
                /* UTF-16 surrogate values are illegal in UTF-32 */
                if (ch >= uni_sur_high_start && ch <= uni_sur_low_end) {
                    --source; /* return to the illegal value itself */
                    result = uni_conversion_result::source_illegal;
                    break;
                }
            }
            /*
             * Figure out how many bytes the result will require. Turn any
             * illegally large UTF32 things (> Plane 17) into replacement chars.
             */
            if (ch < (uint32_t)0x80) {      bytes_to_write = 1;
            } else if (ch < (uint32_t)0x800) {     bytes_to_write = 2;
            } else if (ch < (uint32_t)0x10000) {   bytes_to_write = 3;
            } else if (ch <= uni_max_legal_utf32) {  bytes_to_write = 4;
            } else {                            bytes_to_write = 3;
                                                ch = uni_replacement_char;
                                                result = uni_conversion_result::source_illegal;
            }
            
            target.resize(target.size()+bytes_to_write);
            UTF8* target_ptr = &target[0] + target.length();
            switch (bytes_to_write) { /* note: everything falls through. */
                case 4: *--target_ptr = (UTF8)((ch | byteMark) & byteMask); ch >>= 6;
                case 3: *--target_ptr = (UTF8)((ch | byteMark) & byteMask); ch >>= 6;
                case 2: *--target_ptr = (UTF8)((ch | byteMark) & byteMask); ch >>= 6;
                case 1: *--target_ptr = (UTF8) (ch | first_byte_mark[bytes_to_write]);
            }
        }
        *source_begin = source;
        return result;
    }

    template <class UTF8,class STraits,class SAllocator>
    static typename std::enable_if<std::is_integral<UTF8>::value && sizeof(UTF8) == sizeof(uint8_t),uni_conversion_result>::type 
    from_utf8 (
            const UTF8** source_begin, const UTF8* source_end, 
            std::basic_string<CharT,STraits,SAllocator>& target, 
            uni_conversion_flags flags) {
        uni_conversion_result result = uni_conversion_result::ok;
        const UTF8* source = *source_begin;
        while (source < source_end) {
            uint32_t ch = 0;
            unsigned short extra_bytes_to_read = trailing_bytes_for_utf8[*source];
            if (extra_bytes_to_read >= source_end - source) {
                result = uni_conversion_result::source_exhausted; break;
            }
            /* Do this check whether lenient or strict */
            if (!json_text_traits<UTF8>::is_legal(source, extra_bytes_to_read+1)) {
                result = uni_conversion_result::source_illegal;
                break;
            }
            /*
             * The cases all fall through. See "Note A" below.
             */
            switch (extra_bytes_to_read) {
                case 5: ch += *source++; ch <<= 6;
                case 4: ch += *source++; ch <<= 6;
                case 3: ch += *source++; ch <<= 6;
                case 2: ch += *source++; ch <<= 6;
                case 1: ch += *source++; ch <<= 6;
                case 0: ch += *source++;
            }
            ch -= offsets_from_utf8[extra_bytes_to_read];

            if (ch <= uni_max_legal_utf32) {
                /*
                 * UTF-16 surrogate values are illegal in UTF-32, and anything
                 * over Plane 17 (> 0x10FFFF) is illegal.
                 */
                if (ch >= uni_sur_high_start && ch <= uni_sur_low_end) {
                    if (flags == uni_conversion_flags::strict) {
                        source -= (extra_bytes_to_read+1); /* return to the illegal value itself */
                        result = uni_conversion_result::source_illegal;
                        break;
                    } else {
                        target.push_back(uni_replacement_char);
                    }
                } else {
                    target.push_back(ch);
                }
            } else { /* i.e., ch > uni_max_legal_utf32 */
                result = uni_conversion_result::source_illegal;
                target.push_back(uni_replacement_char);
            }
        }
        *source_begin = source;
        return result;
    }

    template <class UTF32>
    static typename std::enable_if<std::is_integral<UTF32>::value && sizeof(UTF32) == sizeof(uint32_t),uni_conversion_result>::type 
    next_codepoint(const CharT** source_begin, const CharT*, 
                                         UTF32* target, 
                                         uni_conversion_flags) 
    {
        const CharT* source = *source_begin;
        *target = *source++;
        *source_begin = source;

        return uni_conversion_result::ok;
    }

    static size_t detect_bom(const CharT* it, size_t length)
    {
        size_t count = 0;
        if (length >= 1)
        {
            uint32_t bom = static_cast<uint32_t>(it[0]);
            if (bom == 0xFFFE0000)                  
                ++count;
            else if (bom == 0x0000FEFF)             
                ++count;
        }
        return count;
    }

    static void append_codepoint_to_string(uint32_t cp, std::wstring& s)
    {
        if (cp <= 0xFFFF)
        {
            s.push_back(static_cast<CharT>(cp));
        }
        else if (cp <= 0x10FFFF)
        {
            s.push_back(static_cast<CharT>(cp));
        }
    }

    static uint32_t char_sequence_to_codepoint(const CharT* it, const CharT*, const CharT** stop)
    {
        uint32_t cp = static_cast<uint32_t>(*it);
        *stop = it + 1;
        return cp;
    }

    static size_t codepoint_length(const CharT* it, const CharT* end)
    {
        return (end > it) ? 1 : 0;
    }
};

}
#endif

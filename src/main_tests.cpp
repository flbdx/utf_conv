/*
 * Copyright 2020 Florent Bondoux
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cassert>
#include <cinttypes>

#include "charset_conv_iconv.h"
#include "utf_conv.h"

#include <vector>
#include <iterator>
#include <fstream>
#include <chrono>

/*
 * Test a conversion/encoder/decoder function with a src and an expected result
 * This is the version for Iterator style functions
 * As all the functions have the arguments with same semantics, this test works for all the conv_X_to_Y, UTF::decode_X and UTF::encode_X functions.
 */
template <typename src_type, typename dst_type>
static bool do_test_iterator(const char *test_name, const char *func_name,
        UTF::RetCode (*conv)(const src_type *, size_t, std::back_insert_iterator<std::vector<dst_type>>, size_t *, size_t *),
        const src_type *src, size_t src_len, const dst_type *ref, size_t ref_len) {
    std::vector<dst_type> test_conv;
    auto test_conv_inserter = std::back_inserter(test_conv);
    size_t consumed = 0, written = 0;
    UTF::RetCode r = conv(src, src_len, test_conv_inserter, &consumed, &written);
    if (r == UTF::RetCode::OK && written == ref_len && consumed == src_len
            && std::equal(test_conv.begin(), test_conv.end(), ref)) {
//        printf("[%s iterator] %s : OK (%zu)\n", test_name, func_name, written);
        return true;
    } else {
        printf("[%s iterator] %s : KO (%d) (%zu %zu | %zu %zu)\n", test_name, func_name, (int) r, written, ref_len, consumed, src_len);
        assert(r == UTF::RetCode::OK);
        assert(written == ref_len);
        assert(consumed == src_len);
        assert(std::equal(test_conv.begin(), test_conv.end(), ref));
        return false;
    }
}

/*
 * Test a conversion/encoder/decoder function with a src and an expected result
 * This is the version for getline style functions
 * As all the functions have the arguments with same semantics, this test works for all the conv_X_to_Y, UTF::decode_X and UTF::encode_X functions.
 */
template <typename src_type, typename dst_type>
static bool do_test_buffer(const char *test_name, const char *func_name,
        UTF::RetCode (*conv)(const src_type *, size_t, dst_type **, size_t *, size_t *, size_t *),
        const src_type *src, size_t src_len, const dst_type *ref, size_t ref_len) {
    dst_type *test_conv = NULL;
    size_t test_conv_size = 0;
    size_t consumed = 0, written = 0;
    UTF::RetCode r = conv(src, src_len, &test_conv, &test_conv_size, &consumed, &written);
    if (r == UTF::RetCode::OK && written == ref_len && consumed == src_len
            && std::equal(test_conv, test_conv + r, ref)) {
//        printf("[%s buffer] %s : OK (%zu)\n", test_name, func_name, written);
        free(test_conv);
        return true;
    } else {
        printf("[%s buffer] %s : KO (%d) (%zu %zu | %zu %zu)\n", test_name, func_name, r, written, ref_len, consumed, src_len);
        free(test_conv);
        assert(r == UTF::RetCode::OK);
        assert(written == ref_len);
        assert(consumed == src_len);
        assert(std::equal(test_conv, test_conv + r, ref));
        return false;
    }
}

static bool do_test_decode_one(const char *test_name, const char *func_name,
    UTF::RetCode (*decode)(const char *, size_t, uint32_t *, size_t *),
    const char *src, size_t src_len, const uint32_t *ref, size_t ref_len) {
    uint32_t cp;
    size_t consumed = 0;
    while (src_len > 0) {
        UTF::RetCode r = decode(src, src_len, &cp, &consumed);
        if (r == UTF::RetCode::OK) {
            if (ref_len > 0 && cp == *ref) {
                // OK
                src += consumed;
                src_len -= consumed;
                ref ++;
                ref_len --;
            }
            else {
                printf("[%s decode_one] %s : KO (%d)\n", test_name, func_name, r);
                assert(ref_len > 0);
                assert(cp == *ref);
                return false;
            }
        }
        else {
            printf("[%s decode_one] %s : KO (%d)\n", test_name, func_name, r);
            assert(r == UTF::RetCode::OK);
            return false;
        }
    }
    
    return true;
}

/*
 * Run all conversions tests (valid tests)
 * str_utf8 is the source
 */
static void do_tests(const char *test_name, const char *str_utf8) {
    size_t str_utf8_len = strlen(str_utf8);

    // create some references using iconv
    std::vector<char> str_utf16le;
    std::vector<char> str_utf16be;
    std::vector<char> str_utf32le;
    std::vector<char> str_utf32be;

    size_t consumed;
    ssize_t str_utf16le_len = iconv_convert("UTF-16LE", "UTF-8", str_utf8, str_utf8_len, str_utf16le, &consumed);
    assert(consumed == str_utf8_len);
    ssize_t str_utf16be_len = iconv_convert("UTF-16BE", "UTF-8", str_utf8, str_utf8_len, str_utf16be, &consumed);
    assert(consumed == str_utf8_len);
    ssize_t str_utf32le_len = iconv_convert("UTF-32LE", "UTF-8", str_utf8, str_utf8_len, str_utf32le, &consumed);
    assert(consumed == str_utf8_len);
    ssize_t str_utf32be_len = iconv_convert("UTF-32BE", "UTF-8", str_utf8, str_utf8_len, str_utf32be, &consumed);
    assert(consumed == str_utf8_len);

    // for encode/decode functions, the codepoints are the same as the UTF-32 versions in the host encoding
#ifdef LITTLE_ENDIAN
    const auto &unicode_ref = str_utf32le;
    const auto &unicode_ref_len = str_utf32le_len;
#else
    const auto &unicode_ref = str_utf32be;
    const auto &unicode_ref_len = str_utf32be_len;
#endif

    do_test_iterator(test_name, "UTF-8 -> UTF-16LE", UTF::conv_utf8_to_utf16le, str_utf8, str_utf8_len, str_utf16le.data(), str_utf16le_len);
    do_test_iterator(test_name, "UTF-8 -> UTF-16BE", UTF::conv_utf8_to_utf16be, str_utf8, str_utf8_len, str_utf16be.data(), str_utf16be_len);
    do_test_iterator(test_name, "UTF-8 -> UTF-32LE", UTF::conv_utf8_to_utf32le, str_utf8, str_utf8_len, str_utf32le.data(), str_utf32le_len);
    do_test_iterator(test_name, "UTF-8 -> UTF-32BE", UTF::conv_utf8_to_utf32be, str_utf8, str_utf8_len, str_utf32be.data(), str_utf32be_len);
    do_test_iterator(test_name, "UTF-8 -> UNICODE", UTF::decode_utf8, str_utf8, str_utf8_len, (uint32_t *) unicode_ref.data(), unicode_ref_len / 4);
    do_test_iterator(test_name, "UNICODE -> UTF-8", UTF::encode_utf8, (uint32_t *) unicode_ref.data(), unicode_ref_len / 4, str_utf8, str_utf8_len);

    do_test_iterator(test_name, "UTF-16LE -> UTF-8", UTF::conv_utf16le_to_utf8, str_utf16le.data(), str_utf16le_len, str_utf8, str_utf8_len);
    do_test_iterator(test_name, "UTF-16LE -> UTF-16BE", UTF::conv_utf16le_to_utf16be, str_utf16le.data(), str_utf16le_len, str_utf16be.data(), str_utf16be_len);
    do_test_iterator(test_name, "UTF-16LE -> UTF-32LE", UTF::conv_utf16le_to_utf32le, str_utf16le.data(), str_utf16le_len, str_utf32le.data(), str_utf32le_len);
    do_test_iterator(test_name, "UTF-16LE -> UTF-32BE", UTF::conv_utf16le_to_utf32be, str_utf16le.data(), str_utf16le_len, str_utf32be.data(), str_utf32be_len);
    do_test_iterator(test_name, "UTF-16LE -> UNICODE", UTF::decode_utf16le, str_utf16le.data(), str_utf16le_len, (uint32_t *) unicode_ref.data(), unicode_ref_len / 4);
    do_test_iterator(test_name, "UNICODE -> UTF-16LE", UTF::encode_utf16le, (uint32_t *) unicode_ref.data(), unicode_ref_len / 4, str_utf16le.data(), str_utf16le_len);

    do_test_iterator(test_name, "UTF-16BE -> UTF-8", UTF::conv_utf16be_to_utf8, str_utf16be.data(), str_utf16be_len, str_utf8, str_utf8_len);
    do_test_iterator(test_name, "UTF-16BE -> UTF-16LE", UTF::conv_utf16be_to_utf16le, str_utf16be.data(), str_utf16be_len, str_utf16le.data(), str_utf16le_len);
    do_test_iterator(test_name, "UTF-16BE -> UTF-32LE", UTF::conv_utf16be_to_utf32le, str_utf16be.data(), str_utf16be_len, str_utf32le.data(), str_utf32le_len);
    do_test_iterator(test_name, "UTF-16BE -> UTF-32BE", UTF::conv_utf16be_to_utf32be, str_utf16be.data(), str_utf16be_len, str_utf32be.data(), str_utf32be_len);
    do_test_iterator(test_name, "UTF-16BE -> UNICODE", UTF::decode_utf16be, str_utf16be.data(), str_utf16be_len, (uint32_t *) unicode_ref.data(), unicode_ref_len / 4);
    do_test_iterator(test_name, "UNICODE -> UTF-16BE", UTF::encode_utf16be, (uint32_t *) unicode_ref.data(), unicode_ref_len / 4, str_utf16be.data(), str_utf16be_len);

    do_test_iterator(test_name, "UTF-32LE -> UTF-8", UTF::conv_utf32le_to_utf8, str_utf32le.data(), str_utf32le_len, str_utf8, str_utf8_len);
    do_test_iterator(test_name, "UTF-32LE -> UTF-16LE", UTF::conv_utf32le_to_utf16le, str_utf32le.data(), str_utf32le_len, str_utf16le.data(), str_utf16le_len);
    do_test_iterator(test_name, "UTF-32LE -> UTF-16BE", UTF::conv_utf32le_to_utf16be, str_utf32le.data(), str_utf32le_len, str_utf16be.data(), str_utf16be_len);
    do_test_iterator(test_name, "UTF-32LE -> UTF-32BE", UTF::conv_utf32le_to_utf32be, str_utf32le.data(), str_utf32le_len, str_utf32be.data(), str_utf32be_len);
    do_test_iterator(test_name, "UTF-32LE -> UNICODE", UTF::decode_utf32le, str_utf32le.data(), str_utf32le_len, (uint32_t *) unicode_ref.data(), unicode_ref_len / 4);
    do_test_iterator(test_name, "UNICODE -> UTF-32LE", UTF::encode_utf32le, (uint32_t *) unicode_ref.data(), unicode_ref_len / 4, str_utf32le.data(), str_utf32le_len);

    do_test_iterator(test_name, "UTF-32BE -> UTF-8", UTF::conv_utf32be_to_utf8, str_utf32be.data(), str_utf32be_len, str_utf8, str_utf8_len);
    do_test_iterator(test_name, "UTF-32BE -> UTF-16LE", UTF::conv_utf32be_to_utf16le, str_utf32be.data(), str_utf32be_len, str_utf16le.data(), str_utf16le_len);
    do_test_iterator(test_name, "UTF-32BE -> UTF-16BE", UTF::conv_utf32be_to_utf16be, str_utf32be.data(), str_utf32be_len, str_utf16be.data(), str_utf16be_len);
    do_test_iterator(test_name, "UTF-32BE -> UTF-32LE", UTF::conv_utf32be_to_utf32le, str_utf32be.data(), str_utf32be_len, str_utf32le.data(), str_utf32le_len);
    do_test_iterator(test_name, "UTF-32BE -> UNICODE", UTF::decode_utf32be, str_utf32be.data(), str_utf32be_len, (uint32_t *) unicode_ref.data(), unicode_ref_len / 4);
    do_test_iterator(test_name, "UNICODE -> UTF-32BE", UTF::encode_utf32be, (uint32_t *) unicode_ref.data(), unicode_ref_len / 4, str_utf32be.data(), str_utf32be_len);

    do_test_buffer(test_name, "UTF-8 -> UTF-16LE", UTF::conv_utf8_to_utf16le, str_utf8, str_utf8_len, str_utf16le.data(), str_utf16le_len);
    do_test_buffer(test_name, "UTF-8 -> UTF-16BE", UTF::conv_utf8_to_utf16be, str_utf8, str_utf8_len, str_utf16be.data(), str_utf16be_len);
    do_test_buffer(test_name, "UTF-8 -> UTF-32LE", UTF::conv_utf8_to_utf32le, str_utf8, str_utf8_len, str_utf32le.data(), str_utf32le_len);
    do_test_buffer(test_name, "UTF-8 -> UTF-32BE", UTF::conv_utf8_to_utf32be, str_utf8, str_utf8_len, str_utf32be.data(), str_utf32be_len);
    do_test_buffer(test_name, "UTF-8 -> UNICODE", UTF::decode_utf8, str_utf8, str_utf8_len, (uint32_t *) unicode_ref.data(), unicode_ref_len / 4);
    do_test_buffer(test_name, "UNICODE -> UTF-8", UTF::encode_utf8, (uint32_t *) unicode_ref.data(), unicode_ref_len / 4, str_utf8, str_utf8_len);

    do_test_buffer(test_name, "UTF-16LE -> UTF-8", UTF::conv_utf16le_to_utf8, str_utf16le.data(), str_utf16le_len, str_utf8, str_utf8_len);
    do_test_buffer(test_name, "UTF-16LE -> UTF-16BE", UTF::conv_utf16le_to_utf16be, str_utf16le.data(), str_utf16le_len, str_utf16be.data(), str_utf16be_len);
    do_test_buffer(test_name, "UTF-16LE -> UTF-32LE", UTF::conv_utf16le_to_utf32le, str_utf16le.data(), str_utf16le_len, str_utf32le.data(), str_utf32le_len);
    do_test_buffer(test_name, "UTF-16LE -> UTF-32BE", UTF::conv_utf16le_to_utf32be, str_utf16le.data(), str_utf16le_len, str_utf32be.data(), str_utf32be_len);
    do_test_buffer(test_name, "UTF-16LE -> UNICODE", UTF::decode_utf16le, str_utf16le.data(), str_utf16le_len, (uint32_t *) unicode_ref.data(), unicode_ref_len / 4);
    do_test_buffer(test_name, "UNICODE -> UTF-16LE", UTF::encode_utf16le, (uint32_t *) unicode_ref.data(), unicode_ref_len / 4, str_utf16le.data(), str_utf16le_len);

    do_test_buffer(test_name, "UTF-16BE -> UTF-8", UTF::conv_utf16be_to_utf8, str_utf16be.data(), str_utf16be_len, str_utf8, str_utf8_len);
    do_test_buffer(test_name, "UTF-16BE -> UTF-16LE", UTF::conv_utf16be_to_utf16le, str_utf16be.data(), str_utf16be_len, str_utf16le.data(), str_utf16le_len);
    do_test_buffer(test_name, "UTF-16BE -> UTF-32LE", UTF::conv_utf16be_to_utf32le, str_utf16be.data(), str_utf16be_len, str_utf32le.data(), str_utf32le_len);
    do_test_buffer(test_name, "UTF-16BE -> UTF-32BE", UTF::conv_utf16be_to_utf32be, str_utf16be.data(), str_utf16be_len, str_utf32be.data(), str_utf32be_len);
    do_test_buffer(test_name, "UTF-16BE -> UNICODE", UTF::decode_utf16be, str_utf16be.data(), str_utf16be_len, (uint32_t *) unicode_ref.data(), unicode_ref_len / 4);
    do_test_buffer(test_name, "UNICODE -> UTF-16BE", UTF::encode_utf16be, (uint32_t *) unicode_ref.data(), unicode_ref_len / 4, str_utf16be.data(), str_utf16be_len);

    do_test_buffer(test_name, "UTF-32LE -> UTF-8", UTF::conv_utf32le_to_utf8, str_utf32le.data(), str_utf32le_len, str_utf8, str_utf8_len);
    do_test_buffer(test_name, "UTF-32LE -> UTF-16LE", UTF::conv_utf32le_to_utf16le, str_utf32le.data(), str_utf32le_len, str_utf16le.data(), str_utf16le_len);
    do_test_buffer(test_name, "UTF-32LE -> UTF-16BE", UTF::conv_utf32le_to_utf16be, str_utf32le.data(), str_utf32le_len, str_utf16be.data(), str_utf16be_len);
    do_test_buffer(test_name, "UTF-32LE -> UTF-32BE", UTF::conv_utf32le_to_utf32be, str_utf32le.data(), str_utf32le_len, str_utf32be.data(), str_utf32be_len);
    do_test_buffer(test_name, "UTF-32LE -> UNICODE", UTF::decode_utf32le, str_utf32le.data(), str_utf32le_len, (uint32_t *) unicode_ref.data(), unicode_ref_len / 4);
    do_test_buffer(test_name, "UNICODE -> UTF-32LE", UTF::encode_utf32le, (uint32_t *) unicode_ref.data(), unicode_ref_len / 4, str_utf32le.data(), str_utf32le_len);

    do_test_buffer(test_name, "UTF-32BE -> UTF-8", UTF::conv_utf32be_to_utf8, str_utf32be.data(), str_utf32be_len, str_utf8, str_utf8_len);
    do_test_buffer(test_name, "UTF-32BE -> UTF-16LE", UTF::conv_utf32be_to_utf16le, str_utf32be.data(), str_utf32be_len, str_utf16le.data(), str_utf16le_len);
    do_test_buffer(test_name, "UTF-32BE -> UTF-16BE", UTF::conv_utf32be_to_utf16be, str_utf32be.data(), str_utf32be_len, str_utf16be.data(), str_utf16be_len);
    do_test_buffer(test_name, "UTF-32BE -> UTF-32LE", UTF::conv_utf32be_to_utf32le, str_utf32be.data(), str_utf32be_len, str_utf32le.data(), str_utf32le_len);
    do_test_buffer(test_name, "UTF-32BE -> UNICODE", UTF::decode_utf32be, str_utf32be.data(), str_utf32be_len, (uint32_t *) unicode_ref.data(), unicode_ref_len / 4);
    do_test_buffer(test_name, "UNICODE -> UTF-32BE", UTF::encode_utf32be, (uint32_t *) unicode_ref.data(), unicode_ref_len / 4, str_utf32be.data(), str_utf32be_len);
    
    do_test_decode_one(test_name, "UTF-8 -> UNICODE", UTF::decode_one_utf8, str_utf8, str_utf8_len, (uint32_t *) unicode_ref.data(), unicode_ref_len / 4);
    do_test_decode_one(test_name, "UTF-16LE -> UNICODE", UTF::decode_one_utf16le, str_utf16le.data(), str_utf16le_len, (uint32_t *) unicode_ref.data(), unicode_ref_len / 4);
    do_test_decode_one(test_name, "UTF-16BE -> UNICODE", UTF::decode_one_utf16be, str_utf16be.data(), str_utf16be_len, (uint32_t *) unicode_ref.data(), unicode_ref_len / 4);
    do_test_decode_one(test_name, "UTF-32LE -> UNICODE", UTF::decode_one_utf32le, str_utf32le.data(), str_utf32le_len, (uint32_t *) unicode_ref.data(), unicode_ref_len / 4);
    do_test_decode_one(test_name, "UTF-32BE -> UNICODE", UTF::decode_one_utf32be, str_utf32be.data(), str_utf32be_len, (uint32_t *) unicode_ref.data(), unicode_ref_len / 4);
    
}

/*
 * A pseudo container managing a user provided pointer
 * Should be safe to use with any naive type
 * Should be safe to use with a std::back_inserter
 *
 * Using this for the output argument is slower than using the getline style functions
 * because the capacity is checked each time push_back is called by the back_inserter
 */
template<typename _Tp>
class PointerContainer {
    _Tp **m_pp;
    _Tp *m_e;
    _Tp *m_p;
    size_t *m_s;

public:
    typedef _Tp value_type;
    PointerContainer(_Tp **p, size_t *s, size_t idx = 0) :
            m_pp(p), m_e(NULL), m_p(NULL), m_s(s){
        if (*m_pp && 0 < *m_s) {
            m_p = (*m_pp) + idx;
            m_e = (*m_pp) + *m_s;
        }
    }
    inline __attribute__((always_inline)) void push_back(const _Tp &value) {
        if (m_p == m_e) {
            size_t idx = *m_s;
            *m_s += std::min(size_t(1024), std::max(size_t(32), size_t(*m_s * 1.3)));
            *m_pp = (_Tp*) realloc(*m_pp, sizeof(_Tp) * *m_s);
            m_p = (*m_pp) + idx;
            m_e = (*m_pp) + *m_s;
        }
        *m_p++ = value;
    }
    inline void clear() {
        if (*m_pp && 0 < *m_s) {
            m_p = *m_pp;
        }
        else {
            m_p = NULL;
        }
    }
};

/*
 * Basic benchmarks for UTF-8 -> UTF-16LE conversion
 * source data is in str_utf8
 */
static void benchmark_utf8_utf16le(const char *str_utf8, int n_runs=100) {
    size_t str_utf8_len = strlen(str_utf8);
    size_t test_conv_size = str_utf8_len * 4;
    char *test_conv = (char *) malloc(test_conv_size);

    {
        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < n_runs; i++) {
            size_t consumed = 0;
            ssize_t r = iconv_convert("UTF-16LE", "UTF-8", str_utf8, str_utf8_len, &test_conv, &test_conv_size, &consumed);
            assert(r >=0 && consumed == str_utf8_len);
        }
        auto end = std::chrono::high_resolution_clock::now();
        printf("bench iconv utf8 -> utf16le : %" PRIu64 " ns\n", std::chrono::nanoseconds(end - start).count() / (uint64_t) n_runs);
    }
    {
        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < n_runs; i++) {
            size_t consumed = 0, written = 0;
            UTF::RetCode r = UTF::conv_utf8_to_utf16le(str_utf8, str_utf8_len, &test_conv, &test_conv_size, &consumed, &written);
            assert(r == UTF::RetCode::OK && consumed == str_utf8_len);
        }
        auto end = std::chrono::high_resolution_clock::now();
        printf("bench conv_utf8_to_utf16le (buffer) : %" PRIu64 " ns\n", std::chrono::nanoseconds(end - start).count() / (uint64_t) n_runs);
    }
    {
        PointerContainer<char> adapter(&test_conv, &test_conv_size);
        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < n_runs; i++) {
            size_t consumed = 0, written = 0;
            adapter.clear();
            UTF::RetCode r = UTF::conv_utf8_to_utf16le(str_utf8, str_utf8_len, std::back_inserter(adapter), &consumed, &written);
            assert(r == UTF::RetCode::OK && consumed == str_utf8_len);
        }
        auto end = std::chrono::high_resolution_clock::now();
        printf("bench conv_utf8_to_utf16le (back_inserter) : %" PRIu64 " ns\n", std::chrono::nanoseconds(end - start).count() / (uint64_t) n_runs);
    }

    free(test_conv);
}

/*
 * Basic benchmarks for UTF-16LE -> UTF-8 conversion
 * source data is in str_utf8 (in UTF-8)
 */
static void benchmark_utf16le_utf8(const char *str_utf8, int n_runs=100) {
    size_t str_utf8_len = strlen(str_utf8);

    // create the UTF-16LE source
    std::vector<char> str_utf16le;
    ssize_t str_utf16le_len = iconv_convert("UTF-16LE", "UTF-8", str_utf8, str_utf8_len, str_utf16le, NULL);

    size_t test_conv_size = str_utf8_len * 4;
    char *test_conv = (char *) malloc(test_conv_size);

    {
        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < n_runs; i++) {
            size_t consumed = 0;
            ssize_t r = iconv_convert("UTF-8", "UTF-16LE", str_utf16le.data(), str_utf16le_len, &test_conv, &test_conv_size, &consumed);
            assert(r >=0 && (ssize_t) consumed == str_utf16le_len);
        }
        auto end = std::chrono::high_resolution_clock::now();
        printf("bench iconv utf16le -> utf168 : %" PRIu64 " ns\n", std::chrono::nanoseconds(end - start).count() / (uint64_t) n_runs);
    }
    {
        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < n_runs; i++) {
            size_t consumed = 0, written = 0;
            UTF::RetCode r = UTF::conv_utf16le_to_utf8(str_utf16le.data(), str_utf16le_len, &test_conv, &test_conv_size, &consumed, &written);
            assert(r == UTF::RetCode::OK && (ssize_t) consumed == str_utf16le_len);
        }
        auto end = std::chrono::high_resolution_clock::now();
        printf("bench conv_utf16le_to_utf8 (buffer) : %" PRIu64 " ns\n", std::chrono::nanoseconds(end - start).count() / (uint64_t) n_runs);
    }
    {
        PointerContainer<char> adapter(&test_conv, &test_conv_size);
        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < n_runs; i++) {
            size_t consumed = 0, written = 0;
            adapter.clear();
            UTF::RetCode r = UTF::conv_utf16le_to_utf8(str_utf16le.data(), str_utf16le_len, std::back_inserter(adapter), &consumed, &written);
            assert(r == UTF::RetCode::OK && (ssize_t) consumed == str_utf16le_len);
        }
        auto end = std::chrono::high_resolution_clock::now();
        printf("bench conv_utf16le_to_utf8 (back_inserter) : %" PRIu64 " ns\n", std::chrono::nanoseconds(end - start).count() / (uint64_t) n_runs);
    }

    free(test_conv);
}

/*
 * test some decoder errors
 */
static void test_utf8_decode_errors() {
    UTF::RetCode r;
    size_t consumed = 0, written = 0;

    // truncated sequences (2 bytes)
    r = UTF::validate_utf8("aé", 1, &consumed, &written);
    assert(r == UTF::RetCode::OK && consumed == 1);
    r = UTF::validate_utf8("aé", 2, &consumed, &written);
    assert(r == UTF::RetCode::E_TRUNCATED && consumed == 1);
    r = UTF::validate_utf8("aé", 3, &consumed, &written);
    assert(r == UTF::RetCode::OK && consumed == 3);

    // truncated sequences (3 bytes)
    r = UTF::validate_utf8("a€", 1, &consumed, &written);
    assert(r == UTF::RetCode::OK && consumed == 1);
    r = UTF::validate_utf8("a€", 2, &consumed, &written);
    assert(r == UTF::RetCode::E_TRUNCATED && consumed == 1);
    r = UTF::validate_utf8("a€", 3, &consumed, &written);
    assert(r == UTF::RetCode::E_TRUNCATED && consumed == 1);
    r = UTF::validate_utf8("a€", 4, &consumed, &written);
    assert(r == UTF::RetCode::OK && consumed == 4);

    // truncated sequences (4 bytes)
    r = UTF::validate_utf8("a𠜎", 1, &consumed, &written);
    assert(r == UTF::RetCode::OK && consumed == 1);
    r = UTF::validate_utf8("a𠜎", 2, &consumed, &written);
    assert(r == UTF::RetCode::E_TRUNCATED && consumed == 1);
    r = UTF::validate_utf8("a𠜎", 3, &consumed, &written);
    assert(r == UTF::RetCode::E_TRUNCATED && consumed == 1);
    r = UTF::validate_utf8("a𠜎", 4, &consumed, &written);
    assert(r == UTF::RetCode::E_TRUNCATED && consumed == 1);
    r = UTF::validate_utf8("a𠜎", 5, &consumed, &written);
    assert(r == UTF::RetCode::OK && consumed == 5);

    // test overlong encoding
    unsigned char encoding[4];
    encoding[0] = 0b11000000 | ('a' >> 6);
    encoding[1] = 0b10000000 | ('a' & 0b111111);
    r = UTF::validate_utf8((const char *) encoding, 2, &consumed, &written);
    assert(r == UTF::RetCode::E_INVALID && consumed == 0);

    encoding[0] = 0b11100000;
    encoding[1] = 0b10000000 | (('a' >> 6) & 0b111111);
    encoding[2] = 0b10000000 | ('a' & 0b111111);
    r = UTF::validate_utf8((const char *) encoding, 3, &consumed, &written);
    assert(r == UTF::RetCode::E_INVALID && consumed == 0);

    encoding[0] = 0b11110000;
    encoding[1] = 0b10000000;
    encoding[2] = 0b10000000 | (('a' >> 6) & 0b111111);
    encoding[3] = 0b10000000 | ('a' & 0b111111);
    r = UTF::validate_utf8((const char *) encoding, 4, &consumed, &written);
    assert(r == UTF::RetCode::E_INVALID && consumed == 0);

    // invalid codepoint
    encoding[0] = 0b11100000 | ((0xD8aa >> 12) & 0b1111);
    encoding[1] = 0b10000000 | ((0xD8aa >> 6) & 0b111111);
    encoding[2] = 0b10000000 | (0xD8aa & 0b111111);
    r = UTF::validate_utf8((const char *) encoding, 3, &consumed, &written);
    assert(r == UTF::RetCode::E_INVALID && consumed == 0);

    // invalid codepoint
    encoding[0] = 0b11100000 | ((0xDCaa >> 12) & 0b1111);
    encoding[1] = 0b10000000 | ((0xDCaa >> 6) & 0b111111);
    encoding[2] = 0b10000000 | (0xDCaa & 0b111111);
    r = UTF::validate_utf8((const char *) encoding, 3, &consumed, &written);
    assert(r == UTF::RetCode::E_INVALID && consumed == 0);

    // invalid codepoint
    encoding[0] = 0b11110000 | ((0x110000 >> 18) & 0b111);
    encoding[1] = 0b10000000 | ((0x110000 >> 12) & 0b111111);;
    encoding[2] = 0b10000000 | ((0x110000 >> 6) & 0b111111);
    encoding[3] = 0b10000000 | (0x110000 & 0b111111);
    r = UTF::validate_utf8((const char *) encoding, 4, &consumed, &written);
    assert(r == UTF::RetCode::E_INVALID && consumed == 0);
}

/*
 * test some decoder errors
 * There is not much to test for UTF-16 as overlong encoding is not possible and illegal codepoints can't be encoded
 */
static void test_utf16_decode_errors() {
    UTF::RetCode r;
    size_t consumed = 0, written = 0;

    const unsigned char hello_utf16le[] = {0x68, 0x00, 0xe9, 0x00, 0x6c, 0x00, 0x6c, 0x00, 0xf4, 0x00};
    r = UTF::validate_utf16le((const char *) hello_utf16le, sizeof(hello_utf16le), &consumed, &written);
    assert(r == UTF::RetCode::OK && consumed == sizeof(hello_utf16le));
    // truncated sequence
    r = UTF::validate_utf16le((const char *) hello_utf16le, 3, &consumed, &written);
    assert(r == UTF::RetCode::E_TRUNCATED && consumed == 2);

    const unsigned char smileys_utf16le[] = {0x3d, 0xd8, 0x3a, 0xde, 0x3d, 0xd8, 0x26, 0xdc, 0x3d, 0xd8, 0x77, 0xdd};
    r = UTF::validate_utf16le((const char *) smileys_utf16le, sizeof(smileys_utf16le), &consumed, &written);
    assert(r == UTF::RetCode::OK && consumed == sizeof(smileys_utf16le));
    // truncated pair
    r = UTF::validate_utf16le((const char *) smileys_utf16le, 1, &consumed, &written);
    assert(r == UTF::RetCode::E_TRUNCATED && consumed == 0);
    // truncated pair
    r = UTF::validate_utf16le((const char *) smileys_utf16le, 2, &consumed, &written);
    assert(r == UTF::RetCode::E_TRUNCATED && consumed == 0);
    // truncated pair
    r = UTF::validate_utf16le((const char *) smileys_utf16le, 3, &consumed, &written);
    assert(r == UTF::RetCode::E_TRUNCATED && consumed == 0);
    r = UTF::validate_utf16le((const char *) smileys_utf16le, 4, &consumed, &written);
    assert(r == UTF::RetCode::OK && consumed == 4);

    // invalid high surrogate
    r = UTF::validate_utf16le((const char *) smileys_utf16le + 2, sizeof(smileys_utf16le) - 2, &consumed, &written);
    assert(r == UTF::RetCode::E_INVALID && consumed == 0);

    char encoding[4];
    *(uint16_t *)encoding = htole16(0xd83d); // valid high surrogate
    *(uint16_t *)(encoding + 2) = htole16(0xabcd); // invalid low surrogate
    r = UTF::validate_utf16le(encoding, 4, &consumed, &written);
    assert(r == UTF::RetCode::E_INVALID && consumed == 0);
}

/*
 * test some decoder errors
 */
static void test_utf32_decode_errors() {
    UTF::RetCode r;
    size_t consumed = 0, written = 0;

    const unsigned char smileys_utf32le[] = {0x3a, 0xf6, 0x01, 0x00, 0x26, 0xf4, 0x01, 0x00, 0x77, 0xf5, 0x01, 0x00};
    r = UTF::validate_utf32le((const char *) smileys_utf32le, sizeof(smileys_utf32le), &consumed, &written);
    assert(r == UTF::RetCode::OK && consumed == sizeof(smileys_utf32le));
    // truncated sequence
    r = UTF::validate_utf32le((const char *) smileys_utf32le, 5, &consumed, &written);
    assert(r == UTF::RetCode::E_TRUNCATED && consumed == 4);
    // truncated sequence
    r = UTF::validate_utf32le((const char *) smileys_utf32le, 6, &consumed, &written);
    assert(r == UTF::RetCode::E_TRUNCATED && consumed == 4);
    // truncated sequence
    r = UTF::validate_utf32le((const char *) smileys_utf32le, 7, &consumed, &written);
    assert(r == UTF::RetCode::E_TRUNCATED && consumed == 4);

    char encoding[4];
    *(uint32_t *) encoding = htole32(0xd824); // invalid codepoint
    r = UTF::validate_utf32le(encoding, 4, &consumed, &written);
    assert(r == UTF::RetCode::E_INVALID && consumed == 0);
    *(uint32_t *) encoding = htole32(0xdc24); // invalid codepoint
    r = UTF::validate_utf32le(encoding, 4, &consumed, &written);
    assert(r == UTF::RetCode::E_INVALID && consumed == 0);
    *(uint32_t *) encoding = htole32(0x110000); // invalid codepoint
    r = UTF::validate_utf32le(encoding, 4, &consumed, &written);
    assert(r == UTF::RetCode::E_INVALID && consumed == 0);
}

/*
 * Test some encoder errors
 */
static void test_encode_errors() {
    char *test_conv = NULL;
    size_t test_conv_size = 0;
    UTF::RetCode r;
    size_t consumed = 0, written = 0;

    uint32_t encoding;

    encoding = 0xd8aa; // invalid codepoint
    r = UTF::encode_utf8(&encoding, 1, &test_conv, &test_conv_size, &consumed, &written);
    assert(r == UTF::RetCode::E_INVALID && consumed == 0);
    r = UTF::encode_utf16be(&encoding, 1, &test_conv, &test_conv_size, &consumed, &written);
    assert(r == UTF::RetCode::E_INVALID && consumed == 0);
    r = UTF::encode_utf32be(&encoding, 1, &test_conv, &test_conv_size, &consumed, &written);
    assert(r == UTF::RetCode::E_INVALID && consumed == 0);

    encoding = 0xdcaa; // invalid codepoint
    r = UTF::encode_utf8(&encoding, 1, &test_conv, &test_conv_size, &consumed, &written);
    assert(r == UTF::RetCode::E_INVALID && consumed == 0);
    r = UTF::encode_utf16be(&encoding, 1, &test_conv, &test_conv_size, &consumed, &written);
    assert(r == UTF::RetCode::E_INVALID && consumed == 0);
    r = UTF::encode_utf32be(&encoding, 1, &test_conv, &test_conv_size, &consumed, &written);
    assert(r == UTF::RetCode::E_INVALID && consumed == 0);

    encoding = 0x110000; // invalid codepoint
    r = UTF::encode_utf8(&encoding, 1, &test_conv, &test_conv_size, &consumed, &written);
    assert(r == UTF::RetCode::E_INVALID && consumed == 0);
    r = UTF::encode_utf16be(&encoding, 1, &test_conv, &test_conv_size, &consumed, &written);
    assert(r == UTF::RetCode::E_INVALID && consumed == 0);
    r = UTF::encode_utf32be(&encoding, 1, &test_conv, &test_conv_size, &consumed, &written);
    assert(r == UTF::RetCode::E_INVALID && consumed == 0);

    free(test_conv);
}

int main() {
    /* tests with valid datas */
    do_tests("simple", "chaîne UTF-8 simple 42€ çàéù");
    do_tests("empty", "");
    do_tests("smileys", "\xF0\x9F\x98\xBA\xF0\x9F\x90\xA6\xF0\x9F\x95\xB7");
    // from http://www.i18nguy.com/unicode/supplementary-test.html
    do_tests("supplementary",
            "\xf0\xa0\x9c\x8e \xf0\xa0\x9c\xb1 \xf0\xa0\x9d\xb9 \xf0\xa0\xb1\x93 \xf0\xa0\xb1\xb8 \xf0\xa0\xb2\x96 "
            "\xf0\xa0\xb3\x8f \xf0\xa0\xb3\x95 \xf0\xa0\xb4\x95 \xf0\xa0\xb5\xbc \xf0\xa0\xb5\xbf \xf0\xa0\xb8\x8e "
            "\xf0\xa0\xb8\x8f \xf0\xa0\xb9\xb7 \xf0\xa0\xba\x9d \xf0\xa0\xba\xa2 \xf0\xa0\xbb\x97 \xf0\xa0\xbb\xb9 "
            "\xf0\xa0\xbb\xba \xf0\xa0\xbc\xad \xf0\xa0\xbc\xae \xf0\xa0\xbd\x8c \xf0\xa0\xbe\xb4 \xf0\xa0\xbe\xbc "
            "\xf0\xa0\xbf\xaa \xf0\xa1\x81\x9c \xf0\xa1\x81\xaf \xf0\xa1\x81\xb5 \xf0\xa1\x81\xb6 \xf0\xa1\x81\xbb "
            "\xf0\xa1\x83\x81 \xf0\xa1\x83\x89 \xf0\xa1\x87\x99 \xf0\xa2\x83\x87 \xf0\xa2\x9e\xb5 \xf0\xa2\xab\x95 "
            "\xf0\xa2\xad\x83 \xf0\xa2\xaf\x8a \xf0\xa2\xb1\x91 \xf0\xa2\xb1\x95 \xf0\xa2\xb3\x82 \xf0\xa2\xb4\x88 "
            "\xf0\xa2\xb5\x8c \xf0\xa2\xb5\xa7 \xf0\xa2\xba\xb3 \xf0\xa3\xb2\xb7 \xf0\xa4\x93\x93 \xf0\xa4\xb6\xb8 "
            "\xf0\xa4\xb7\xaa \xf0\xa5\x84\xab \xf0\xa6\x89\x98 \xf0\xa6\x9f\x8c \xf0\xa6\xa7\xb2 \xf0\xa6\xa7\xba "
            "\xf0\xa7\xa8\xbe \xf0\xa8\x85\x9d \xf0\xa8\x88\x87 \xf0\xa8\x8b\xa2 \xf0\xa8\xb3\x8a \xf0\xa8\xb3\x8d "
            "\xf0\xa8\xb3\x92 \xf0\xa9\xb6\x98 ");
    for (std::ifstream input("test_file_chinese_utf8"); input;) {
        std::string input_data;
        std::copy(std::istream_iterator<char>(input), std::istream_iterator<char>(), std::back_inserter(input_data));
        do_tests("test_file_chinese_utf8", input_data.c_str());
        break;
    }

    /* test illegal sequences */

    test_utf8_decode_errors();
    test_utf16_decode_errors();
    test_utf32_decode_errors();
    test_encode_errors();

    /* test and benchmark on a utf-8 sample file */

    for (std::ifstream input("test_file_big"); input;) {
        std::string input_data;
        while (input_data.size() < (1<<20)) { // at least 1 MB
            std::copy(std::istream_iterator<char>(input), std::istream_iterator<char>(), std::back_inserter(input_data));
            input.clear();
            input.seekg(0);
        }
        do_tests("test_file_big", input_data.c_str());
        benchmark_utf8_utf16le(input_data.c_str());
        benchmark_utf16le_utf8(input_data.c_str());
        break;
    }
    return 0;
}


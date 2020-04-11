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

#include "charset_conv_iconv.h"

#include <cstdlib>
#include <cstdio>
#include <cerrno>
#include <iconv.h>

ssize_t iconv_convert(const char *to_charset, const char *from_charset, const char *input, size_t input_len, char **output, size_t *output_len,
        size_t *consumed) {
    iconv_t cd = iconv_open(to_charset, from_charset);
    if (cd == (iconv_t) -1) {
        perror("iconv_open");
        return false;
    }

    const char *inbuf = input;
    size_t inbuf_left = input_len;
    char *outbuf;
    size_t outbuf_left;
    size_t converted = 0;
    if (consumed) {
        *consumed = 0;
    }

    if (*output == NULL) {
        *output = (char*) malloc(input_len + 8);
        *output_len = input_len + 8;
    }

    do {
        errno = 0;
        outbuf = *output + converted;
        outbuf_left = *output_len - converted;

        size_t r = iconv(cd, (char**) &inbuf, &inbuf_left, &outbuf, &outbuf_left);
        if (r != (size_t) -1) {
            break;
        }
        if (errno == EINVAL) {
            break;
        }
        if (errno == EILSEQ) {
            iconv_close(cd);
            return -1;
        }
        if (errno == E2BIG) {
            converted = outbuf - *output;
            *output_len += inbuf_left * 2 + 8;
            *output = (char*) realloc(*output, *output_len);
            outbuf = *output + converted;
            continue;
        }
        perror("iconv");
        iconv_close(cd);
        return -1;
    } while (1);

    iconv_close(cd);
    if (consumed) {
        *consumed = input_len - inbuf_left;
    }
    return ssize_t(outbuf - *output);
}

ssize_t iconv_convert(const char *to_charset, const char *from_charset, const char *input, size_t input_len, std::vector<char> &output, size_t *consumed) {
    iconv_t cd = iconv_open(to_charset, from_charset);
    if (cd == (iconv_t) -1) {
        perror("iconv_open");
        return false;
    }

    const char *inbuf = input;
    size_t inbuf_left = input_len;
    char *outbuf;
    size_t outbuf_left;
    size_t converted = 0;
    if (consumed) {
        *consumed = 0;
    }

    output.clear();
    output.resize(8);

    do {
        errno = 0;
        outbuf = output.data() + converted;
        outbuf_left = output.size() - converted;

        size_t r = iconv(cd, (char**) &inbuf, &inbuf_left, &outbuf, &outbuf_left);
        if (r != (size_t) -1) {
            break;
        }
        if (errno == EINVAL) {
            break;
        }
        if (errno == EILSEQ) {
            iconv_close(cd);
            return -1;
        }
        if (errno == E2BIG) {
            converted = outbuf - output.data();
            output.resize(output.size() + inbuf_left * 2 + 8);
            outbuf = output.data() + converted;
            continue;
        }
        perror("iconv");
        iconv_close(cd);
        return -1;
    } while (1);

    iconv_close(cd);
    if (consumed) {
        *consumed = input_len - inbuf_left;
    }
    output.resize(outbuf - output.data());
    return ssize_t(outbuf - output.data());
}

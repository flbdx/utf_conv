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

#ifndef CHARSET_CONV_ICONV_H_
#define CHARSET_CONV_ICONV_H_


#include <sys/types.h>
#include <vector>

ssize_t iconv_convert(const char *to_charset, const char *from_charset, const char *input, size_t input_len, char **output, size_t *output_len,
        size_t *consumed);
ssize_t iconv_convert(const char *to_charset, const char *from_charset, const char *input, size_t input_len, std::vector<char> &output, size_t *consumed);


#endif /* CHARSET_CONV_ICONV_H_ */

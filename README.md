
# utf_conv

## Overview

A set of C++ utility functions for the main UTF unicode encodings. The project includes :
- conversion functions
- decoding functions
- encoding functions
- validation functions

The sources also include a wrapper for `iconv` used as a reference for correctness tests.

The functions cover the following UTF encodings :
- UTF-8
- UTF-16LE / UTF-16BE
- UTF-32LE / UTF-32BE

## Functions

```C++
// Stream conversion functions
// (1)
template<typename OutputIt>
UTF::RetCode UTF::conv_XXX_to_YYY(
	const char *input, size_t input_len, OutputIt iOutput, size_t *consumed, size_t *written);
// (2)
UTF::RetCode UTF::conv_XXX_to_YYY(
	const char *input, size_t input_len, char **output, size_t *output_size, size_t *consumed, size_t *written);

// Stream decoding functions
// (3)
template<typename OutputIt>
UTF::RetCode UTF::decode_XXX(
	const char *input, size_t input_len, OutputIt iOutput, size_t *consumed, size_t *written);
// (4)
UTF::RetCode UTF::decode_XXX(
	const char *input, size_t input_len, uint32_t **output, size_t *output_size, size_t *consumed, size_t *written);

// Codepoint encoding functions
// (5)
template<typename OutputIt>
UTF::RetCode UTF::encode_XXX(
	const uint32_t *input, size_t input_len, OutputIt iOutput, size_t *consumed, size_t *written);
// (6)
UTF::RetCode UTF::encode_XXX(
	const uint32_t *input, size_t input_len, char **output, size_t *output_size, size_t *consumed, size_t *written);

// Stream validation functions
// (7)
UTF::RetCode UTF::validate_XXX(const uint32_t *input, size_t input_len, size_t *consumed, size_t *length);

```

where `XXX` or `YYY` are two differents words between `utf8`, `utf16le`, `utf16be`, `utf32le` and `utf32be`.

### Parameters

- `input` :  beginning of the input stream
- `input_len` : number of elements (type of `input`) to read from `input`
- `iOutput`: beginning of the output range (`LegacyOutputIterator`)
	Depending of the operation, `iOutput` must accept either single byte assignements (stream conversion or encoding) or 32 bits interger assignements (stream decoding)
- `output`: store the address of the beginning of the output stream `*output`
- `output_size` : store the malloc-allocated memory for `*output`.
	If `*output` is `NULL` and `*output_size` is 0, then the function will allocate a new buffer with `malloc`. 
	If the allocated size is too small, `*output` is reallocated (`realloc`) and `*output_size` is updated.
- `consumed` : store the number of bytes read from input. If *consumed == input_len, there was no error
- `written` : store the number of elements (type of `output` or `iOutput`) written into the output parameter
- `length`: store the number of unicode characters read from the input stream

### Return value

- `RetCode::OK` : no error
- `RetCode::E_INVALID` : invalid sequence or codepoint encountered
- `RetCode::E_TRUNCATED` : truncated sequence encountered (for stream conversions, decoding and validation)
- `RetCode::E_PARAMS` : invalid parameters (only returned by `(2)`, `(4)` and `(6)`)

## Examples

### Stream conversion

```C++
static void sample_1() {
    const char *input_utf8 = "✏ this is a useless UTF-8 string 😷";
    size_t input_utf8_len = strlen(input_utf8);
    std::vector<unsigned char> conv_utf32be;
    size_t conv_consumed(-1), conv_written(0);
    UTF::RetCode r = UTF::conv_utf8_to_utf32be(input_utf8, input_utf8_len, std::back_inserter(conv_utf32be), &conv_consumed, &conv_written);
    if (r == UTF::RetCode::OK && conv_consumed == input_utf8_len) {
        for (unsigned char b : conv_utf32be) {printf("%02X", b);}
        puts("");
    }
    else {
        fprintf(stderr, "error, consumed %zu bytes out of %zu\n", conv_consumed, input_utf8_len);
    }
}

static void sample_2() {
    const char *input_utf8 = "✏ this is a useless UTF-8 string 😷";
    size_t input_utf8_len = strlen(input_utf8);
    char *conv_utf32be = NULL;
    size_t conv_utf32be_size = 0;
    size_t conv_consumed(-1), conv_written(0);;
    UTF::RetCode r = UTF::conv_utf8_to_utf32be(input_utf8, input_utf8_len, &conv_utf32be, &conv_utf32be_size, &conv_consumed, &conv_written);
    if (r == UTF::RetCode::OK && conv_consumed == input_utf8_len) {
        for (size_t i = 0; i < conv_written; i++) {printf("%02X", (unsigned char) (conv_utf32be[i]));}
        puts("");
    }
    else {
        fprintf(stderr, "error, consumed %zu bytes out of %zu\n", conv_consumed, input_utf8_len);
    }
    free(conv_utf32be);
}
```

### Stream decoding

```C++
static void sample_1() {
    const char *input_utf8 = "✏ this is a useless UTF-8 string 😷";
    size_t input_utf8_len = strlen(input_utf8);
    std::vector<uint32_t> codepoints;
    size_t conv_consumed(-1), conv_written(0);
    UTF::RetCode r = UTF::decode_utf8(input_utf8, input_utf8_len, std::back_inserter(codepoints), &conv_consumed, &conv_written);
    if (r == UTF::RetCode::OK && conv_consumed == input_utf8_len) {
        for (uint32_t b : codepoints) {printf("%08X", b);}
        puts("");
    }
    else {
        fprintf(stderr, "error, consumed %zu bytes out of %zu\n", conv_consumed, input_utf8_len);
    }
}

static void sample_2() {
    const char *input_utf8 = "✏ this is a useless UTF-8 string 😷";
    size_t input_utf8_len = strlen(input_utf8);
    uint32_t *codepoints = NULL;
    size_t codepoints_size = 0;
    size_t conv_consumed(-1), conv_written(0);
    UTF::RetCode r = UTF::decode_utf8(input_utf8, input_utf8_len, &codepoints, &codepoints_size, &conv_consumed, &conv_written);
    if (r == UTF::RetCode::OK && conv_consumed == input_utf8_len) {
        for (size_t i = 0; i < conv_written; i++) {printf("%08X", codepoints[i]);}
        puts("");
    }
    else {
        fprintf(stderr, "error, consumed %zu bytes out of %zu\n", conv_consumed, input_utf8_len);
    }
    free(codepoints);
}
```

### Stream encoding

```C++
static uint32_t codepoints[] =
    {0x0000270F, 0x00000020, 0x00000074, 0x00000068, 0x00000069, 0x00000073, 0x00000020, 0x00000069, 0x00000073,
     0x00000020, 0x00000061, 0x00000020, 0x00000075, 0x00000073, 0x00000065, 0x0000006C, 0x00000065, 0x00000073,
     0x00000073, 0x00000020, 0x00000055, 0x00000054, 0x00000046, 0x0000002D, 0x00000038, 0x00000020, 0x00000073,
     0x00000074, 0x00000072, 0x00000069, 0x0000006E, 0x00000067, 0x00000020, 0x0001F637};
static void sample_1() {
    size_t codepoints_len = sizeof(codepoints) / sizeof(uint32_t);
    std::vector<unsigned char> conv_utf16le;
    size_t conv_consumed(-1), conv_written(0);
    UTF::RetCode r = UTF::encode_utf16le(codepoints, codepoints_len, std::back_inserter(conv_utf16le), &conv_consumed, &conv_written);
    if (r == UTF::RetCode::OK && conv_consumed == codepoints_len) {
        for (unsigned char b : conv_utf16le) {printf("%02X", b);}
        puts("");
    }
    else {
        fprintf(stderr, "error, consumed %zu bytes out of %zu\n", conv_consumed, codepoints_len);
    }
}

static void sample_2() {
    size_t codepoints_len = sizeof(codepoints) / sizeof(uint32_t);
    char *conv_utf16le = NULL;
    size_t conv_utf16le_size = 0;
    size_t conv_consumed(-1), conv_written(0);
    UTF::RetCode r = UTF::encode_utf16le(codepoints, codepoints_len, &conv_utf16le, &conv_utf16le_size, &conv_consumed, &conv_written);
    if (r == UTF::RetCode::OK && conv_consumed == codepoints_len) {
        for (size_t i = 0; i < conv_written; i++) {printf("%02X", (unsigned char) (conv_utf16le[i]));}
        puts("");
    }
    else {
        fprintf(stderr, "error, consumed %zu bytes out of %zu\n", conv_consumed, codepoints_len);
    }
    free(conv_utf16le);
}
```

### Stream validation

```C++
static void sample() {
    const char *input_utf8 = "✏ this is a useless UTF-8 string 😷";
    size_t input_utf8_len = strlen(input_utf8);
    size_t conv_consumed(-1), length(0);
    UTF::RetCode r = UTF::validate_utf8(input_utf8, input_utf8_len, &conv_consumed, &length);
    if (r == UTF::RetCode::OK && conv_consumed == input_utf8_len) {
        printf("the string is valid and its length is %zu characters\n", length);
    }
    else {
        fprintf(stderr, "error, consumed %zu bytes out of %zu\n", conv_consumed, input_utf8_len);
    }
}
```

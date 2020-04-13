# utf_conv
A set of C++ conversion functions between the main UTF unicode encodings.
The sources also include a wrapper for `iconv`.
## Stream conversion functions
The following functions for stream conversions are defined :
```C++
// (1)
template<typename OutputIt>
ssize_t UTF::conv_XXX_to_YYY(const char *input, size_t input_len, OutputIt output, size_t *consumed);

// (2)
ssize_t UTF::conv_XXX_to_YYY(const char *input, size_t input_len, char **output, size_t *output_size, size_t *consumed);
```
where `XXX` and `YYY` are two differents words between `utf8`, `utf16le`, `utf16be`, `utf32le` and `utf32be`.

### Parameters
- `input` :  input data
- `input_len` : number of bytes to read from `input`
- `output (1)`: beginning of the output range (`LegacyOutputIterator`), which must accept `char` `unsigned char` assignments.
- `output (2)`: store the address of the beginning of the output stream `*output`
- `output_size (2)` : store the malloc-allocated memory for `*output`.
	If `*output` is `NULL` and `*output_size` is 0, then the function will allocate a new buffer with `malloc`. 
	If the allocated size is too small, `*output` is reallocated (`realloc`) and `*output_size` is updated.
- `consumed` : store the number of bytes read from input. If *consumed == input_len, there were no error
- `[return value]` : number of bytes written into the output parameter

### Examples
```C++
static void sample_1() {
    const char *input_utf8 = "✏ this is a useless UTF-8 string 😷";
    size_t input_utf8_len = strlen(input_utf8);
    std::vector<unsigned char> conv_utf32be;
    size_t conv_consumed(-1);
    UTF::conv_utf8_to_utf32be(input_utf8, input_utf8_len, std::back_inserter(conv_utf32be), &conv_consumed);
    if (conv_consumed == input_utf8_len) {
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
    size_t conv_consumed(-1);
    ssize_t w = UTF::conv_utf8_to_utf32be(input_utf8, input_utf8_len, &conv_utf32be, &conv_utf32be_size, &conv_consumed);
    if (conv_consumed == input_utf8_len && w > 0) {
        for (size_t i = 0; i < (size_t) w; i++) {printf("%02X", (unsigned char) (conv_utf32be[i]));}
        puts("");
    }
    else {
        fprintf(stderr, "error, consumed %zu bytes out of %zu\n", conv_consumed, input_utf8_len);
    }
    free(conv_utf32be);
}
```

## Decoding functions
The following functions decode UTF sequences into codepoints stored on 32 bits integers :
```C++
// (1)
template<typename OutputIt>
ssize_t UTF::decode_XXX(const char *input, size_t input_len, OutputIt output, size_t *consumed);

ssize_t UTF::decode_XXX(const char *input, size_t input_len, uint32_t **output, size_t *output_size, size_t *consumed);
```
where `XXX` is a word between `utf8`, `utf16le`, `utf16be`, `utf32le` and `utf32be`.

### Parameters
- `input` :  input data
- `input_len` : number of bytes to read from `input`
- `output (1)`: beginning of the output range (`LegacyOutputIterator`), which must accept `uint32_t`  (`unsigned int`) assignments.
- `output (2)`: store the address of the beginning of the output stream `*output`
- `output_size (2)` : store the malloc-allocated memory for `*output`.
	If `*output` is `NULL` and `*output_size` is 0, then the function will allocate a new buffer with `malloc`. 
	If the allocated size is too small, `*output` is reallocated (`realloc`) and `*output_size` is updated.
- `consumed` : store the number of bytes read from input. If *consumed == input_len, there were no error
- `[return value]` : number of 32 bits elements written into the output parameter

### Examples
```C++
static void sample_1() {
    const char *input_utf8 = "✏ this is a useless UTF-8 string 😷";
    size_t input_utf8_len = strlen(input_utf8);
    std::vector<uint32_t> codepoints;
    size_t conv_consumed(-1);
    UTF::decode_utf8(input_utf8, input_utf8_len, std::back_inserter(codepoints), &conv_consumed);
    if (conv_consumed == input_utf8_len) {
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
    size_t conv_consumed(-1);
    ssize_t w = UTF::decode_utf8(input_utf8, input_utf8_len, &codepoints, &codepoints_size, &conv_consumed);
    if (conv_consumed == input_utf8_len && w > 0) {
        for (size_t i = 0; i < (size_t) w; i++) {printf("%08X", codepoints[i]);}
        puts("");
    }
    else {
        fprintf(stderr, "error, consumed %zu bytes out of %zu\n", conv_consumed, input_utf8_len);
    }
    free(codepoints);
}
```

## Encoding functions
The following functions encode codepoints stored on 32 bits integers into UTF streams :
```C++
// (1)
template<typename OutputIt>
ssize_t UTF::encode_XXX(const uint32_t *input, size_t input_len, OutputIt output, size_t *consumed);

ssize_t UTF::encode_XXX(const uint32_t *input, size_t input_len, char **output, size_t *output_size, size_t *consumed);
```
where `XXX` is a word between `utf8`, `utf16le`, `utf16be`, `utf32le` and `utf32be`.

### Parameters
- `input` :  input data
- `input_len` : number of 32 bits elements to read from `input`
- `output (1)`: beginning of the output range (`LegacyOutputIterator`), which must accept `char` or `unsigned char` assignments.
- `output (2)`: store the address of the beginning of the output stream `*output`
- `output_size (2)` : store the malloc-allocated memory for `*output`.
	If `*output` is `NULL` and `*output_size` is 0, then the function will allocate a new buffer with `malloc`. 
	If the allocated size is too small, `*output` is reallocated (`realloc`) and `*output_size` is updated.
- `consumed` : store the number of 32 bits elements read from input. If *consumed == input_len, there were no error
- `[return value]` : number of bytes written into the output parameter

### Examples
```C++
static uint32_t codepoints[] =
    {0x0000270F, 0x00000020, 0x00000074, 0x00000068, 0x00000069, 0x00000073, 0x00000020, 0x00000069, 0x00000073,
     0x00000020, 0x00000061, 0x00000020, 0x00000075, 0x00000073, 0x00000065, 0x0000006C, 0x00000065, 0x00000073,
     0x00000073, 0x00000020, 0x00000055, 0x00000054, 0x00000046, 0x0000002D, 0x00000038, 0x00000020, 0x00000073,
     0x00000074, 0x00000072, 0x00000069, 0x0000006E, 0x00000067, 0x00000020, 0x0001F637};
static void sample_1() {
    size_t codepoints_len = sizeof(codepoints) / sizeof(uint32_t);
    std::vector<unsigned char> conv_utf16le;
    size_t conv_consumed(-1);
    UTF::encode_utf16le(codepoints, codepoints_len, std::back_inserter(conv_utf16le), &conv_consumed);
    if (conv_consumed == codepoints_len) {
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
    size_t conv_consumed(-1);
    ssize_t w = UTF::encode_utf16le(codepoints, codepoints_len, &conv_utf16le, &conv_utf16le_size, &conv_consumed);
    if (conv_consumed == codepoints_len && w > 0) {
        for (size_t i = 0; i < (size_t) w; i++) {printf("%02X", (unsigned char) (conv_utf16le[i]));}
        puts("");
    }
    else {
        fprintf(stderr, "error, consumed %zu bytes out of %zu\n", conv_consumed, codepoints_len);
    }
    free(conv_utf16le);
}
```

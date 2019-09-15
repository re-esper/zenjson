namespace zjson {

static inline double pow10(int64_t exponent) {
    if (exponent > 308) {
        return std::numeric_limits<double>::infinity();
    }
    else if (exponent < -323) {
        return 0.0;
    }
    static const double constants[] = { 1e-323,1e-322,1e-321,1e-320,1e-319,1e-318,1e-317,1e-316,1e-315,1e-314,1e-313,1e-312,1e-311,1e-310,1e-309,1e-308,1e-307,1e-306,1e-305,1e-304,1e-303,1e-302,1e-301,1e-300,1e-299,1e-298,1e-297,1e-296,1e-295,1e-294,1e-293,1e-292,1e-291,1e-290,1e-289,1e-288,1e-287,1e-286,1e-285,1e-284,1e-283,1e-282,1e-281,1e-280,1e-279,1e-278,1e-277,1e-276,1e-275,1e-274,1e-273,1e-272,1e-271,1e-270,1e-269,1e-268,1e-267,1e-266,1e-265,1e-264,1e-263,1e-262,1e-261,1e-260,1e-259,1e-258,1e-257,1e-256,1e-255,1e-254,1e-253,1e-252,1e-251,1e-250,1e-249,1e-248,1e-247,1e-246,1e-245,1e-244,1e-243,1e-242,1e-241,1e-240,1e-239,1e-238,1e-237,1e-236,1e-235,1e-234,1e-233,1e-232,1e-231,1e-230,1e-229,1e-228,1e-227,1e-226,1e-225,1e-224,1e-223,1e-222,1e-221,1e-220,1e-219,1e-218,1e-217,1e-216,1e-215,1e-214,1e-213,1e-212,1e-211,1e-210,1e-209,1e-208,1e-207,1e-206,1e-205,1e-204,1e-203,1e-202,1e-201,1e-200,1e-199,1e-198,1e-197,1e-196,1e-195,1e-194,1e-193,1e-192,1e-191,1e-190,1e-189,1e-188,1e-187,1e-186,1e-185,1e-184,1e-183,1e-182,1e-181,1e-180,1e-179,1e-178,1e-177,1e-176,1e-175,1e-174,1e-173,1e-172,1e-171,1e-170,1e-169,1e-168,1e-167,1e-166,1e-165,1e-164,1e-163,1e-162,1e-161,1e-160,1e-159,1e-158,1e-157,1e-156,1e-155,1e-154,1e-153,1e-152,1e-151,1e-150,1e-149,1e-148,1e-147,1e-146,1e-145,1e-144,1e-143,1e-142,1e-141,1e-140,1e-139,1e-138,1e-137,1e-136,1e-135,1e-134,1e-133,1e-132,1e-131,1e-130,1e-129,1e-128,1e-127,1e-126,1e-125,1e-124,1e-123,1e-122,1e-121,1e-120,1e-119,1e-118,1e-117,1e-116,1e-115,1e-114,1e-113,1e-112,1e-111,1e-110,1e-109,1e-108,1e-107,1e-106,1e-105,1e-104,1e-103,1e-102,1e-101,1e-100,1e-99,1e-98,1e-97,1e-96,1e-95,1e-94,1e-93,1e-92,1e-91,1e-90,1e-89,1e-88,1e-87,1e-86,1e-85,1e-84,1e-83,1e-82,1e-81,1e-80,1e-79,1e-78,1e-77,1e-76,1e-75,1e-74,1e-73,1e-72,1e-71,1e-70,1e-69,1e-68,1e-67,1e-66,1e-65,1e-64,1e-63,1e-62,1e-61,1e-60,1e-59,1e-58,1e-57,1e-56,1e-55,1e-54,1e-53,1e-52,1e-51,1e-50,1e-49,1e-48,1e-47,1e-46,1e-45,1e-44,1e-43,1e-42,1e-41,1e-40,1e-39,1e-38,1e-37,1e-36,1e-35,1e-34,1e-33,1e-32,1e-31,1e-30,1e-29,1e-28,1e-27,1e-26,1e-25,1e-24,1e-23,1e-22,1e-21,1e-20,1e-19,1e-18,1e-17,1e-16,1e-15,1e-14,1e-13,1e-12,1e-11,1e-10,1e-9,1e-8,1e-7,1e-6,1e-5,1e-4,1e-3,1e-2,1e-1,1e0,1e1,1e2,1e3,1e4,1e5,1e6,1e7,1e8,1e9,1e10,1e11,1e12,1e13,1e14,1e15,1e16,1e17,1e18,1e19,1e20,1e21,1e22,1e23,1e24,1e25,1e26,1e27,1e28,1e29,1e30,1e31,1e32,1e33,1e34,1e35,1e36,1e37,1e38,1e39,1e40,1e41,1e42,1e43,1e44,1e45,1e46,1e47,1e48,1e49,1e50,1e51,1e52,1e53,1e54,1e55,1e56,1e57,1e58,1e59,1e60,1e61,1e62,1e63,1e64,1e65,1e66,1e67,1e68,1e69,1e70,1e71,1e72,1e73,1e74,1e75,1e76,1e77,1e78,1e79,1e80,1e81,1e82,1e83,1e84,1e85,1e86,1e87,1e88,1e89,1e90,1e91,1e92,1e93,1e94,1e95,1e96,1e97,1e98,1e99,1e100,1e101,1e102,1e103,1e104,1e105,1e106,1e107,1e108,1e109,1e110,1e111,1e112,1e113,1e114,1e115,1e116,1e117,1e118,1e119,1e120,1e121,1e122,1e123,1e124,1e125,1e126,1e127,1e128,1e129,1e130,1e131,1e132,1e133,1e134,1e135,1e136,1e137,1e138,1e139,1e140,1e141,1e142,1e143,1e144,1e145,1e146,1e147,1e148,1e149,1e150,1e151,1e152,1e153,1e154,1e155,1e156,1e157,1e158,1e159,1e160,1e161,1e162,1e163,1e164,1e165,1e166,1e167,1e168,1e169,1e170,1e171,1e172,1e173,1e174,1e175,1e176,1e177,1e178,1e179,1e180,1e181,1e182,1e183,1e184,1e185,1e186,1e187,1e188,1e189,1e190,1e191,1e192,1e193,1e194,1e195,1e196,1e197,1e198,1e199,1e200,1e201,1e202,1e203,1e204,1e205,1e206,1e207,1e208,1e209,1e210,1e211,1e212,1e213,1e214,1e215,1e216,1e217,1e218,1e219,1e220,1e221,1e222,1e223,1e224,1e225,1e226,1e227,1e228,1e229,1e230,1e231,1e232,1e233,1e234,1e235,1e236,1e237,1e238,1e239,1e240,1e241,1e242,1e243,1e244,1e245,1e246,1e247,1e248,1e249,1e250,1e251,1e252,1e253,1e254,1e255,1e256,1e257,1e258,1e259,1e260,1e261,1e262,1e263,1e264,1e265,1e266,1e267,1e268,1e269,1e270,1e271,1e272,1e273,1e274,1e275,1e276,1e277,1e278,1e279,1e280,1e281,1e282,1e283,1e284,1e285,1e286,1e287,1e288,1e289,1e290,1e291,1e292,1e293,1e294,1e295,1e296,1e297,1e298,1e299,1e300,1e301,1e302,1e303,1e304,1e305,1e306,1e307,1e308 };
    return constants[exponent + 323];
}

enum JsonParseFlag {
    FLAG_TEXT_BREAK         = 1,        // '\n' '\r' '\0' '\\' '"'
    FLAG_WHITESPACE         = 2,
    FLAG_DIGIT              = 4,        // '0'~'9'
    FLAG_NUMBER             = 8         // '0'~'9' 'e' 'E' '.'
};
static inline bool matchFlag(uint8_t c, JsonParseFlag flag) {
    static const uint8_t flags[256] = {
    //  0    1    2    3    2    3    6    7    8    9    A    B    C    D    E    F
        3,   2,   2,   2,   2,   2,   2,   2,   2,   2,   3,   2,   2,   3,   2,   2, // 0
        2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2, // 1
        2,   0,   1,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   8,   0, // 2
        12,  12,  12,  12,  12,  12,  12,  12,  12,  12,  0,   0,   0,   0,   0,   0, // 3
        0,   0,   0,   0,   0,   8,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, // 2
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   1,   0,   0,   0, // 3
        0,   0,   0,   0,   0,   8,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, // 6
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, // 7
    // 128-255
        0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0
    };
    return flags[c] & flag;
}

ZJSON_FORCE_INLINE bool parseStringInternal(char*& s)
{    
    if (!matchFlag(*s, FLAG_TEXT_BREAK)) {
        do {            
            if (matchFlag(s[1], FLAG_TEXT_BREAK)) { s += 1; break; } 
            if (matchFlag(s[2], FLAG_TEXT_BREAK)) { s += 2; break; }
            if (matchFlag(s[3], FLAG_TEXT_BREAK)) { s += 3; break; }
            s += 4;
        } while (!matchFlag(*s, FLAG_TEXT_BREAK));
    }
    uint8_t* pend = (uint8_t*)s;
    uint8_t ch = *s++;
    while (ch != '"') {
        if (ch != '\\') return false; // unexpected characters '\0' '\n' '\r'
        // handle the escape characters
        ch = *s++;
        if (ch == 'u') {
            uint32_t u = 0;
            for (uint32_t i = 0; i < 4; i++) {
                u <<= 4;
                int c = *s++;
                if (c >= 'A' && c <= 'F')
                    u += c - 'A' + 10;
                else if (c >= 'a' && c <= 'f')
                    u += c - 'a' + 10;
                else if (c >= '0' && c <= '9')
                    u += c - '0';
                else // unexpected hex char
                    return false;
            }
            if (u && u < 0x80) {
                *pend++ = u;
            }
            else if (u < 0x800) {
                *pend++ = 0xC0 | (u >> 6);
                *pend++ = 0x80 | (u & 0x3F);
            }
            else {
                *pend++ = 0xE0 | (u >> 12);
                *pend++ = 0x80 | ((u >> 6) & 0x3F);
                *pend++ = 0x80 | (u & 0x3F);
            }
        }
        else {
            switch (ch) {
            case 'b': ch = '\b'; break;
            case 'f': ch = '\f'; break;
            case 'n': ch = '\n'; break;
            case 'r': ch = '\r'; break;
            case 't': ch = '\t'; break;
            case '\\':
            case '"':
                break;
            case '\0':
                return false;
            default: // unrecognized escape, so forcefully escape the backslash (not standard)
                *pend++ = '\\';
                break;
            }
            *pend++ = ch;
        }
        ch = *s++;
        while (!matchFlag(ch, FLAG_TEXT_BREAK)) {
            pend[0] = ch; ch = s[0];
            if (matchFlag(ch, FLAG_TEXT_BREAK)) { pend += 1; s += 1; break; }
            pend[1] = ch; ch = s[1];
            if (matchFlag(ch, FLAG_TEXT_BREAK)) { pend += 2; s += 2; break; }
            pend[2] = ch; ch = s[2];
            if (matchFlag(ch, FLAG_TEXT_BREAK)) { pend += 3; s += 3; break; }
            pend[3] = ch; ch = s[3];
            s += 4; pend += 4;
        }
    }
    *pend++ = '\0';
    return true;
}

#define ZJSON_STACK_SIZE 32
#define ZJSON_SKIP_WHITESPACE \
    while (matchFlag(*s, FLAG_WHITESPACE)) {                            \
        do {                                                            \
            if (!matchFlag(s[1], FLAG_WHITESPACE)) { s += 1; break; }   \
            if (!matchFlag(s[2], FLAG_WHITESPACE)) { s += 2; break; }   \
            if (!matchFlag(s[3], FLAG_WHITESPACE)) { s += 3; break; }   \
            s += 4;                                                     \
        } while (matchFlag(*s, FLAG_WHITESPACE));                       \
        if (s[0] != '/' || s[1] != '/') break;                          \
        s += 2;                                                         \
        while (*s && *s != '\n' && *s != '\r') ++s;                     \
    }

int jsonParse(char *s, Value *value, Allocator &allocator) {
    Node *tails[ZJSON_STACK_SIZE];
    uint8_t endchars[ZJSON_STACK_SIZE];
    int top = -1;
    Node *node;

    ZJSON_SKIP_WHITESPACE;

    uint8_t ch = *s;
    if (ch == '{' || ch == '[') {
        ++top;
        tails[top] = nullptr;
        endchars[top] = ch + 2;
    }
    else {
        return ERROR_BAD_ROOT; // Root value must be an object or array
    }

    // deserialize main loop
    ++s;
    for (;;) {
        ZJSON_SKIP_WHITESPACE;
        ch = *s;
        if (ch == ',') {
            if (ZJSON_UNLIKELY(!tails[top]))
                return ERROR_MISMATCH_BRACKET;
            ++s;
            ZJSON_SKIP_WHITESPACE;
            ch = *s;
        }
        else if (ZJSON_UNLIKELY(tails[top] && (ch != endchars[top])))
            return ERROR_MISMATCH_BRACKET;

        while (ch == endchars[top]) {
            ++s;
            for (;;) {
                if (ZJSON_UNLIKELY(top == -1))
                    return ERROR_STACK_UNDERFLOW;
                Type t = endchars[top] == '}' ? JSON_OBJECT : JSON_ARRAY;
                Value v = listToValue(t, tails[top--]);

                if (top == -1) {
                    *value = v;
                    return ERROR_NO_ERROR;
                }
                tails[top]->value = v;

                ZJSON_SKIP_WHITESPACE;
                if (*s == ',') {
                    ++s;
                    ZJSON_SKIP_WHITESPACE;
                    ch = *s;
                    break;
                }
                if (ZJSON_UNLIKELY(*s++ != endchars[top]))
                    return ERROR_MISMATCH_BRACKET;
            }
        }

        if (endchars[top] == ']') { // JSON_ARRAY
            if (ZJSON_UNLIKELY((node = (Node *)allocator.allocate(sizeof(Node) - sizeof(char *))) == nullptr))
                return ERROR_OUT_OF_MEMORY;
            tails[top] = insertAfter(tails[top], node);
        }
        else { // JSON_OBJECT
            if (ZJSON_UNLIKELY((node = (Node *)allocator.allocate(sizeof(Node))) == nullptr))
                return ERROR_OUT_OF_MEMORY;
            tails[top] = insertAfter(tails[top], node);

            // parse a key
            if (ZJSON_UNLIKELY(ch != '"')) return ERROR_UNEXPECTED_CHARACTER;
            tails[top]->name = ++s;
            if (ZJSON_UNLIKELY(!parseStringInternal(s))) return ERROR_BAD_STRING;

            ZJSON_SKIP_WHITESPACE;
            if (ZJSON_UNLIKELY(*s != ':')) return ERROR_UNEXPECTED_CHARACTER;
            ++s;
            ZJSON_SKIP_WHITESPACE;
            ch = *s;
        }

        // parse a value
        switch (ch) {
        case '{':
        case '[': { // start a JSON object or a JSON array
            ++s;
            if (ZJSON_UNLIKELY(++top == ZJSON_STACK_SIZE))
                return ERROR_STACK_OVERFLOW;
            tails[top] = nullptr;
            endchars[top] = ch + 2;
            break;
        }
        case '"': { // JSON string
            ++s;
            tails[top]->value = Value(JSON_STRING, s);
            if (ZJSON_UNLIKELY(!parseStringInternal(s))) return ERROR_BAD_STRING;
            break;
        }
        case 'n': { // JSON null
            if (ZJSON_LIKELY(s[1] == 'u' && s[2] == 'l' && s[3] == 'l')) {
                s += 4;
                tails[top]->value = Value(JSON_NULL);
            }
            else return ERROR_BAD_IDENTIFIER;
            break;
        }
        case 't': { // JSON true
            if (ZJSON_LIKELY(s[1] == 'r' && s[2] == 'u' && s[3] == 'e')) {
                s += 4;
                tails[top]->value = Value(JSON_TRUE);
            }
            else return ERROR_BAD_IDENTIFIER;
            break;
        }
        case 'f': { // JSON false
            if (ZJSON_LIKELY(s[1] == 'a' && s[2] == 'l' && s[3] == 's' && s[4] == 'e')) {
                s += 5;
                tails[top]->value = Value(JSON_FALSE);
            }
            else return ERROR_BAD_IDENTIFIER;
            break;
        }
        case '0': // JSON number
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
        case '-':
        case '.': {
            int32_t n32 = 0;
            int negative = (ch == '-');
            ch = *(s += negative);

#define PARSE_INT_1(x) x }
#define PARSE_INT_2(x) x PARSE_INT_1(x) }
#define PARSE_INT_3(x) x PARSE_INT_2(x) }
#define PARSE_INT_4(x) x PARSE_INT_3(x) }
#define PARSE_INT_5(x) x PARSE_INT_4(x) }
#define PARSE_INT_6(x) x PARSE_INT_5(x) }
#define PARSE_INT_7(x) x PARSE_INT_6(x) }
#define PARSE_INT_8(x) x PARSE_INT_7(x) }
#define PARSE_INT_9(x) x PARSE_INT_8(x) }

            PARSE_INT_9(if (matchFlag(ch, FLAG_DIGIT)) {
                n32 = n32 * 10 + (ch - '0');
                ch = *++s;)

            if (!matchFlag(ch, FLAG_NUMBER)) {
                if (negative) n32 = -n32;
                tails[top]->value = Value(n32);
            }
            else {
                double d = static_cast<double>(n32);
                int64_t exponent = 0;
                int scale = 0, escalesign = 1, escale = 0;
                // before dot
                while (matchFlag(ch, FLAG_DIGIT)) {
                    d = d * 10.0f + (ch - '0');
                    ch = *++s;
                    if (!matchFlag(ch, FLAG_DIGIT)) break;
                    d = d * 10.0f + (ch - '0');
                    ch = *++s;
                }
                // dot and after dot
                if (ch == '.') {
                    ch = *++s;
                    while (matchFlag(ch, FLAG_DIGIT)) {
                        exponent--;
                        d = d * 10.0f + (ch - '0');
                        ch = *++s;
                        if (!matchFlag(ch, FLAG_DIGIT)) break;
                        exponent--;
                        d = d * 10.0f + (ch - '0');
                        ch = *++s;
                    }
                }
                // exponent
                bool negativeE = false;
                int exp = 0;
                if (ch == 'e' || ch == 'E') {
                    ch = *++s;
                    if (ch == '-') {
                        negativeE = true;
                        ch = *++s;
                    }
                    else if (ch == '+') {
                        ch = *++s;
                    }
                    while (matchFlag(ch, FLAG_DIGIT)) {
                        if (ZJSON_UNLIKELY(exp >= 214748364))
                            return ERROR_BAD_NUMBER;
                        exp = exp * 10 + (ch - '0');
                        ch = *++s;
                    }
                }
                exponent += (negativeE ? -exp : exp);
                if (exponent) d *= pow10(exponent);
                if (negative) d = -d;
                tails[top]->value = Value(d);
            }
            break;
        }
        case '\0':
        default:
            return ERROR_BREAKING_BAD;
        }
    }
    return ERROR_BREAKING_BAD;
}

} // namespace zjson

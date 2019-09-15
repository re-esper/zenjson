/*
 * Copyright (c) 2019 esper. All rights reserved.
 *
 * Licensed under the MIT License (the "License"); you may not use this file except
 * in compliance with the License. You may obtain a copy of the License at
 *
 * http://opensource.org/licenses/MIT
 *
 * Unless required by applicable law or agreed to in writing, software distributed
 * under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations under the License.
 * SOFTWARE.
 */

#pragma once

// #include "base.h"

#include <stdint.h>
#include <stddef.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#if defined(__GNUC__) || defined(__clang__)
#define ZJSON_LIKELY(x) __builtin_expect(!!(x), 1)
#define ZJSON_UNLIKELY(x) __builtin_expect(!!(x), 0)
#define ZJSON_FORCE_INLINE __attribute__((always_inline))
#elif defined(_MSC_VER)
#define ZJSON_LIKELY(x) x
#define ZJSON_UNLIKELY(x) x
#define ZJSON_FORCE_INLINE __forceinline
#else
#define ZJSON_LIKELY(x) x
#define ZJSON_UNLIKELY(x) x
#define ZJSON_FORCE_INLINE inline
#endif

namespace zjson {

enum Type {
    JSON_NUMBER = 0,
    JSON_INT,
    JSON_STRING,
    JSON_ARRAY,
    JSON_OBJECT,
    JSON_TRUE,
    JSON_FALSE,
    JSON_NULL = 0xF
};
enum Error {
    ERROR_NO_ERROR,
    ERROR_BAD_NUMBER,
    ERROR_BAD_STRING,
    ERROR_BAD_IDENTIFIER,
    ERROR_BAD_ROOT,
    ERROR_STACK_OVERFLOW,
    ERROR_STACK_UNDERFLOW,
    ERROR_MISMATCH_BRACKET,
    ERROR_UNEXPECTED_CHARACTER,
    ERROR_BREAKING_BAD,
    ERROR_OUT_OF_MEMORY
};

} // namespace zjson


// #include "allocator.h"

namespace zjson {

#define ZJSON_BLOCK_SIZE    8192
class Allocator {
public:
    Allocator() : blocksHead(nullptr), freeBlocksHead(nullptr) {};
    Allocator(const Allocator &) = delete;
    Allocator &operator=(const Allocator &) = delete;
    ~Allocator() {
        deallocate();
    }
    inline void *allocate(size_t size) {
        size = (size + 7) & ~7;
        if (blocksHead && blocksHead->used + size <= ZJSON_BLOCK_SIZE) {
            char *p = (char *)blocksHead + blocksHead->used;
            blocksHead->used += size;
            return p;
        }
        size_t allocSize = sizeof(Block) + size;
        Block *block = freeBlocksHead;
        if (block && block->size >= allocSize) { // reuse free block
            freeBlocksHead = block->next;
        }
        else { // allocate new block
            block = (Block *)malloc(allocSize <= ZJSON_BLOCK_SIZE ? ZJSON_BLOCK_SIZE : allocSize);
            if (!block) return nullptr;
            block->size = allocSize;
        }
        block->used = allocSize;
        if (allocSize <= ZJSON_BLOCK_SIZE || !blocksHead) { // push_front
            block->next = blocksHead;
            blocksHead = block;
        }
        else { // insert
            block->next = blocksHead->next;
            blocksHead->next = block;
        }
        return (char *)block + sizeof(Block);
    }
    inline void reset() {
        if (blocksHead) {
            Block* block = blocksHead;
            while (block && block->next) block = block->next;
            block->next = freeBlocksHead;
            freeBlocksHead = blocksHead;
            blocksHead = nullptr;
        }
    }
    void deallocate() {
        freeBlockChain(blocksHead);
        blocksHead = nullptr;
        freeBlockChain(freeBlocksHead);
        freeBlocksHead = nullptr;
    }
private:
    struct Block {
        Block *next;
        size_t used;
        size_t size;
    } *blocksHead, *freeBlocksHead;
    inline void freeBlockChain(Block* block) {
        while (block) {
            Block* nextblock = block->next;
            free(block);
            block = nextblock;
        }
    }
};

} // namespace zjson


// #include "value.h"

namespace zjson {

#define JSON_VALUE_PAYLOAD_MASK 0x00007FFFFFFFFFFFULL
#define JSON_VALUE_NAN_MASK 0x7FF8000000000000ULL
#define JSON_VALUE_TAG_MASK 0xF
#define JSON_VALUE_TAG_SHIFT 47

struct Node;
struct Value {
    friend class Json;
    inline Value(double x) : fval(x) {
    }
    explicit inline Value(Type type, void *payload = nullptr) {
        assert((uintptr_t)payload <= JSON_VALUE_PAYLOAD_MASK);
        ival = JSON_VALUE_NAN_MASK | ((uint64_t)type << JSON_VALUE_TAG_SHIFT) | (uintptr_t)payload;
    }
    inline Value(int32_t x) {
        ival = JSON_VALUE_NAN_MASK | ((uint64_t)JSON_INT << JSON_VALUE_TAG_SHIFT) | (x & 0xFFFFFFFF);
    }
    explicit inline Value(nullptr_t) {
        ival = JSON_VALUE_NAN_MASK | ((uint64_t)JSON_NULL << JSON_VALUE_TAG_SHIFT);
    }
    explicit inline Value(bool x) {
        ival = JSON_VALUE_NAN_MASK | ((uint64_t)(x ? JSON_TRUE : JSON_FALSE) << JSON_VALUE_TAG_SHIFT);
    }
    inline Type getType() const {
        return isDouble() ? JSON_NUMBER : Type((ival >> JSON_VALUE_TAG_SHIFT) & JSON_VALUE_TAG_MASK);
    }
    inline uint64_t getPayload() const {
        assert(!isDouble());
        return ival & JSON_VALUE_PAYLOAD_MASK;
    }
    inline double toNumber() const {
        assert(getType() == JSON_NUMBER);
        return fval;
    }
    inline int32_t toInt() const {
        assert(getType() == JSON_INT);
        return ival & 0xFFFFFFFF;
    }
    inline char *toString() const {
        assert(getType() == JSON_STRING);
        return (char *)getPayload();
    }
    inline Node *toNode() const {
        assert(getType() == JSON_ARRAY || getType() == JSON_OBJECT);
        return (Node *)getPayload();
    }
private:
    union {
        uint64_t ival;
        double fval;
    };
    inline bool isDouble() const {
        return (int64_t)ival <= (int64_t)JSON_VALUE_NAN_MASK;
    }
    // serialization
    template <typename T>
    void dump(T& out, bool formatted, int indent = 0) const;
};

struct Node {
    Value value;
    Node *next;
    char *name;
};

struct NodeIterator {
    Node *p;
    void operator++() { p = p->next; }
    bool operator!=(const NodeIterator &x) const { return p != x.p; }
    Node *operator*() const { return p; }
    Node *operator->() const { return p; }
};
inline NodeIterator begin(Value& v) {
    return NodeIterator { v.toNode() };
}
inline NodeIterator end(Value&) {
    return NodeIterator { nullptr };
}

inline Node *insertAfter(Node *tail, Node *node) {
    if (!tail)
        return node->next = node;
    node->next = tail->next;
    tail->next = node;
    return node;
}
inline Value listToValue(Type type, Node *tail) {
    if (tail) {
        auto head = tail->next;
        tail->next = nullptr;
        return Value(type, head);
    }
    return Value(type, nullptr);
}

// free as a crt-allocated value
void freeCrtAllocatedValue(Value value) {
    Type type = value.getType();
    if (type == JSON_STRING) {
        free(value.toString());
    }
    else if (type == JSON_ARRAY || type == JSON_OBJECT) {
        Node* node = value.toNode();
        while (node) {
            if (type == JSON_OBJECT) free(node->name); // free key
            freeCrtAllocatedValue(node->value); // free value
            void* ptr = node;
            node = node->next;
            free(ptr); // free node
        }
    }
}

} // namespace zjson


// #include "deserialize.h"

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


// #include "dtoa_milo.h"

#include <assert.h>
#include <math.h>
#include <stdint.h>

inline const char* GetDigitsLut() {
    static const char cDigitsLut[200] = {
        '0','0','0','1','0','2','0','3','0','4','0','5','0','6','0','7','0','8','0','9',
        '1','0','1','1','1','2','1','3','1','4','1','5','1','6','1','7','1','8','1','9',
        '2','0','2','1','2','2','2','3','2','4','2','5','2','6','2','7','2','8','2','9',
        '3','0','3','1','3','2','3','3','3','4','3','5','3','6','3','7','3','8','3','9',
        '4','0','4','1','4','2','4','3','4','4','4','5','4','6','4','7','4','8','4','9',
        '5','0','5','1','5','2','5','3','5','4','5','5','5','6','5','7','5','8','5','9',
        '6','0','6','1','6','2','6','3','6','4','6','5','6','6','6','7','6','8','6','9',
        '7','0','7','1','7','2','7','3','7','4','7','5','7','6','7','7','7','8','7','9',
        '8','0','8','1','8','2','8','3','8','4','8','5','8','6','8','7','8','8','8','9',
        '9','0','9','1','9','2','9','3','9','4','9','5','9','6','9','7','9','8','9','9'
    };
    return cDigitsLut;
}

inline char* i32toa(int32_t value, char* buffer) {
    uint32_t u = static_cast<uint32_t>(value);
    if (value < 0) {
        *buffer++ = '-';
        u = ~u + 1;
    }
    const char* cDigitsLut = GetDigitsLut();
    if (u < 10000) {
        const uint32_t d1 = (u / 100) << 1;
        const uint32_t d2 = (u % 100) << 1;
        if (u >= 1000)
            *buffer++ = cDigitsLut[d1];
        if (u >= 100)
            *buffer++ = cDigitsLut[d1 + 1];
        if (u >= 10)
            *buffer++ = cDigitsLut[d2];
        *buffer++ = cDigitsLut[d2 + 1];
    }
    else if (u < 100000000) {
        // u = bbbbcccc
        const uint32_t b = u / 10000;
        const uint32_t c = u % 10000;
        const uint32_t d1 = (b / 100) << 1;
        const uint32_t d2 = (b % 100) << 1;
        const uint32_t d3 = (c / 100) << 1;
        const uint32_t d4 = (c % 100) << 1;
        if (u >= 10000000)
            *buffer++ = cDigitsLut[d1];
        if (u >= 1000000)
            *buffer++ = cDigitsLut[d1 + 1];
        if (u >= 100000)
            *buffer++ = cDigitsLut[d2];
        *buffer++ = cDigitsLut[d2 + 1];
        *buffer++ = cDigitsLut[d3];
        *buffer++ = cDigitsLut[d3 + 1];
        *buffer++ = cDigitsLut[d4];
        *buffer++ = cDigitsLut[d4 + 1];
    }
    else {
        // u = aabbbbcccc in decimal
        const uint32_t a = u / 100000000; // 1 to 42
        u %= 100000000;
        if (a >= 10) {
            const unsigned i = a << 1;
            *buffer++ = cDigitsLut[i];
            *buffer++ = cDigitsLut[i + 1];
        }
        else
            *buffer++ = static_cast<char>('0' + static_cast<char>(a));
        const uint32_t b = u / 10000; // 0 to 9999
        const uint32_t c = u % 10000; // 0 to 9999
        const uint32_t d1 = (b / 100) << 1;
        const uint32_t d2 = (b % 100) << 1;
        const uint32_t d3 = (c / 100) << 1;
        const uint32_t d4 = (c % 100) << 1;
        *buffer++ = cDigitsLut[d1];
        *buffer++ = cDigitsLut[d1 + 1];
        *buffer++ = cDigitsLut[d2];
        *buffer++ = cDigitsLut[d2 + 1];
        *buffer++ = cDigitsLut[d3];
        *buffer++ = cDigitsLut[d3 + 1];
        *buffer++ = cDigitsLut[d4];
        *buffer++ = cDigitsLut[d4 + 1];
    }
    return buffer;
}

#define UINT64_C2(h, l) ((static_cast<uint64_t>(h) << 32) | static_cast<uint64_t>(l))

struct DiyFp {
	DiyFp() {}

	DiyFp(uint64_t f, int e) : f(f), e(e) {}

	DiyFp(double d) {
		union {
			double d;
			uint64_t u64;
		} u = { d };

		int biased_e = (u.u64 & kDpExponentMask) >> kDpSignificandSize;
		uint64_t significand = (u.u64 & kDpSignificandMask);
		if (biased_e != 0) {
			f = significand + kDpHiddenBit;
			e = biased_e - kDpExponentBias;
		} 
		else {
			f = significand;
			e = kDpMinExponent + 1;
		}
	}

	DiyFp operator-(const DiyFp& rhs) const {
		assert(e == rhs.e);
		assert(f >= rhs.f);
		return DiyFp(f - rhs.f, e);
	}

	DiyFp operator*(const DiyFp& rhs) const {
#if defined(_MSC_VER) && defined(_M_AMD64)
		uint64_t h;
		uint64_t l = _umul128(f, rhs.f, &h);
		if (l & (uint64_t(1) << 63)) // rounding
			h++;
		return DiyFp(h, e + rhs.e + 64);
#elif (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)) && defined(__x86_64__)
		unsigned __int128 p = static_cast<unsigned __int128>(f) * static_cast<unsigned __int128>(rhs.f);
		uint64_t h = p >> 64;
		uint64_t l = static_cast<uint64_t>(p);
		if (l & (uint64_t(1) << 63)) // rounding
			h++;
		return DiyFp(h, e + rhs.e + 64);
#else
		const uint64_t M32 = 0xFFFFFFFF;
		const uint64_t a = f >> 32;
		const uint64_t b = f & M32;
		const uint64_t c = rhs.f >> 32;
		const uint64_t d = rhs.f & M32;
		const uint64_t ac = a * c;
		const uint64_t bc = b * c;
		const uint64_t ad = a * d;
		const uint64_t bd = b * d;
		uint64_t tmp = (bd >> 32) + (ad & M32) + (bc & M32);
		tmp += 1U << 31;  /// mult_round
		return DiyFp(ac + (ad >> 32) + (bc >> 32) + (tmp >> 32), e + rhs.e + 64);
#endif
	}

	DiyFp Normalize() const {
#if defined(_MSC_VER) && defined(_M_AMD64)
		unsigned long index;
		_BitScanReverse64(&index, f);
		return DiyFp(f << (63 - index), e - (63 - index));
#elif defined(__GNUC__)
		int s = __builtin_clzll(f);
		return DiyFp(f << s, e - s);
#else
		DiyFp res = *this;
		while (!(res.f & kDpHiddenBit)) {
			res.f <<= 1;
			res.e--;
		}
		res.f <<= (kDiySignificandSize - kDpSignificandSize - 1);
		res.e = res.e - (kDiySignificandSize - kDpSignificandSize - 1);
		return res;
#endif
	}

	DiyFp NormalizeBoundary() const {
#if defined(_MSC_VER) && defined(_M_AMD64)
		unsigned long index;
		_BitScanReverse64(&index, f);
		return DiyFp (f << (63 - index), e - (63 - index));
#else
		DiyFp res = *this;
		while (!(res.f & (kDpHiddenBit << 1))) {
			res.f <<= 1;
			res.e--;
		}
		res.f <<= (kDiySignificandSize - kDpSignificandSize - 2);
		res.e = res.e - (kDiySignificandSize - kDpSignificandSize - 2);
		return res;
#endif
	}

	void NormalizedBoundaries(DiyFp* minus, DiyFp* plus) const {
		DiyFp pl = DiyFp((f << 1) + 1, e - 1).NormalizeBoundary();
		DiyFp mi = (f == kDpHiddenBit) ? DiyFp((f << 2) - 1, e - 2) : DiyFp((f << 1) - 1, e - 1);
		mi.f <<= mi.e - pl.e;
		mi.e = pl.e;
		*plus = pl;
		*minus = mi;
	}

	static const int kDiySignificandSize = 64;
	static const int kDpSignificandSize = 52;
	static const int kDpExponentBias = 0x3FF + kDpSignificandSize;
	static const int kDpMinExponent = -kDpExponentBias;
	static const uint64_t kDpExponentMask = UINT64_C2(0x7FF00000, 0x00000000);
	static const uint64_t kDpSignificandMask = UINT64_C2(0x000FFFFF, 0xFFFFFFFF);
	static const uint64_t kDpHiddenBit = UINT64_C2(0x00100000, 0x00000000);

	uint64_t f;
	int e;
};

inline DiyFp GetCachedPower(int e, int* K) {
	// 10^-348, 10^-340, ..., 10^340
	static const uint64_t kCachedPowers_F[] = {
		UINT64_C2(0xfa8fd5a0, 0x081c0288), UINT64_C2(0xbaaee17f, 0xa23ebf76),
		UINT64_C2(0x8b16fb20, 0x3055ac76), UINT64_C2(0xcf42894a, 0x5dce35ea),
		UINT64_C2(0x9a6bb0aa, 0x55653b2d), UINT64_C2(0xe61acf03, 0x3d1a45df),
		UINT64_C2(0xab70fe17, 0xc79ac6ca), UINT64_C2(0xff77b1fc, 0xbebcdc4f),
		UINT64_C2(0xbe5691ef, 0x416bd60c), UINT64_C2(0x8dd01fad, 0x907ffc3c),
		UINT64_C2(0xd3515c28, 0x31559a83), UINT64_C2(0x9d71ac8f, 0xada6c9b5),
		UINT64_C2(0xea9c2277, 0x23ee8bcb), UINT64_C2(0xaecc4991, 0x4078536d),
		UINT64_C2(0x823c1279, 0x5db6ce57), UINT64_C2(0xc2109436, 0x4dfb5637),
		UINT64_C2(0x9096ea6f, 0x3848984f), UINT64_C2(0xd77485cb, 0x25823ac7),
		UINT64_C2(0xa086cfcd, 0x97bf97f4), UINT64_C2(0xef340a98, 0x172aace5),
		UINT64_C2(0xb23867fb, 0x2a35b28e), UINT64_C2(0x84c8d4df, 0xd2c63f3b),
		UINT64_C2(0xc5dd4427, 0x1ad3cdba), UINT64_C2(0x936b9fce, 0xbb25c996),
		UINT64_C2(0xdbac6c24, 0x7d62a584), UINT64_C2(0xa3ab6658, 0x0d5fdaf6),
		UINT64_C2(0xf3e2f893, 0xdec3f126), UINT64_C2(0xb5b5ada8, 0xaaff80b8),
		UINT64_C2(0x87625f05, 0x6c7c4a8b), UINT64_C2(0xc9bcff60, 0x34c13053),
		UINT64_C2(0x964e858c, 0x91ba2655), UINT64_C2(0xdff97724, 0x70297ebd),
		UINT64_C2(0xa6dfbd9f, 0xb8e5b88f), UINT64_C2(0xf8a95fcf, 0x88747d94),
		UINT64_C2(0xb9447093, 0x8fa89bcf), UINT64_C2(0x8a08f0f8, 0xbf0f156b),
		UINT64_C2(0xcdb02555, 0x653131b6), UINT64_C2(0x993fe2c6, 0xd07b7fac),
		UINT64_C2(0xe45c10c4, 0x2a2b3b06), UINT64_C2(0xaa242499, 0x697392d3),
		UINT64_C2(0xfd87b5f2, 0x8300ca0e), UINT64_C2(0xbce50864, 0x92111aeb),
		UINT64_C2(0x8cbccc09, 0x6f5088cc), UINT64_C2(0xd1b71758, 0xe219652c),
		UINT64_C2(0x9c400000, 0x00000000), UINT64_C2(0xe8d4a510, 0x00000000),
		UINT64_C2(0xad78ebc5, 0xac620000), UINT64_C2(0x813f3978, 0xf8940984),
		UINT64_C2(0xc097ce7b, 0xc90715b3), UINT64_C2(0x8f7e32ce, 0x7bea5c70),
		UINT64_C2(0xd5d238a4, 0xabe98068), UINT64_C2(0x9f4f2726, 0x179a2245),
		UINT64_C2(0xed63a231, 0xd4c4fb27), UINT64_C2(0xb0de6538, 0x8cc8ada8),
		UINT64_C2(0x83c7088e, 0x1aab65db), UINT64_C2(0xc45d1df9, 0x42711d9a),
		UINT64_C2(0x924d692c, 0xa61be758), UINT64_C2(0xda01ee64, 0x1a708dea),
		UINT64_C2(0xa26da399, 0x9aef774a), UINT64_C2(0xf209787b, 0xb47d6b85),
		UINT64_C2(0xb454e4a1, 0x79dd1877), UINT64_C2(0x865b8692, 0x5b9bc5c2),
		UINT64_C2(0xc83553c5, 0xc8965d3d), UINT64_C2(0x952ab45c, 0xfa97a0b3),
		UINT64_C2(0xde469fbd, 0x99a05fe3), UINT64_C2(0xa59bc234, 0xdb398c25),
		UINT64_C2(0xf6c69a72, 0xa3989f5c), UINT64_C2(0xb7dcbf53, 0x54e9bece),
		UINT64_C2(0x88fcf317, 0xf22241e2), UINT64_C2(0xcc20ce9b, 0xd35c78a5),
		UINT64_C2(0x98165af3, 0x7b2153df), UINT64_C2(0xe2a0b5dc, 0x971f303a),
		UINT64_C2(0xa8d9d153, 0x5ce3b396), UINT64_C2(0xfb9b7cd9, 0xa4a7443c),
		UINT64_C2(0xbb764c4c, 0xa7a44410), UINT64_C2(0x8bab8eef, 0xb6409c1a),
		UINT64_C2(0xd01fef10, 0xa657842c), UINT64_C2(0x9b10a4e5, 0xe9913129),
		UINT64_C2(0xe7109bfb, 0xa19c0c9d), UINT64_C2(0xac2820d9, 0x623bf429),
		UINT64_C2(0x80444b5e, 0x7aa7cf85), UINT64_C2(0xbf21e440, 0x03acdd2d),
		UINT64_C2(0x8e679c2f, 0x5e44ff8f), UINT64_C2(0xd433179d, 0x9c8cb841),
		UINT64_C2(0x9e19db92, 0xb4e31ba9), UINT64_C2(0xeb96bf6e, 0xbadf77d9),
		UINT64_C2(0xaf87023b, 0x9bf0ee6b)
	};
	static const int16_t kCachedPowers_E[] = {
		-1220, -1193, -1166, -1140, -1113, -1087, -1060, -1034, -1007,  -980,
		 -954,  -927,  -901,  -874,  -847,  -821,  -794,  -768,  -741,  -715,
		 -688,  -661,  -635,  -608,  -582,  -555,  -529,  -502,  -475,  -449,
		 -422,  -396,  -369,  -343,  -316,  -289,  -263,  -236,  -210,  -183,
		 -157,  -130,  -103,   -77,   -50,   -24,     3,    30,    56,    83,
		  109,   136,   162,   189,   216,   242,   269,   295,   322,   348,
		  375,   402,   428,   455,   481,   508,   534,   561,   588,   614,
		  641,   667,   694,   720,   747,   774,   800,   827,   853,   880,
		  907,   933,   960,   986,  1013,  1039,  1066
	};

	//int k = static_cast<int>(ceil((-61 - e) * 0.30102999566398114)) + 374;
	double dk = (-61 - e) * 0.30102999566398114 + 347;	// dk must be positive, so can do ceiling in positive
	int k = static_cast<int>(dk);
	if (k != dk)
		k++;

	unsigned index = static_cast<unsigned>((k >> 3) + 1);
	*K = -(-348 + static_cast<int>(index << 3));	// decimal exponent no need lookup table

	assert(index < sizeof(kCachedPowers_F) / sizeof(kCachedPowers_F[0]));
	return DiyFp(kCachedPowers_F[index], kCachedPowers_E[index]);
}

inline void GrisuRound(char* buffer, int len, uint64_t delta, uint64_t rest, uint64_t ten_kappa, uint64_t wp_w) {
	while (rest < wp_w && delta - rest >= ten_kappa &&
		   (rest + ten_kappa < wp_w ||  /// closer
			wp_w - rest > rest + ten_kappa - wp_w)) {
		buffer[len - 1]--;
		rest += ten_kappa;
	}
}

inline unsigned CountDecimalDigit32(uint32_t n) {
	// Simple pure C++ implementation was faster than __builtin_clz version in this situation.
	if (n < 10) return 1;
	if (n < 100) return 2;
	if (n < 1000) return 3;
	if (n < 10000) return 4;
	if (n < 100000) return 5;
	if (n < 1000000) return 6;
	if (n < 10000000) return 7;
	if (n < 100000000) return 8;
    return 9;
}

inline void DigitGen(const DiyFp& W, const DiyFp& Mp, uint64_t delta, char* buffer, int* len, int* K) {
    static const uint32_t kPow10[] = { 1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000 };
    const DiyFp one(uint64_t(1) << -Mp.e, Mp.e);
    const DiyFp wp_w = Mp - W;
    uint32_t p1 = static_cast<uint32_t>(Mp.f >> -one.e);
    uint64_t p2 = Mp.f & (one.f - 1);
    int kappa = CountDecimalDigit32(p1); // kappa in [0, 9]
    *len = 0;

    while (kappa > 0) {
        uint32_t d = 0;
        switch (kappa) {
        case  9: d = p1 / 100000000; p1 %= 100000000; break;
        case  8: d = p1 / 10000000; p1 %= 10000000; break;
        case  7: d = p1 / 1000000; p1 %= 1000000; break;
        case  6: d = p1 / 100000; p1 %= 100000; break;
        case  5: d = p1 / 10000; p1 %= 10000; break;
        case  4: d = p1 / 1000; p1 %= 1000; break;
        case  3: d = p1 / 100; p1 %= 100; break;
        case  2: d = p1 / 10; p1 %= 10; break;
        case  1: d = p1;              p1 = 0; break;
        }
        if (d || *len)
            buffer[(*len)++] = static_cast<char>('0' + static_cast<char>(d));
        kappa--;
        uint64_t tmp = (static_cast<uint64_t>(p1) << -one.e) + p2;
        if (tmp <= delta) {
            *K += kappa;
            GrisuRound(buffer, *len, delta, tmp, static_cast<uint64_t>(kPow10[kappa]) << -one.e, wp_w.f);
            return;
        }
    }

    // kappa = 0
    for (;;) {
        p2 *= 10;
        delta *= 10;
        char d = static_cast<char>(p2 >> -one.e);
        if (d || *len)
            buffer[(*len)++] = static_cast<char>('0' + d);
        p2 &= one.f - 1;
        kappa--;
        if (p2 < delta) {
            *K += kappa;
            GrisuRound(buffer, *len, delta, p2, one.f, wp_w.f * kPow10[-kappa]);
            return;
        }
    }
}

inline void Grisu2(double value, char* buffer, int* length, int* K) {
	const DiyFp v(value);
	DiyFp w_m, w_p;
	v.NormalizedBoundaries(&w_m, &w_p);

	const DiyFp c_mk = GetCachedPower(w_p.e, K);
	const DiyFp W = v.Normalize() * c_mk;
	DiyFp Wp = w_p * c_mk;
	DiyFp Wm = w_m * c_mk;
	Wm.f++;
	Wp.f--;
	DigitGen(W, Wp, Wp.f - Wm.f, buffer, length, K);
}

inline void WriteExponent(int K, char* buffer) {
	if (K < 0) {
		*buffer++ = '-';
		K = -K;
	}

	if (K >= 100) {
		*buffer++ = '0' + static_cast<char>(K / 100);
		K %= 100;
		const char* d = GetDigitsLut() + K * 2;
		*buffer++ = d[0];
		*buffer++ = d[1];
	}
	else if (K >= 10) {
		const char* d = GetDigitsLut() + K * 2;
		*buffer++ = d[0];
		*buffer++ = d[1];
	}
	else
		*buffer++ = '0' + static_cast<char>(K);

	*buffer = '\0';
}

inline void Prettify(char* buffer, int length, int k) {
	const int kk = length + k;	// 10^(kk-1) <= v < 10^kk

	if (length <= kk && kk <= 21) {
		// 1234e7 -> 12340000000
		for (int i = length; i < kk; i++)
			buffer[i] = '0';
		buffer[kk] = '\0';
	}
	else if (0 < kk && kk <= 21) {
		// 1234e-2 -> 12.34
		memmove(&buffer[kk + 1], &buffer[kk], length - kk);
		buffer[kk] = '.';
		buffer[length + 1] = '\0';
	}
	else if (-6 < kk && kk <= 0) {
		// 1234e-6 -> 0.001234
		const int offset = 2 - kk;
		memmove(&buffer[offset], &buffer[0], length);
		buffer[0] = '0';
		buffer[1] = '.';
		for (int i = 2; i < offset; i++)
			buffer[i] = '0';
		buffer[length + offset] = '\0';
	}
	else if (length == 1) {
		// 1e30
		buffer[1] = 'e';
		WriteExponent(kk - 1, &buffer[2]);
	}
	else {
		// 1234e30 -> 1.234e33
		memmove(&buffer[2], &buffer[1], length - 1);
		buffer[1] = '.';
		buffer[length + 1] = 'e';
		WriteExponent(kk - 1, &buffer[0 + length + 2]);
	}
}

inline void dtoa_milo(double value, char* buffer) {
	// Not handling NaN and inf
	assert(!isnan(value));
	assert(!isinf(value));

	if (value == 0) {
		buffer[0] = '0';
        buffer[1] = '\0';
	}
	else {
		if (value < 0) {
			*buffer++ = '-';
			value = -value;
		}
		int length, K;
		Grisu2(value, buffer, &length, &K);
		Prettify(buffer, length, K);
	}
}

// #include "serialize.h"

namespace zjson {

template <typename T>
class Writer : public T {
public:
    template <typename I>
    inline Writer(I& init) : T(init) {}
    template <typename I, typename J>
    inline Writer(I& init1, J& init2) : T(init1, init2) {}
    inline void puts(const char* pStr, size_t l) { T::puts(pStr, l); }
    inline void putc(char c) { T::putc(c); }
    inline void writeNumber(double d) {
        static char buffer[25];
        dtoa_milo(d, buffer);
        puts(buffer, strlen(buffer));
    }
    inline void writeInt(int32_t n) {
        static char buffer[10];
        char* end = i32toa(n, buffer);
        puts(buffer, end - buffer);
    }
    inline void writeEscaped(const char* str) {
        static const char* s_to_hex = "0123456789abcdef";
        const char* s = str;
        putc('"');
        while (*s) {
            uint8_t c = *s++;
            if (c >= ' ' && c != '"' && c != '\\')
                putc(c);
            else {
                putc('\\');
                switch (c) {
                case '\b': putc('b'); break;
                case '\r': putc('r'); break;
                case '\t': putc('t'); break;
                case '\f': putc('f'); break;
                case '\n': putc('n'); break;
                case '\\': putc('\\'); break;
                case '\"': putc('\"'); break;
                default:
                    puts("u00", 3);
                    putc(s_to_hex[c >> 4]);
                    putc(s_to_hex[c & 0xF]);
                    break;
                }
            }
        }
        putc('"');
    }
};

class BufferWriter {
public:
    inline BufferWriter(char* buffer, size_t bufferSize)
        : _dst(buffer), _head(buffer), _end(buffer + bufferSize)
    {
    }
    inline size_t size() const { return _dst - _head; }
    inline void reset() { _dst = _head; }
    inline void puts(const char* str, size_t l) {
        if (_dst + l <= _end)
            memcpy(_dst, str, l);
        _dst += l;
    }
    inline void putc(char c) {
        if (_dst < _end) *_dst = c;
        _dst++;
    }
    inline void writeTabs(size_t n) {
        if (_dst + n <= _end)
            memset(_dst, '\t', n);
        _dst += n;
    }
private:
    char* _dst;
    char* _head;
    char* _end;
};

#define ZJSON_WRITER_CAPACITY  256
class StringWriter {
public:
    inline StringWriter(std::string& buffer)
        : _buffer(buffer)
    {
        if (_buffer.capacity() < ZJSON_WRITER_CAPACITY)
            _buffer.reserve(ZJSON_WRITER_CAPACITY);
        _buffer.clear();
    }
    inline size_t size() const { return _buffer.size(); }
    inline void reset() { _buffer.clear(); }
    inline void puts(const char* str, size_t l) { _buffer.insert(_buffer.end(), str, str + l); }
    inline void putc(char c) { _buffer.push_back(c); }
    inline void writeTabs(size_t n) { _buffer.insert(_buffer.end(), n, '\t'); }
private:
    std::string& _buffer;
};

template <typename T>
void Value::dump(T& out, bool formatted, int indent) const
{
    switch (getType()) {
    case JSON_NUMBER:
        out.writeNumber(toNumber());
        break;
    case JSON_INT:
        out.writeInt(toInt());
        break;
    case JSON_STRING:
        out.writeEscaped(toString());
        break;
    case JSON_ARRAY:
        if (!toNode()) {
            static const char* s_empty_array[4] = { "[]", "[ ]" };
            out.puts(s_empty_array[formatted], 2 + formatted);
            break;
        }
        out.putc('[');
        if (formatted) out.putc('\n');
        indent++;
        for (Node* n = toNode(); n; n = n->next) {
            if (formatted) out.writeTabs(indent);
            n->value.dump(out, formatted, indent);
            if (n->next) {
                out.putc(',');
                if (formatted) out.putc(' ');
            }
            if (formatted) out.putc('\n');
        }
        indent--;
        if (formatted) out.writeTabs(indent);
        out.putc(']');
        break;
    case JSON_OBJECT:
        if (!toNode()) {
            static const char* s_empty_array[4] = { "{}", "{ }" };
            out.puts(s_empty_array[formatted], 2 + formatted);
            break;
        }
        out.putc('{');
        if (formatted) out.putc('\n');
        indent++;
        for (Node* n = toNode(); n; n = n->next) {
            if (formatted) out.writeTabs(indent);
            out.writeEscaped(n->name);
            if (formatted)
                out.puts(" : ", 3);
            else
                out.putc(':');
            n->value.dump(out, formatted, indent);
            if (n->next) {
                out.putc(',');
                if (formatted) out.putc(' ');
            }
            if (formatted) out.putc('\n');
        }
        indent--;
        if (formatted) out.writeTabs(indent);
        out.putc('}');
        break;
    case JSON_TRUE:
        out.puts("true", 4);
        break;
    case JSON_FALSE:
        out.puts("false", 5);
        break;
    case JSON_NULL:
        out.puts("null", 4);
        break;
    }
}

} // namespace zjson


// #include "wrapper.h"

#include <initializer_list>
#include <type_traits>

namespace zjson {

// A wrapper class holds a Value or a Value view, with or without allocator.
class Json {
public:
    friend struct Value;
    explicit Json(Value* value, Allocator* allocator = nullptr)
        : _value(value), _allocator(allocator)
    {
        _isValueView = _value != nullptr;
        if (!_isValueView) _value = new Value(JSON_NULL);
    }
    // copy constructor
    Json(const Json& o) : _allocator(o._allocator), _isValueView(o._isValueView)
    {
        if (_isValueView)
            _value = o._value;
        else {
            if (_allocator) { // has allocator
                _value = new Value(o.toValue());
            }
            else { // no allocator + not a view, deep copy
                _value = new Value(clone(o.toValue()));
            }
        }
    }
    // move constructor
    Json(Json&& o) noexcept
        : _value(o._value), _allocator(o._allocator), _isValueView(o._isValueView)
    {
        o._value = nullptr;
        o._isValueView = false;
    }
    // constructors
    Json(Value value, Allocator* allocator = nullptr) : _allocator(allocator), _isValueView(false) {
        _value = new Value(value);
    }
    Json(bool value, Allocator* allocator = nullptr) : _allocator(allocator), _isValueView(false) {
        _value = new Value(value);
    }
    Json(nullptr_t, Allocator* allocator = nullptr) : _allocator(allocator), _isValueView(false) {
        _value = new Value(JSON_NULL);
    }
    Json(double value, Allocator* allocator = nullptr) : _allocator(allocator), _isValueView(false) {
        _value = new Value(value);
    }
    Json(int32_t value, Allocator* allocator = nullptr) : _allocator(allocator), _isValueView(false) {
        _value = new Value(value);
    }
    Json(const char* str, Allocator* allocator = nullptr)
        : _allocator(allocator), _isValueView(false)
    {
        _value = new Value(JSON_STRING, clone(str));
    }
    Json(std::initializer_list<Json> init, Allocator* allocator = nullptr)
        : _allocator(allocator), _isValueView(false)
    {
        bool isMatchObject = true;
        for (auto& json : init) {
            if (json.getType() != JSON_ARRAY || json.getLength() != 2 ||
                json.getElement(0)->value.getType() != JSON_STRING) {
                isMatchObject = false;
                break;
            }
        }
        if (isMatchObject) {
            Node *node, *tail = nullptr;
            for (auto& json : init) {
                node = (Node*)allocate(sizeof(Node));
                char* name = json.getElement(0)->value.toString();
                node->name = clone(name);
                node->value = clone(json.getElement(1)->value);
                tail = insertAfter(tail, node);
            }
            _value = new Value(listToValue(JSON_OBJECT, tail));
        }
        else {
            Node *node, *tail = nullptr;
            bool first = true;
            for (auto& json : init) {
                node = (Node*)allocate(sizeof(Node) - sizeof(char*));
                node->value = clone(json.toValue());
                tail = insertAfter(tail, node);
            }
            _value = new Value(listToValue(JSON_ARRAY, tail));
        }
    }
    // destructor
    virtual ~Json() {
        destruct();
        if (!_isValueView) delete _value;
    }
    
    Allocator* getAllocator() const { return _allocator; }

    inline Type getType() const {
        return _value->getType();
    }
    inline Value toValue() const {
        return *_value;
    }
    inline bool isNull() const { return getType() == JSON_NULL; }
    inline bool isBool() const {
        Type type = getType();
        return type == JSON_FALSE || type == JSON_TRUE;
    }
    inline bool isInt32() const { return getType() == JSON_INT; }
    inline bool isNumber() const {
        Type type = getType();
        return type == JSON_INT || type == JSON_NUMBER;
    }
    inline bool isString() const { return getType() == JSON_STRING; }
    inline bool isObject() const { return getType() == JSON_OBJECT; }
    inline bool isArray() const { return getType() == JSON_ARRAY; }

    // getters
    inline int32_t getInt(int32_t def = 0) const {
        if (getType() == JSON_INT) return _value->toInt();
        else if (getType() == JSON_NUMBER)  return static_cast<int32_t>(_value->toNumber());
        return def;
    }
    inline int64_t getInt64(int64_t def = 0) const {
        if (getType() == JSON_INT) return _value->toInt();
        else if (getType() == JSON_NUMBER)  return static_cast<int64_t>(_value->toNumber());
        return def;
    }
    inline double getDouble(double def = 0.f) const {
        if (getType() == JSON_INT) return static_cast<double>(_value->toInt());
        else if (getType() == JSON_NUMBER)  return _value->toNumber();
        return def;
    }
    inline bool getBool(bool def = false) const {
        if (getType() == JSON_TRUE) return true;
        else if(getType() == JSON_FALSE) return false;
        return def;
    }
    inline char* getString(char* def = nullptr) const {
        if (getType() == JSON_STRING) return _value->toString();
        return def;
    }
    template<typename T, typename std::enable_if<std::is_arithmetic<T>::value, int>::type = 0>
    operator T() const {
        if (getType() == JSON_INT) return static_cast<T>(_value->toInt());
        else if (getType() == JSON_NUMBER)  return static_cast<T>(_value->toNumber());
        return 0;
    }
    operator bool() const { return getBool(); }
    operator char*() const { return getString(); }

    // setters
    inline void set(Value value) {
        destruct();
        *_value = clone(value);
    }
    inline void set(const char* value) {
        assert(value);
        destruct();
        *_value = Value(JSON_STRING, clone(value));
    }
    inline Json& operator=(const Json& json) {
        if (_allocator && _allocator == json._allocator) {
            *_value = json.toValue();
        }
        else {
            destruct();
            *_value = clone(json.toValue());
        }        
        return *this;
    }
    template<typename T = char>
    inline Json& operator=(const T* value) {
        set(value);
        return *this;
    }

    // Array/Object generic functions
    inline uint32_t getLength() const {
        assert(getType() == JSON_ARRAY || getType() == JSON_OBJECT);
        Node* n = _value->toNode();
        uint32_t l = 0;
        while (n) {
            l++;
            n = n->next;
        }
        return l;
    }
    inline bool remove(Node* node) {
        assert(getType() == JSON_ARRAY || getType() == JSON_OBJECT);
        assert(node);
        Node* n = _value->toNode();
        Node* prev = nullptr;
        while (n) {
            if (n == node) {
                if (prev)
                    prev->next = n->next;
                else
                    *_value = Value(getType(), n->next);
                if (!_allocator) {
                    node->next = nullptr;
                    freeCrtAllocatedValue(Value(getType(), node));
                }
                return true;
            }
            prev = n;
            n = n->next;
        }
        return false;
    }
    // Array functions
    inline Node* getElement(uint32_t index) const {
        assert(getType() == JSON_ARRAY);
        Node* n = _value->toNode();
        uint32_t i = 0;
        while (n) {
            if (index == i++) break;
            n = n->next;
        }
        return n;
    }
    inline Node* pushBack(Value val) {
        assert(getType() == JSON_ARRAY || (getType() == JSON_OBJECT && getLength() == 0));
        Node* n = (Node*)allocate(sizeof(Node) - sizeof(char*));
        n->value = clone(val);
        Node* tail = _value->toNode();
        if (tail) {
            while (tail->next) tail = tail->next;
            insertAfter(tail, n);
        }
        else {
            n->next = n;
            *_value = listToValue(JSON_ARRAY, n);
        }
        return n;
    }
    inline Node* insertAt(uint32_t index, Value val) {
        assert(getType() == JSON_ARRAY || (getType() == JSON_OBJECT && getLength() == 0 && index == 0));
        assert(index <= getLength());
        Node* n = (Node*)allocate(sizeof(Node) - sizeof(char*));
        n->value = clone(val);
        Node* tail = _value->toNode();
        if (tail) {
            if (index == 0) {
                n->next = tail;
                *_value = Value(JSON_ARRAY, n);
            }
            else {
                uint32_t i = 0;
                while (tail->next) {
                    if (index == ++i) break;
                    tail = tail->next;
                }
                insertAfter(tail, n);
            }
        }
        else {
            n->next = nullptr;
            *_value = Value(JSON_ARRAY, n);
        }
    }
    // Object functions
    inline Node* findMember(const char* name) const {
        assert(getType() == JSON_OBJECT);
        Node* n = _value->toNode();
        while (n) {
            if (strcmp(n->name, name) == 0) break;
            n = n->next;
        }
        return n;
    }
    inline Node* addMember(const char* name, Value val) {
        assert(getType() == JSON_OBJECT);
        Node* n = (Node*)allocate(sizeof(Node));
        n->name = clone(name);
        n->value = clone(val);
        Node* tail = _value->toNode();
        if (tail) {
            while (tail->next) tail = tail->next;
            insertAfter(tail, n);
        }
        else {
            n->next = nullptr;
            *_value = Value(JSON_OBJECT, n);
        }
        return n;
    }
    // subscript
    inline Json operator[](uint32_t index) {
        if (getType() == JSON_ARRAY) {
            uint32_t length = getLength();
            if (index < length) {
                Node* n = getElement(index);
                return Json(&n->value, _allocator);
            }
            else if (index == length) { // if index == length, push back
                Node* n = pushBack(Value(JSON_NULL));
                return Json(&n->value, _allocator);
            }
        }
        else if (index == 0 && getType() == JSON_OBJECT && getLength() == 0) {
            Node* n = pushBack(Value(JSON_NULL));
            return Json(&n->value, _allocator);
        }
        return Json(nullptr);
    }
    template<typename T = char> // to prevent zero's ambiguous
    inline Json operator[](const T* name) {
        if (getType() == JSON_OBJECT) {
            Node* n = findMember(name);
            if (!n) {
                n = addMember(name, Value(JSON_NULL));
            }
            return Json(&n->value, _allocator); // RVO
        }
        return Json(nullptr);
    }
    // dump
    bool dump(char* buffer, size_t bufferSize, size_t* pSize = nullptr, bool formatted = true) {
        Writer<BufferWriter> writer(buffer, bufferSize);
        _value->dump(writer, formatted);
        writer.putc('\0');
        if (pSize) *pSize = writer.size();
        return writer.size() <= bufferSize;
    }
    std::string dump(bool formatted = true) {
        std::string buffer;
        Writer<StringWriter> writer(buffer);
        _value->dump(writer, formatted);
        return buffer;
    }
protected:
    // allocate with or without allocator
    inline void *allocate(size_t size) {
        if (_allocator) return _allocator->allocate(size);
        return malloc(size);
    }
    // deep copying
    inline char* clone(const char* str) {
        size_t l = strlen(str) + 1;
        char* rstr = (char*)allocate(l);
        memcpy(rstr, str, l);
        return rstr;
    }
    Node* clone(const Node* node, Type nodeType) {
        assert(node);
        Node* n;
        if (nodeType == JSON_OBJECT) {
            n = (Node*)allocate(sizeof(Node));
            n->name = clone(node->name);
        }
        else { // JSON_ARRAY
            n = (Node*)allocate(sizeof(Node) - sizeof(char*));
        }
        n->value = clone(node->value);
        return n;
    }
    Value clone(const Value value) {
        Type type = value.getType();
        if (type == JSON_STRING) {
            return Value(JSON_STRING, clone(value.toString()));
        }
        else if (type == JSON_ARRAY || type == JSON_OBJECT) {
            Node *n, *tail = nullptr;
            for (Node *node = value.toNode(); node; node = node->next) {
                n = clone(node, type);
                tail = insertAfter(tail, n);
            }
            return listToValue(type, tail);
        }
        return value;
    }
    // destruction
    void destruct() {
        if (_value && !_isValueView && !_allocator) {
            freeCrtAllocatedValue(*_value);
        }
    }
protected:
	Value* _value;
	Allocator* _allocator;
	bool _isValueView;
};

inline NodeIterator begin(Json& json) {
    return NodeIterator { json.toValue().toNode() };
}
inline NodeIterator end(Json&) {
    return NodeIterator { nullptr };
}

// A document for parsing JSON text as DOM.
class Document final : public Json {
public:
    Document() : Json(nullptr) {
        _allocator = new Allocator();
    }
    ~Document() {
        delete _allocator;
    }
    int parse(char* content) {
        _allocator->reset();
        return jsonParse(content, _value, *_allocator);
    }
};

} // namespace zjson


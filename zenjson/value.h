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

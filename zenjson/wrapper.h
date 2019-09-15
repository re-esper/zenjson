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

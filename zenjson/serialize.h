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

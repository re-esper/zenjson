#include <cstdio>
#include <time.h>
#include <algorithm>
#include <vector>
#include <memory>

#define ZENJSON     0
#define RAPIDJSON   1
#define SIMDJSON    2 // need C++17
#define NLOHMANN    3
#define TARGET      ZENJSON

const size_t N = 1000;
const char* jsonFiles[] = {
    "data/twitter.json",
    "data/canada.json",
    "data/citm_catalog.json",
    NULL
};

#if TARGET == ZENJSON
#include "zenjson.h"
#elif TARGET == RAPIDJSON
#define RAPIDJSON_SSE2
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h"
#elif TARGET == SIMDJSON
#include "simdjson/simdjson.h"
#include "simdjson/simdjson.cpp"
#elif TARGET == NLOHMANN
#include "json.hpp"
#endif

void benchmark(const char* filename) {
    FILE* file = fopen(filename, "rb");
    fseek(file, 0, SEEK_END);
    size_t length = ftell(file);
    fseek(file, 0, SEEK_SET);
    std::vector<char> buffer(length);
    fread(buffer.data(), length, 1, file);
    fclose(file);
    
    auto printResult = [&](const std::string& name, clock_t elapsed) {
        double average = 1000.0 * elapsed / CLOCKS_PER_SEC / N;
        if (name.rfind("parse") == 0) {
            double throughput = length / (1024.0 * 1024.0) / (average * 0.001);
            printf("%12s %24s    %0.3f ms    %3.3f MB/s\n", name.c_str(), filename, average, throughput);
        }
        else {
            printf("%12s %24s    %0.3f ms\n", name.c_str(), filename, average);
        }
    };

#if TARGET == ZENJSON
    // parse
    zjson::Document d;
    char* xbuf = new char[buffer.size() + 1];
    xbuf[buffer.size()] = 0;
    clock_t start = clock();
    for (size_t i = 0; i < N; ++i) {
        memcpy(xbuf, buffer.data(), buffer.size());
        int err = d.parse(xbuf);
    }
    printResult("parse", clock() - start);
    // stringify
    char* outbuf = new char[1024 * 1024 * 4];
    size_t outSize = 0;
    start = clock();
    for (size_t i = 0; i < N; ++i) {
        d.dump(outbuf, 1024 * 1024 * 4, &outSize);
    }
    printResult("dump", clock() - start);
    delete[] xbuf;
    delete[] outbuf;
#elif TARGET == RAPIDJSON
    // parse
    rapidjson::Document d;
    char* xbuf = new char[buffer.size() + 1];
    xbuf[buffer.size()] = 0;
    clock_t start = clock();
    for (size_t i = 0; i < N; ++i) {
        memcpy(xbuf, buffer.data(), buffer.size());
        d.ParseInsitu(xbuf);
    }
    printResult("parse", clock() - start);
    // stringify
    rapidjson::StringBuffer outbuf;
    outbuf.Reserve(1024 * 1024 * 16);
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(outbuf);
    start = clock();
    for (size_t i = 0; i < N; ++i) {
        d.Accept(writer);
    }
    printResult("dump", clock() - start);
    delete[] xbuf;
#elif TARGET == SIMDJSON
    // parse
    simdjson::padded_string xbuf((char*)buffer.data(), buffer.size());
    simdjson::ParsedJson pj;
    pj.allocate_capacity(xbuf.size());
    clock_t start = clock();
    for (size_t i = 0; i < N; ++i) {
        int err = json_parse(xbuf, pj);
    }
    printResult("parse", clock() - start);
#elif TARGET == NLOHMANN
    // parse
    std::string xbuf(buffer.begin(), buffer.end());
    clock_t start = clock();
    nlohmann::json json;
    for (size_t i = 0; i < N; ++i) {
        json = nlohmann::json::parse(xbuf);
    }
    printResult("parse", clock() - start);
    // stringify
    std::string outstr;
    start = clock();
    for (size_t i = 0; i < N; ++i) {
        outstr = json.dump();
    }
    printResult("dump", clock() - start);
#endif
}

int main()
{
    for (size_t i = 0; jsonFiles[i]; ++i) {
        benchmark(jsonFiles[i]);
    }
}

# ZenJSON
zenjson is a JSON parser/generator for C++.

zenjson is very [high-performance](#performance). It is much faster than RapidJSON both in parsing/serializing, and only lose to simdjson a little in parsing normal json files.

zenjson also has easy-to-use, intuitive syntax, benifits from modern C++.

zenjson is highly inspired from Richard Geldreich's [pjson](https://twitter.com/richgel999/status/810342151477493761), vivkin's [gason](https://github.com/vivkin/gason) and [RapidJSON](https://github.com/Tencent/rapidjson).

## Features
* No dependencies.
* Single header file. Simply drop zenjson.h into your project.
* Small code size (<2k loc) and memory-friendly (16-24bytes per value).
* In-stu parsing. That means it modifies and depends the input string.

## Usage

### Initialize
```cpp
// create a json with value "null"
zjson::Json json1 = nullptr;
// create a json string
zjson::Json json2 = "hello world";
// create a empty json object. empty object is also treated as empty array in zenjson
zjson::Json json3 = {};
// create a complex json, using an initializer list
zjson::Json json = {
	{ "pi", 3.141 },
	{ "nothing", nullptr },
	{ "variable", json2 },
	{ "list", { 1, 0, 2 } },
	{ "object", {
		{"currency", "USD"},
		{"value", 42.99}
	    }
	}
};
```
### Modify
```cpp
// create a empty json object (or array)
zjson::Json json = {};
// you can modify or add members using operator []
json["pi"] = 3.141;
json["nothing"] = nullptr;
json["data"] = {};
// you can modify or push back elements too
json["data"][0] = 22;
json["data"][1] = "33";
// add an object inside the object
json["answer"]["everything"] = 42;
// add a complex json
json["pi"] = {
        { "list", { 1, 0, 2 } },
        { "object", {
                {"currency", "USD"},
                {"value", 42.99}
            }
	}
};
```
### Obtain
```cpp
double pi = json["pi"];
std::string hello = json["variable"];
bool enabled = json["enabled"];
int64_t twentytwo = json["data"][0];
// or uses getInt/Double/String/... interface, they support passing default values
// if node "backend" does not exist, got "opengl"
std::string backend = json["config"]["backend"].getString("opengl");
```
### Serialization / Deserialization
```cpp
zjson::Document doc;
// parsing from buffer
int error = doc.parse(jsonstr);
// dump to std::string
std::string jsonstr = doc.dump();
// dump to a buffer, outSize return the buffer size actully needed
bool ok = doc.dump(buffer, 1024 * 1024 * 4, &outSize, true);
```

## Performance
All tested JSON files are same with [nativejson-benchmark](https://github.com/miloyip/nativejson-benchmark/tree/master/data).

The following results run on my Intel Core i7-7700K, compiled by Visual Studio 2017, 64-bit build.

### Parsing

| Library      | twitter.json<br>/time | twitter.json<br>/throughput | canada.json<br>/time | canada.json<br>/throughput | citm_catalog<br>.json/time | citm_catalog<br>.json/throughput |
| ------------ | ------------ | ------------ | ------------ | ------------ | ------------ | ------------ |
| zenjson      | 0.296 ms     | 2034.7 MB/s  | 3.048 ms     | 704.3 MB/s   | 0.715 ms     | 2303.8 MB/s  |
| RapidJSON    | 1.056 ms     | 570.3 MB/s   | 5.509 ms     | 389.7 MB/s   | 1.869 ms     | 881.3 MB/s   |
| simdjson     | 0.292 ms     | 2062.5 MB/s  | 2.093 ms     | 1025.7 MB/s  | 0.692 ms     | 2380.3 MB/s  |
| nlohmann/json| 7.910 ms     | 76.1 MB/s    | 41.988 ms    | 51.1 MB/s    | 14.094 ms    | 116.9 MB/s   |

Notes:
1. RapidJSON uses RAPIDJSON_SSE2/ParseInsitu.
2. simdjson uses allocate_capacity for pre-allocation. It is compiled with /std:c++17, others are default.

### Stringifying

| Library      | twitter.json | canada.json  | citm_catalog.json |
| ------------ | ------------ | ------------ | ------------ |
| zenjson      | 0.712 ms     | 8.723 ms     | 0.777 ms     |
| RapidJSON    | 1.877 ms     | 24.125 ms    | 3.299 ms     |
| nlohmann/json| 5.584 ms     | 83.046 ms    | 11.850 ms    |


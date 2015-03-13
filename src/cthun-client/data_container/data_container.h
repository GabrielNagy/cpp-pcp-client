#ifndef CTHUN_CLIENT_SRC_DATA_CONTAINER_DATA_CONTAINER_H_
#define CTHUN_CLIENT_SRC_DATA_CONTAINER_DATA_CONTAINER_H_

#include <vector>
#include <cstdarg>
#include <iostream>
#include <tuple>
#include <typeinfo>

// Forward declarations for rapidjson
namespace rapidjson {
    class CrtAllocator;
    template <typename BaseAllocator> class MemoryPoolAllocator;
    template <typename Encoding, typename Allocator> class GenericValue;
    template<typename CharType> struct UTF8;
    using Value = GenericValue<UTF8<char>, MemoryPoolAllocator<CrtAllocator>>;
    using Allocator = MemoryPoolAllocator<CrtAllocator>;
    template <typename Encoding,
              typename Allocator,
              typename StackAllocator> class GenericDocument;
    using Document = GenericDocument<UTF8<char>, MemoryPoolAllocator<CrtAllocator>, CrtAllocator>;
 }

namespace CthunClient {

// Data abstraction overlaying the raw JSON objects

// Errors

/// Error thrown when a message string is invalid.
class parse_error : public std::runtime_error  {
  public:
    explicit parse_error(std::string const& msg) : std::runtime_error(msg) {}
};

/// Error thrown when a nested message index is invalid.
class index_error : public std::runtime_error  {
  public:
    explicit index_error(std::string const& msg) : std::runtime_error(msg) {}
};

/// Error thrown when trying to set or get invalid type.
class data_type_error : public std::runtime_error  {
  public:
    explicit data_type_error(std::string const& msg) : std::runtime_error(msg) {}
};

// Usage:
// NOTE SUPPORTED SCALARS
// int, float, double, bool, std::string, nullptr
//
// To set a key to a scalar value in object x
//    x.set<int>("foo", 1);
//    x.set<std::string>(foo", "bar");
//
// To set a nested key to a scalar value in object x
//    x.set<bool>("foo", "bar", "baz", true);
//
// NOTE THAT LOCATION IN STUCTURE IS A VARAIDIC ARGUMENT
//
// To set a key to a vector value in object x
//    std::vector<int> tmp { 1, 2, 3 };
//    x.set<std::vector<int>>("foo", tmp);
//
// To get a scalar value from a key in object x
//    x.get<std::string>("foo");
//    x.get<int>("bar");
//
// To get a vector from a key in object x
//    x.get<std::vector<float>>("foo");
//
// To get a result object (json object) from object x
//    x.get<Message>("foo");
//    x.get<Data>("foo");
//
// To get a null value from a key in object x
//    x.get<std::string>("foo") == "";
//    x.get<int>("foo") == 0;
//    x.get<bool>("foo") == false;
//    x.get<float>("foo") == 0.0f;
//    x.get<double>("foo") == 0.0;
//
// To get a json string representation of object x
//    x.toString();
//
// To check if a key is set in object x
//    x.includes("foo");
//    x.includes("foo", "bar", "baz");

class DataContainer {
  public:
    DataContainer();
    explicit DataContainer(const std::string& json_txt);
    explicit DataContainer(const rapidjson::Value& value);
    DataContainer(const DataContainer& data);
    DataContainer(const DataContainer&& data);
    DataContainer& operator=(DataContainer other);
    ~DataContainer();

    std::vector<std::string> keys();

    // TODO(ale): add an empty() method

    rapidjson::Document getRaw() const;

    std::string toString() const;

    bool includes(const std::string& first) const {
        return includes_(*reinterpret_cast<rapidjson::Value*>(document_root_.get()),
                         first.data());
    }

    template <typename ... Args>
    bool includes(const std::string& first, Args... rest) const {
        return includes_(*reinterpret_cast<rapidjson::Value*>(document_root_.get()),
                         first.data(), rest...);
    }

    template <typename T>
    T get() {
        return getValue<T>(document_root_);
    }

    template <typename T>
    T get(const std::string& first) const {
        return get_<T>(*reinterpret_cast<rapidjson::Value*>(document_root_.get()),
                       first.data());
    }

    template <typename T, typename ... Args>
    T get(const std::string& first, Args ... rest) const {
        return get_<T>(*reinterpret_cast<rapidjson::Value*>(document_root_.get()),
                       first.data(), rest...);
    }

    template <typename T>
    void set(const std::string& first, T new_value) {
        set_<T>(*reinterpret_cast<rapidjson::Value*>(document_root_.get()),
                first.data(), new_value);
    }

    template <typename T, typename ... Args>
    void set(const std::string& first, std::string second, Args ... rest) {
        set_<T>(*reinterpret_cast<rapidjson::Value*>(document_root_.get()),
                first.data(), second.data(), rest...);
    }

  private:
    std::unique_ptr<rapidjson::Document> document_root_;

    bool hasKey(const rapidjson::Value& jval, const char* key) const;
    bool isNotObject(const rapidjson::Value& jval, const char* key) const;
    rapidjson::Value* getValueInJson(const rapidjson::Value& jval, const char* key) const;
    void createKeyInJson(const char* key, rapidjson::Value& jval);

    template <typename ... Args>
    bool includes_(const rapidjson::Value& jval, const char * first) const {
        if (hasKey(jval, first)) {
            return true;
        } else {
            return false;
        }
    }

    template <typename ... Args>
    bool includes_(const rapidjson::Value& jval, const char * first, Args ... rest) const {
        if (hasKey(jval, first)) {
            return includes_(*getValueInJson(jval, first), rest...);
        } else {
            return false;
        }
    }

    template <typename T>
    T get_(const rapidjson::Value& jval, const char * first) const {
        if (hasKey(jval, first)) {
            return getValue<T>(*getValueInJson(jval, first));
        }

        return getValue<T>();
    }

    template <typename T, typename ... Args>
    T get_(const rapidjson::Value& jval, const char * first, Args ... rest) const {
        if (hasKey(jval, first)) {
            return get_<T>(*getValueInJson(jval, first), rest...);
        }

        return getValue<T>();
    }

    // Last recursive call from the variadic template ends here
    template<typename T>
    void set_(rapidjson::Value& jval) {
        return;
    }

    template <typename T>
    void set_(rapidjson::Value& jval, const char* first, T new_val) {
        if (!hasKey(jval, first)) {
            createKeyInJson(first, jval);
        }
        setValue<T>(*getValueInJson(jval, first), new_val);
    }

    // deal with the case where parameter pack is a tuple of only strings
    // we need this because varaidic templates are greedy and won't call the
    // correct set_
    template <typename T>
    void set_string_(rapidjson::Value& jval, const char* new_val) {
        setValue<T>(jval, new_val);
    }

    template <typename T, typename ... Args>
    void set_(rapidjson::Value& jval, const char* first, Args ... rest) {
        if (sizeof...(rest) > 0) {
            if (isNotObject(jval, first)) {
                throw index_error { "invalid message index supplied" };
            }

            if (!hasKey(jval, first)) {
                createKeyInJson(first, jval);
            }

            set_<T>(*getValueInJson(jval, first), rest...);
        } else {
            set_string_<std::string>(jval, first);
            return;
        }
    }

    template<typename T>
    T getValue(const rapidjson::Value& value) const;

    template<typename T>
    T getValue() const;

    template<typename T>
    void setValue(rapidjson::Value& jval, T new_value);

};


template<typename T>
T DataContainer::getValue(const rapidjson::Value& Value) const {
    throw data_type_error { "invalid type for DataContainer" };
}

template<typename T>
void DataContainer::setValue(rapidjson::Value& jval, T new_value) {
    throw data_type_error { "invalid type for DataContainer" };
}

template<>
void DataContainer::setValue<>(rapidjson::Value& jval, const std::string& new_value);

template<>
void DataContainer::setValue<>(rapidjson::Value& jval, const char* new_value);

template<>
void DataContainer::setValue<>(rapidjson::Value& jval, bool new_value);

template<>
void DataContainer::setValue<>(rapidjson::Value& jval, int new_value);

template<>
void DataContainer::setValue<>(rapidjson::Value& jval, double new_value);

template<>
void DataContainer::setValue<>(rapidjson::Value& jval, std::vector<std::string> new_value);

template<>
void DataContainer::setValue<>(rapidjson::Value& jval, std::vector<bool> new_value);

template<>
void DataContainer::setValue<>(rapidjson::Value& jval, std::vector<int> new_value);

template<>
void DataContainer::setValue<>(rapidjson::Value& jval, std::vector<double> new_value);

template<>
void DataContainer::setValue<>(rapidjson::Value& jval, std::vector<DataContainer> new_value);

template<>
void DataContainer::setValue<>(rapidjson::Value& jval, DataContainer new_value);


}  // namespace CthunClient

#endif  // CTHUN_CLIENT_SRC_DATA_CONTAINER_DATA_CONTAINER_H_
#ifndef APP_ARG_PARSER_HPP
#define APP_ARG_PARSER_HPP

#include <unordered_map>
#include <unordered_set>
#include <type_traits>
#include <string>
#include <vector>

class ArgsParser {
private:
    std::unordered_map<std::string, std::string> values;
    std::unordered_set<std::string> flags;
    std::vector<std::string> ordered;

public:
    ArgsParser(const std::vector<std::string>& args);

    inline bool HasValue(const std::string& arg) const {
        return values.contains(arg);
    }

    inline bool HasFlag(const std::string& arg) const {
        return flags.contains(arg);
    }

    template <typename T>
    inline T GetValue(const std::string& arg) {
        if (!HasValue(arg))
            return T();
        
        if constexpr (std::is_integral_v<T>) {
            if constexpr (std::is_unsigned_v<T>)
                return std::stoull(values[arg]);
            
            else
                return std::stoll(values[arg]);
        }
        else
            return T(values[arg]);
    }

    template <typename T>
    inline T GetValueOr(const std::string& arg, T default_value) {
        if (!HasValue(arg))
            return default_value;
        
        if constexpr (std::is_integral_v<T>) {
            if constexpr (std::is_unsigned_v<T>)
                return std::stoull(values[arg]);
            
            else
                return std::stoll(values[arg]);
        }
        else
            return T(values[arg]);
    }

    inline const std::vector<std::string> GetOrdered() const {
        return ordered;
    }
};

#endif
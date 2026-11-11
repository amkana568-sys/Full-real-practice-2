#include "depviz.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <regex>
#include <sstream>
#include <stdexcept>

namespace depviz {
namespace {

const int MAX_DEPTH = 100;

std::string trim(const std::string& value) {
    const std::string spaces = " \t\r\n";
    const auto first = value.find_first_not_of(spaces);
    if (first == std::string::npos) {
        return "";
    }
    const auto last = value.find_last_not_of(spaces);
    return value.substr(first, last - first + 1);
}

std::string removeComment(const std::string& line) {
    bool quoted = false;
    bool escaped = false;
    for (std::size_t i = 0; i < line.size(); ++i) {
        const char ch = line[i];
        if (escaped) {
            escaped = false;
            continue;
        }
        if (quoted && ch == '\\') {
            escaped = true;
            continue;
        }
        if (ch == '"') {
            quoted = !quoted;
        } else if (ch == '#' && !quoted) {
            return line.substr(0, i);
        }
    }
    if (quoted) {
        throw std::runtime_error("Незакрытая строка TOML.");
    }
    return line;
}

std::string parseString(const std::string& raw, const std::string& key) {
    const std::string value = trim(raw);
    if (value.size() < 2 || value.front() != '"' || value.back() != '"') {
        throw std::runtime_error(
            "Параметр '" + key + "' должен быть строкой в двойных кавычках."
        );
    }

    std::string result;
    for (std::size_t i = 1; i + 1 < value.size(); ++i) {
        if (value[i] == '\\') {
            ++i;
            if (i + 1 > value.size()) {
                throw std::runtime_error("Некорректная escape-последовательность.");
            }
            if (value[i] == 'n') {
                result.push_back('\n');
            } else if (value[i] == 't') {
                result.push_back('\t');
            } else if (value[i] == '"' || value[i] == '\\') {
                result.push_back(value[i]);
            } else {
                throw std::runtime_error("Недопустимая escape-последовательность.");
            }
        } else {
            result.push_back(value[i]);
        }
    }
    return result;
}

bool parseBool(const std::string& raw, const std::string& key) {
    const std::string value = trim(raw);
    if (value == "true") {
        return true;
    }
    if (value == "false") {
        return false;
    }
    throw std::runtime_error(
        "Параметр '" + key + "' должен иметь значение true или false."
    );
}

int parseDepth(const std::string& raw) {
    const std::string value = trim(raw);
    std::size_t used = 0;
    int depth = 0;
    try {
        depth = std::stoi(value, &used);
    } catch (const std::exception&) {
        throw std::runtime_error(
            "Параметр 'max_depth' должен быть целым числом от 0 до 100."
        );
    }
    if (used != value.size() || depth < 0 || depth > MAX_DEPTH) {
        throw std::runtime_error(
            "Параметр 'max_depth' должен быть целым числом от 0 до 100."
        );
    }
    return depth;
}

std::string required(
    const std::map<std::string, std::string>& values,
    const std::string& key
) {
    const auto found = values.find(key);
    if (found == values.end()) {
        throw std::runtime_error("В config.toml отсутствует параметр '" + key + "'.");
    }
    return found->second;
}

void validateConfig(const Config& config, const std::string& filename) {
    const std::regex packagePattern("[A-Za-z0-9][A-Za-z0-9._+-]*");
    if (!std::regex_match(config.package, packagePattern)) {
        throw std::runtime_error(
            "Параметр 'package' содержит недопустимое имя APK-пакета."
        );
    }
    if (config.repository.empty()) {
        throw std::runtime_error("Параметр 'repository' не должен быть пустым.");
    }
    if (config.maxDepth < 0 || config.maxDepth > MAX_DEPTH) {
        throw std::runtime_error("Параметр 'max_depth' должен быть от 0 до 100.");
    }

    if (config.testMode) {
        std::filesystem::path path(config.repository);
        if (path.is_relative()) {
            path = std::filesystem::path(filename).parent_path() / path;
        }
        if (!std::filesystem::is_regular_file(path)) {
            throw std::runtime_error(
                "Параметр 'repository': тестовый файл не найден: " + path.string()
            );
        }
        return;
    }

    const bool http = config.repository.rfind("http://", 0) == 0;
    const bool https = config.repository.rfind("https://", 0) == 0;
    if (!http && !https) {
        throw std::runtime_error(
            "Параметр 'repository' должен содержать URL http:// или https://."
        );
    }
    if (config.repository.find("APKINDEX.tar.gz") == std::string::npos) {
        throw std::runtime_error(
            "Укажите прямой URL к файлу APKINDEX.tar.gz."
        );
    }
}

}  // namespace

Config loadConfig(const std::string& filename) {
    std::ifstream file(filename);
    if (!file) {
        throw std::runtime_error("Не удалось открыть файл конфигурации: " + filename);
    }

    const std::map<std::string, bool> allowed = {
        {"package", true},
        {"repository", true},
        {"test_mode", true},
        {"ascii_tree", true},
        {"max_depth", true}
    };
    std::map<std::string, std::string> values;
    std::string line;
    int lineNumber = 0;

    while (std::getline(file, line)) {
        ++lineNumber;
        line = trim(removeComment(line));
        if (line.empty()) {
            continue;
        }
        const auto equal = line.find('=');
        if (equal == std::string::npos) {
            throw std::runtime_error(
                "Ошибка TOML в строке " + std::to_string(lineNumber) + ": нет '='."
            );
        }
        const std::string key = trim(line.substr(0, equal));
        const std::string value = trim(line.substr(equal + 1));
        if (allowed.find(key) == allowed.end()) {
            throw std::runtime_error("Неизвестный параметр TOML: '" + key + "'.");
        }
        if (value.empty()) {
            throw std::runtime_error("Параметр '" + key + "' не должен быть пустым.");
        }
        if (values.find(key) != values.end()) {
            throw std::runtime_error("Параметр '" + key + "' указан дважды.");
        }
        values[key] = value;
    }

    Config config;
    config.package = parseString(required(values, "package"), "package");
    config.repository = parseString(required(values, "repository"), "repository");
    config.testMode = parseBool(required(values, "test_mode"), "test_mode");
    config.asciiTree = parseBool(required(values, "ascii_tree"), "ascii_tree");
    config.maxDepth = parseDepth(required(values, "max_depth"));
    validateConfig(config, filename);
    return config;
}

void printConfig(const Config& config) {
    std::cout << "Параметры конфигурации:\n";
    std::cout << "package = " << config.package << '\n';
    std::cout << "repository = " << config.repository << '\n';
    std::cout << "test_mode = " << (config.testMode ? "true" : "false") << '\n';
    std::cout << "ascii_tree = " << (config.asciiTree ? "true" : "false") << '\n';
    std::cout << "max_depth = " << config.maxDepth << "\n\n";
}

}  // namespace depviz

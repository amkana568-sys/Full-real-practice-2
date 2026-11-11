#include "depviz.hpp"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <sys/wait.h>

namespace depviz {
namespace {

std::string trim(const std::string& value) {
    const std::string spaces = " \t\r\n";
    const auto first = value.find_first_not_of(spaces);
    if (first == std::string::npos) {
        return "";
    }
    const auto last = value.find_last_not_of(spaces);
    return value.substr(first, last - first + 1);
}

bool isUpperPackage(const std::string& name) {
    if (name.empty()) {
        return false;
    }
    for (const char ch : name) {
        if (ch < 'A' || ch > 'Z') {
            return false;
        }
    }
    return true;
}

std::vector<std::string> splitWords(const std::string& value) {
    std::istringstream input(value);
    std::vector<std::string> words;
    std::string word;
    while (input >> word) {
        words.push_back(word);
    }
    return words;
}

std::string packageName(const std::string& dependency) {
    const auto position = dependency.find_first_of("=<>~");
    if (position == std::string::npos) {
        return dependency;
    }
    return dependency.substr(0, position);
}

std::string shellQuote(const std::string& value) {
    std::string result = "'";
    for (const char ch : value) {
        if (ch == '\'') {
            result += "'\\''";
        } else {
            result.push_back(ch);
        }
    }
    result.push_back('\'');
    return result;
}

struct ApkRecord {
    std::string name;
    int priority = 0;
    std::vector<std::string> dependencies;
    std::vector<std::string> provides;
};

void addRecord(
    const ApkRecord& record,
    std::map<std::string, ApkRecord>& records
) {
    if (record.name.empty()) {
        return;
    }
    if (records.find(record.name) != records.end()) {
        throw std::runtime_error(
            "APKINDEX содержит повторную запись пакета: " + record.name
        );
    }
    records[record.name] = record;
}

std::map<std::string, ApkRecord> parseApkIndex(std::istream& input) {
    std::map<std::string, ApkRecord> records;
    ApkRecord current;
    std::string line;
    while (std::getline(input, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line.empty()) {
            addRecord(current, records);
            current = ApkRecord{};
            continue;
        }
        if (line.size() < 2 || line[1] != ':') {
            continue;
        }
        const std::string value = trim(line.substr(2));
        if (line[0] == 'P') {
            current.name = value;
        } else if (line[0] == 'D') {
            current.dependencies = splitWords(value);
        } else if (line[0] == 'p') {
            current.provides = splitWords(value);
        } else if (line[0] == 'k') {
            try {
                current.priority = std::stoi(value);
            } catch (const std::exception&) {
                current.priority = 0;
            }
        }
    }
    addRecord(current, records);
    if (records.empty()) {
        throw std::runtime_error("В APKINDEX не найдены записи пакетов.");
    }
    return records;
}

Repository makeRepository(const std::map<std::string, ApkRecord>& records) {
    std::map<std::string, std::pair<int, std::string>> providers;
    for (const auto& item : records) {
        for (const auto& provided : item.second.provides) {
            const std::string alias = packageName(provided);
            const auto found = providers.find(alias);
            if (found == providers.end() || item.second.priority > found->second.first) {
                providers[alias] = {item.second.priority, item.first};
            }
        }
    }

    Repository repository;
    for (const auto& item : records) {
        std::vector<std::string> dependencies;
        for (const auto& raw : item.second.dependencies) {
            const std::string name = packageName(raw);
            if (records.find(name) != records.end()) {
                dependencies.push_back(name);
            } else if (providers.find(name) != providers.end()) {
                dependencies.push_back(providers.at(name).second);
            } else {
                dependencies.push_back(name);
            }
        }
        repository[item.first] = dependencies;
    }
    return repository;
}

}  // namespace

Repository loadTestRepository(const std::string& filename) {
    std::ifstream file(filename);
    if (!file) {
        throw std::runtime_error("Не удалось открыть тестовый репозиторий: " + filename);
    }

    Repository repository;
    std::string line;
    int lineNumber = 0;
    while (std::getline(file, line)) {
        ++lineNumber;
        line = trim(line);
        if (line.empty() || line[0] == '#') {
            continue;
        }
        const auto colon = line.find(':');
        if (colon == std::string::npos) {
            throw std::runtime_error(
                "Ошибка тестового репозитория в строке " +
                std::to_string(lineNumber) + ": ожидается 'ПАКЕТ: ЗАВИСИМОСТИ'."
            );
        }
        const std::string package = trim(line.substr(0, colon));
        if (!isUpperPackage(package)) {
            throw std::runtime_error(
                "Имя пакета в строке " + std::to_string(lineNumber) +
                " должно состоять из больших латинских букв."
            );
        }
        if (repository.find(package) != repository.end()) {
            throw std::runtime_error("Пакет указан дважды в тестовом файле: " + package);
        }
        const auto dependencies = splitWords(line.substr(colon + 1));
        for (const auto& dependency : dependencies) {
            if (!isUpperPackage(dependency)) {
                throw std::runtime_error(
                    "Некорректная зависимость '" + dependency +
                    "' в строке " + std::to_string(lineNumber) + "."
                );
            }
        }
        repository[package] = dependencies;
    }

    if (repository.empty()) {
        throw std::runtime_error("Тестовый репозиторий не содержит пакетов.");
    }
    for (const auto& item : repository) {
        for (const auto& dependency : item.second) {
            if (repository.find(dependency) == repository.end()) {
                throw std::runtime_error(
                    "Пакет '" + item.first + "' ссылается на отсутствующий пакет '" +
                    dependency + "'."
                );
            }
        }
    }
    return repository;
}

Repository loadApkRepository(const std::string& url) {
    const std::string command =
        "curl --fail --location --silent --show-error -- " + shellQuote(url) +
        " | tar -xzO APKINDEX";
    FILE* pipe = popen(command.c_str(), "r");
    if (pipe == nullptr) {
        throw std::runtime_error("Не удалось запустить curl/tar для чтения APKINDEX.");
    }

    std::string content;
    char buffer[4096];
    while (std::fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        content += buffer;
    }
    const int status = pclose(pipe);
    if (status == -1 || !WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        throw std::runtime_error(
            "Не удалось загрузить APKINDEX. Проверьте URL, сеть и доступность curl/tar."
        );
    }

    std::istringstream input(content);
    const auto records = parseApkIndex(input);
    return makeRepository(records);
}

}  // namespace depviz

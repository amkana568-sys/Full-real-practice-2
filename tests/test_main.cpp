#include "depviz.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

void check(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error("Тест не пройден: " + message);
    }
}

void writeFile(const std::filesystem::path& path, const std::string& text) {
    std::ofstream file(path);
    file << text;
    if (!file) {
        throw std::runtime_error("Не удалось записать тестовый файл.");
    }
}

std::string validConfig(const std::string& repository = "repo.txt") {
    return "package = \"A\"\nrepository = \"" + repository +
        "\"\ntest_mode = true\nascii_tree = false\nmax_depth = 3\n";
}

void expectConfigError(
    const std::filesystem::path& configPath,
    const std::string& content,
    const std::string& name
) {
    writeFile(configPath, content);
    bool failed = false;
    try {
        (void)depviz::loadConfig(configPath.string());
    } catch (const std::exception&) {
        failed = true;
    }
    check(failed, name);
}

void testConfig(const std::filesystem::path& folder) {
    const auto repo = folder / "repo.txt";
    const auto config = folder / "config.toml";
    writeFile(repo, "A: B\nB:\n");
    writeFile(config, validConfig());
    const auto loaded = depviz::loadConfig(config.string());
    check(loaded.package == "A", "чтение package");
    check(loaded.repository == "repo.txt", "чтение repository");
    check(loaded.testMode && !loaded.asciiTree, "чтение логических значений");
    check(loaded.maxDepth == 3, "чтение max_depth");

    const std::string realConfig =
        "package = \"A\"\n"
        "repository = \"https://example.test/APKINDEX.tar.gz\"\n"
        "test_mode = false\nascii_tree = true\nmax_depth = 0\n";
    writeFile(config, realConfig);
    const auto real = depviz::loadConfig(config.string());
    check(!real.testMode && real.asciiTree && real.maxDepth == 0,
        "конфигурация реального репозитория");

    expectConfigError(config, validConfig("absent.txt"), "ошибка пути repository");
    expectConfigError(config, validConfig() + "unknown = 1\n", "лишний ключ");
    expectConfigError(config, validConfig() + "package = \"B\"\n", "дубликат ключа");
}

void testConfigParameterErrors(const std::filesystem::path& folder) {
    const auto config = folder / "invalid.toml";
    auto text = validConfig();
    const auto package = text.find("\"A\"");
    text.replace(package, 3, "\"../bad\"");
    expectConfigError(config, text, "ошибка package");
    expectConfigError(config, validConfig().replace(
        validConfig().find("repository = \"repo.txt\""), 23,
        "repository = \"\""
    ), "пустой repository");
    expectConfigError(config, validConfig("file:///tmp/APKINDEX.tar.gz")
        .replace(validConfig().find("test_mode = true"), 16, "test_mode = false"),
        "ошибка URL repository");
    expectConfigError(config, validConfig().replace(
        validConfig().find("test_mode = true"), 16, "test_mode = maybe"),
        "ошибка test_mode");
    expectConfigError(config, validConfig().replace(
        validConfig().find("ascii_tree = false"), 18, "ascii_tree = yes"),
        "ошибка ascii_tree");
    expectConfigError(config, validConfig().replace(
        validConfig().find("max_depth = 3"), 13, "max_depth = -1"),
        "отрицательная глубина");
    expectConfigError(config, validConfig().replace(
        validConfig().find("max_depth = 3"), 13, "max_depth = 101"),
        "слишком большая глубина");
    expectConfigError(config, validConfig().replace(
        validConfig().find("max_depth = 3"), 13, "max_depth = two"),
        "нечисловая глубина");
}

void testRepository(const std::filesystem::path& folder) {
    const auto path = folder / "graph.txt";
    writeFile(path, "A: B C\nB: D\nC:\nD:\n");
    const auto repository = depviz::loadTestRepository(path.string());
    check(repository.size() == 4, "чтение тестового репозитория");
    check(repository.at("A").size() == 2, "прямые зависимости тестового пакета");

    writeFile(path, "A: b\n");
    bool failed = false;
    try {
        (void)depviz::loadTestRepository(path.string());
    } catch (const std::exception&) {
        failed = true;
    }
    check(failed, "проверка больших латинских букв");
}

void testApkIndexParser() {
    const std::string index =
        "P:alpha\nV:1.0-r0\nD:beta>=1.0 so:libsample.so.1\n\n"
        "P:beta\nV:2.0-r0\n\n"
        "P:other-provider\nV:1.0-r0\np:so:libsample.so.1\nk:2\n\n"
        "P:sample-provider\nV:1.0-r0\np:so:libsample.so.1\nk:10\n";
    const auto repository = depviz::parseApkIndexText(index);
    check(repository.find("alpha") != repository.end(), "чтение поля P");
    check(repository.at("alpha").size() == 2, "чтение поля D");
    check(repository.at("alpha")[0] == "beta", "удаление ограничения версии");
    check(repository.at("alpha")[1] == "sample-provider",
        "разрешение виртуального provides");
}

void testDfsDepthAndSharedNodes() {
    depviz::Repository repository = {
        {"A", {"B", "C"}}, {"B", {"D"}}, {"C", {"E"}},
        {"D", {"F"}}, {"E", {"F"}}, {"F", {}}
    };
    const auto graph = depviz::buildGraphDfs("A", repository, 2);
    check(graph.size() == 5, "DFS на глубину 2");
    check(graph.at("D").empty(), "обрезка ребер на максимальной глубине");
    check(graph.at("E").empty(), "обрезка второй ветви на глубине");
    const auto rootOnly = depviz::buildGraphDfs("A", repository, 0);
    check(rootOnly.size() == 1 && rootOnly.at("A").empty(), "нулевая глубина");
}

void testDfsShortestPath() {
    depviz::Repository repository = {
        {"A", {"B", "C"}}, {"B", {"C"}}, {"C", {"D"}},
        {"D", {"E"}}, {"E", {}}
    };
    const auto graph = depviz::buildGraphDfs("A", repository, 3);
    check(graph.find("E") != graph.end(), "повторное раскрытие по короткому пути");
    check(graph.at("D").size() == 1 && graph.at("D")[0] == "E",
        "корректная глубина после короткого пути");
}

void testDfsCycleAndReverse() {
    depviz::Repository repository = {
        {"P", {"Q"}}, {"Q", {"R"}}, {"R", {"P"}},
        {"U", {"P"}}, {"V", {"U"}}
    };
    const auto cycle = depviz::buildGraphDfs("P", repository, 5);
    check(cycle.size() == 3, "цикл не вызывает бесконечный обход");
    check(cycle.at("R").size() == 1 && cycle.at("R")[0] == "P",
        "сохранение замыкающего ребра цикла");
    const auto reverse = depviz::buildReverseGraph("P", repository, 2);
    check(reverse.find("U") != reverse.end(), "прямая обратная зависимость");
    check(reverse.find("V") != reverse.end(), "транзитивная обратная зависимость");
}

}  // namespace

int main() {
    try {
        const auto folder = std::filesystem::temp_directory_path() / "depviz_tests";
        std::filesystem::remove_all(folder);
        std::filesystem::create_directories(folder);
        testConfig(folder);
        testConfigParameterErrors(folder);
        testRepository(folder);
        testApkIndexParser();
        testDfsDepthAndSharedNodes();
        testDfsShortestPath();
        testDfsCycleAndReverse();
        std::filesystem::remove_all(folder);
        std::cout << "Все автоматические тесты пройдены успешно.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

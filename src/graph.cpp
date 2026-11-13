#include "depviz.hpp"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <queue>
#include <set>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace depviz {
namespace {

struct TreeItem {
    std::string name;
    int depth;
    std::vector<std::string> path;
};

std::string xmlEscape(const std::string& value) {
    std::string result;
    for (const char ch : value) {
        if (ch == '&') {
            result += "&amp;";
        } else if (ch == '<') {
            result += "&lt;";
        } else if (ch == '>') {
            result += "&gt;";
        } else if (ch == '"') {
            result += "&quot;";
        } else {
            result.push_back(ch);
        }
    }
    return result;
}

std::map<int, std::vector<std::string>> makeLayers(
    const Graph& graph,
    const std::string& root
) {
    std::map<std::string, int> depth;
    std::queue<std::string> queue;
    depth[root] = 0;
    queue.push(root);

    while (!queue.empty()) {
        const std::string node = queue.front();
        queue.pop();
        const auto found = graph.find(node);
        if (found == graph.end()) {
            continue;
        }
        for (const auto& dependency : found->second) {
            if (depth.find(dependency) == depth.end()) {
                depth[dependency] = depth[node] + 1;
                queue.push(dependency);
            }
        }
    }

    std::map<int, std::vector<std::string>> layers;
    for (const auto& item : depth) {
        layers[item.second].push_back(item.first);
    }
    return layers;
}

}  // namespace

Graph buildGraphDfs(
    const std::string& start,
    const Repository& repository,
    int maxDepth
) {
    if (repository.find(start) == repository.end()) {
        throw std::runtime_error("Пакет не найден в репозитории: " + start);
    }
    if (maxDepth < 0) {
        throw std::runtime_error("Глубина анализа не может быть отрицательной.");
    }

    Graph graph;
    std::map<std::string, int> bestDepth;
    std::vector<std::pair<std::string, int>> stack = {{start, 0}};

    while (!stack.empty()) {
        const auto current = stack.back();
        stack.pop_back();
        const std::string& node = current.first;
        const int depth = current.second;
        if (depth > maxDepth) {
            continue;
        }
        const auto visited = bestDepth.find(node);
        if (visited != bestDepth.end() && visited->second <= depth) {
            continue;
        }
        bestDepth[node] = depth;

        std::vector<std::string> dependencies;
        if (depth < maxDepth) {
            const auto found = repository.find(node);
            if (found != repository.end()) {
                dependencies = found->second;
            }
        }
        graph[node] = dependencies;

        for (auto it = dependencies.rbegin(); it != dependencies.rend(); ++it) {
            stack.push_back({*it, depth + 1});
        }
    }
    return graph;
}

Graph buildReverseGraph(
    const std::string& start,
    const Repository& repository,
    int maxDepth
) {
    Repository reverse;
    for (const auto& item : repository) {
        reverse[item.first];
        for (const auto& dependency : item.second) {
            reverse[dependency].push_back(item.first);
        }
    }
    return buildGraphDfs(start, reverse, maxDepth);
}

void printDirectDependencies(
    const std::string& package,
    const Repository& repository
) {
    const auto found = repository.find(package);
    if (found == repository.end()) {
        throw std::runtime_error("Пакет не найден: " + package);
    }
    std::cout << "Прямые зависимости пакета " << package << ":\n";
    if (found->second.empty()) {
        std::cout << "  нет\n\n";
        return;
    }
    for (const auto& dependency : found->second) {
        std::cout << "  " << dependency << '\n';
    }
    std::cout << '\n';
}

void printGraph(const Graph& graph, const std::string& title) {
    std::cout << title << ":\n";
    for (const auto& item : graph) {
        std::cout << "  " << item.first << " -> ";
        if (item.second.empty()) {
            std::cout << "[]";
        } else {
            std::cout << "[";
            for (std::size_t i = 0; i < item.second.size(); ++i) {
                if (i != 0) {
                    std::cout << ", ";
                }
                std::cout << item.second[i];
            }
            std::cout << "]";
        }
        std::cout << '\n';
    }
    std::cout << '\n';
}

void printReverseDependencies(const Graph& graph, const std::string& root) {
    std::cout << "Обратные зависимости пакета " << root << ":\n";
    bool foundAny = false;
    for (const auto& item : graph) {
        if (item.first == root) {
            continue;
        }
        std::cout << "  " << item.first << '\n';
        foundAny = true;
    }
    if (!foundAny) {
        std::cout << "  нет\n";
    }
    std::cout << '\n';
}

void printAsciiTree(const Graph& graph, const std::string& root, int maxDepth) {
    std::cout << "ASCII-дерево зависимостей:\n";
    std::vector<TreeItem> stack = {{root, 0, {}}};
    std::set<std::string> expanded;

    while (!stack.empty()) {
        TreeItem item = std::move(stack.back());
        stack.pop_back();
        std::cout << std::string(static_cast<std::size_t>(item.depth * 2), ' ')
                  << "- " << item.name;

        const bool cycle = std::find(
            item.path.begin(), item.path.end(), item.name
        ) != item.path.end();
        if (cycle) {
            std::cout << " [цикл]\n";
            continue;
        }
        if (expanded.find(item.name) != expanded.end()) {
            std::cout << " [уже показан]\n";
            continue;
        }
        std::cout << '\n';
        expanded.insert(item.name);
        if (item.depth >= maxDepth) {
            continue;
        }

        item.path.push_back(item.name);
        const auto found = graph.find(item.name);
        if (found == graph.end()) {
            continue;
        }
        for (auto it = found->second.rbegin(); it != found->second.rend(); ++it) {
            stack.push_back({*it, item.depth + 1, item.path});
        }
    }
    std::cout << '\n';
}

void writePlantUml(const Graph& graph, const std::string& filename) {
    std::ofstream output(filename);
    if (!output) {
        throw std::runtime_error("Не удалось создать файл: " + filename);
    }

    std::map<std::string, std::string> aliases;
    int number = 0;
    for (const auto& item : graph) {
        aliases[item.first] = "N" + std::to_string(number++);
    }

    output << "@startuml\n";
    output << "left to right direction\n";
    for (const auto& item : aliases) {
        output << "component \"" << item.first << "\" as "
               << item.second << '\n';
    }
    for (const auto& item : graph) {
        for (const auto& dependency : item.second) {
            output << aliases.at(item.first) << " --> "
                   << aliases.at(dependency) << '\n';
        }
    }
    output << "@enduml\n";
}

void writeSvg(
    const Graph& graph,
    const std::string& root,
    const std::string& filename
) {
    const auto layers = makeLayers(graph, root);
    const int nodeWidth = 160;
    const int nodeHeight = 56;
    const int xGap = 220;
    const int yGap = 100;
    std::map<std::string, std::pair<int, int>> positions;
    int maxRows = 1;
    int layerIndex = 0;

    for (const auto& layer : layers) {
        maxRows = std::max(maxRows, static_cast<int>(layer.second.size()));
        for (std::size_t row = 0; row < layer.second.size(); ++row) {
            positions[layer.second[row]] = {
                50 + layerIndex * xGap,
                45 + static_cast<int>(row) * yGap
            };
        }
        ++layerIndex;
    }

    const int width = 100 + std::max(1, layerIndex) * xGap;
    bool hasBackEdge = false;
    for (const auto& item : graph) {
        for (const auto& dependency : item.second) {
            const auto from = positions.at(item.first);
            const auto to = positions.at(dependency);
            if (to.first < from.first ||
                (to.first == from.first && to.second <= from.second)) {
                hasBackEdge = true;
            }
        }
    }
    const int height = 90 + maxRows * yGap + (hasBackEdge ? 80 : 0);
    std::ofstream output(filename);
    if (!output) {
        throw std::runtime_error("Не удалось создать SVG: " + filename);
    }

    output << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\""
           << width << "\" height=\"" << height << "\" viewBox=\"0 0 "
           << width << ' ' << height << "\">\n";
    output << "<defs><marker id=\"arrow\" markerWidth=\"10\" "
           << "markerHeight=\"10\" refX=\"8\" refY=\"3\" "
           << "orient=\"auto\"><path d=\"M0,0 L0,6 L9,3 z\" "
           << "fill=\"#34495e\"/></marker></defs>\n";
    output << "<rect width=\"100%\" height=\"100%\" fill=\"white\"/>\n";

    for (const auto& item : graph) {
        const auto from = positions.find(item.first);
        if (from == positions.end()) {
            continue;
        }
        for (const auto& dependency : item.second) {
            const auto to = positions.find(dependency);
            if (to == positions.end()) {
                continue;
            }
            const int x1 = from->second.first + nodeWidth;
            const int y1 = from->second.second + nodeHeight / 2;
            const int x2 = to->second.first;
            const int y2 = to->second.second + nodeHeight / 2;
            const bool backEdge = to->second.first < from->second.first ||
                (to->second.first == from->second.first &&
                 to->second.second <= from->second.second);
            if (backEdge) {
                const int startX = from->second.first + nodeWidth / 2;
                const int endX = to->second.first + nodeWidth / 2;
                const int startY = from->second.second + nodeHeight;
                const int endY = to->second.second + nodeHeight;
                const int curveY = height - 20;
                output << "<path d=\"M " << startX << ' ' << startY
                       << " C " << startX << ' ' << curveY << ' '
                       << endX << ' ' << curveY << ' ' << endX << ' ' << endY
                       << "\" stroke=\"#34495e\" fill=\"none\" "
                       << "stroke-width=\"1.6\" marker-end=\"url(#arrow)\"/>\n";
            } else if (to->second.first == from->second.first) {
                const int startX = from->second.first + nodeWidth / 2;
                const int startY = from->second.second + nodeHeight;
                const int endY = to->second.second;
                output << "<path d=\"M " << startX << ' ' << startY
                       << " L " << startX << ' ' << endY
                       << "\" stroke=\"#34495e\" fill=\"none\" "
                       << "stroke-width=\"1.6\" marker-end=\"url(#arrow)\"/>\n";
            } else {
                output << "<path d=\"M " << x1 << ' ' << y1 << " L " << x2
                       << ' ' << y2 << "\" stroke=\"#34495e\" fill=\"none\" "
                       << "stroke-width=\"1.6\" marker-end=\"url(#arrow)\"/>\n";
            }
        }
    }

    for (const auto& item : positions) {
        const int x = item.second.first;
        const int y = item.second.second;
        output << "<rect x=\"" << x << "\" y=\"" << y
               << "\" width=\"" << nodeWidth << "\" height=\""
               << nodeHeight << "\" rx=\"8\" fill=\"none\" "
               << "stroke=\"#34495e\" stroke-width=\"1.5\"/>\n";
        output << "<text x=\"" << x + nodeWidth / 2 << "\" y=\""
               << y + 34 << "\" text-anchor=\"middle\" "
               << "font-family=\"Arial,sans-serif\" font-size=\"16\" "
               << "fill=\"#17202a\">" << xmlEscape(item.first)
               << "</text>\n";
    }
    output << "</svg>\n";
}

void openSvg(const std::string& filename) {
#if defined(_WIN32)
    const std::string command = "start \"\" \"" + filename + "\"";
    std::system(command.c_str());
#elif defined(__linux__)
    if (std::getenv("DISPLAY") == nullptr && std::getenv("WAYLAND_DISPLAY") == nullptr) {
        return;
    }
    const std::string command = "xdg-open '" + filename + "' >/dev/null 2>&1 &";
    std::system(command.c_str());
#elif defined(__APPLE__)
    const std::string command = "open '" + filename + "' >/dev/null 2>&1 &";
    std::system(command.c_str());
#else
    (void)filename;
#endif
}

}  // namespace depviz

#pragma once

#include <map>
#include <string>
#include <vector>

namespace depviz {

struct Config {
    std::string package;
    std::string repository;
    bool testMode = false;
    bool asciiTree = false;
    int maxDepth = 0;
};

using Repository = std::map<std::string, std::vector<std::string>>;
using Graph = Repository;

Config loadConfig(const std::string& filename);
void printConfig(const Config& config);

Repository loadTestRepository(const std::string& filename);
Repository loadApkRepository(const std::string& url);

Graph buildGraphDfs(
    const std::string& start,
    const Repository& repository,
    int maxDepth
);

Graph buildReverseGraph(
    const std::string& start,
    const Repository& repository,
    int maxDepth
);

void printDirectDependencies(
    const std::string& package,
    const Repository& repository
);

void printGraph(const Graph& graph, const std::string& title);
void printReverseDependencies(const Graph& graph, const std::string& root);
void printAsciiTree(const Graph& graph, const std::string& root, int maxDepth);

void writePlantUml(const Graph& graph, const std::string& filename);
void writeSvg(
    const Graph& graph,
    const std::string& root,
    const std::string& filename
);
void openSvg(const std::string& filename);

}  // namespace depviz

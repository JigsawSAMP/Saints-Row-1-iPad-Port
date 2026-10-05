#pragma once
#include <filesystem>
namespace rex { class Runtime; }
void SRInitializeCoop(rex::Runtime* runtime, const std::filesystem::path& documents);

#pragma once

#include <filesystem>

enum class PathType
{
    File,
    Dir,
    Both,
    MatchFile,
};

struct IgnoreAttribute
{
    std::u8string base;
    std::u8string path;
    // 是否递归
    bool isRecursive;
    PathType type;
};

class CollectFiles
{
public:

    CollectFiles(const std::u8string& path, const std::vector<std::u8string>& ignoreLines = {});

    ~CollectFiles();

    const std::vector<std::filesystem::path>& Files();

    std::filesystem::path GetRelativePath(const std::filesystem::path& path);

    std::filesystem::path GetRootPath();

private:

    void WalkFiles(const std::filesystem::path& path);

    bool Ignore(const std::filesystem::path& path);

    void ParseGitIgnore(const std::filesystem::path& path);

    void ParseIgnoreLine(const std::u8string& str, const std::u8string& parent_path);

    std::u8string FmtPath(const std::u8string& path);

protected:
    std::u8string m_rootPath;
    std::vector<IgnoreAttribute> m_ignores;
    std::vector<std::filesystem::path> m_files;
    std::u8string m_preferredSeparator;
};


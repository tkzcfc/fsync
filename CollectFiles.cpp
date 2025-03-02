﻿#include "CollectFiles.h"
#include <fstream>
#include <iostream>

CollectFiles::CollectFiles(const std::u8string& path, const std::vector<std::u8string>& ignoreLines)
{
    m_preferredSeparator = std::filesystem::path::preferred_separator;

    m_rootPath = FmtPath(path);

    while (!m_rootPath.empty() && (m_rootPath.back() == '/' || m_rootPath.back() == '\\'))
    {
        m_rootPath.pop_back();
    }

    IgnoreAttribute attri;
    attri.base = m_rootPath;
    attri.path = u8".git";
    attri.isRecursive = true;
    attri.type = PathType::Dir;
    m_ignores.push_back(attri);

    for (auto& line : ignoreLines)
    {
        ParseIgnoreLine(line, m_rootPath);
    }

    WalkFiles(m_rootPath);
}

CollectFiles::~CollectFiles()
{}

const std::vector<std::filesystem::path>& CollectFiles::Files()
{
    return m_files;
}

std::filesystem::path CollectFiles::GetRelativePath(const std::filesystem::path& path)
{
    if (path.u8string().size() >= m_rootPath.size() && path.u8string().substr(0, m_rootPath.size()) == m_rootPath)
    {
        return path.u8string().substr(m_rootPath.size() + 1);
    }
    return "";
}

std::filesystem::path CollectFiles::GetRootPath()
{
    return m_rootPath;
}

void CollectFiles::WalkFiles(const std::filesystem::path& path)
{
    std::vector<std::filesystem::path> dirs;
    std::vector<std::filesystem::path> files;
    for (const auto& entry : std::filesystem::directory_iterator(path)) 
    {
        if (std::filesystem::is_directory(entry.path())) 
        {
            dirs.push_back(entry.path());
        }
        else
        {
            files.push_back(entry.path());
        }
    }

    for (auto& file : files)
    {
        if (file.filename().u8string() == u8".gitignore" || file.extension().u8string() == u8".gitignore")
        {
            ParseGitIgnore(file);
        }
    }

    for (auto& file : files)
    {
        if (!Ignore(file))
        {
            m_files.push_back(file);
        }
    }

    for (auto& dir : dirs) 
    {
        if (!Ignore(dir))
        {
            WalkFiles(dir);
        }
    }
}

bool CollectFiles::Ignore(const std::filesystem::path& path)
{
    bool isDirectory = std::filesystem::is_directory(path);
    auto pathStr = path.u8string();
    auto fileName = path.filename().u8string();

    for (auto& ignore : m_ignores)
    {
        // 有效路径
        if (!pathStr.starts_with(ignore.base))
        {
            continue;
        }

        // 不递归则判断是否在有效路径内 d:\new
        if (!ignore.isRecursive && path.parent_path().u8string() != ignore.base)
        {
            continue;
        }

        // 文件后缀通配
        if (ignore.type == PathType::MatchFile)
        {
            if (path.filename().u8string() == ignore.path)
                return true;
            if (path.extension().u8string() == ignore.path)
                return true;
        }

        if (isDirectory)
        {
            switch (ignore.type)
            {
            case PathType::Dir:
            case PathType::Both:
            {
                if (path.filename().u8string() == ignore.path)
                {
                    return true;
                }
            }break;
            default:
                break;
            }
        }
        else
        {
            switch (ignore.type)
            {
            case PathType::File:
            case PathType::Both:
            {
                if (path.filename().u8string() == ignore.path)
                {
                    return true;
                }
            }break;
            default:
                break;
            }
        }
    }
    return false;
}

inline std::u8string trim(const std::u8string& str, const std::u8string& whitespace = u8" \t\n\r") 
{
    size_t start = str.find_first_not_of(whitespace);
    if (start == std::u8string::npos)
        return u8""; // no content except whitespace
    size_t end = str.find_last_not_of(whitespace);
    return str.substr(start, end - start + 1);
}

void CollectFiles::ParseGitIgnore(const std::filesystem::path& path)
{
    std::ifstream file(path);
    if (!file.is_open()) 
    {
        std::cerr << "Unable to open file\n";
        return;
    }

    std::u8string parent_path = path.parent_path().u8string();
    std::string line;
    while (std::getline(file, line)) 
    {
        std::u8string u8line = std::u8string(reinterpret_cast<const char8_t*>(line.c_str()));
        ParseIgnoreLine(u8line, parent_path);
    }

    file.close();
}

void CollectFiles::ParseIgnoreLine(const std::u8string& str, const std::u8string& parent_path)
{
    auto line = trim(str);
    if (line.empty() || line[0] == '#')
        return;

    for (size_t i = 0; i < line.size(); ++i)
    {
        if (line[i] == '\\')
            line[i] = '/';
    }

    IgnoreAttribute attri;
    attri.base = parent_path;
    attri.isRecursive = line[0] != '/';

    if (line[0] == '/')
    {
        line = line.substr(1);
    }

    line = trim(line);
    if (line.empty())
        return;

    if (line[line.size() - 1] == '/')
    {
        attri.type = PathType::Dir;
        line = line.substr(0, line.find_last_of('/'));
    }
    else
    {
        if (line[0] == '*')
        {
            line = line.substr(1);
            attri.type = PathType::MatchFile;
        }
        else
        {
            attri.type = PathType::Both;
        }
    }

    if (line.empty())
        return;

    auto separatorPos = line.find_last_of('/');
    if (separatorPos != std::u8string::npos && separatorPos > 0)
    {
        attri.base = attri.base + m_preferredSeparator + line.substr(0, separatorPos);
        line = line.substr(separatorPos + 1);
    }

    attri.base = FmtPath(attri.base);
    attri.path = line;
    m_ignores.push_back(attri);
}

std::u8string CollectFiles::FmtPath(const std::u8string& path)
{
    std::u8string fmt = path;
    if(m_preferredSeparator == u8"\\")
    {
        for (size_t i = 0; i < fmt.size(); ++i)
        {
            if (fmt[i] == '/')
            {
                fmt[i] = '\\';
            }
        }
    }
    else
    {
        for (size_t i = 0; i < fmt.size(); ++i)
        {
            if (fmt[i] == '\\')
            {
                fmt[i] = '/';
            }
        }
    }
    return fmt;
}

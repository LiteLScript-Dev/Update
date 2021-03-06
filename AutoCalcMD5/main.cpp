#include <iostream>
#include <filesystem>
#include <string>
#include <fstream>
#include <vector>
#include <cstdlib>
#include <optional>
#include "openssl/md5.h"
using namespace std;

#define IGNORE_FILES "IgnoreDirs.conf"

vector<string> ignores;

std::optional<std::string> ReadAllFile(const std::string& filePath, bool isBinary)
{
    std::ifstream fRead;

    std::ios_base::openmode mode = ios_base::in;
    if (isBinary)
        mode |= ios_base::binary;

    fRead.open(filePath, mode);
    if (!fRead.is_open())
    {
        return std::nullopt;
    }
    std::string data((std::istreambuf_iterator<char>(fRead)),
        std::istreambuf_iterator<char>());
    fRead.close();
    return data;
}

string CalcMD5(const string& path)
{
    auto content = ReadAllFile(path, true);

    unsigned char md5[MD5_DIGEST_LENGTH];
    const char map[] = "0123456789abcdef";
    string hexmd5;

    MD5((const unsigned char*)content->c_str(), content->length(), md5);
    for (size_t i = 0; i < MD5_DIGEST_LENGTH; ++i) {
        hexmd5 += map[md5[i] / 16];
        hexmd5 += map[md5[i] % 16];
    }
    return hexmd5;
}

bool MD5Updated = false;

void UpdateMD5(const string& path)
{
    string md5Path = path + ".md5";
    string md5, newMd5 = CalcMD5(path);

    ifstream md5in(md5Path);
    if (md5in)
    {
        getline(md5in, md5);
        md5in.close();

        if (md5.back() == '\n')
            md5.pop_back();
        if (md5.back() == '\r')
            md5.pop_back();

        if (md5 == newMd5)
            return;
    }

    MD5Updated = true;
    cout << "Update MD5 for " << path << endl;

    ofstream md5out(md5Path);
    md5out << newMd5;
    md5out.close();
}

bool ForEachFile(string strPath)
{
    char cEnd = strPath.back();
    if (cEnd == '\\' || cEnd == '/')
    {
        strPath = strPath.substr(0, strPath.length() - 1);
    }

    if (strPath.empty() || strPath == (".") || strPath == (".."))
        return false;

    std::error_code ec;
    std::filesystem::path fsPath(strPath);
    if (!std::filesystem::exists(strPath, ec))
    {
        return false;
    }

    cout << "Checking " << strPath << endl;

    for (auto& itr : std::filesystem::directory_iterator(fsPath))
    {
        string path = itr.path().string();
        if (itr.is_directory() && find(ignores.begin(), ignores.end(), path) == ignores.end())
        {
            ForEachFile(path);
        }
        else
        {
            auto& filePath = itr.path();
            if (filePath.extension() != ".md5")
                UpdateMD5(path);
        }
    }
    return true;
}

int main()
{
	cout << "Beginning to Calculate MD5..." << endl;

    ifstream fin(IGNORE_FILES);
    string ig;
    while (fin)
    {
        getline(fin, ig);
        if (ig.back() == '\n')
            ig.pop_back();
        if (ig.back() == '\r')
            ig.pop_back();
        if (!ig.empty())
            ignores.push_back(ig);
    }

    for (auto& itr : std::filesystem::directory_iterator("."))
    {
        string path = itr.path().string();
        if (itr.is_directory() && find(ignores.begin(),ignores.end(),path) == ignores.end())
        {
            cout << "Working for Project: " << path << endl;
            ForEachFile(path);
        }
    }

    if (MD5Updated)
    {
        cout << "Finish updating. Pushing back to GitHub..." << endl;
        //system("git push origin");
    }
    else
    {
        cout << "No change found in files." << endl;
    }
}
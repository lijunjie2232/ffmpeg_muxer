//
// Created by li on 2023/02/08.
//

#include <iostream>
#include <string>
#include <cstring>

#ifdef WIN32

#include <direct.h>
#include <io.h>

#elifdef linux

#include <sys/io.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

#endif

#include "DirUtil.h"
#include "Dialog.h"


DirUtil::DirUtil(const char path[], char *audiospec, char *videospec) {
    this->path = path;
    if (!this->path) {
        std::cout << "invalid path" << std::endl;
        this->path = GetPwd();
    } else if (!this->PathChecker())
        throw std::exception();
    this->audiospec = audiospec;
    this->videospec = videospec;
};

const char *DirUtil::GetPwd() {
    std::cout << "get current path" << std::endl;
    const char *path = new char[256];
    path = getcwd(NULL, 128);
    if (path == nullptr) {
        std::cout << "get path error" << std::endl;
        throw std::exception();
    }
    std::cout << path << std::endl;
    return path;
}

bool DirUtil::PathChecker() const {
    if (access(this->path, 2) == 0) {
        std::cout << "valid dir path" << std::endl;
        return true;
    } else {
        std::cout << "invalid dir path" << std::endl;
        return false;
    }
}

void DirUtil::GetTargetFiles(std::vector<std::string> &filenames, const char *filespec, const char *path) const {
    if (!path)
        path = this->path;
    DIR *pDir = opendir(path);
    struct dirent *ptr;
    if (!pDir) {
        std::cout << "Open folder error" << std::endl;
        throw std::exception();
    }
    if (filespec) {
        while ((ptr = readdir(pDir)) != 0) {
            if (strcmp(ptr->d_name, ".") != 0 && strcmp(ptr->d_name, "..") != 0 &&
                strstr(ptr->d_name, filespec) != nullptr &&
                strlen(filespec) == strlen(strstr(ptr->d_name, filespec))) {
//                filenames.push_back(std::string(path) + "/" + std::string(ptr->d_name));
                filenames.push_back(std::string(ptr->d_name));
            }
        }
    } else {
        while ((ptr = readdir(pDir)) != 0) {
            if (strcmp(ptr->d_name, ".") != 0 && strcmp(ptr->d_name, "..") != 0) {
//                filenames.push_back(std::string(path) + "/" + std::string(ptr->d_name));
                filenames.push_back(std::string(ptr->d_name));
            }
        }
    }
    closedir(pDir);
}

void DirUtil::OutEnvCheck(const char *output) const {
    char *dir = new char[256];
    sprintf(dir, "%s%s%s", this->path, "/" , output);
    if (access(dir, 0) == -1) {
#ifdef WIN32
        int flag = mkdir(dir.c_str());  //Windows创建文件夹
#elifdef linux
        int flag = mkdir(dir, S_IRWXU);  //Linux创建文件夹
#endif
        if (flag == 0) {  //创建成功
            std::cout << "Create directory successfully." << std::endl;
        } else { //创建失败
            std::cout << "Fail to create directory." << std::endl;
            throw std::exception();
        }
    }
    delete[] dir;
}


void DirUtil::PreProcess(std::vector<std::string> &filenames, const char *filespec) {
    for (auto f: filenames) {
        f = this->getAbsolutePath(f);
//        std::cout << (f.substr(0,  f.length() - strlen(filespec))).c_str() << std::endl;
        LOGD("rename %s to %s", f.c_str(), (f.substr(0, f.length() - strlen(filespec))).c_str());
        rename(f.c_str(), (f.substr(0, f.length() - strlen(filespec))).c_str());
    }

    filenames.clear();

    this->GetTargetFiles(filenames, this->audiospec);
    for (auto &filename: filenames) {
        filename = filename.substr(0, filename.length() - strlen(this->audiospec));
    }

    for (std::vector<std::string>::iterator filename = filenames.begin(); filename != filenames.end(); filename++) {
        if (access(getAbsolutePath(*filename, this->videospec), R_OK) != 0 |
            access(getAbsolutePath(*filename, this->audiospec), R_OK) != 0) {
            LOGI("%s: return: %d", getAbsolutePath(*filename, this->videospec),
                 access(getAbsolutePath(*filename, this->videospec), 0));
            LOGI("%s: return: %d", getAbsolutePath(*filename, this->audiospec),
                 access(getAbsolutePath(*filename, this->audiospec), 0))

            filenames.erase(filename--);

        }
    }

    for (auto f: filenames)
        std::cout << f << std::endl;
    copy(filenames.begin(), filenames.end(), std::back_inserter(this->files));
}

DirUtil::~DirUtil() {
    if (path == NULL)
        delete[] path;
}

void DirUtil::setAudiospec(char *audiospec) {
    DirUtil::audiospec = audiospec;
}

void DirUtil::setVideospec(char *videospec) {
    DirUtil::videospec = videospec;
}

std::vector<std::string> DirUtil::getFiles() const {
    return this->files;
}

char *DirUtil::getAudiospec() const {
    return this->audiospec;
}

char *DirUtil::getVideospec() const {
    return this->videospec;
}

const char *DirUtil::getPath() const {
    LOGD("path: %s", this->path);
    return this->path;
}

char *DirUtil::getAbsolutePath(std::string filename, const char *spec) {
    std::string abpath = std::string(this->path) + "/" + filename;
    if(spec != nullptr)
        abpath += spec;
    return strdup(abpath.c_str());
}
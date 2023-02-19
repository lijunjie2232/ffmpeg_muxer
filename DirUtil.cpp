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
        LOGE("invalid path: %s", this->path);
        this->path = GetPwd();
    } else if (!this->PathChecker())
        throw std::exception();
    this->audiospec = audiospec;
    this->videospec = videospec;
};

const char *DirUtil::GetPwd() {
    LOGD("get current path");
    const char *path = new char[256];
    path = getcwd(NULL, 128);
    if (path == nullptr) {
        LOGE("get path error");
        throw std::exception();
    }
    LOGD("current path has been get: %s", path);
    return path;
}

bool DirUtil::PathChecker() const {
    if (access(this->path, 2) == 0) {
        LOGD("valid dir path");
        return true;
    } else {
        LOGE("invalid dir path");
        return false;
    }
}

void DirUtil::GetTargetFiles(std::vector<std::string> &filenames, const char *filespec, const char *path) const {
    if (!path)
        path = this->path;
    DIR *pDir = opendir(path);
    struct dirent *ptr;
    if (!pDir) {
        LOGE("Open folder error： %s", path);
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

int32_t DirUtil::OutEnvCheck(const char *output) const {

    if (output[0] != '/') {
        char *dir = new char[256];
        sprintf(dir, "%s%s%s", this->path, "/", output);
        if (access(dir, 0) == -1) {
            //创建目录文件
//#ifdef WIN32
//                int flag = mkdir(dir);  //Windows创建文件夹
//#elifdef linux
            int flag = mkdir(dir, S_IRWXU);  //Linux创建文件夹
//#endif
            if (flag == 0) {  //创建成功
                LOGD("Create directory successfully.");
            } else { //创建失败
                LOGE("Fail to create directory. %s", dir);
                delete[] dir;
                return -2;
            }
        }
        delete[] dir;
    } else {
        if (access(output, 0) == -1) {
            return this->CreatePath(output);
        }
    }
    return 0;
}

int32_t DirUtil::CreatePath(const char *dir) const {
    int m = 0;
    std::string str1, str2;
    str1 = dir;
    str2 = str1.substr(0, 1);
    str1 = str1.substr(1, str1.size());
    str1 += '/';
    m = str1.find('/');
    while (m >= 0) {
        str2 += str1.substr(0, m + 1);
        //判断该目录是否存在
        if (access(str2.c_str(), 0) == -1) {
            //创建目录文件
//#ifdef WIN32
//                int flag = mkdir(dstr2.c_str());  //Windows创建文件夹
//#elifdef linux
            int flag = mkdir(str2.c_str(), S_IRWXU);  //Linux创建文件夹
//#endif
            if (flag == 0) {  //创建成功
                LOGD("Create directory successfully.");
            } else { //创建失败
                LOGE("Fail to create directory. %s", str2.c_str());
                return -2;
            }
        }
        str1 = str1.substr(m + 1, str1.size());
        m = str1.find('/');
    }
    return 0;
}

void DirUtil::PreProcess(std::vector<std::string> &filenames, const char *filespec) {
    for (auto f: filenames) {
        f = this->getAbsolutePath(f);
//        LOGD((f.substr(0,  f.length() - strlen(filespec))).c_str());
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

    for (auto f: filenames) {
        LOGD("masked file: %s has been processed.", f.c_str());
    }
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
    if (spec != nullptr)
        abpath += spec;
    return strdup(abpath.c_str());
}
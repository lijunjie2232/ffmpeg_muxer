#include <iostream>
#include <string>
#include <cstring>
//#include <bits/stdc++.h>
#include <fmt/format.h>

#include "DirUtil.h"
#include "MediaFile.h"
#include "Dialog.h"
#include "AVMuxer.h"



void color_test() {
    std::cout << "这是原始颜色" << std::endl;
    std::cout << "\033[30;11m这是前景黑色\033[0m" << std::endl;
    std::cout << "\033[31;11m这是前景红色\033[0m" << std::endl;
    std::cout << "\033[32;11m这是前景绿色\033[0m" << std::endl;
    std::cout << "\033[33;11m这是前景黄色\033[0m" << std::endl;
    std::cout << "\033[34;11m这是前景蓝色\033[0m" << std::endl;
    std::cout << "\033[35;11m这是前景紫色\033[0m" << std::endl;
    std::cout << "\033[36;11m这是前景青色\033[0m" << std::endl;
    std::cout << "\033[37;11m这是前景白色\033[0m" << std::endl;
    std::cout << "这是原始颜色" << std::endl;
    std::cout << "\033[;40m这是背景黑色\033[0m" << std::endl;
    std::cout << "\033[;41m这是背景红色\033[0m" << std::endl;
    std::cout << "\033[;42m这是背景绿色\033[0m" << std::endl;
    std::cout << "\033[;43m这是背景黄色\033[0m" << std::endl;
    std::cout << "\033[;44m这是背景蓝色\033[0m" << std::endl;
    std::cout << "\033[;45m这是背景紫色\033[0m" << std::endl;
    std::cout << "\033[;46m这是背景青色\033[0m" << std::endl;
    std::cout << "\033[;47m这是背景白色\033[0m" << std::endl;
}

void test() {
    const char *filename = "/home/li/data/code/c/ffmpeg_project/cmake-build-debug/ffmpeg_project/暗示伏尔泰.cpp";
    std::cout << filename << std::endl << std::string(filename).data() << std::endl << std::string(filename).c_str()
              << std::endl;
    const char *ap = ".cpp";
    auto spt = strstr(filename, ap);
    std::cout << spt << std::endl;
    if (strlen(ap) == strlen(spt))
        std::cout << "true" << std::endl;
    else
        std::cout << "ap: " << ap << " spt: " << spt << std::endl;
    std::cout << spt - filename << " " << strlen(filename) << std::endl;
    for (auto *pt = filename; pt != spt; pt++)
        std::cout << *pt;
    std::cout << std::endl;
    std::cout << std::string(filename).substr(0, spt - filename) << std::endl;
}

void file_junbe() {
    std::cout << "Hello, World!" << std::endl;
    const char *path = "/home/li/data/Videos/test";
//    char *path;
//    path = new char[256];
//    std::cin.getline(path, 256);
//    std::cout << path << std::endl;
    DirUtil dir = DirUtil(path);
    dir.setAudiospec(".mp3");
    dir.setVideospec(".mp4");
    std::vector<std::string> filenames;
    const char *filespec = ".pdf";
    dir.GetTargetFiles(filenames, filespec);
    dir.PreProcess(filenames, filespec);
}

void test2() {
    const char *FileName1 = "/home/li/data/Videos/test/01_Study Japanese 日常生活常用形容詞 第一課  赤い.mp4";
    MediaFile media = MediaFile(FileName1);
    media.Info();
    media.Close();
    const char *FileName2 = "/home/li/data/Videos/test/01_Study Japanese 日常生活常用形容詞 第一課  赤い.mp3";
    media.Open(FileName2);
    media.Info();
    media.Close();
}

void test3() {
    LOGD("Hello, World!");
    const char *path = "/home/li/data/Videos/test";
//    char *path;
//    path = new char[256];
//    std::cin.getline(path, 256);
//    std::cout << path << std::endl;
    DirUtil *dir = new DirUtil(path);
    dir->setAudiospec(".mp3");
    dir->setVideospec(".mp4");
    std::vector<std::string> filenames;
    const char *maskspec = ".pdf";
    dir->GetTargetFiles(filenames, maskspec);
    dir->PreProcess(filenames, maskspec);
    AVMuxer muxer = AVMuxer(dir, "output");
//    muxer.MuxAV();
}


const char *getHelp() {
    std::cout << "Usage:\n"
                 "muxer [-h] [-o outputpath] [-d inputpath]\n"
                 "[-ms masksuffx] [-vs videosuffx] [-as audiosuffx]\n\n"
                 "options:\n"
                 "-h   --help        : get help\n"
                 "-o   --output_dir  : output dir\n"
                 "-d   --input_dir   : input fir\n"
                 "-ms  --masksuffx   : suffx of mask\n"
                 "-vs  --videosuffx  : suffx of video\n"
                 "-as  --audiosuffx  : suffx of audio\n"
              << std::endl;
}

void getVersion() {
//    printf("\033[33;40m%s\033[0m", "Muxer version: muxer/0.1 by lijunjie2232\n\n");
    printf("\033[33;11m%s\033[0m", "Muxer version: muxer/0.1 by git@lijunjie2232\n\n");
}

int main(int argc, char **argv) {
//    file_junbe();
//    test2();
//    test3();
//    color_test();
//    if (path != NULL)
//        delete[] path;

    char *ipath = nullptr;
    bool ipath_n = false;
    char *odir = nullptr;
    bool odir_n = true;
    char *maskspec = nullptr;
    bool maskspec_n = true;
    char *vspec = nullptr;
    bool vspec_n = true;
    char *aspec = nullptr;
    bool aspec_n = true;
    int i = 1;
    while (i < argc) {
//        LOGD("args%d: %s", i, argv[i]);
        if (!strcmp(argv[i], "-h") | !strcmp(argv[i], "--help")) {
            getVersion();
            getHelp();
            return 0;
        } else if (!strcmp(argv[i], "-d") | !strcmp(argv[i], "--input_dir")) {
            ipath = argv[++i];
        } else if (!strcmp(argv[i], "-v") | !strcmp(argv[i], "--version")) {
            getVersion();
            return 0;
        } else if (!strcmp(argv[i], "-o") | !strcmp(argv[i], "--output_dir")) {
            odir = argv[++i];
        } else if (!strcmp(argv[i], "-ms") | !strcmp(argv[i], "--masksuffx")) {
            maskspec = argv[++i];
        } else if (!strcmp(argv[i], "-vs") | !strcmp(argv[i], "--videosuffx")) {
            vspec = argv[++i];
        } else if (!strcmp(argv[i], "-as") | !strcmp(argv[i], "--audiosuffx")) {
            aspec = argv[++i];
        }
        i++;
    }

    getVersion();

    if (ipath == nullptr) {
        ipath = new char[512];
        ipath_n = true;
        std::cout << "Enter input dir path:" << std::endl;
        std::cin.getline(ipath, 512);
    }
    if (odir == nullptr) {
        odir_n = false;
        odir = "output";
    }

    if (maskspec == nullptr) {
        maskspec_n = false;
        maskspec = ".pdf";
    }

    if (vspec == nullptr) {
        vspec_n = false;
        vspec = ".mp4";
    }

    if (aspec == nullptr) {
        aspec_n = false;
        aspec = ".mp3";
    }


    LOGD("%s", ipath);

    DirUtil *dir = new DirUtil(ipath);
    dir->setVideospec(vspec);
    dir->setAudiospec(aspec);
    std::vector<std::string> filenames;
    dir->GetTargetFiles(filenames, maskspec);
    dir->PreProcess(filenames, maskspec);
    AVMuxer muxer = AVMuxer(dir, odir);
    muxer.MuxAV();
    delete dir;
    if (ipath_n)
        delete[] ipath;
//    if (odir_n)
//        delete[] odir;
//    if (maskspec_n)
//        delete[] maskspec;
//    if (vspec_n)
//        delete[] vspec;
//    if (aspec_n)
//        delete[] aspec;
    return 0;
}

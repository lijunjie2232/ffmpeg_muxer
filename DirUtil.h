//
// Created by li on 2023/02/08.
//

#ifndef C_UTILS_H
#define C_UTILS_H

#include <vector>
#include <string>

class DirUtil {
public:
    std::vector<std::string> files;
    char *audiospec;
    char *videospec;
    const char *path;

    std::vector<std::string> getFiles() const;

    char *getAudiospec() const;

    char *getVideospec() const;

    const char *getPath() const;

    char *getAbsolutePath(std::string filename, const char *spec = nullptr);

    void setAudiospec(char *audiospec);

    void setVideospec(char *videospec);

    DirUtil(const char path[] = nullptr, char *audiospec = ".mp3", char *videospec = ".mp4");

    ~DirUtil();

    bool PathChecker() const;

    static const char *GetPwd();

    void GetTargetFiles(std::vector<std::string> &filenames, const char *filespec = nullptr,
                        const char *path = nullptr) const;

    void PreProcess(std::vector<std::string> &filenames, const char *filespec);

    void OutEnvCheck(const char *output = "output") const;


};


//class FfmpegUtil{
//public:
//
//};

#endif //C_UTILS_H
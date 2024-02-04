#ifndef CONVERT_h
#define CONVERT_h

#include <string>
#include <signalslot/Signal.hpp>

class Convert
{
signals:
    //当前处理进度,   当前项          总项
    Gallant::Signal2<unsigned long, unsigned long> signal_progress;

public:
    Convert(const std::string input, const std::string output);

    /// @brief 
    void start();

private:
    //获得input总秒数
    void getSecondsCount();
    //开始转换
    void startConvert();
    //输出一秒所有帧到temp目录
    void outputOneSecondsToTemp(unsigned long currentSeconds);
    //剥离一秒音频
    void peelOneSecondsAudio(unsigned long currentSeconds);
    //抽取一秒视频包含的所有图片
    void outputOneSecondsImage(unsigned long currentSeconds);
    //清晰化处理
    void distinctProcess(unsigned long currentSeconds);
    //合并图片到视频
    void mergeImagesToVideo(unsigned long currentSeconds);


private:
    std::string input;
    std::string output;

    unsigned long currentSeconds;
    unsigned long secondsCount;
    std::string format_name;

    const unsigned long bufferSize = 40960;

    std::string ffmpegHome;
    std::string realesrganHome;
    std::string codeFormerNcnnHome;
    std::string codeFormerHome;

    char* command;
    char* buffer;
};





#endif //CONVERT_h

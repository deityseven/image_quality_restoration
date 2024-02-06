#include "convert.h"
#include <stdio.h>
#include <stdlib.h>
#include <json/json.hpp>
#include <direct.h>
#include <qprocess.h>
#include <qdir.h>
#include <qdebug.h>

Convert::Convert(const std::string input, const std::string output)
	:input(input),
	output(output),
	currentSeconds(0),
	secondsCount(0)
{
	this->ffmpegHome = getenv("ffmpeg_home");
	this->realesrganHome = getenv("realesrgan_home");
	this->codeFormerNcnnHome = getenv("CodeFormer_ncnn_home");
	this->codeFormerHome = getenv("CodeFormer_home");

	this->command = (char*)malloc(bufferSize);
	this->buffer = (char*)malloc(bufferSize);

	if (command == nullptr || buffer == nullptr)
	{
		exit(-666);
	}
}

void Convert::start()
{
	getSecondsCount();
	startConvert();
}

void Convert::getSecondsCount()
{
	// 获得input总秒数
	memset(this->command, 0, this->bufferSize);
	memset(this->buffer, 0, this->bufferSize);

	sprintf(this->command, "%s/bin/ffprobe.exe -loglevel quiet -print_format json -show_format -i %s", this->ffmpegHome.c_str(), this->input.c_str());
	
	QProcess process;
	process.start(this->command, QIODevice::OpenModeFlag::ReadWrite);
	process.waitForStarted();
	process.waitForReadyRead();
	process.waitForFinished(-1);
	std::string jsonData = process.readAll().toStdString();

	auto json = nlohmann::json::parse(jsonData);

	this->secondsCount = std::stoi(json["format"]["duration"].get<std::string>());
	this->format_name = json["format"]["format_name"].get<std::string>();
}

void Convert::startConvert()
{
	system("rmdir .\\merge /s /q");
	system("mkdir .\\merge");
	system("rmdir .\\secondsVideos /s /q");
	system("mkdir .\\secondsVideos");
	system("mkdir .\\finalVideos");

	memset(this->buffer, 0, this->bufferSize);
	_getcwd(this->buffer, this->bufferSize);
	std::string pwd(this->buffer);

	for (this->currentSeconds = 0; this->currentSeconds < this->secondsCount; ++this->currentSeconds)
	{
		_chdir(pwd.c_str());
		system("rmdir .\\temp /s /q");
		system("rmdir .\\merge /s /q");
		system("mkdir .\\merge");

		outputOneSecondsToTemp(this->currentSeconds);
		peelOneSecondsAudio(this->currentSeconds);
		outputOneSecondsImage(this->currentSeconds);
		distinctProcess(this->currentSeconds);
		mergeImagesToVideo(this->currentSeconds);
		mergeAudioToVideo(this->currentSeconds);

		signal_progress(this->currentSeconds, this->secondsCount);
	}

	mergeAllVideo();

	signal_progress(this->secondsCount, this->secondsCount);
}

void Convert::outputOneSecondsToTemp(unsigned long currentSeconds)
{
	system("mkdir .\\temp");
	memset(this->command, 0, this->bufferSize);
	sprintf(this->command, "%s/bin/ffmpeg.exe -loglevel quiet"
		" -ss %d"
		" -t 1"
		" -i %s"
		" -c copy"
		" ./temp/%08d.mp4",
		this->ffmpegHome.c_str(),
		currentSeconds,
		this->input.c_str(),
		currentSeconds);

	QProcess process;
	process.start(this->command, QIODevice::OpenModeFlag::ReadOnly);
	process.waitForStarted();
	process.waitForFinished(-1);
}

void Convert::peelOneSecondsAudio(unsigned long currentSeconds)
{
	memset(this->command, 0, this->bufferSize);
	sprintf(this->command, "%s/bin/ffmpeg.exe -loglevel quiet -i ./temp/%08d.mp4 -vn -acodec copy -c:a aac ./merge/%08d.aac -y", this->ffmpegHome.c_str(), currentSeconds, currentSeconds);

	QProcess process;
	process.start(this->command, QIODevice::OpenModeFlag::ReadOnly);
	process.waitForStarted();
	process.waitForFinished(-1);

	memset(this->command, 0, this->bufferSize);
	sprintf(this->command, "%s/bin/ffmpeg.exe -loglevel quiet -i ./temp/%08d.mp4 -an -vcodec copy ./temp/%08d.mp4 -y", this->ffmpegHome.c_str(), currentSeconds, currentSeconds);

	process.start(this->command, QIODevice::OpenModeFlag::ReadOnly);
	process.waitForStarted();
	process.waitForFinished(-1);
}

void Convert::outputOneSecondsImage(unsigned long currentSeconds)
{
	system("mkdir .\\temp\\images");
	memset(this->command, 0, this->bufferSize);
	sprintf(this->command, "%s/bin/ffmpeg.exe -loglevel quiet -i ./temp/%08d.mp4 ", this->ffmpegHome.c_str(), currentSeconds);
	strcat(this->command,"./temp/images/%08d.png -y");

	QProcess process;
	process.start(this->command, QIODevice::OpenModeFlag::ReadOnly);
	process.waitForStarted();
	process.waitForFinished(-1);
}

void Convert::distinctProcess(unsigned long currentSeconds)
{
	system("mkdir .\\temp\\x4images");
	system("mkdir .\\temp\\x8images");
	system("mkdir .\\temp\\codeformer");

	memset(this->command, 0, this->bufferSize);
	sprintf(this->command, "%s/realesrgan-ncnn-vulkan.exe -i ./temp/images -o ./temp/x4images -n realesrgan-x4plus", 
		this->realesrganHome.c_str(), currentSeconds);

	QProcess process;
	process.start(this->command, QIODevice::OpenModeFlag::ReadOnly);
	process.waitForStarted();
	process.waitForFinished(-1);

	//////////////////////////////////////
	//保存当前工作目录
	memset(this->buffer, 0, this->bufferSize);
	_getcwd(this->buffer, this->bufferSize);
	std::string pwd(this->buffer);

	std::string newPwd = this->codeFormerNcnnHome + "/bin";
	
	memset(this->command, 0, this->bufferSize);
	sprintf(this->command, "%s/bin/ncnn_codeformer.exe %s/temp/x4images 0 %s/temp/x8images",
		this->codeFormerNcnnHome.c_str(),
		pwd.c_str(),
		pwd.c_str()
		);

	process.setWorkingDirectory(QString::fromStdString(newPwd));
	process.start(this->command, QIODevice::OpenModeFlag::ReadOnly);
	process.waitForStarted();
	process.waitForFinished(-1);

	////////////////////////////////////
	//进入虚拟环境 codefomer
	//保存当前工作目录
	memset(this->buffer, 0, this->bufferSize);
	newPwd = this->codeFormerHome;

	memset(this->command, 0, this->bufferSize);
	sprintf(this->command, "C:/Users/deityqi/.conda/envs/codefomer/python inference_codeformer.py -w 0 -i %s/temp/x8images -o %s/temp/codeformer",
		pwd.c_str(),
		pwd.c_str()
	);

	process.setWorkingDirectory(QString::fromStdString(newPwd));
	//process.start("conda activate codefomer" , QIODevice::OpenModeFlag::ReadOnly);
	process.start(this->command, QIODevice::OpenModeFlag::ReadOnly);
	process.waitForStarted();
	process.waitForFinished(-1);
}

void Convert::mergeImagesToVideo(unsigned long currentSeconds)
{
	memset(this->command, 0, this->bufferSize);
	memset(this->buffer, 0, this->bufferSize);
	//ffmpeg -f image2 -i dirname/%05d.jpg -vcodec libx264 -r 25 -b:v 5969k test.mp4
	sprintf(this->command, "%s/bin/ffmpeg.exe -f image2", this->ffmpegHome.c_str());
	char outFile[40];
	memset(outFile, 0, 40);
	sprintf(outFile, " ./merge/%08d.mp4", currentSeconds);
	strcat(this->command, " -i ./temp/codeformer/final_results/%08d.png -vcodec libx264 -r 60 -b:v 35m -crf 0 -vf scale=2560:1440");
	strcat(this->command, outFile);

	QProcess process;
	process.start(this->command, QIODevice::OpenModeFlag::ReadOnly);
	process.waitForStarted();
	process.waitForFinished(-1);
}

void Convert::mergeAudioToVideo(unsigned long currentSeconds)
{
	memset(this->command, 0, this->bufferSize);
	memset(this->buffer, 0, this->bufferSize);

	char videoPath[40];
	char audioPath[40];
	char outputPath[40];
	memset(videoPath, 0, 40);
	memset(audioPath, 0, 40);
	memset(outputPath, 0, 40);
	sprintf(videoPath, "./merge/%08d.mp4", currentSeconds);
	sprintf(audioPath, "./merge/%08d.aac", currentSeconds);
	sprintf(outputPath, "./secondsVideos/%08d.mp4", currentSeconds);
	//ffmpeg -i video.mp4 -i audio.mp3 -c:v copy -c:a aac output.mp4
	sprintf(this->command, "%s/bin/ffmpeg.exe -i %s -i %s -c:v copy -c:a aac %s", 
		this->ffmpegHome.c_str(),
		videoPath,
		audioPath,
		outputPath);
	
	QProcess process;
	process.start(this->command, QIODevice::OpenModeFlag::ReadOnly);
	process.waitForStarted();
	process.waitForFinished(-1);
}

void Convert::mergeAllVideo()
{
	memset(this->command, 0, this->bufferSize);

	//mp4转到ts流 
	//ffmpeg -i X.mp4 -vcodec copy -acodec copy -vbsf h264_mp4toannexb X.ts
	sprintf(this->command, "%s/bin/ffmpeg.exe",
		this->ffmpegHome.c_str());
	strcat(this->command, " -i ./secondsVideos/%08d.mp4 -vcodec copy -acodec copy -vbsf h264_mp4toannexb ./secondsVideos/%08d.ts");

	QProcess process;
	process.start(this->command, QIODevice::OpenModeFlag::ReadOnly);
	process.waitForStarted();
	process.waitForFinished(-1);

	//ts流 合并 mp4
	//ffmpeg -i ./secondsVideos/%08d.ts output.mp4
    QDir secondsVideos("./secondsVideos");
	auto secondsVideosFileList = secondsVideos.entryList(QDir::Files);
    std::string concat = "concat:";
    char buf[100];
    for(auto i = 0; i < secondsVideosFileList.size(); ++i)
    {
        memset(buf, 0, sizeof(buf));
        sprintf(buf, " %08d.ts |", i);
        concat += buf;
    }
    
    concat.pop_back();

	sprintf(this->command, "%s/bin/ffmpeg.exe -i \"%s\"",
		this->ffmpegHome.c_str(),
		concat.c_str());

    QDir finalVideos("./finalVideos");
	auto finalVideosList = finalVideos.entryList(QDir::Files);

	char outputPath[40];
	memset(outputPath, 0, 40);
	sprintf(outputPath, " ./finalVideos/%08d.mp4", finalVideosList.size());
	strcat(this->command, outputPath);

	process.start(this->command, QIODevice::OpenModeFlag::ReadOnly);
	process.waitForStarted();
	process.waitForFinished(-1);
}

#include "convert.h"
#include <stdio.h>
#include <stdlib.h>
#include <json/json.hpp>
#include <direct.h>

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

	sprintf(this->command, "%s/bin/ffprobe.exe -loglevel quiet -print_format json -show_format -i %s 2>&1", this->ffmpegHome.c_str(), this->input.c_str());

	FILE* pipe = _popen(this->command, "r");
	if (pipe == nullptr)
	{
		perror("_popen return nullptr\n");
		exit(-1);
	}

	do
	{
		fseek(pipe, 0, SEEK_END);
		long pipeSize = ftell(pipe);
		fread(this->buffer, sizeof(char), pipeSize, pipe);
	} while (!feof(pipe));

	_pclose(pipe);
	/*
	std::string content;
	while (fgets(this->buffer, this->bufferSize, pipe))
	{
		content += this->buffer;
		memset(this->buffer, 0, this->bufferSize);
	}
	*/

	auto json = nlohmann::json::parse(this->buffer);

	this->secondsCount = std::stoi(json["format"]["duration"].get<std::string>());
	this->format_name = json["format"]["format_name"].get<std::string>();
}

void Convert::startConvert()
{
	system("rmdir .\\merge /s /q");
	system("mkdir .\\merge");

	for (this->currentSeconds = 0; this->currentSeconds < this->secondsCount; ++this->currentSeconds)
	{
		system("rmdir .\\temp /s /q");

		outputOneSecondsToTemp(this->currentSeconds);
		peelOneSecondsAudio(this->currentSeconds);
		outputOneSecondsImage(this->currentSeconds);
		distinctProcess(this->currentSeconds);
		mergeImagesToVideo(this->currentSeconds);

		signal_progress(this->currentSeconds, this->secondsCount);
	}
}

void Convert::outputOneSecondsToTemp(unsigned long currentSeconds)
{
	system("mkdir .\\temp");
	memset(this->command, 0, this->bufferSize);
	sprintf(this->command, "%s/bin/ffmpeg.exe -loglevel quiet"
		" -ss %d"
		" -t %d"
		" -i %s"
		" -c copy"
		" ./temp/%d.mp4"
		" 2>&1",
		this->ffmpegHome.c_str(),
		currentSeconds,
		currentSeconds + 1,
		this->input.c_str(),
		currentSeconds);

	FILE* pipe = _popen(this->command, "r");
	if (pipe == nullptr)
	{
		perror("_popen return nullptr\n");
		exit(-1);
	}

	_pclose(pipe);
}

void Convert::peelOneSecondsAudio(unsigned long currentSeconds)
{
	memset(this->command, 0, this->bufferSize);
	sprintf(this->command, "%s/bin/ffmpeg.exe -loglevel quiet -i ./temp/%d.mp4 -vn -acodec copy ./temp/%d.mp2 -y", this->ffmpegHome.c_str(), currentSeconds, currentSeconds);

	FILE* pipe = _popen(this->command, "r");
	if (pipe == nullptr)
	{
		perror("_popen return nullptr\n");
		exit(-1);
	}

	_pclose(pipe);

	memset(this->command, 0, this->bufferSize);
	sprintf(this->command, "%s/bin/ffmpeg.exe -loglevel quiet -i ./temp/%d.mp4 -an -vcodec copy ./temp/%d.mp4 -y", this->ffmpegHome.c_str(), currentSeconds, currentSeconds);

	pipe = _popen(this->command, "r");
	if (pipe == nullptr)
	{
		perror("_popen return nullptr\n");
		exit(-1);
	}

	_pclose(pipe);
}

void Convert::outputOneSecondsImage(unsigned long currentSeconds)
{
	system("mkdir .\\temp\\images");
	memset(this->command, 0, this->bufferSize);
	sprintf(this->command, "%s/bin/ffmpeg.exe -loglevel quiet -i ./temp/%d.mp4 ", this->ffmpegHome.c_str(), currentSeconds);
	strcat(this->command,"./temp/images/%3d.png -y");

	FILE* pipe = _popen(this->command, "r");
	if (pipe == nullptr)
	{
		perror("_popen return nullptr\n");
		exit(-1);
	}

	_pclose(pipe);
}

void Convert::distinctProcess(unsigned long currentSeconds)
{
	system("mkdir .\\temp\\x4images");
	system("mkdir .\\temp\\x8images");
	system("mkdir .\\temp\\codeformer");

	memset(this->command, 0, this->bufferSize);
	sprintf(this->command, "%s/realesrgan-ncnn-vulkan.exe -i ./temp/images -o ./temp/x4images -n realesrgan-x4plus 2>&1", 
		this->realesrganHome.c_str(), currentSeconds);

	FILE* pipe = _popen(this->command, "r");
	if (pipe == nullptr)
	{
		perror("_popen return nullptr\n");
		exit(-1);
	}

	do
	{
		fseek(pipe, 0, SEEK_END);
		long pipeSize = ftell(pipe);
		fread(this->buffer, sizeof(char), pipeSize, pipe);
	} while (!feof(pipe));

	_pclose(pipe);

	//////////////////////////////////////
	//保存当前工作目录
	memset(this->buffer, 0, this->bufferSize);
	_getcwd(this->buffer, this->bufferSize);
	std::string pwd(this->buffer);

	std::string newPwd = this->codeFormerNcnnHome + "/bin";
	_chdir(newPwd.c_str());

	memset(this->command, 0, this->bufferSize);
	sprintf(this->command, "%s/bin/ncnn_codeformer.exe %s/temp/x4images 0 %s/temp/x8images 2>&1",
		this->codeFormerNcnnHome.c_str(),
		pwd.c_str(),
		pwd.c_str()
		);

	pipe = _popen(this->command, "r");
	if (pipe == nullptr)
	{
		perror("_popen return nullptr\n");
		exit(-1);
	}

	do
	{
		fseek(pipe, 0, SEEK_END);
		long pipeSize = ftell(pipe);
		fread(this->buffer, sizeof(char), pipeSize, pipe);
	} while (!feof(pipe));

	_pclose(pipe);

	_chdir(pwd.c_str());

	////////////////////////////////////
	//进入虚拟环境 codefomer
	//保存当前工作目录
	memset(this->buffer, 0, this->bufferSize);
	newPwd = this->codeFormerHome;
	_chdir(newPwd.c_str());

	memset(this->command, 0, this->bufferSize);
	sprintf(this->command, "conda activate codefomer & python inference_codeformer.py -w 0 -i %s/temp/x8images -o %s/temp/codeformer 2>&1",
		pwd.c_str(),
		pwd.c_str()
	);

	pipe = _popen(this->command, "r");
	if (pipe == nullptr)
	{
		perror("_popen return nullptr\n");
		exit(-1);
	}

	do
	{
		fseek(pipe, 0, SEEK_END);
		long pipeSize = ftell(pipe);
		fread(this->buffer, sizeof(char), pipeSize, pipe);
	} while (!feof(pipe));

	_pclose(pipe);

	_chdir(pwd.c_str());
}

void Convert::mergeImagesToVideo(unsigned long currentSeconds)
{
	memset(this->command, 0, this->bufferSize);
	memset(this->buffer, 0, this->bufferSize);
	//ffmpeg -f image2 -i dirname/%05d.jpg -vcodec libx264 -r 25 -b:v 5969k test.mp4
	sprintf(this->command, "%s/bin/ffmpeg.exe -f image2", this->ffmpegHome.c_str());
	char outFile[40];
	memset(outFile, 0, 40);
	sprintf(outFile, " ./merge/%d.mp4 2>&1", currentSeconds);
	strcat(this->command, " -i ./temp/codeformer/final_results/%3d.png -vcodec libx264 -r 60 -b:v 35m -crf 0 -vf scale=2560:1440");
	strcat(this->command, outFile);

	FILE* pipe = _popen(this->command, "r");
	if (pipe == nullptr)
	{
		perror("_popen return nullptr\n");
		exit(-1);
	}

	do
	{
		fseek(pipe, 0, SEEK_END);
		long pipeSize = ftell(pipe);
		fread(this->buffer, sizeof(char), pipeSize, pipe);
	} while (!feof(pipe));

	_pclose(pipe);
}

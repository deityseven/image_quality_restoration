#include <iostream>
#include <getopt/getopt.hpp>
#include "convert.h"
#include <qapplication.h>

void useage()
{
	std::cerr << "use : dvd video -> 4X mp4" << std::endl;
	std::cerr << "useage : [input] [output]" << std::endl;
}

int main(int argc, char *argv[])
{
	bool help = getarg(false, "-h", "--help", "-?");
	std::string input = getarg("", "-i", "--input");
	std::string output = getarg("", "-o", "--output");

	if (help == true || input.empty() || output.empty())
	{
		useage();
		return -3;
	}

	QCoreApplication a(argc, argv);

	Convert c(input,output);
	c.start();

	return a.exec();
}
#include <iostream>
#include <stdio.h>
#include <sstream>
#include "flacscrubber.h"
#include "optionparser.h"

struct Arguments : public option::Arg {
	static option::ArgStatus argumentError(bool msg, const char * msg1, const option::Option & option, const char * msg2)
	{
		if(msg) {
			std::cerr << "Error: " << msg1 << std::string(option.name).substr(0, option.namelen) << msg2 << std::endl;
		}
		return option::ARG_ILLEGAL;
	}
	static option::ArgStatus Integer(const option::Option & option, bool msg) {
		if(!option.arg) {
			return argumentError(msg, "Option ", option, " cannot be empty.");
		}
		std::istringstream stream(option.arg);
		int i;
		stream >> std::noskipws >> i;
		if(!stream.eof() || stream.fail()) {
			return argumentError(msg, "Option ", option, " must be an integer.");
		}
		return option::ARG_OK;
	}
	static option::ArgStatus Rate(const option::Option & option, bool msg) {
		if(!option.arg) {
			return argumentError(msg, "Option ", option, " cannot be empty.");
		}
		std::istringstream stream(option.arg);
		float f;
		stream >> std::noskipws >> f;
		if(!stream.eof() || stream.fail()) {
			return argumentError(msg, "Option ", option, " must be an number.");
		}
		if(f < 0.f || f > 1.f) {
			return argumentError(msg, "Option ", option, " must be between 0 and 1.");
		}
		return option::ARG_OK;
	}
};

int main(int argc, char ** argv) {
	enum optionIndex {
		UNKNOWN,
		HELP,
		FIRST_SIZE,
		LAST_SIZE,
		FIRST_MAX_OFFSET,
		LAST_MAX_OFFSET,
		OTHER_MAX_OFFSET,
		MAX_OFFSET,
		FIRST_RATE,
		LAST_RATE,
		OTHER_RATE,
		RATE,
		FORCE_NONZERO
	};
	option::Descriptor usage[] = {
		{UNKNOWN,          0, "", "",                 option::Arg::None,  std::string("Usage: " + std::string(argc > 0 ? argv[0] : "ascrubber") + " [options] file1.flac file2.flac ...\n\n"
		                                                                              "This program replaces the files you tell it it. Make backups as necessary prior to using this program.\n\n"
		                                                                              "Options:").data()},
		{HELP,             0, "", "help",             option::Arg::None,  "  --help               \tPrint usage and exit.\n"},
		{FIRST_SIZE,       0, "", "first-size",       Arguments::Integer, "  --first-size N       \tSize of the samples window considered to be the beginning of the file.\n"
		                                                                  "                       \tDefault value: 4096 samples.\n"},
		{LAST_SIZE,        0, "", "last-size",        Arguments::Integer, "  --last-size N        \tSize of the samples window considered to be the end of the file.\n"
		                                                                  "                       \tDefault value: 2048 samples.\n"},
		{FIRST_MAX_OFFSET, 0, "", "first-max-offset", Arguments::Integer, "  --first-max-offset N \tMaximum offset that can be applied to samples in the beginning sample window, in any direction.\n"
		                                                                  "                       \tIf set to 0, no samples in the beginning window will be scrubbed.\n"
		                                                                  "                       \tSamples range over all 32-bit integers.\n"
		                                                                  "                       \tDefault value: 256.\n"},
		{LAST_MAX_OFFSET,  0, "", "last-max-offset",  Arguments::Integer, "  --last-max-offset N  \tMaximum offset that can be applied to samples in the end sample window, in any direction.\n"
		                                                                  "                       \tIf set to 0, no samples in the end window will be scrubbed.\n"
		                                                                  "                       \tSamples range over all 32-bit integers.\n"
		                                                                  "                       \tDefault value: 256.\n"},
		{OTHER_MAX_OFFSET, 0, "", "other-max-offset", Arguments::Integer, "  --other-max-offset N \tMaximum offset that can be applied to samples in the middle of the file, in any direction.\n"
		                                                                  "                       \tIf set to 0, no samples in the middle of the file will be scrubbed.\n"
		                                                                  "                       \tSamples range over all 32-bit integers.\n"
		                                                                  "                       \tDefault value: 2.\n"},
		{MAX_OFFSET,       0, "", "all-max-offset",   Arguments::Integer, "  --all-max-offset N   \tShortcut to specify the maximum allowed offset to all 3 possible locations of a sample.\n"
		                                                                  "                       \tIf set to 0, nothing will be scrubbed, which is probably not what you want."},
		{FIRST_RATE,       0, "", "first-rate",       Arguments::Rate,    "  --first-rate R       \tProbably to scrubbing a sample in the beginning sample window.\n"
		                                                                  "                       \tFor example, a rate of 0.25 would scrub about a fourth of all the samples in the beginning window.\n"
		                                                                  "                       \tDefault value: 1 (100%).\n"},
		{LAST_RATE,        0, "", "last-rate",        Arguments::Rate,    "  --last-rate R        \tProbably to scrubbing a sample in the end sample window.\n"
		                                                                  "                       \tFor example, a rate of 0.75 would scrub about three fourths of all the samples in the end window.\n"
		                                                                  "                       \tDefault value: 1 (100%).\n"},
		{OTHER_RATE,       0, "", "other-rate",       Arguments::Rate,    "  --other-rate R       \tProbably to scrubbing a sample in the middle of the file.\n"
		                                                                  "                       \tFor example, a rate of 0 would scrub leave all the samples in middle of the file intact.\n"
		                                                                  "                       \tDefault value: 0.2 (20%).\n"},
		{RATE,             0, "", "all-rate",         Arguments::Rate,    "  --all-rate R         \tShortcut to specify the probability to scrub a sample in all 3 possible locations.\n"
		                                                                  "                       \tFor example, a rate of 1 would scrub every single sample in the file.\n"},
		{FORCE_NONZERO,    0, "", "force-nonzero",    option::Arg::None,  "  --force-nonzero      \tIf specified, the random offset applied to any scrubbed sample cannot be 0.\n"
		                                                                  "                       \tThis ensures that the scrubbed samples are different to the original\n"
		                                                                  "                       \tBy default, this feature is off (scrubbing can leave a sample untouched).\n"},
		{0,                0, 0,  0,                  0,                  0}
	};
	if(argc > 0) { // Strip argv[0]
		argc--;
		argv++;
	}
	option::Stats stats(usage, argc, argv);
	option::Option options[stats.options_max], buffer[stats.buffer_max];
	option::Parser parse(usage, argc, argv, options, buffer);
	if(parse.error()) {
		return 1;
	}
	if(options[HELP] || !argc) {
		option::printUsage(std::cerr, usage);
		return 0;
	}
	for (int i = 0; i < parse.nonOptionsCount(); i++) {
		std::cerr << "Processing file: " << parse.nonOption(i) << std::endl;
		FLACScrubber scrubber(parse.nonOption(i));
		if(scrubber.hasError()) {
			scrubber.cancel();
			continue;
		}
		if(options[FIRST_SIZE]) {
			scrubber.setFirstSamplesSize(atoi(options[FIRST_SIZE].arg));
		}
		if(options[LAST_SIZE]) {
			scrubber.setLastSamplesSize(atoi(options[LAST_SIZE].arg));
		}
		if(options[MAX_OFFSET]) {
			scrubber.scrubFirstSamples(atoi(options[MAX_OFFSET].arg));
			scrubber.scrubLastSamples(atoi(options[MAX_OFFSET].arg));
			scrubber.scrubOtherSamples(atoi(options[MAX_OFFSET].arg));
		}
		if(options[FIRST_MAX_OFFSET]) {
			scrubber.scrubFirstSamples(atoi(options[FIRST_MAX_OFFSET].arg));
		}
		if(options[LAST_MAX_OFFSET]) {
			scrubber.scrubLastSamples(atoi(options[LAST_MAX_OFFSET].arg));
		}
		if(options[OTHER_MAX_OFFSET]) {
			scrubber.scrubOtherSamples(atoi(options[OTHER_MAX_OFFSET].arg));
		}
		if(options[RATE]) {
			scrubber.setFirstSamplesScrubRate(atof(options[RATE].arg));
			scrubber.setLastSamplesScrubRate(atof(options[RATE].arg));
			scrubber.setOtherSamplesScrubRate(atof(options[RATE].arg));
		}
		if(options[FIRST_RATE]) {
			scrubber.setFirstSamplesScrubRate(atof(options[FIRST_RATE].arg));
		}
		if(options[LAST_RATE]) {
			scrubber.setLastSamplesScrubRate(atof(options[LAST_RATE].arg));
		}
		if(options[OTHER_RATE]) {
			scrubber.setOtherSamplesScrubRate(atof(options[OTHER_RATE].arg));
		}
		if(options[FORCE_NONZERO]) {
			scrubber.setForceNonZero(true);
		}
		scrubber.processEverything(true);
		if(scrubber.hasError()) {
			scrubber.cancel();
			continue;
		}
		scrubber.overwrite();
	}
}

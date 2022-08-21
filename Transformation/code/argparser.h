#ifndef __ARG_PARSER_H__
#define __ARG_PARSER_H__

#include <cstdlib>
#include <iostream>
#include <cassert>
#include <string>
#include "MersenneTwister.h"

// ====================================================================
// ====================================================================

class ArgParser {

public:

  ArgParser() { DefaultValues(); }

  ArgParser(int argc, char *argv[]) {
    DefaultValues();
    // parse the command line arguments
    for (int i = 1; i < argc; i++) {
      if (argv[i] == std::string("-input")) {
	i++; assert (i < argc); 
	input_file = argv[i];
      } else if (argv[i] == std::string("-points")) {
	i++; assert (i < argc); 
	points = atoi(argv[i]);
      } else if (argv[i] == std::string("-iters")) {
	i++; assert (i < argc); 
	iters = atoi(argv[i]);
      } else if (argv[i] == std::string("-size")) {
	i++; assert (i < argc); 
	width = height = atoi(argv[i]);
      } else if (argv[i] == std::string("-cubes")) {
        cubes = true;
      } else {
	std::cout << "ERROR: unknown command line argument " << i << ": '" << argv[i] << "'" << std::endl;
	exit(1);
      }
    }
  }

  void DefaultValues() {
	input_file = "cube.txt";
    points = 10000;
    iters = 3;
    width = 400;
    height = 400;
    cubes = 0;
  }

  // ==============
  // REPRESENTATION
  // all public! (no accessors)

  std::string input_file;
  int points;
  int iters;
  int width;
  int height;
  int cubes;

  // default initialization
  MTRand mtrand;
};

#endif

#include "boost/program_options.hpp"

#define BOOST_FILESYSTEM_NO_DEPRECATED
#include "boost/filesystem.hpp"

#include "Molecule.h"
#include "IO.h"

#include <iostream>

int main(int argc, char* argv[]) {
  using namespace molassembler;

  // Set up option parsing
  boost::program_options::options_description options_description("Recognized options");
  options_description.add_options()
    ("help", "Produce help message")
    ("f", boost::program_options::value<std::string>(), "Which file to make an isomorphism to.")
  ;

  // Parse
  boost::program_options::variables_map options_variables_map;
  boost::program_options::store(
    boost::program_options::parse_command_line(argc, argv, options_description),
    options_variables_map
  );
  boost::program_options::notify(options_variables_map);

  if(options_variables_map.count("f")) {

    boost::filesystem::path filepath {
      options_variables_map["f"].as<std::string>()
    };

    if(!boost::filesystem::exists(filepath)) {
      std::cout << "That file does not exist!" << std::endl;
      return 0;
    }

    Molecule a = IO::read(
      filepath.string()
    );

    // TODO this is not ideal, reads the file twice
    IO::MOLFileHandler molHandler;

    molHandler.write(
      filepath.stem().string() + "_isomorphism.mol",
      a,
      molHandler.read(filepath.string()).atoms.getPositions(),
      IO::MOLFileHandler::IndexPermutation::Random
    );
  } else {
    std::cout << options_description << std::endl;
  }

  return 0;
}

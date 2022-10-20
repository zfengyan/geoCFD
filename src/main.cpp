﻿/*
* process geometry for cfd simulation
* main.cpp
*/

#include "JsonWriter.hpp"
#include "cmdline.h" // for cmd line parser
#include "MultiThread.hpp"



//#define _ENABLE_CONVEX_HULL_ // switch on/off convex hull method
#define _ENABLE_MULTI_THREADING_ // switch on/off multi-threading



/* user defined parameters --------------------------------------------------------------------------------------------------*/
double lod = 2.2; /* lod level */
double minkowski_param = 0.1; /* minkowski parameter */
/* user defined parameters --------------------------------------------------------------------------------------------------*/



/* optional parameters ------------------------------------------------------------------------------------------------------*/
unsigned int adjacency_size = 50; /* number of adjacent buildings in one block */
bool print_building_info = false; /* whether to print the building info to the console */
/* optional parameters ------------------------------------------------------------------------------------------------------*/



/* input files and output location ------------------------------------------------------------------------------------------*/
std::string srcFile = "D:\\SP\\geoCFD\\data\\3dbag_v210908_fd2cee53_5907.json";
std::string path = "D:\\SP\\geoCFD\\data";
std::string delimiter = "\\";
/* input files and output location ------------------------------------------------------------------------------------------*/



/* output files -------------------------------------------------------------------------------------------------------------*/
bool OUTPUT_JSON = true;
bool OUTPUT_STL = false; // currently STL output function is not working
bool OUTPUT_OFF = true;
/* output files -------------------------------------------------------------------------------------------------------------*/



// entry point
int main(int argc, char* argv[])
{
	std::cout << "this is: " << argv[0] << '\n';
	std::cout << "current lod level: " << lod << '\n';

	if (argc <= 1) {
		std::cout << "please provide adjacency file and multi thread tag" << std::endl;
		return 0;
	}

	// get adjacency file (if any)
	std::string adjacencyFile = argv[1];

	// get multi thread tag
	std::string _ENABLE_MULTITHREAD_ = argv[2];

	std::ifstream input(srcFile);
	if (!input.is_open()) {
		std::cerr << "Error: Unable to open cityjson file \"" << srcFile << "\" for reading!" << std::endl;
		return 1;
	}
	json j;
	input >> j;
	input.close();

	// get ids of adjacent buildings	
	std::vector<std::string> adjacency;
	adjacency.reserve(adjacency_size);
	FileIO::read_adjacency_from_txt(adjacencyFile, adjacency);

	// read buildings
	std::vector<JsonHandler> jhandles;
	jhandles.reserve(adjacency_size); // use reserve() to avoid extra copies


	if (print_building_info)std::cout << "------------------------ building(part) info ------------------------\n";

	for (auto const& building_name : adjacency) // get each building
	{
		JsonHandler jhandle;
		jhandle.read_certain_building(j, building_name, lod); // read in the building
		jhandles.emplace_back(jhandle); // add to the jhandlers vector

		if (print_building_info) {
			jhandle.message();
		}
	}

	if (print_building_info)std::cout << "---------------------------------------------------------------------\n";


	/* begin counting */
	Timer timer; // count the run time


	/* build the nef and stored in nefs vector */
	std::vector<Nef_polyhedron> nefs; // hold the nefs
	nefs.reserve(adjacency_size); // avoid reallocation, use reserve() whenever possible
	for (const auto& jhdl : jhandles) {
		Build::build_nef_polyhedron(jhdl, nefs); // triangulation tag can be passed as parameters, set to true by default
	}std::cout << "there are " << nefs.size() << " " << "nef polyhedra in total" << '\n';


	/* perform minkowski sum operation and store expanded nefs in nefs_expanded vector */
	std::vector<Nef_polyhedron> expanded_nefs;
	expanded_nefs.reserve(adjacency_size); // avoid reallocation, use reserve() whenever possible



	/* performing minkowski operations -------------------------------------------------------------------------*/
	std::cout << "performing minkowski sum ... " << '\n';
	if (_ENABLE_MULTITHREAD_ == "t" || _ENABLE_MULTITHREAD_ == "T") {
		std::cout << "multi threading is enabled" << '\n';
		MT::expand_nefs_async(nefs, expanded_nefs, minkowski_param);
	}
	else {
		MT::expand_nefs(nefs, expanded_nefs, minkowski_param);
	}
	std::cout << "done" << '\n';
	/* building nefs and performing minkowski operations -------------------------------------------------------------------------*/



	// merging nefs into one big nef
	std::cout << "building big nef ..." << '\n';
	Nef_polyhedron big_nef;
	for (auto& nef : expanded_nefs) {
		big_nef += nef;
	}
	std::cout << "done" << '\n';
	
	

#ifdef _ENABLE_CONVEX_HULL_
	/* get the convex hull of the big_nef, use all cleaned vertices of all shells */
	// get cleaned vertices of shell_explorers[0] - the shell indicating the exterior of the big nef
	std::vector<Point_3>& convex_vertices = shell_explorers[0].cleaned_vertices;

	// build convex hull of the big nef
	Polyhedron convex_polyhedron; // define polyhedron to hold convex hull
	Nef_polyhedron convex_big_nef;
	CGAL::convex_hull_3(convex_vertices.begin(), convex_vertices.end(), convex_polyhedron);
	std::cout << "is convex closed? " << convex_polyhedron.is_closed() << '\n';
	if (convex_polyhedron.is_closed()) {
		std::cout << "build convex hull for the big nef...\n";
		Nef_polyhedron convex_nef(convex_polyhedron);
		convex_big_nef = convex_nef;
		std::cout << "build convex hull for the big nef done\n";
	}

	// process the convex big nef to make it available for output
	std::vector<Shell_explorer> convex_shell_explorers;
	NefProcessing::extract_nef_geometries(convex_big_nef, convex_shell_explorers);
	NefProcessing::process_shells_for_cityjson(convex_shell_explorers);
#endif



	// extracting geometries
	std::vector<Shell_explorer> shell_explorers; // store the extracted geometries
	NefProcessing::extract_nef_geometries(big_nef, shell_explorers); // extract geometries of the bignef
	NefProcessing::process_shells_for_cityjson(shell_explorers); // process shells for writing to cityjson


    // write file
	// json
	if (OUTPUT_JSON) {
		std::string writeFilename = "interior_multi_m=0.1.json";
		const Shell_explorer& shell = shell_explorers[1]; // which shell is going to be written to the file, 0 - exterior, 1 - interior
		std::cout << "writing the result to cityjson file...\n";
		FileIO::write_JSON(path + delimiter + writeFilename, shell, lod);
	}
	
	// write file
	// OFF
	if (OUTPUT_OFF) {
		std::string writeFilename = "exterior_multi_m=0.1.off";
		std::cout << "writing the result to OFF file...\n";
		FileIO::write_OFF(path + delimiter + writeFilename, big_nef);
		//if (!status)return 1;
	}

	return EXIT_SUCCESS;
}
#pragma once

#include <helpers/RootDir.h>
#include <string>
#include <vector>

using std::string;

inline string appendToRootStr ( const string& str )
{
	return string ( ROOT_DIR ) + str;
}

inline string appendToRoot ( const char* str )
{
	return string ( ROOT_DIR ) + string ( str );
}

inline std::vector<const char*> getShaderFilenamesWithRoot ( const string& vtxFilename, const string& fragFilename )
{
	string vtxFilenameWRoot = appendToRootStr ( vtxFilename );
	string fragFilenameWRoot = appendToRootStr ( fragFilename );

	std::vector<const char*> shaderFilenames = {
		vtxFilenameWRoot.c_str (),
		fragFilenameWRoot.c_str ()
	};

	return shaderFilenames;
}

inline std::vector<const char*> getShaderFilenamesWithRoot ( const string& vtxFilename, const string& fragFilename, const string& geomFilename )
{
	string vtxFilenameWRoot = appendToRootStr ( vtxFilename );
	string fragFilenameWRoot = appendToRootStr ( fragFilename );
	string geomFilenameWRoot = appendToRootStr ( geomFilename );

	std::vector<const char*> shaderFilenames = {
		vtxFilenameWRoot.c_str (),
		fragFilenameWRoot.c_str (),
		geomFilenameWRoot.c_str()
	};

	return shaderFilenames;
}
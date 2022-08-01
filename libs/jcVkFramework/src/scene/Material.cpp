#include <jcVkFramework/scene/Material.h>
#include <jcVkFramework/Utils.h>
#include <unordered_map>

void saveStringList ( FILE* f, const vector<string>& lines )
{
	uint32_t sz = (uint32_t)lines.size ();
	fwrite ( &sz, sizeof ( uint32_t ), 1, f );
	for ( const auto& s : lines )
	{
		sz = (uint32_t)s.length ();
		fwrite ( &sz, sizeof ( uint32_t ), 1, f );
		fwrite ( s.c_str (), sz + 1, 1, f );
	}
}

void loadStringList ( FILE* f, vector<string>& lines )
{
	{
		uint32_t sz = 0;
		fread ( &sz, sizeof ( uint32_t ), 1, f );
		lines.resize ( sz );
	}

	vector<char> inBytes;
	for ( auto& s : lines )
	{
		uint32_t sz = 0;
		fread ( &sz, sizeof ( uint32_t ), 1, f );
		inBytes.resize ( sz + 1 );
		fread ( inBytes.data (), sz + 1, 1, f );
		s = string ( inBytes.data () );
	}
}

void saveMaterials ( const char* filename, const vector<MaterialDescription>& materials, const vector<string>& files )
{
	FILE* f = fopen ( filename, "wb" );
	if ( !f ) return;

	uint32_t sz = (uint32_t)materials.size ();
	fwrite ( &sz, 1, sizeof ( uint32_t ), f );
	fwrite ( materials.data (), sizeof ( MaterialDescription ), sz, f );
	saveStringList ( f, files );
	fclose ( f );
}

void loadMaterials ( const char* filename, vector<MaterialDescription>& materials, vector<string>& files )
{
	FILE* f = fopen ( filename, "rb" );
	if ( !f )
	{
		printf ( "Cannot load file %s\nPlease run SceneConverter tool from Chapter7\n", filename );
		exit ( 255 );
	}

	uint32_t sz;
	fread ( &sz, 1, sizeof ( uint32_t ), f );
	materials.resize ( sz );
	fread ( materials.data (), sizeof ( MaterialDescription ), materials.size (), f );
	loadStringList ( f, files );
	fclose ( f );
}

// TODO
void mergeMaterialLists ( const vector<vector<MaterialDescription>*>& oldMaterials, const vector<vector<string>*>& oldTextures, vector<MaterialDescription>& allMaterials, vector<string>& newTextures )
{}
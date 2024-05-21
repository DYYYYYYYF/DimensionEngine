#include "MeshLoader.h"

#include "Core/DMemory.hpp"
#include "Core/EngineLogger.hpp"

#include "Containers/TString.hpp"
#include "Platform/FileSystem.hpp"
#include "Systems/ResourceSystem.h"
#include "Systems/GeometrySystem.h"
#include "Math/GeometryUtils.hpp"

#include <vector>
#include <stdio.h>	//sscanf

MeshLoader::MeshLoader() {
	Type = ResourceType::eResource_type_Static_Mesh;
	CustomType = nullptr;
	TypePath = "Models";
}

bool MeshLoader::Load(const char* name, Resource* resource) {
	if (name == nullptr || resource == nullptr) {
		return false;
	}

	char* FormatStr = "%s/%s/%s%s";
	FileHandle f;

#define SUPPORTED_FILETYPE_COUNT 2
	SupportedMeshFileType SupportedFileTypes[SUPPORTED_FILETYPE_COUNT];
	SupportedFileTypes[0] = SupportedMeshFileType{ ".dsm", MeshFileType::eMesh_File_Type_DSM, true };
	SupportedFileTypes[1] = SupportedMeshFileType{ ".obj", MeshFileType::eMesh_File_Type_OBJ, false };

	char FullFilePath[512];
	MeshFileType Type = MeshFileType::eMesh_File_Type_Not_Found;
	// Try each supported extension.
	for (uint32_t i = 0; i < SUPPORTED_FILETYPE_COUNT; ++i) {
		sprintf(FullFilePath, FormatStr, ResourceSystem::GetRootPath(), TypePath, name, SupportedFileTypes[i].extension);
		// If the file exists, open it and stop finding.
		if (FileSystemExists(FullFilePath)) {
			if (FileSystemOpen(FullFilePath, FileMode::eFile_Mode_Read, SupportedFileTypes[i].is_binary, &f)) {
				Type = SupportedFileTypes[i].type;
				break;
			}
		}
	}

	if (Type == MeshFileType::eMesh_File_Type_Not_Found) {
		UL_ERROR("Unable to find mesh of supported type called '%s'.", name);
		return false;
	}

	resource->FullPath = StringCopy(FullFilePath);

	// The resource data is just an array of configs.
	std::vector<SGeometryConfig> ResourceDatas;
	ResourceDatas.reserve(25968);
	bool Result = false;
	switch (Type) {
	case MeshFileType::eMesh_File_Type_OBJ:
		// Generate the dsm filename.
		char DsmFileName[512];
		sprintf(DsmFileName, "%s/%s/%s%s", ResourceSystem::GetRootPath(), TypePath, name, ".dsm");
		Result = ImportObjFile(&f, DsmFileName, ResourceDatas);
		break;
	case MeshFileType::eMesh_File_Type_DSM:
		Result = LoadDsmFile(&f, ResourceDatas);
		break;
	case MeshFileType::eMesh_File_Type_Not_Found:
		UL_ERROR("Unable to find mesh of supported type called '%s'.", name);
		Result = false;
		break;
	}

	FileSystemClose(&f);

	if (!Result) {
		UL_ERROR("Failed to process mesh file '%s'.", FullFilePath);
		ResourceDatas.clear();
		resource->Data = nullptr;
		resource->DataSize = 0;
		return false;
	}

	resource->Data = Memory::Allocate(sizeof(SGeometryConfig) * ResourceDatas.size(), MemoryType::eMemory_Type_Array);
	Memory::Zero(resource->Data, sizeof(SGeometryConfig) * ResourceDatas.size());
	Memory::Copy(resource->Data, ResourceDatas.data(), sizeof(SGeometryConfig) * ResourceDatas.size());
	resource->DataSize = sizeof(SGeometryConfig);
	resource->DataCount = ResourceDatas.size();
	return true;
}

void MeshLoader::Unload(Resource* resource) {
	for (uint32_t i = 0; i < resource->DataCount; ++i) {
		SGeometryConfig* Config = &((SGeometryConfig*)resource->Data)[i];
		GeometrySystem::ConfigDispose(Config);
	}

	Memory::Free(resource->Data, resource->DataSize, MemoryType::eMemory_Type_Array);
	resource->Data = nullptr;
	resource->DataSize = 0;
	resource->DataCount = 0;
}

bool MeshLoader::ImportObjFile(FileHandle* obj_file, const char* out_dsm_filename, std::vector<SGeometryConfig>& out_geometries) {
	// Positions
	std::vector<Vec3> Positions;
	Positions.reserve(16384);
	// Normals
	std::vector<Vec3> Normals;
	Normals.reserve(16384);
	// Texcoords
	std::vector<Vec2> Texcoords;
	Texcoords.reserve(16384);

	//Groups
	std::vector<MeshGroupData> Groups;
	Groups.reserve(4);

	char MaterialFileName[512] = "";

	char name[512];
	unsigned short CurrentMatNameCount = 0;
	char MaterialNames[32][64];

	char LineBuf[512] = "";
	char* p = &LineBuf[0];
	size_t LineLength = 0;

	// index 0 is previous, 1 is previous before that.
	char PrevFirstChars[2] = { '\0', '\0'};
	while (true) {
		if (!FileSystemReadLine(obj_file, 511, &p, &LineLength)) {
			break;
		}

		// Skip blank lines.
		if (LineLength < 1) {
			continue;
		}

		char FirstChar = LineBuf[0];

		switch (FirstChar)
		{
		case '#':
			continue;
		case 'v':
		{
			char SecondChar = LineBuf[1];
			switch (SecondChar)
			{
			case ' ':{
				// Vertex position
				Vec3 Pos;
				char t[2];
				int Result = sscanf(LineBuf, "%s %f %f %f", t, &Pos.x, &Pos.y, &Pos.z);
				if (Result == -1) {
					UL_ERROR("Mesh loader ImportObjFile: sscanf failed. succeed num: %i  Line: %s", Result, LineBuf);
				}

				Pos.y *= -1.0f;
				Positions.push_back(Pos);
			}break;
			case 'n': {
				// Vertex normal
				Vec3 Norm;
				char t[3];
				int Result = sscanf(LineBuf, "%s %f %f %f", t, &Norm.x, &Norm.y, &Norm.z);
				if (Result == -1) {
					UL_ERROR("Mesh loader ImportObjFile: sscanf failed. succeed num: %i  Line: %s", Result, LineBuf);
				}

				Normals.push_back(Norm);
			}break;
			case 't': {
				// Vertex texcoords
				Vec2 Texcoord;
				char t[3];
				int Result = sscanf(LineBuf, "%s %f %f", t, &Texcoord.x, &Texcoord.y);
				if (Result == -1) {
					UL_ERROR("Mesh loader ImportObjFile: sscanf failed. succeed num: %i  Line: %s", Result, LineBuf);
				}

				Texcoords.push_back(Texcoord);
			}break;
			}
		}break;
		case 's': {

		}break;
		case 'f': {
			// face
			// f 1/2/3
			// f 1/1/1 2/2/2 3/3/3 = pos/tex/norm pos/tex/norm pos/tex/norm
			MeshFaceData Face;
			char t[3];

			size_t NormalCount = Normals.size();
			size_t TexcoordCount = Texcoords.size();
			if (NormalCount == 0 || TexcoordCount == 0) {
				int Result = sscanf(LineBuf, "%s %d %d %d", 
					t, 
					&Face.vertices[0].position_index, 
					&Face.vertices[1].position_index,
					&Face.vertices[2].position_index);
				if (Result == -1) {
					UL_ERROR("Mesh loader ImportObjFile: sscanf failed. succeed num: %i  Line: %s", Result, LineBuf);
				}
			}
			else {
				int Result = sscanf(LineBuf, "%s %d/%d/%d %d/%d/%d %d/%d/%d",
					t, 
					&Face.vertices[0].position_index, 
					&Face.vertices[0].texcoord_index, 
					&Face.vertices[0].normal_index,

					&Face.vertices[1].position_index, 
					&Face.vertices[1].texcoord_index, 
					&Face.vertices[1].normal_index,

					&Face.vertices[2].position_index, 
					&Face.vertices[2].texcoord_index, 
					&Face.vertices[2].normal_index);
				if (Result == -1) {
					UL_ERROR("Mesh loader ImportObjFile: sscanf failed. succeed num: %i  Line: %s", Result, LineBuf);
				}
			}

			size_t GroupIndex = Groups.size() - 1;
			Groups[GroupIndex].Faces.push_back(Face);
		}break;
		case 'm': {
			// Material library file.
			char SubStr[8];
			int Result = sscanf(LineBuf, "%s %s", SubStr, MaterialFileName);
			if (Result == -1) {
				UL_ERROR("Mesh loader ImportObjFile: sscanf failed. succeed num: %i  Line: %s", Result, LineBuf);
			}

			// If found, save off the material file name.
			if (StringNequali(SubStr, "mtllib", 6)) {
				// TODO verification
			}
		}break;
		case 'u': {
			// Any time there is a usemtl, assume a new group.
			// New named group or smoothing group, all faces coming after should be added to it.
			MeshGroupData NewGroup;
			NewGroup.Faces.reserve(16384);
			Groups.push_back(NewGroup);

			// usemtl
			// Read the material name.
			char t[8];
			int Result = sscanf(LineBuf, "%s %s", t, MaterialNames[CurrentMatNameCount]);
			if (Result == -1) {
				UL_ERROR("Mesh loader ImportObjFile: sscanf failed. succeed num: %i  Line: %s", Result, LineBuf);
			}

			strncpy(name, MaterialNames[CurrentMatNameCount], 511);
			CurrentMatNameCount++;
		} break;
		case 'g': {
			size_t GroupCount = Groups.size();
			// Process each group as a subobject.
			for (size_t i = 0; i < GroupCount; ++i) {
				SGeometryConfig NewData;
				strncpy(NewData.name, name, 255);

				if (i > 0) {
					String::Append(NewData.name, NewData.name, (int)i);
				}
				strncpy(NewData.material_name, MaterialNames[i], 255);

				ProcessSubobject(Positions, Normals, Texcoords, Groups[i].Faces, &NewData);
				out_geometries.push_back(NewData);

				// Increment the number of objects.
				Groups[i].Faces.clear();
				Memory::Zero(MaterialNames[i], 64);
			}

			CurrentMatNameCount = 0;
			Groups.clear();
			Memory::Zero(name, 512);

			// Read the name.
			char t[3];
			int Result = sscanf(LineBuf, "%s %s", t, name);
			if (Result == -1) {
				UL_ERROR("Mesh loader ImportObjFile: sscanf failed. succeed num: %i  Line: %s", Result, LineBuf);
			}
		}break;
		}

		PrevFirstChars[1] = PrevFirstChars[0];
		PrevFirstChars[0] = FirstChar;
	}	// Each line

	// Process the remaining group since the last one will not have been trigged
	// by the finding of a new name.
	// Process each group as a subobject.
	size_t GroupCount = Groups.size();
	SGeometryConfig NewData;
	for (size_t i = 0; i < GroupCount; ++i) {
		strncpy(NewData.name, name, 255);

		if (i > 0) {
			String::Append(NewData.name, NewData.name, (int)i);
		}
		strncpy(NewData.material_name, MaterialNames[i], 255);

		ProcessSubobject(Positions, Normals, Texcoords, Groups[i].Faces, &NewData);
		out_geometries.push_back(NewData);

		// Increment the number of objects.
		Groups[i].Faces.clear();
	}

	Groups.clear();
	Positions.clear();
	Normals.clear();
	Texcoords.clear();

	if (strlen(MaterialFileName) > 0) {
		// Load up the material file.
		char FullMtlPath[512];
		StringDirectoryFromPath(FullMtlPath, out_dsm_filename);
		String::Append(FullMtlPath, FullMtlPath, MaterialFileName);

		// Process material library file.
		if (!ImportObjMaterialLibraryFile(FullMtlPath)) {
			UL_ERROR("Error reading obj material file.");
		}
	}

	// De-duplicate geometry.
	size_t Count = out_geometries.size();
	uint32_t NewVertCount = 0;
	Vertex* UniqueVerts = nullptr;
	for (size_t i = 0; i < Count; ++i) {
		SGeometryConfig* g = &out_geometries[i];
		UL_DEBUG("Geometry de-duplication process starting on geometry object named '%s'.", g->name);
		GeometryUtils::DeduplicateVertices(g->vertex_count, (Vertex*)g->vertices, g->index_count, (uint32_t*)g->indices, &NewVertCount, &UniqueVerts);

		// Destroy the old, large array.
		Memory::Free(g->vertices, g->vertex_count * g->vertex_size, MemoryType::eMemory_Type_Array);

		// And replace with the de-duplicated one.
		g->vertex_count = NewVertCount;
		g->vertices = UniqueVerts;

		// Take a copy of the indices as a normal.
		uint32_t* Indices = (uint32_t*)Memory::Allocate(sizeof(uint32_t) * g->index_count, MemoryType::eMemory_Type_Array);
		Memory::Copy(Indices, g->indices, sizeof(uint32_t)* g->index_count);
		// Destroy.
		Memory::Free(g->indices, sizeof(uint32_t)* g->index_count, MemoryType::eMemory_Type_Array);
		g->indices = Indices;
	}

	// Output a dsm file, which will be loaded in the future.
	return WriteDsmFile(out_dsm_filename, name, (uint32_t)Count, out_geometries);
}

void MeshLoader::ProcessSubobject(std::vector<Vec3>& positions, std::vector<Vec3>& normals, std::vector<Vec2>& texcoords, std::vector<MeshFaceData>& faces, SGeometryConfig* out_data) {
	std::vector<uint32_t> Indices;
	std::vector<Vertex> Vertices;
	Indices.reserve(25968);
	Vertices.reserve(25968);
	
	bool ExtentSet = false;
	Memory::Zero(&out_data->min_extents, sizeof(Vec3));
	Memory::Zero(&out_data->max_extents, sizeof(Vec3));
	
	size_t FaceCount = faces.size();
	size_t NormalCount = normals.size();
	size_t TexcoordCount = texcoords.size();

	bool SkipNormal = false;
	bool SkipTexcoord = false;

	if (NormalCount == 0) {
		UL_WARN("No normals are present in this model.");
		SkipNormal = true;
	}

	if (TexcoordCount == 0) {
		UL_WARN("No tex-coord are present in this model.");
		SkipTexcoord = true;
	}

	for (size_t f = 0; f < FaceCount; ++f) {
		MeshFaceData Face = faces[f];

		// Each vertex
		for (size_t i = 0; i < 3; ++i) {
			MeshVertexIndexData IndexData = Face.vertices[i];
			Indices.push_back((uint32_t)(i + (f * 3)));

			Vertex Vert;
			Vec3 Pos = positions[IndexData.position_index - 1];
			Vert.position = Pos;

			// Check extens - min
			if (Pos.x < out_data->min_extents.x || !ExtentSet) {
				out_data->min_extents.x = Pos.x;
			}
			if (Pos.y < out_data->min_extents.y || !ExtentSet) {
				out_data->min_extents.y = Pos.y;
			}
			if (Pos.z < out_data->min_extents.z || !ExtentSet) {
				out_data->min_extents.z = Pos.z;
			}

			// Check extens - max
			if (Pos.x >out_data->min_extents.x || !ExtentSet) {
				out_data->max_extents.x = Pos.x;
			}
			if (Pos.y > out_data->min_extents.y || !ExtentSet) {
				out_data->max_extents.y = Pos.y;
			}
			if (Pos.z > out_data->min_extents.z || !ExtentSet) {
				out_data->max_extents.z = Pos.z;
			}

			ExtentSet = true;

			if (SkipNormal) {
				Vert.normal = Vec3(0, 0, 1);
			}
			else {
				Vert.normal = normals[IndexData.normal_index - 1];
			}

			if (SkipTexcoord) {
				Vert.texcoord = Vec2(0, 0);
			}
			else {
				Vert.texcoord = texcoords[IndexData.texcoord_index - 1];
			}

			// TODO: Color
			Vert.color = Vec4(1, 1, 1, 1);
			Vertices.push_back(Vert);
		}
	}

	out_data->vertex_count = (uint32_t)Vertices.size();
	out_data->vertex_size = sizeof(Vertex);
	out_data->vertices = Memory::Allocate(out_data->vertex_count * out_data->vertex_size, MemoryType::eMemory_Type_Array);
	Memory::Copy(out_data->vertices, Vertices.data(), out_data->vertex_count * out_data->vertex_size);

	out_data->index_count = (uint32_t)Indices.size();
	out_data->index_size = sizeof(uint32_t);
	out_data->indices = Memory::Allocate(out_data->index_count * out_data->index_size, MemoryType::eMemory_Type_Array);
	Memory::Copy(out_data->indices, Indices.data(), out_data->index_count * out_data->index_size);

	// Calculate the center based on the extents.
	for (unsigned short i = 0; i < 3; ++i) {
		out_data->center.elements[i] = (out_data->min_extents.elements[i] + out_data->max_extents.elements[i]) / 2.0f;
	}

	// Calculate tangents.
	GeometryUtils::GenerateTangents(out_data->vertex_count, (Vertex*)out_data->vertices, out_data->index_count, (uint32_t*)out_data->indices);
}

bool MeshLoader::ImportObjMaterialLibraryFile(const char* mtl_file_path) {
	UL_DEBUG("Importing obj .mtl file '%s'.", mtl_file_path);

	// Grab the .mtl file, if it exists, and read the material information.
	FileHandle MtlFile;
	if (!FileSystemOpen(mtl_file_path, FileMode::eFile_Mode_Read, false, &MtlFile)) {
		UL_ERROR("Unable to open mtl file: '%s'", mtl_file_path);
		return false;
	}

	SMaterialConfig CurrentConfig;
	Memory::Zero(&CurrentConfig, sizeof(CurrentConfig));

	bool HitName = false;

	char* Line = nullptr;
	char LineBuf[512];
	char* p = &LineBuf[0];
	size_t LineLength = 0;
	while (true) {
		if (!FileSystemReadLine(&MtlFile, 512, &p, &LineLength)) {
			break;
		}

		// Trim the line first.
		Line = Strtrim(LineBuf);
		LineLength = strlen(Line);

		// Skip blank lines.
		if (LineLength < 1) {
			continue;
		}

		char FirstChar = Line[0];
		switch (FirstChar)
		{
		case '#':
			continue;
		case 'K': {
			char SecondChar = Line[1];
			switch (SecondChar)
			{
			case 'a':
			case 'd':
			{
				// Ambient/Diffuse color are treated the same at this level.
				// ambient color is determined by the level.
				char t[3];
				int Result = sscanf(Line, "%s %f %f %f", t, &CurrentConfig.diffuse_color.r, &CurrentConfig.diffuse_color.g, &CurrentConfig.diffuse_color.b);
				if (Result == -1) {
					UL_ERROR("Mesh loader ImportObjFile: sscanf failed. succeed num: %i  Line: %s", Result, LineBuf);
				}

				// NOTE: This is only used by the color shader, and will set to max_norm by default.
				// Transparency could be added as a material property all its own at a later time.
				CurrentConfig.diffuse_color.a = 1.0f;
			}break;
			case 's':
			{
				// Specular color
				char t[3];

				// NOTE: Not using this for now.
				float SpecRubbish = 0.0f;
				int Result = sscanf(Line, "%s %f %f %f", t, &SpecRubbish, &SpecRubbish, &SpecRubbish);
				if (Result == -1) {
					UL_ERROR("Mesh loader ImportObjFile: sscanf failed. succeed num: %i  Line: %s", Result, LineBuf);
				}
			} break;
			}
		}break;
		case 'N':
		{
			char SecondChar = Line[1];
			switch (SecondChar)
			{
			case 's':
			{
				// Specular exponent.
				char t[3];
				int Result = sscanf(Line, "%s %f", t, &CurrentConfig.shininess);
				if (Result == -1) {
					UL_ERROR("Mesh loader ImportObjFile: sscanf failed. succeed num: %i  Line: %s", Result, LineBuf);
				}
			}break;
			}
		} break;
		case 'm':
		{
			// Map
			char SubStr[10];
			char TextureFileName[512];

			int Result = sscanf(Line, "%s %s", SubStr, TextureFileName);
			if (Result == -1) {
				UL_ERROR("Mesh loader ImportObjFile: sscanf failed. succeed num: %i  Line: %s", Result, LineBuf);
			}

			if (StringNequali(SubStr, "map_Kd", 6)) {
				// Is a diffuse texture map.
				StringFilenameNoExtensionFromPath(CurrentConfig.diffuse_map_name, TextureFileName);
			}
			else if (StringNequali(SubStr, "map_Ks", 6)) {
				// Is a specular texture map.
				StringFilenameNoExtensionFromPath(CurrentConfig.specular_map_name, TextureFileName);
			}
			else if (StringNequali(SubStr, "map_bump", 8)) {
				// Is a normal texture map.
				StringFilenameNoExtensionFromPath(CurrentConfig.normal_map_name, TextureFileName);
			}
		}break;
		case 'b':
		{
			// Some implementations use 'bump' instead of 'map_bump'.
			char SubStr[10];
			char TextureFileName[512];

			int Result = sscanf(Line, "%s %s", SubStr, TextureFileName);
			if (Result == -1) {
				UL_ERROR("Mesh loader ImportObjFile: sscanf failed. succeed num: %i  Line: %s", Result, LineBuf);
			}

			if (StringNequali(SubStr, "bump", 4)) {
				// Is a bump(normal) texture map.
				StringFilenameNoExtensionFromPath(CurrentConfig.normal_map_name, TextureFileName);
			}
		} break;
		case 'n':
		{
			// Some implementations use 'bump' instead of 'map_bump'.
			char SubStr[10];
			char MaterialName[512];

			int Result = sscanf(Line, "%s %s", SubStr, MaterialName);
			if (Result == -1) {
				UL_ERROR("Mesh loader ImportObjFile: sscanf failed. succeed num: %i  Line: %s", Result, LineBuf);
			}

			if (StringNequali(SubStr, "newmtl", 6)) {
				// Is a material name.

				// NOTE: Hardcoding default material shader name because all objects imported this way
				// will be treated same.
				CurrentConfig.shader_name = "Shader.Builtin.Material";
				// NOTE:Shininess of 0 will cause problems in the shader. Use default.
				if (CurrentConfig.shininess == 0.0f) {
					CurrentConfig.shininess = 8.0f;
				}
				if (HitName) {
					// Write out a dmt file and move on.
					if (!WriteDmtFile(mtl_file_path, &CurrentConfig)) {
						UL_ERROR("Unable to write dmt file.");
						return false;
					}

					// Reset material for the next round.
					Memory::Zero(&CurrentConfig, sizeof(CurrentConfig));
				}

				HitName = true;
				strncpy(CurrentConfig.name, MaterialName, 256);
			}
		}
		}
	}	// Each line

	// Write out the remaining dmt file.
	// NOTE: Hardcoding default material shader name because all objects imported this way
	// will be treated same.
	CurrentConfig.shader_name = "Shader.Builtin.Material";
	// NOTE:Shininess of 0 will cause problems in the shader. Use default.
	if (CurrentConfig.shininess == 0.0f) {
		CurrentConfig.shininess = 8.0f;
	}

	if (!WriteDmtFile(mtl_file_path, &CurrentConfig)) {
		UL_ERROR("Unable to write dmt file.");
		return false;
	}

	FileSystemClose(&MtlFile);
	return true;
}

bool MeshLoader::WriteDmtFile(const char* mtl_file_path, SMaterialConfig* config) {
	// NOTE: The .obj file this came from (and resulting .mtl file) sit in the
	// models directory. This moves up a level and back into the materials folder.
	// TODO: Read from config and get an avsolute path for output.
	char* FormatStr = "%s../Materials/%s%s";
	FileHandle f;
	char Directory[320];
	StringDirectoryFromPath(Directory, mtl_file_path);

	char FullFilePath[512];
	sprintf(FullFilePath, FormatStr, Directory, config->name, ".dmt");
	if (!FileSystemOpen(FullFilePath, FileMode::eFile_Mode_Write, false, &f)) {
		UL_ERROR("Error opening material file for writing: '%s'.", FullFilePath);
		return false;
	}
	UL_DEBUG("Writing .dmt file '%s'.", FullFilePath);

	char LineBuf[512];
	FileSystemWriteLine(&f, "#material file");
	FileSystemWriteLine(&f, "");
	FileSystemWriteLine(&f, "version=0.1");	// TODO: hardcoded version.
	sprintf(LineBuf, "name=%s", config->name);
	FileSystemWriteLine(&f, LineBuf);
	sprintf(LineBuf, "diffuse_color=%.6f %.6f %.6f %.6f", config->diffuse_color.r, config->diffuse_color.g, config->diffuse_color.b, config->diffuse_color.a);
	FileSystemWriteLine(&f, LineBuf);
	sprintf(LineBuf, "shininess=%.6f", config->shininess);
	FileSystemWriteLine(&f, LineBuf);
	if (config->diffuse_map_name[0]) {
		sprintf(LineBuf, "diffuse_map_name=%s", config->diffuse_map_name);
		FileSystemWriteLine(&f, LineBuf);
	}
	if (config->specular_map_name[0]) {
		sprintf(LineBuf, "specular_map_name=%s", config->specular_map_name);
		FileSystemWriteLine(&f, LineBuf);
	}
	if (config->normal_map_name[0]) {
		sprintf(LineBuf, "normal_map_name=%s", config->normal_map_name);
		FileSystemWriteLine(&f, LineBuf);
	}
	sprintf(LineBuf, "shader=%s", config->shader_name);
	FileSystemWriteLine(&f, LineBuf);

	FileSystemClose(&f);
	return true;
}

bool MeshLoader::LoadDsmFile(FileHandle* dsm_file, std::vector<SGeometryConfig>& out_geometries) {
	// TODO:
	return true;
}

bool MeshLoader::WriteDsmFile(const char* path, const char* name, uint32_t geometry_count, std::vector<SGeometryConfig>& geometries) {
	// TODO:
	return true;
}


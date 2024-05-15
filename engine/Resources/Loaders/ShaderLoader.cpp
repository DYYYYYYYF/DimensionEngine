#include "ShaderLoader.h"

#include "Core/DMemory.hpp"
#include "Core/EngineLogger.hpp"

#include "Resources/Shader.hpp"
#include "Systems/ResourceSystem.h"
#include "Platform/FileSystem.hpp"
#include "Containers/TString.hpp"

ShaderLoader::ShaderLoader() {
	Type = eResource_type_Binary;
	CustomType = nullptr;
	TypePath = "";
}

bool ShaderLoader::Load(const char* name, Resource* resource) {
	if (name == nullptr || resource == nullptr) {
		return false;
	}

	char* FormatStr = "%s/%s/%s%s";
	char FullFilePath[512];
	sprintf(FullFilePath, FormatStr, ResourceSystem::GetRootPath(), TypePath, name, ".scfg");	// shader config

	// TODO: Should be using an allocator here.
	size_t FullPathLength = sizeof(char) * strlen(FullFilePath);
	resource->FullPath = (char*)Memory::Allocate(FullPathLength, MemoryType::eMemory_Type_String);
	strncpy(resource->FullPath, FullFilePath, FullPathLength);

	FileHandle File;
	if (!FileSystemOpen(FullFilePath, eFile_Mode_Read, true, &File)) {
		UL_ERROR("Shader loader load. Unable to open file for binary reading: '%s'.", FullFilePath);
		return false;
	}

	// Set some defaults, create arrays.
	ShaderConfig* ResourceData = (ShaderConfig*)Memory::Allocate(sizeof(ShaderConfig), MemoryType::eMemory_Type_Resource);
	ResourceData->attribute_count = 0;
	ResourceData->uniform_count = 0;
	ResourceData->use_instances = false;
	ResourceData->use_local = false;
	ResourceData->renderpass_name = nullptr;
	ResourceData->name = nullptr;

	// Read each line of the file.
	char LineBuf[512] = "";
	char* p = &LineBuf[0];
	size_t LineLength = 0;
	uint32_t LineNumber = 1;
	while (FileSystemRead(&File, 511, &p, &LineLength)) {
		// Trim the string.
		char* Trimmed = Strtrim(LineBuf);

		// Get the trimmed length.
		LineLength = strlen(Trimmed);

		// Skip blank lines and comments.
		if (LineLength < 1 || Trimmed[0] == '#') {
			LineNumber++;
			continue;
		}

		// Split into var/ value
		int EqualIndex = StringIndexOf(Trimmed, '=');
		if (EqualIndex == -1) {
			UL_WARN("Potential formatting issue found in file '%s': '=' token not found. Skipping line %ui.", FullFilePath, LineNumber);
			LineNumber++;
			continue;
		}

		// Assume a max of 64 characters for the variable name.
		char RawVarName[64];
		Memory::Zero(RawVarName, sizeof(char) * 64);
		StringMid(RawVarName, Trimmed, 0, EqualIndex);
		char* TrimmedVarName = Strtrim(RawVarName);

		// Assume a max of 511-64 (446) for the max length of the value to account for the variable name and the '='.
		char RawValue[446];
		Memory::Zero(RawValue, sizeof(char) * 446);
		StringMid(RawValue, Trimmed, EqualIndex + 1, -1);
		char* TrimmedValue = Strtrim(RawValue);

		// Process the variable.
		if (strcmp(RawVarName, "version") == 0) {
			// TODO: version.
		}
		else if (strcmp(RawVarName, "name") == 0) {
			strcpy(ResourceData->name, TrimmedValue);
		}
		else if (strcmp(RawVarName, "renderpass") == 0) {
			strcpy(ResourceData->renderpass_name, TrimmedValue);
		}
		else if (strcmp(RawVarName, "stages") == 0) {
			// Parse the stages.
			std::vector<char*> StageNames = StringSplit(TrimmedValue, ',', true, true);
			ResourceData->stage_names = StageNames;

			// Ensue stage name and stage filename count are the same.
			if (ResourceData->stage_cout == 0) {
				ResourceData->stage_cout = (unsigned short)StageNames.size();
			}
			else if (ResourceData->stage_cout != StageNames.size()) {
				UL_ERROR("shader_loader_load: Invalid file layout. Count mismatch between stage names and stage filenames.");
			}

			// Parse each stage and add the right type to the array.
			for (unsigned short i = 0; i < ResourceData->stage_cout; ++i) {
				if (strcmp(StageNames[i], "frag") == 0 || strcmp(StageNames[i], "fragment") == 0) {
					ResourceData->stages.push_back(ShaderStage::eShader_Stage_Fragment);
				}
				else if (strcmp(StageNames[i], "vert") == 0 || strcmp(StageNames[i], "vertex") == 0) {
					ResourceData->stages.push_back(ShaderStage::eShader_Stage_Vertex);
				}
				else if (strcmp(StageNames[i], "geom") == 0 || strcmp(StageNames[i], "geometry") == 0) {
					ResourceData->stages.push_back(ShaderStage::eShader_Stage_Geometry);
				}
				else if (strcmp(StageNames[i], "comp") == 0 || strcmp(StageNames[i], "compute") == 0) {
					ResourceData->stages.push_back(ShaderStage::eShader_Stage_Compute);
				}
				else {
					UL_ERROR("shader_loader_load: Invalid file layout. Unrecognized stage '%s'", StageNames[i]);
				}
			}
		}
		else if (strcmp(TrimmedVarName, "stagefiles") == 0) {
			ResourceData->stage_filenames = StringSplit(TrimmedValue, ',', true, true);
			if (ResourceData->stage_cout == 0) {
				ResourceData->stage_cout = (unsigned short)ResourceData->stage_filenames.size();
			}
			else if (ResourceData->stage_cout != ResourceData->stage_filenames.size()) {
				UL_ERROR("shader_loader_load: Invalid file layout. Attribute fields must be 'type,name'. Skipping.");
			}
		}
		else if (strcmp(TrimmedVarName, "use_instance") == 0) {
			ResourceData->use_instances = StringToBool(TrimmedValue);
		}
		else if (strcmp(TrimmedVarName, "use_local") == 0) {
			ResourceData->use_local = StringToBool(TrimmedValue);
		}
		else if (strcmp(TrimmedVarName, "attribute") == 0) {
			// Parse attribute.
			std::vector<char*> Fields = StringSplit(TrimmedValue, ',', true, true);
			if (Fields.size() != 2) {
				UL_ERROR("shader_loader_load: Invalid file layout. Attribute fields must be 'type,name'. Skipping.");
			}
			else {
				ShaderAttributeConfig Attribute;
				// Parse field type.
				if (strcmp(Fields[0], "float") == 0) {
					Attribute.type = ShaderAttributeType::eShader_Attribute_Type_Float;
					Attribute.size = 4;
				}
				else if (strcmp(Fields[0], "vec2") == 0) {
					Attribute.type = ShaderAttributeType::eShader_Attribute_Type_Float_2;
					Attribute.size = 8;
				}
				else if (strcmp(Fields[0], "vec3") == 0) {
					Attribute.type = ShaderAttributeType::eShader_Attribute_Type_Float_3;
					Attribute.size = 12;
				}
				else if (strcmp(Fields[0], "vec4") == 0) {
					Attribute.type = ShaderAttributeType::eShader_Attribute_Type_Float_4;
					Attribute.size = 16;
				}
				else if (strcmp(Fields[0], "u8") == 0) {
					Attribute.type = ShaderAttributeType::eShader_Attribute_Type_UInt8;
					Attribute.size = 1;
				}
				else if (strcmp(Fields[0], "u16") == 0) {
					Attribute.type = ShaderAttributeType::eShader_Attribute_Type_UInt16;
					Attribute.size = 2;
				}
				else if (strcmp(Fields[0], "u32") == 0) {
					Attribute.type = ShaderAttributeType::eShader_Attribute_Type_UInt32;
					Attribute.size = 4;
				}
				else if (strcmp(Fields[0], "i8") == 0) {
					Attribute.type = ShaderAttributeType::eShader_Attribute_Type_Int8;
					Attribute.size = 1;
				}
				else if (strcmp(Fields[0], "i16") == 0) {
					Attribute.type = ShaderAttributeType::eShader_Attribute_Type_Int16;
					Attribute.size = 2;
				}
				else if (strcmp(Fields[0], "i32") == 0) {
					Attribute.type = ShaderAttributeType::eShader_Attribute_Type_Int32;
					Attribute.size = 4;
				}
				else {
					UL_ERROR("shader_loader_load: Invalid file layout. Attribute type must be f32, vec2, vec3, vec4, i8, i16, i32, u8, u16, or u32.");
					UL_WARN("Defaulting to f32.");
					Attribute.type = ShaderAttributeType::eShader_Attribute_Type_Float;
					Attribute.size = 4;
				}

				// Take a copy of the attribute name.
				Attribute.name_length = (unsigned short)strlen(Fields[1]);
				strcpy(Attribute.name, Fields[1]);

				// Add the attribute.
				ResourceData->attributes.push_back(Attribute);
				ResourceData->attribute_count++;
			}

			Fields.clear();
			// TODO: Free Memory in fields.
		}
		else if (strcmp(TrimmedVarName, "uniform") == 0) {
			// Parse field type.
			std::vector<char*> Fields = StringSplit(TrimmedValue, ',', true, true);
			if (Fields.size() != 3) {
				UL_ERROR("shader_loader_load: Invalid file layout. Uniform fields must be 'type,scope,name'. Skipping.");
			}
			else {
				ShaderUniformConfig Uniform;
				if (strcmp(Fields[0], "float") == 0) {
					Uniform.type = ShaderUniformType::eShader_Uniform_Type_Float;
					Uniform.size = 4;
				}
				else if (strcmp(Fields[0], "vec2") == 0) {
					Uniform.type = ShaderUniformType::eShader_Uniform_Type_Float_2;
					Uniform.size = 8;
				}
				else if (strcmp(Fields[0], "vec3") == 0) {
					Uniform.type = ShaderUniformType::eShader_Uniform_Type_Float_3;
					Uniform.size = 12;
				}
				else if (strcmp(Fields[0], "vec4") == 0) {
					Uniform.type = ShaderUniformType::eShader_Uniform_Type_Float_4;
					Uniform.size = 16;
				}
				else if (strcmp(Fields[0], "u8") == 0) {
					Uniform.type = ShaderUniformType::eShader_Uniform_Type_UInt8;
					Uniform.size = 1;
				}
				else if (strcmp(Fields[0], "u16") == 0) {
					Uniform.type = ShaderUniformType::eShader_Uniform_Type_UInt16;
					Uniform.size = 2;
				}
				else if (strcmp(Fields[0], "u32") == 0) {
					Uniform.type = ShaderUniformType::eShader_Uniform_Type_UInt32;
					Uniform.size = 4;
				}
				else if (strcmp(Fields[0], "i8") == 0) {
					Uniform.type = ShaderUniformType::eShader_Uniform_Type_Int8;
					Uniform.size = 1;
				}
				else if (strcmp(Fields[0], "i16") == 0) {
					Uniform.type = ShaderUniformType::eShader_Uniform_Type_Int16;
					Uniform.size = 2;
				}
				else if (strcmp(Fields[0], "i32") == 0) {
					Uniform.type = ShaderUniformType::eShader_Uniform_Type_Int32;
					Uniform.size = 4;
				}
				else if (strcmp(Fields[0], "mat4") == 0 || strcmp(Fields[0], "matrix") == 0) {
					Uniform.type = ShaderUniformType::eShader_Uniform_Type_Matrix;
					Uniform.size = 4;
				}
				else if (strcmp(Fields[0], "samp") == 0 || strcmp(Fields[0], "sampler") == 0) {
					Uniform.type = ShaderUniformType::eShader_Uniform_Type_Sampler;
					Uniform.size = 4;
				}
				else {
					UL_ERROR("shader_loader_load: Invalid file layout. Uniform type must be f32, vec2, vec3, vec4, i8, i16, i32, u8, u16, u32 or mat4.");
					UL_WARN("Defaulting to f32.");
					Uniform.type = ShaderUniformType::eShader_Uniform_Type_Float;
					Uniform.size = 4;
				}

				// Parse the scope.
				if (strcmp(Fields[1], "0") == 0) {
					Uniform.scope = ShaderScope::eShader_Scope_Global;
				}
				else if (strcmp(Fields[1], "1") == 0) {
					Uniform.scope = ShaderScope::eShader_Scope_Instance;
				}
				else if (strcmp(Fields[1], "2") == 0) {
					Uniform.scope = ShaderScope::eShader_Scope_Local;
				}
				else {
					UL_ERROR("shader_loader_load: Invalid file layout: Uniform scope must be 0 for global, 1 for instance or 2 for local.");
					UL_WARN("Defaulting to global.");
					Uniform.scope = ShaderScope::eShader_Scope_Global;
				}

				// Take a copy of the attribute name.
				Uniform.name_length = (unsigned short)strlen(Fields[2]);
				strcpy(Uniform.name, Fields[2]);

				// Add the attribute.
				ResourceData->uniforms.push_back(Uniform);
				ResourceData->uniform_count++;
			}

			Fields.clear();
		}

		// TODO: more fields.

		// Clear the line buffer.
		Memory::Free(LineBuf, sizeof(char) * 512, MemoryType::eMemory_Type_String);
		LineNumber++;
	}

	FileSystemClose(&File);

	resource->Data = ResourceData;
	resource->DataSize = sizeof(ShaderConfig);

	return false;
}

void ShaderLoader::Unload(Resource* resource) {
	ShaderConfig* Data = (ShaderConfig*)resource->Data;

	Data->stage_filenames.clear();
	Data->stage_names.clear();

	// Clean up attributes.
	uint32_t Count = (uint32_t)Data->attributes.size();
	for (uint32_t i = 0; i < Count; ++i) {
		uint32_t Len = (uint32_t)strlen(Data->attributes[i].name);
		Memory::Free(Data->attributes[i].name, sizeof(char) * (Len + 1), eMemory_Type_String);
	}
	Data->attributes.clear();

	// Clean up uniforms.
	Count = (uint32_t)Data->uniforms.size();
	for (uint32_t i = 0; i < Count; ++i) {
		uint32_t Len = (uint32_t)strlen(Data->uniforms[i].name);
		Memory::Free(Data->uniforms[i].name, sizeof(char) * (Len + 1), eMemory_Type_String);
	}
	Data->uniforms.clear();

	Memory::Free(Data->renderpass_name, sizeof(char) * (strlen(Data->renderpass_name) + 1), eMemory_Type_String);
	Memory::Free(Data->name, sizeof(char) * (strlen(Data->name) + 1), eMemory_Type_String);
	Memory::Zero(Data, sizeof(ShaderConfig));

}
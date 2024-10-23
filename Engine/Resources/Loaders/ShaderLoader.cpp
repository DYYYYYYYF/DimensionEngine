#include "ShaderLoader.h"

#include "Core/DMemory.hpp"
#include "Core/EngineLogger.hpp"

#include "Resources/Shader.hpp"
#include "Systems/ResourceSystem.h"
#include "Platform/FileSystem.hpp"
#include "Containers/TString.hpp"

ShaderLoader::ShaderLoader() {
	Type = eResource_Type_Shader;
	CustomType = nullptr;
	TypePath = "Shaders";
}

bool ShaderLoader::Load(const char* name, void* params, Resource* resource) {
	if (name == nullptr || resource == nullptr) {
		return false;
	}

	const char* FormatStr = "%s/%s/%s%s";
	char FullFilePath[512];
	StringFormat(FullFilePath, 512, FormatStr, ResourceSystem::GetRootPath(), TypePath, name, ".scfg");	// shader config


	FileHandle File;
	if (!FileSystemOpen(FullFilePath, eFile_Mode_Read, false, &File)) {
		LOG_ERROR("Shader loader load. Unable to open file for binary reading: '%s'.", FullFilePath);
		return false;
	}
	resource->FullPath = StringCopy(FullFilePath);
	resource->Name = StringCopy(name);

	// Set some defaults, create arrays.
	ShaderConfig* ResourceData = (ShaderConfig*)Memory::Allocate(sizeof(ShaderConfig), MemoryType::eMemory_Type_Resource);
	ResourceData = new(ResourceData)ShaderConfig();
	ASSERT(ResourceData);

	ResourceData->cull_mode = FaceCullMode::eFace_Cull_Mode_Back;
	ResourceData->polygon_mode = PolygonMode::ePology_Mode_Fill;
	ResourceData->name = nullptr;

	// Read each line of the file.
	char LineBuf[512] = "";
	char* p = &LineBuf[0];
	size_t LineLength = 0;
	uint32_t LineNumber = 1;
	while (FileSystemReadLine(&File, 511, &p, &LineLength)) {
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
			LOG_WARN("Potential formatting issue found in file '%s': '=' token not found. Skipping line %ui.", FullFilePath, LineNumber);
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
			ResourceData->name = StringCopy(TrimmedValue);
		}
		else if (strcmp(RawVarName, "renderpass") == 0) {
			// ResourceData->renderpass_name = StringCopy(TrimmedValue);
		}
		else if (strcmp(RawVarName, "stages") == 0) {
			// Parse the stages.
			std::vector<char*> StageNames = StringSplit(TrimmedValue, ',', true, true);
			ResourceData->stage_names = StageNames;

			// Ensue stage name and stage filename count are the same.
			ResourceData->stages.resize(StageNames.size());

			// Parse each stage and add the right type to the array.
			for (unsigned short i = 0; i < ResourceData->stages.size(); ++i) {
				if (strcmp(StageNames[i], "frag") == 0 || strcmp(StageNames[i], "fragment") == 0) {
					ResourceData->stages[i] = ShaderStage::eShader_Stage_Fragment;
				}
				else if (strcmp(StageNames[i], "vert") == 0 || strcmp(StageNames[i], "vertex") == 0) {
					ResourceData->stages[i] = ShaderStage::eShader_Stage_Vertex;
				}
				else if (strcmp(StageNames[i], "geom") == 0 || strcmp(StageNames[i], "geometry") == 0) {
					ResourceData->stages[i] = ShaderStage::eShader_Stage_Geometry;
				}
				else if (strcmp(StageNames[i], "comp") == 0 || strcmp(StageNames[i], "compute") == 0) {
					ResourceData->stages[i] = ShaderStage::eShader_Stage_Compute;
				}
				else {
					LOG_ERROR("shader_loader_load: Invalid file layout. Unrecognized stage '%s'", StageNames[i]);
				}
			}
		}
		else if (strcmp(TrimmedVarName, "stagefiles") == 0) {
			ResourceData->stage_filenames = StringSplit(TrimmedValue, ',', true, true);
			if (ResourceData->stages.size() != ResourceData->stage_filenames.size()) {
				LOG_ERROR("shader_loader_load: Invalid file layout. Attribute fields must be 'type,name'. Skipping.");
			}
		}
		else if (strcmp(TrimmedVarName, "cull_mode") == 0) {
			if (strcmp(TrimmedValue, "front") == 0) {
				ResourceData->cull_mode = FaceCullMode::eFace_Cull_Mode_Front;
			}
			else if (strcmp(TrimmedValue, "front_and_back") == 0) {
				ResourceData->cull_mode = FaceCullMode::eFace_Cull_Mode_Front_And_Back;
			}
			else if (strcmp(TrimmedValue, "none") == 0) {
				ResourceData->cull_mode = FaceCullMode::eFace_Cull_Mode_None;
			}
		}
		else if (strcmp(TrimmedVarName, "polygon_mode") == 0) {
			if (strcmp(TrimmedValue, "line") == 0) {
				ResourceData->polygon_mode = PolygonMode::ePology_Mode_Line;
			}
			else if (strcmp(TrimmedValue, "fill") == 0) {
				ResourceData->polygon_mode = PolygonMode::ePology_Mode_Fill;
			}
		}
		else if (strcmp(TrimmedVarName, "depth_test") == 0) {
			ResourceData->depthTest = StringToBool(TrimmedValue);
		}
		else if (strcmp(TrimmedVarName, "depth_write") == 0) {
			ResourceData->depthWrite = StringToBool(TrimmedValue);
		}
		else if (strcmp(TrimmedVarName, "attribute") == 0) {
			// Parse attribute.
			std::vector<char*> Fields = StringSplit(TrimmedValue, ',', true, true);
			if (Fields.size() != 2) {
				LOG_ERROR("shader_loader_load: Invalid file layout. Attribute fields must be 'type,name'. Skipping.");
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
					LOG_ERROR("shader_loader_load: Invalid file layout. Attribute type must be float, vec2, vec3, vec4, i8, i16, i32, u8, u16, or u32.");
					LOG_WARN("Defaulting to float.");
					Attribute.type = ShaderAttributeType::eShader_Attribute_Type_Float;
					Attribute.size = 4;
				}

				// Take a copy of the attribute name.
				Attribute.name_length = (unsigned short)strlen(Fields[1]);
				Attribute.name = StringCopy(Fields[1]);

				// Add the attribute.
				ResourceData->attributes.push_back(Attribute);
			}

			for (uint32_t i = 0; i < Fields.size(); ++i) {
				StringFree(Fields[i]);
			}
			Fields.clear();
			// TODO: Free Memory in fields.
		}
		else if (strcmp(TrimmedVarName, "uniform") == 0) {
			// Parse field type.
			std::vector<char*> Fields = StringSplit(TrimmedValue, ',', true, true);
			if (Fields.size() != 3) {
				LOG_ERROR("shader_loader_load: Invalid file layout. Uniform fields must be 'type,scope,name'. Skipping.");
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
					Uniform.size = 64;
				}
				else if (strcmp(Fields[0], "samp") == 0 || strcmp(Fields[0], "sampler") == 0) {
					Uniform.type = ShaderUniformType::eShader_Uniform_Type_Sampler;
					Uniform.size = 0;
				}
				else {
					LOG_ERROR("shader_loader_load: Invalid file layout. Uniform type must be f32, vec2, vec3, vec4, i8, i16, i32, u8, u16, u32 or mat4.");
					LOG_WARN("Defaulting to f32.");
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
					LOG_ERROR("shader_loader_load: Invalid file layout: Uniform scope must be 0 for global, 1 for instance or 2 for local.");
					LOG_WARN("Defaulting to global.");
					Uniform.scope = ShaderScope::eShader_Scope_Global;
				}

				// Take a copy of the uniform name.
				Uniform.name_length = (unsigned short)strlen(Fields[2]);
				Uniform.name = StringCopy(Fields[2]);

				// Add the uniform.
				ResourceData->uniforms.push_back(Uniform);
			}

			for (uint32_t i = 0; i < Fields.size(); ++i) {
				size_t len = strlen(Fields[i]);
				Memory::Free(Fields[i], sizeof(char) * (len + 1), MemoryType::eMemory_Type_String);
			}
			Fields.clear();
		}

		// TODO: more fields.

		// Clear the line buffer.
		Memory::Zero(LineBuf, sizeof(char) * 512);
		LineNumber++;
	}

	FileSystemClose(&File);

	resource->Data = ResourceData;
	resource->DataCount = 1;
	resource->DataSize = sizeof(ShaderConfig);

	return true;
}

void ShaderLoader::Unload(Resource* resource) {
	ShaderConfig* Data = (ShaderConfig*)resource->Data;

	for (uint32_t i = 0; i < Data->stage_filenames.size(); ++i) {
		size_t len = strlen(Data->stage_filenames[i]);
		Memory::Free(Data->stage_filenames[i], sizeof(char) * (len + 1), MemoryType::eMemory_Type_String);
	}
	Data->stage_filenames.clear();

	for (uint32_t i = 0; i < Data->stage_names.size(); ++i) {
		size_t len = strlen(Data->stage_names[i]);
		Memory::Free(Data->stage_names[i], sizeof(char) * (len + 1), MemoryType::eMemory_Type_String);
	}
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

	if (Data->name) {
		Memory::Free(Data->name, sizeof(char) * (strlen(Data->name) + 1), MemoryType::eMemory_Type_String);
		Data->name = nullptr;
	}

	if (resource->Name) {
		Memory::Free(resource->Name, sizeof(char) * (strlen(resource->Name) + 1), MemoryType::eMemory_Type_String);
		resource->Name = nullptr;
	}

	if (resource->FullPath) {
		Memory::Free(resource->FullPath, sizeof(char) * (strlen(resource->FullPath) + 1), MemoryType::eMemory_Type_String);
		resource->FullPath = nullptr;
	}

	if (resource->Data) {
		Memory::Free(resource->Data, resource->DataSize * resource->DataCount, MemoryType::eMemory_Type_Texture);
		resource->Data = nullptr;
		resource->DataSize = 0;
		resource->DataCount = 0;
		resource->LoaderID = INVALID_ID;
	}

	resource = nullptr;
}

#pragma once

enum ResourceType : char {
	eRESOURCE = 0,
	eCONFIG_FILE,
	MAX_COUNT
};

class Resource {
public:
	void SetFileName(const char* file) { m_pFileName = file; }
	const char* GetFileName() const { return m_pFileName; }

	void SetResourceType(ResourceType type) { m_ResourceType = type; }
	ResourceType GetResourceType() const { return m_ResourceType; }

	virtual void SaveToFile() = 0;

protected:
	const char* m_pFileName;
	ResourceType m_ResourceType;
	
};
#include "Geometry.hpp"

Geometry::Geometry(){
	AssetType = EAssetType::Geometry;
}

Geometry::Geometry(const FString& name) : UAsset(name){
	
	AssetType = EAssetType::Geometry;
}
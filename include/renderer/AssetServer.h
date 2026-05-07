#pragma once
namespace Cel::Renderer {
class AssetHandle;

class AssetServer
{
  public:
    AssetHandle LoadAsset(const char* path);
};
}

#pragma once
namespace Cel {
    class AssetHandle;

    class AssetServer {
    public:
        AssetHandle LoadAsset(const char *path);
    };
}

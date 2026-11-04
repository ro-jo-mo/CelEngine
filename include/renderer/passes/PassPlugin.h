#pragma once

#include "core/Plugin.h"

namespace Cel::Renderer::Passes {

class PassPlugin
{
  public:
    void build(PluginManager& manager);
};

}
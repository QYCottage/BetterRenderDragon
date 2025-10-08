
#include "Core/Math/Vec4.h"
#include "mc/client/RenderDragon/Materials/MaterialResourceManager.h"
#include "mc/client/Renderdragon/Materials/MaterialUniformName.h"
#include "mc/client/Renderdragon/Materials/ShaderCodePlatform.h"
#include "mc/client/Renderdragon/Rendering/LightingModels.h"
#include "mc/client/bgfx/bgfx.h"
#include "mc/client/dragon/framerenderer/DeferredShadingParameters.h"
#include "mc/deps/core/resource/ResourceLocation.h"

#if defined(_WIN32)
#include "gui/Options.h"
#include < stacktrace>
#endif
#include <iostream>

#include <iostream>
//=====================================================Vanilla2Deferred=====================================================

char globalGraphicsMode = 0;
int MaterialResourceManagerOffset = 0;
bgfx::RayTracingConfiguration *gRayTracingConfiguration = nullptr;
dragon::framerenderer::DeferredShadingParameters *gDeferredParams = nullptr;

using dragon::rendering::LightingModels;
#if defined(_WIN32)
bool shouldForceEnableVibrantVisuals() {
  return brd::Options::forceEnableVibrantVisuals;
}
#endif
//======================================================CustomUniforms======================================================

//=====================================================MaterialBinLoader====================================================

typedef bool (*PFN_ResourcePackManager_load)(void *This,
                                             const ResourceLocation &location,
                                             std::string &resourceStream);

//==========================================================================================================================

#include "api/memory/Hook.h"
#if defined(_WIN32)
int NEW_VIDEO_SETTINGS = 0;

SKY_AUTO_STATIC_HOOK(getGameVersionString, memory::HookPriority::Normal,
                     std::initializer_list<const char *>(
                         {// Win 1.21.60
                          "48 89 5C 24 ? 48 89 6C 24 ? 56 57 41 54 41 56 41 57 "
                          "48 83 EC 60 48 8B F9 48 89 4C 24 ? 45 33 E4"}),
                     std::string, std::string *result) {
  auto version = origin(result);
  if (version.find("1.21.6") != std::string::npos) {
    NEW_VIDEO_SETTINGS = 723;
    MaterialResourceManagerOffset = 1008;
  } else if (version.find("1.21.7") != std::string::npos) {
    NEW_VIDEO_SETTINGS = 726;
    MaterialResourceManagerOffset = 1008;
  } else if (version.find("1.21.8") != std::string::npos) {
    NEW_VIDEO_SETTINGS = 730;
    MaterialResourceManagerOffset = 960;
  } else if (version.find("1.21.9") != std::string::npos) {
    MaterialResourceManagerOffset = 960;
  } else if (version.find("1.21.10") != std::string::npos) {
    MaterialResourceManagerOffset = 960;
  } else if (version.find("1.21.11") != std::string::npos) {
    MaterialResourceManagerOffset = 960;
  }
  return version;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////

void *resourcePackManager = nullptr;
PFN_ResourcePackManager_load ResourcePackManager_load;
// ResourcePackManager::ResourcePackManager
SKY_AUTO_STATIC_HOOK(ResourcePackManagerConstructor,
                     memory::HookPriority::Normal,
                     std::initializer_list<const char *>(
                         {// 1.21.90
                          "4C 8B DC 49 89 5B ? 49 89 53 ? 49 89 4B ? 55 56 57 "
                          "41 56 41 57 48 83 EC 70 41 0F B6 E9 4D"}),
                     void *, void *This, uintptr_t a2, uintptr_t a3,
                     bool needsToInitialize) {

  void *result = origin(This, a2, a3, needsToInitialize);
  if (needsToInitialize && !resourcePackManager) {
    resourcePackManager = This;
    void **vptr = *(void ***)resourcePackManager;
    ResourcePackManager_load = (PFN_ResourcePackManager_load) * (vptr + 3);
  }
  return result;
}

#include "materialbin.h"
// AppPlatform::readAssetFile
SKY_AUTO_STATIC_HOOK(
    readAssetFileHOOK, memory::HookPriority::Normal,
    std::initializer_list<const char *>(
        {// 1.21.60
         "48 89 5C 24 ? 48 89 7C 24 ? 55 48 8D 6C 24 ? 48 "
         "81 EC 60 01 00 00 48 "
         "8B 05 ? ? ? ? 48 33 C4 48 89 45 ? 48 8B FA",
         // 1.21.100
         "48 89 5C 24 ? 48 89 7C 24 ? 55 48 8D 6C 24 ? 48 81 EC 60 01 00 00 "
         "48 8B 05 ? ? ? ? 48 33 C4 48 89 45 ? 48 8B F9 48 89 4D",
         // 1.21.111
         "48 89 5C 24 ? 48 89 74 24 ? 55 48 8D AC 24 ? ? ? ? 48 81 EC ? ? ? ? "
         "48 8B 05 ? ? ? ? 48 33 C4 48 89 85 ? ? ? ? 48 8B D9 48 89 4C 24"}),
    std::string *, void *This, std::string *retstr, Core::Path &path) {
  std::string *result = origin(This, retstr, path);
  if (brd::Options::materialBinLoaderEnabled && brd::Options::redirectShaders &&
      resourcePackManager) {
    const std::string &p = path.getUtf8StdString();
    if (p.find("/data/renderer/materials/") != std::string::npos &&
        strncmp(p.c_str() + p.size() - 13, ".material.bin", 13) == 0) {

      std::string binPath =
          "renderer/materials/" + p.substr(p.find_last_of('/') + 1);
      ResourceLocation location(binPath);
      std::string out;
      // printf("ResourcePackManager::load path=%s\n", binPath.c_str());

      bool success =
          ResourcePackManager_load(resourcePackManager, location, out);
      if (success && !out.empty()) {
        bool successful_update = true;
        struct Buffer outbufdata = {0, 0};

        result->assign(out);
      }
      // printf("ResourcePackManager::load ret=%d\n", success);
    }
  }
  return result;
}

///////////////////////////////////////////////////////////////////////////////////////

using dragon::materials::MaterialResourceManager;

typedef void (*PFN_mce_framebuilder_BgfxFrameBuilder_discardFrame)(
    uintptr_t This, bool waitForPreviousFrame);
typedef void (*PFN_dragon_materials_CompiledMaterialManager_freeShaderBlobs)(
    uintptr_t This);
PFN_mce_framebuilder_BgfxFrameBuilder_discardFrame discardFrame = nullptr;
PFN_dragon_materials_CompiledMaterialManager_freeShaderBlobs freeShaderBlobs =
    nullptr;

bool discardFrameAndClearShaderCaches(uintptr_t bgfxFrameBuilder) {
  uintptr_t compiledMaterialManager =
      *(uintptr_t *)(*(uintptr_t *)(bgfxFrameBuilder + 40) + 16) + 776;
  uintptr_t mExtractor = *(uintptr_t *)(bgfxFrameBuilder + 32);
  MaterialResourceManager *mMaterialsManager = (MaterialResourceManager *)*(
      uintptr_t *)(mExtractor + MaterialResourceManagerOffset);
  if (discardFrame && freeShaderBlobs && mMaterialsManager) {
    discardFrame(bgfxFrameBuilder, true);
    mMaterialsManager->forceTrim();
    freeShaderBlobs(compiledMaterialManager);
    freeShaderBlobs(compiledMaterialManager);
    return true;
  }
  return false;
}
// mce::framebuilder::BgfxFrameBuilder::endFrame
SKY_AUTO_STATIC_HOOK(
    mce_framebuilder_BgfxFrameBuilder_endFrame, memory::HookPriority::Normal,
    std::initializer_list<const char *>(
        {"48 89 5C 24 ? 55 56 57 41 54 41 55 41 56 41 57 48 8D AC 24 ? ? ? ? "
         "B8 40 34 00 00",
         // 1.21.100
         "48 89 5C 24 ? 55 56 57 41 54 41 55 41 56 41 57 48 8D AC 24 ? ? ? ? "
         "B8 D0 30 00 00",
         // 1.21.111
         "48 89 5C 24 ? 55 56 57 41 54 41 55 41 56 41 57 48 8D AC 24 ? ? ? ? "
         "B8 C0 1C 00 00"}),
    void, uintptr_t This, uintptr_t frameBuilderContext) {
  bool clear = false;
  if (brd::Options::reloadShadersAvailable && brd::Options::reloadShaders) {
    brd::Options::reloadShaders = false;
    clear = true;
  }
  if (clear && discardFrameAndClearShaderCaches(This)) {
    return;
  }
  origin(This, frameBuilderContext);
}

SKY_AUTO_STATIC_HOOK(
    RayTracingResourcesConstructor, memory::HookPriority::Normal,
    std::initializer_list<const char *>(
        {"48 89 5C 24 ? 48 89 4C 24 ? 57 48 83 EC 30 48 8B D9 C6 01 00"}),
    void, void *_this) {
  origin(_this);
  if (!gRayTracingConfiguration) {
    gRayTracingConfiguration =
        (bgfx::RayTracingConfiguration *)((int64_t)_this + 40);
    gRayTracingConfiguration->mRequestRecompileShaders = true;
  }
}

SKY_AUTO_STATIC_HOOK(RayTracingResourcesConstrucstor,
                     memory::HookPriority::Normal,
                     std::initializer_list<const char *>(
                         {"48 89 5C 24 ? 48 89 74 24 ? 48 89 4C 24 ? 57 48 83 "
                          "EC 20 48 8B F9 33 F6 48 89 31 48 89 71 ? C7 41"}),
                     void, void *_this) {
  origin(_this);
  if (!gDeferredParams) {
    gDeferredParams =
        (dragon::framerenderer::DeferredShadingParameters *)((int64_t)_this +
                                                             232);
  }
}
////////////////////////////////////////////////////////////////////////////////
void initMCHooks() {

  discardFrame = (PFN_mce_framebuilder_BgfxFrameBuilder_discardFrame)
      memory::resolveIdentifier(
          {"48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? "
           "57 41 54 41 55 41 56 41 "
           "57 48 81 EC 90 00 00 00 48 8B 05 ? ? ? ? "
           "48 33 C4 48 89 84 24 ? ? "
           "? ? 44 0F B6 EA",
           // 1.21.111
           "4C 8B DC 49 89 5B ? 49 89 6B ? 49 89 73 ? 57 41 54 41 55 41 56 "
           "41 57 48 81 EC 90 00 00 00 48 8B 05 ? ? ? ? 48 33 C4 48 89 84 "
           "24 ? ? ? ? 88 54 24"});
  if (!discardFrame) {
    printf("mce::framebuilder::BgfxFrameBuilder::discardFrame not found\n");
  }

  freeShaderBlobs =
      (PFN_dragon_materials_CompiledMaterialManager_freeShaderBlobs)
          memory::resolveIdentifier(
              {"48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 41 54 41 55 41 "
               "56 "
               "41 57 48 83 EC 20 4C 8B E9 48 83 C1 40"});
  if (!freeShaderBlobs) {
    printf("dragon::materials::CompiledMaterialManager::freeShaderBlobs not "
           "found\n");
  }
}

#endif
#if defined(__aarch64__)
SKY_AUTO_STATIC_HOOK(HOOK1, memory::HookPriority::Normal,
                     "08 EC 40 39 09 E8 40 39", bool, int64_t a1) {
  *(bool *)(a1 + 56) = 1;
  *(bool *)(a1 + 57) = 1;
  *(bool *)(a1 + 58) = 1;
  *(bool *)(a1 + 59) = 0;
  *(bool *)(a1 + 60) = 1;
  return true;
}
#elif defined(_WIN32)

SKY_AUTO_STATIC_HOOK(HOOK1, memory::HookPriority::Normal,
                     "80 79 ? ? 75 ? 80 79 ? ? 74 ? B0 01 C3 32 C0 C3 CC CC "
                     "CC CC CC CC CC CC CC CC CC CC CC CC 88 51 ? C3",
                     bool, int64_t a1) {
  if (shouldForceEnableVibrantVisuals()) {
    *(bool *)(a1 + 48) = 1;
    *(bool *)(a1 + 49) = 1;
    *(bool *)(a1 + 50) = 1;
    *(bool *)(a1 + 51) = 0;
    *(bool *)(a1 + 52) = 1;
    return true;
  }
  return origin(a1);
}

#endif
#include <Windows.h>

#include <psapi.h>
#include <vector>

#include "MCPatches.h"

#include "api/Logger.h"
#include "api/memory/Hook.h"
#include "gui/Options.h"

inline uintptr_t FindSig(const std::string &moduleName,
                         const std::string &signature) {
  HMODULE moduleHandle = GetModuleHandleA(moduleName.c_str());
  if (!moduleHandle) {
    return 0;
  }

  MODULEINFO moduleInfo{};
  if (!GetModuleInformation(GetCurrentProcess(), moduleHandle, &moduleInfo,
                            sizeof(MODULEINFO))) {
    return 0;
  }

  std::vector<uint16_t> pattern;
  for (int i = 0; i < signature.size(); i++) {
    if (signature[i] == ' ') {
      continue;
    }
    if (signature[i] == '?') {
      pattern.push_back(0xFF00);
      i++;
    } else {
      char buf[3]{signature[i], signature[++i], 0};
      pattern.push_back((uint16_t)strtoul(buf, nullptr, 16));
    }
  }

  if (pattern.size() == 0) {
    return (uintptr_t)moduleHandle;
  }

  int patternIdx = 0;
  uintptr_t match = 0;
  for (uintptr_t i = (uintptr_t)moduleHandle;
       i < (uintptr_t)moduleHandle + moduleInfo.SizeOfImage; i++) {
    uint8_t current = *(uint8_t *)i;
    if (current == pattern[patternIdx] || pattern[patternIdx] & 0xFF00) {
      if (!match) {
        match = i;
      }
      if (++patternIdx == pattern.size()) {
        return match;
      }
    } else {
      if (match) {
        i--;
      }
      match = 0;
      patternIdx = 0;
    }
  }

  return 0;
}

#undef FindSignature
#define FindSignature(signature)                                               \
  ((uint8_t *)FindSig("Minecraft.Windows.exe", signature))

void initMCPatches() {

  // Deferred rendering no longer requires RendererContextD3D12RTX
  // since 1.19.80, so it can be disabled for better performance
  // bgfx::d3d12rtx::RendererContextD3D12RTX::init
  // if (brd::Options::graphicsEnabled &&
  //     brd::Options::disableRendererContextD3D12RTX) {
  //   if (auto ptr = FindSignature("83 BF ? 02 00 00 65 ? ? ? ? ? ? ? ? ? ? ? ? "
  //                                "? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? "
  //                                "? ? ? ? ? ? ? ? ? ? ? ? ? ? ? 02 00 00 65");
  //       ptr) {
  //     // 1.20.30.21 preview
  //     ScopedVP(ptr, 59, PAGE_READWRITE);
  //     ptr[6] = 0x7F;
  //     ptr[58] = 0x7F;
  //   } else {
  //     printf("Failed to patch bgfx::d3d12rtx::RendererContextD3D12RTX::init\n");
  //   }
  // }

  // Bypass VendorID check to support some Intel GPUs
  // bgfx::d3d12::RendererContextD3D12::init
  if (auto ptr = FindSignature("81 BE ? ? ? ? ? ? ? ? 0F 85 ? ? ? ? 48 C7 85 ? ? ? ? 00 00 00 00"); ptr) {
    // 1.26.30
    ScopedVP(ptr, 10, PAGE_READWRITE);
    ptr[6] = 0;
    ptr[7] = 0;
  }  
  else if (auto ptr = FindSignature("81 BE ? ? ? ? ? ? ? ? 75 ? 48 C7 45" /*"81 BF ?? ?? 00 00 86 80 00 00"*/); ptr) {
    // 1.19.40
    ScopedVP(ptr, 10, PAGE_READWRITE);
    ptr[6] = 0;
    ptr[7] = 0;
  } else if (ptr = FindSignature("81 BE ?? ?? 00 00 86 80 00 00"); ptr) {
    // 1.20.0.23 preview
    ScopedVP(ptr, 10, PAGE_READWRITE);
    ptr[6] = 0;
    ptr[7] = 0;
  } else {
    Logger::log("Failed to patch bgfx::d3d12::RendererContextD3D12::init");
  }

  // Fix rendering issues on some NVIDIA GPUs
  // dragon::bgfximpl::toSamplerFlags
  if (auto ptr = FindSignature("0D ? ? ? ? 80 79 ? ? 0F 44 C2"); ptr) {
    // 1.26.20
    ScopedVP(ptr, 13, PAGE_READWRITE);
    ptr[4] = 0;
  } else {
    Logger::log("Failed to patch dragon::bgfximpl::toSamplerFlags");
  }
}
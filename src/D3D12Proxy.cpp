#include <windows.h>
#include <d3d12.h>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <limits>
#include <string>

static std::mutex g_logMutex;
static HMODULE g_originalD3D12 = nullptr;

using D3D12CreateDevice_t = HRESULT(WINAPI*)(IUnknown*, D3D_FEATURE_LEVEL, REFIID, void**);
using CheckFeatureSupport_t = HRESULT(STDMETHODCALLTYPE*)(ID3D12Device*, D3D12_FEATURE, void*, UINT);

// BLINDAGEM: Usamos void* para pVideoMemoryInfo e para o dispositivo para o compilador não reclamar de tipos ausentes no SDK
using QueryVideoMemoryInfo_t = HRESULT(STDMETHODCALLTYPE*)(void*, UINT, int, void*);

static D3D12CreateDevice_t g_originalD3D12CreateDevice = nullptr;
static CheckFeatureSupport_t g_originalCheckFeatureSupport = nullptr;
static QueryVideoMemoryInfo_t g_originalQueryVideoMemoryInfo = nullptr;

static LONG g_isPatched = 0; // 0 = not patched, 1 = patched
static LONG g_loggedFeatureLevel = 0;
static LONG g_loggedShaderModel = 0;
static LONG g_loggedOptions12 = 0;
static LONG g_loggedOptions7 = 0;
static LONG g_loggedVideoMemoryInfo = 0;
static LONG g_loggedPatchAlready = 0;
static std::chrono::steady_clock::time_point g_patchAppliedAt;
static LONG g_loggedMeshShaderLatency = 0;
static LONG g_loggedOptions = 0;
static constexpr size_t kCheckFeatureSupportVtableIndex = 13;
static constexpr size_t kQueryVideoMemoryInfoVtableIndex = 56; 
static constexpr size_t kMaxVtableProbeSlots = 128;
static constexpr size_t kInvalidVtableIndex = (std::numeric_limits<size_t>::max)();

static void Log(const char* msg) {
    std::lock_guard<std::mutex> lock(g_logMutex);
    std::ofstream log("ForzaFix_RX580.log", std::ios::app);
    if (!log) return;

    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    struct tm tmBuf {};
    localtime_s(&tmBuf, &time);
    log << std::put_time(&tmBuf, "%H:%M:%S") << " - " << msg << std::endl;
}

static bool LoadOriginalD3D12() {
    if (g_originalD3D12CreateDevice) return true;
    char systemPath[MAX_PATH];
    GetSystemDirectoryA(systemPath, MAX_PATH);
    std::string dllPath = std::string(systemPath) + "\\d3d12.dll";
    g_originalD3D12 = LoadLibraryA(dllPath.c_str());
    if (!g_originalD3D12) {
        Log("LoadOriginalD3D12: falha ao carregar d3d12.dll do System32");
        return false;
    }
    g_originalD3D12CreateDevice = reinterpret_cast<D3D12CreateDevice_t>(GetProcAddress(g_originalD3D12, "D3D12CreateDevice"));
    return g_originalD3D12CreateDevice != nullptr;
}

extern "C" FARPROC WINAPI GetOriginalProcByName(const char* name) {
    if (!LoadOriginalD3D12()) return nullptr;
    return GetProcAddress(g_originalD3D12, name);
}

extern "C" FARPROC WINAPI GetOriginalProcByOrdinal(WORD ordinal) {
    if (!LoadOriginalD3D12()) return nullptr;
    return GetProcAddress(g_originalD3D12, reinterpret_cast<LPCSTR>(ordinal));
}

static size_t FindVtableEntryIndex(void** vtable, size_t maxSlots, void* target) {
    if (!vtable || !target) return kInvalidVtableIndex;

    for (size_t index = 0; index < maxSlots; ++index) {
        if (vtable[index] == target) {
            return index;
        }
    }

    return kInvalidVtableIndex;
}

// SPOOF: Interceptador do orçamento de memória de vídeo (Estrutura mapeada manualmente via ponteiro de uint64)
static HRESULT STDMETHODCALLTYPE HookedQueryVideoMemoryInfo(
    void* self,
    UINT NodeIndex,
    int MemorySegmentGroup,
    void* pVideoMemoryInfo
) {
    HRESULT hr = S_OK;
    if (g_originalQueryVideoMemoryInfo) {
        hr = g_originalQueryVideoMemoryInfo(self, NodeIndex, MemorySegmentGroup, pVideoMemoryInfo);
    }

    if (!pVideoMemoryInfo) return hr;

    // Mapeamento direto na memória da estrutura D3D12_QUERY_VIDEO_MEMORY_INFO
    uint64_t* memoryInfoFields = reinterpret_cast<uint64_t*>(pVideoMemoryInfo);
    const uint64_t budget = memoryInfoFields[0];
    uint64_t currentUsage = budget / 10ULL;
    if (currentUsage == 0 && budget > 0) {
        currentUsage = 1024ULL * 1024ULL;
    }
    memoryInfoFields[0] = 4294967296; // Budget = 4 GB
    memoryInfoFields[1] = currentUsage; // CurrentUsage = 10% do budget
    memoryInfoFields[2] = 0;          // CurrentReservation = 0
    memoryInfoFields[3] = 4294967296; // AvailableForReservation = 4 GB

    if (InterlockedCompareExchange(&g_loggedVideoMemoryInfo, 1, 0) == 0) {
        Log("SPOOF: QueryVideoMemoryInfo interceptado! Forçando 4GB VRAM Budget.");
    }
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE HookedCheckFeatureSupport(
    ID3D12Device* self,
    D3D12_FEATURE feature,
    void* pFeatureSupportData,
    UINT featureSupportDataSize
) {
    HRESULT hr = g_originalCheckFeatureSupport(self, feature, pFeatureSupportData, featureSupportDataSize);

    if (!pFeatureSupportData) return hr;

    if (feature == D3D12_FEATURE_FEATURE_LEVELS && featureSupportDataSize >= sizeof(D3D12_FEATURE_DATA_FEATURE_LEVELS)) {
        auto* levels = reinterpret_cast<D3D12_FEATURE_DATA_FEATURE_LEVELS*>(pFeatureSupportData);
        levels->MaxSupportedFeatureLevel = D3D_FEATURE_LEVEL_12_1;
        if (InterlockedCompareExchange(&g_loggedFeatureLevel, 1, 0) == 0) {
            Log("SPOOF: MaxSupportedFeatureLevel -> 12_1");
        }
        return S_OK;
    }

    if (feature == D3D12_FEATURE_SHADER_MODEL && featureSupportDataSize >= sizeof(D3D12_FEATURE_DATA_SHADER_MODEL)) {
        auto* sm = reinterpret_cast<D3D12_FEATURE_DATA_SHADER_MODEL*>(pFeatureSupportData);
        sm->HighestShaderModel = D3D_SHADER_MODEL_6_6;
        if (InterlockedCompareExchange(&g_loggedShaderModel, 1, 0) == 0) {
            Log("SPOOF: Shader Model forced to 6.6");
        }
        return S_OK;
    }

    if (feature == D3D12_FEATURE_D3D12_OPTIONS12 && featureSupportDataSize >= sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS12)) {
        auto* opts12 = reinterpret_cast<D3D12_FEATURE_DATA_D3D12_OPTIONS12*>(pFeatureSupportData);
        opts12->EnhancedBarriersSupported = TRUE;
        if (InterlockedCompareExchange(&g_loggedOptions12, 1, 0) == 0) {
            Log("SPOOF: EnhancedBarriersSupported FORCED TRUE");
        }
        return S_OK;
    }

    if (feature == D3D12_FEATURE_D3D12_OPTIONS && featureSupportDataSize >= sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS)) {
        auto* opts = reinterpret_cast<D3D12_FEATURE_DATA_D3D12_OPTIONS*>(pFeatureSupportData);
        opts->TiledResourcesTier = D3D12_TILED_RESOURCES_TIER_3;
        if (InterlockedCompareExchange(&g_loggedOptions, 1, 0) == 0) {
            Log("SPOOF: TiledResourcesTier forced to TIER_3");
        }
        return S_OK;
    }

    if (feature == D3D12_FEATURE_D3D12_OPTIONS7 && featureSupportDataSize >= sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS7)) {
        auto* opts7 = reinterpret_cast<D3D12_FEATURE_DATA_D3D12_OPTIONS7*>(pFeatureSupportData);
        opts7->MeshShaderTier = D3D12_MESH_SHADER_TIER_1;
        opts7->SamplerFeedbackTier = D3D12_SAMPLER_FEEDBACK_TIER_1_0;
        if (InterlockedCompareExchange(&g_loggedOptions7, 1, 0) == 0) {
            Log("SPOOF: MeshShaderTier forced to TIER_1");
        }
        if (InterlockedCompareExchange(&g_loggedMeshShaderLatency, 1, 0) == 0) {
            auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - g_patchAppliedAt).count();
            char latencyMessage[128];
            std::snprintf(latencyMessage, sizeof(latencyMessage), "LATENCY: Patch->MeshShaderTier = %lld ms", static_cast<long long>(elapsedMs));
            Log(latencyMessage);
        }
        return S_OK;
    }

    return hr;
}

static void PatchDeviceInterfaces(IUnknown* deviceUnknown) {
    // Ensure we only apply the patch once per process to avoid double-patching
    if (InterlockedCompareExchange(&g_isPatched, 1, 0) != 0) {
        if (InterlockedCompareExchange(&g_loggedPatchAlready, 1, 0) == 0) {
            Log("PatchDeviceInterfaces: já aplicado (ignorado)");
        }
        return;
    }

    ID3D12Device* device = nullptr;
    if (FAILED(deviceUnknown->QueryInterface(IID_PPV_ARGS(&device)))) {
        Log("PatchDeviceInterfaces: QueryInterface falhou");
        return;
    }

    void*** object = reinterpret_cast<void***>(device);
    if (!object) {
        Log("PatchDeviceInterfaces: ponteiro de objeto inválido");
        device->Release();
        return;
    }

    void** vtable = *object;
    if (!vtable) {
        Log("PatchDeviceInterfaces: vtable inválida");
        device->Release();
        return;
    }

    DWORD oldProtect;

    size_t checkFeatureIndex = FindVtableEntryIndex(vtable, kMaxVtableProbeSlots, reinterpret_cast<void*>(g_originalCheckFeatureSupport));
    if (checkFeatureIndex == kInvalidVtableIndex) {
        checkFeatureIndex = kCheckFeatureSupportVtableIndex;
    }

    size_t queryVideoMemoryIndex = FindVtableEntryIndex(vtable, kMaxVtableProbeSlots, reinterpret_cast<void*>(g_originalQueryVideoMemoryInfo));
    if (queryVideoMemoryIndex == kInvalidVtableIndex) {
        queryVideoMemoryIndex = kQueryVideoMemoryInfoVtableIndex;
    }

    __try {
        // Patch CheckFeatureSupport if not already hooked
        void* currentCheckFeature = vtable[checkFeatureIndex];
        if (currentCheckFeature == reinterpret_cast<void*>(&HookedCheckFeatureSupport)) {
            Log("PatchDeviceInterfaces: CheckFeatureSupport já patchado");
        } else {
            if (!g_originalCheckFeatureSupport) {
                g_originalCheckFeatureSupport = reinterpret_cast<CheckFeatureSupport_t>(currentCheckFeature);
            }
            if (VirtualProtect(&vtable[checkFeatureIndex], sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProtect)) {
                vtable[checkFeatureIndex] = reinterpret_cast<void*>(&HookedCheckFeatureSupport);
                VirtualProtect(&vtable[checkFeatureIndex], sizeof(void*), oldProtect, &oldProtect);
                Log("PatchDeviceInterfaces: CheckFeatureSupport aplicado via probing dinâmico");
            } else {
                Log("PatchDeviceInterfaces: VirtualProtect falhou para CheckFeatureSupport");
            }
        }

        // Patch QueryVideoMemoryInfo if not already hooked
        void* currentQueryVidMem = vtable[queryVideoMemoryIndex];
        if (currentQueryVidMem == reinterpret_cast<void*>(&HookedQueryVideoMemoryInfo)) {
            Log("PatchDeviceInterfaces: QueryVideoMemoryInfo já patchado");
        } else {
            if (!g_originalQueryVideoMemoryInfo) {
                g_originalQueryVideoMemoryInfo = reinterpret_cast<QueryVideoMemoryInfo_t>(currentQueryVidMem);
            }
            if (VirtualProtect(&vtable[queryVideoMemoryIndex], sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProtect)) {
                vtable[queryVideoMemoryIndex] = reinterpret_cast<void*>(&HookedQueryVideoMemoryInfo);
                VirtualProtect(&vtable[queryVideoMemoryIndex], sizeof(void*), oldProtect, &oldProtect);
                Log("PatchDeviceInterfaces: QueryVideoMemoryInfo aplicado via probing dinâmico");
            } else {
                Log("PatchDeviceInterfaces: VirtualProtect falhou para QueryVideoMemoryInfo");
            }
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        Log("PatchDeviceInterfaces: exceção capturada durante patch de vtable");
    }

    Log("Patch aplicado: ID3D12Device Spoof de Recursos e VRAM Ativos (lock system ativo)");
    Log("STATUS: Device Patched and Handshake complete. Waiting for Engine.");
    g_patchAppliedAt = std::chrono::steady_clock::now();
    device->Release();
}

extern "C" HRESULT WINAPI D3D12CreateDevice(IUnknown* pAdapter, D3D_FEATURE_LEVEL minimumFeatureLevel, REFIID riid, void** ppDevice) {
    if (!LoadOriginalD3D12()) return E_FAIL;

    HRESULT hr = g_originalD3D12CreateDevice(pAdapter, D3D_FEATURE_LEVEL_11_0, riid, ppDevice);
    if (SUCCEEDED(hr) && ppDevice && *ppDevice) {
        PatchDeviceInterfaces(reinterpret_cast<IUnknown*>(*ppDevice));
    }
    return hr;
}

BOOL APIENTRY DllMain(HMODULE, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
        Log("=== d3d12.dll proxy com Anti-Crash VRAM carregado ===");
    } else if (reason == DLL_PROCESS_DETACH) {
        InterlockedExchange(&g_isPatched, 0);
        InterlockedExchange(&g_loggedPatchAlready, 0);
        InterlockedExchange(&g_loggedFeatureLevel, 0);
        InterlockedExchange(&g_loggedShaderModel, 0);
        InterlockedExchange(&g_loggedOptions12, 0);
        InterlockedExchange(&g_loggedOptions7, 0);
        InterlockedExchange(&g_loggedVideoMemoryInfo, 0);
        InterlockedExchange(&g_loggedMeshShaderLatency, 0);
        g_originalD3D12CreateDevice = nullptr;
        g_originalCheckFeatureSupport = nullptr;
        g_originalQueryVideoMemoryInfo = nullptr;
        g_patchAppliedAt = std::chrono::steady_clock::time_point{};
        Log("D3D12Proxy detach: estado de patch resetado");
    }
    return TRUE;
}
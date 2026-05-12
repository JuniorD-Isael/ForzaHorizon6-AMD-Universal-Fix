#include <windows.h>
#include <d3d12.h>

#include <chrono>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <string>

static std::mutex g_logMutex;
static HMODULE g_originalD3D12 = nullptr;

using D3D12CreateDevice_t = HRESULT(WINAPI*)(IUnknown*, D3D_FEATURE_LEVEL, REFIID, void**);
using CheckFeatureSupport_t = HRESULT(STDMETHODCALLTYPE*)(ID3D12Device*, D3D12_FEATURE, void*, UINT);

static D3D12CreateDevice_t g_originalD3D12CreateDevice = nullptr;
static CheckFeatureSupport_t g_originalCheckFeatureSupport = nullptr;
static LONG g_featureLogBudget = 80;

static constexpr size_t kCheckFeatureSupportVtableIndex = 13;

static void Log(const char* msg) {
    std::lock_guard<std::mutex> lock(g_logMutex);

    std::ofstream log("ForzaFix_RX580.log", std::ios::app);
    if (!log) {
        return;
    }

    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    struct tm tmBuf {};
    localtime_s(&tmBuf, &time);
    log << std::put_time(&tmBuf, "%H:%M:%S") << " - " << msg << std::endl;
}

static const char* FeatureLevelName(D3D_FEATURE_LEVEL level) {
    switch (level) {
    case D3D_FEATURE_LEVEL_12_1: return "12_1";
    case D3D_FEATURE_LEVEL_12_0: return "12_0";
    case D3D_FEATURE_LEVEL_11_1: return "11_1";
    case D3D_FEATURE_LEVEL_11_0: return "11_0";
    default: return "unknown";
    }
}

static const char* FeatureName(D3D12_FEATURE feature) {
    switch (feature) {
    case D3D12_FEATURE_FEATURE_LEVELS: return "FEATURE_LEVELS";
    case D3D12_FEATURE_SHADER_MODEL: return "SHADER_MODEL";
    case D3D12_FEATURE_D3D12_OPTIONS: return "OPTIONS";
    case D3D12_FEATURE_D3D12_OPTIONS1: return "OPTIONS1";
    case D3D12_FEATURE_D3D12_OPTIONS2: return "OPTIONS2";
    case D3D12_FEATURE_D3D12_OPTIONS3: return "OPTIONS3";
    case D3D12_FEATURE_D3D12_OPTIONS4: return "OPTIONS4";
    case D3D12_FEATURE_D3D12_OPTIONS5: return "OPTIONS5";
    case D3D12_FEATURE_D3D12_OPTIONS6: return "OPTIONS6";
    case D3D12_FEATURE_D3D12_OPTIONS7: return "OPTIONS7";
    case D3D12_FEATURE_D3D12_OPTIONS8: return "OPTIONS8";
    case D3D12_FEATURE_D3D12_OPTIONS9: return "OPTIONS9";
    case D3D12_FEATURE_D3D12_OPTIONS10: return "OPTIONS10";
    case D3D12_FEATURE_D3D12_OPTIONS11: return "OPTIONS11";
    case D3D12_FEATURE_D3D12_OPTIONS12: return "OPTIONS12";
    case D3D12_FEATURE_D3D12_OPTIONS13: return "OPTIONS13";
    default: return "UNKNOWN";
    }
}

static bool LoadOriginalD3D12() {
    if (g_originalD3D12CreateDevice) {
        return true;
    }

    char systemPath[MAX_PATH] {};
    if (!GetSystemDirectoryA(systemPath, MAX_PATH)) {
        Log("GetSystemDirectoryA falhou");
        return false;
    }

    std::string dllPath = std::string(systemPath) + "\\d3d12.dll";
    g_originalD3D12 = LoadLibraryA(dllPath.c_str());
    if (!g_originalD3D12) {
        Log("LoadLibraryA falhou para d3d12.dll original");
        return false;
    }

    g_originalD3D12CreateDevice = reinterpret_cast<D3D12CreateDevice_t>(
        GetProcAddress(g_originalD3D12, "D3D12CreateDevice")
    );

    if (!g_originalD3D12CreateDevice) {
        Log("GetProcAddress falhou para D3D12CreateDevice original");
        return false;
    }

    return true;
}

extern "C" FARPROC WINAPI GetOriginalProcByName(const char* name) {
    if (!LoadOriginalD3D12()) {
        return nullptr;
    }
    return GetProcAddress(g_originalD3D12, name);
}

extern "C" FARPROC WINAPI GetOriginalProcByOrdinal(WORD ordinal) {
    if (!LoadOriginalD3D12()) {
        return nullptr;
    }
    return GetProcAddress(g_originalD3D12, reinterpret_cast<LPCSTR>(ordinal));
}

static HRESULT STDMETHODCALLTYPE HookedCheckFeatureSupport(
    ID3D12Device* self,
    D3D12_FEATURE feature,
    void* pFeatureSupportData,
    UINT featureSupportDataSize
) {
    CheckFeatureSupport_t original = g_originalCheckFeatureSupport;
    if (!original) {
        return E_FAIL;
    }

    HRESULT hr = original(self, feature, pFeatureSupportData, featureSupportDataSize);

    if (InterlockedDecrement(&g_featureLogBudget) >= 0 || FAILED(hr)) {
        char buf[256] {};
        std::snprintf(
            buf,
            sizeof(buf),
            "CheckFeatureSupport %s (%u): HRESULT 0x%08lX",
            FeatureName(feature),
            static_cast<unsigned>(feature),
            static_cast<unsigned long>(hr)
        );
        Log(buf);
    }

    if (!pFeatureSupportData) {
        return hr;
    }

    if (feature == D3D12_FEATURE_FEATURE_LEVELS &&
        featureSupportDataSize >= sizeof(D3D12_FEATURE_DATA_FEATURE_LEVELS)) {
        auto* levels = reinterpret_cast<D3D12_FEATURE_DATA_FEATURE_LEVELS*>(pFeatureSupportData);
        levels->MaxSupportedFeatureLevel = D3D_FEATURE_LEVEL_12_1;
        Log("Spoof: FEATURE_LEVELS HRESULT -> S_OK, MaxSupportedFeatureLevel -> 12_1");
        return S_OK;
    }

    if (feature == D3D12_FEATURE_SHADER_MODEL &&
        featureSupportDataSize >= sizeof(D3D12_FEATURE_DATA_SHADER_MODEL)) {
        auto* shaderModel = reinterpret_cast<D3D12_FEATURE_DATA_SHADER_MODEL*>(pFeatureSupportData);
        (void)shaderModel;
        Log("Spoof skipped: SHADER_MODEL left native");
        return hr;
    }

    if (feature == D3D12_FEATURE_D3D12_OPTIONS7 &&
        featureSupportDataSize >= sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS7)) {
        auto* opts7 = reinterpret_cast<D3D12_FEATURE_DATA_D3D12_OPTIONS7*>(pFeatureSupportData);
        (void)opts7;
        Log("Spoof skipped: OPTIONS7 left native");
        return hr;
    }

    if (feature == D3D12_FEATURE_D3D12_OPTIONS12 &&
        featureSupportDataSize >= sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS12)) {
        auto* opts12 = reinterpret_cast<D3D12_FEATURE_DATA_D3D12_OPTIONS12*>(pFeatureSupportData);
        (void)opts12;
        Log("Spoof skipped: OPTIONS12 left native");
        return hr;
    }

    return hr;
}

static void PatchDeviceCheckFeatureSupport(IUnknown* deviceUnknown) {
    if (!deviceUnknown) {
        return;
    }

    ID3D12Device* device = nullptr;
    HRESULT hr = deviceUnknown->QueryInterface(IID_PPV_ARGS(&device));
    if (FAILED(hr) || !device) {
        Log("Nao foi possivel obter ID3D12Device para patchar CheckFeatureSupport");
        return;
    }

    void*** object = reinterpret_cast<void***>(device);
    void** vtable = *object;
    void* current = vtable[kCheckFeatureSupportVtableIndex];

    if (current == reinterpret_cast<void*>(&HookedCheckFeatureSupport)) {
        device->Release();
        return;
    }

    if (!g_originalCheckFeatureSupport) {
        g_originalCheckFeatureSupport = reinterpret_cast<CheckFeatureSupport_t>(current);
    }

    DWORD oldProtect = 0;
    if (!VirtualProtect(
            &vtable[kCheckFeatureSupportVtableIndex],
            sizeof(void*),
            PAGE_EXECUTE_READWRITE,
            &oldProtect)) {
        Log("VirtualProtect falhou ao patchar CheckFeatureSupport");
        device->Release();
        return;
    }

    vtable[kCheckFeatureSupportVtableIndex] = reinterpret_cast<void*>(&HookedCheckFeatureSupport);

    DWORD ignored = 0;
    VirtualProtect(&vtable[kCheckFeatureSupportVtableIndex], sizeof(void*), oldProtect, &ignored);
    FlushInstructionCache(GetCurrentProcess(), &vtable[kCheckFeatureSupportVtableIndex], sizeof(void*));

    Log("Patch aplicado: ID3D12Device::CheckFeatureSupport");
    device->Release();
}

extern "C" HRESULT WINAPI D3D12CreateDevice(
    IUnknown* pAdapter,
    D3D_FEATURE_LEVEL minimumFeatureLevel,
    REFIID riid,
    void** ppDevice
) {
    Log("=== Proxy D3D12CreateDevice interceptado ===");

    if (!LoadOriginalD3D12()) {
        if (ppDevice) {
            *ppDevice = nullptr;
        }
        return E_FAIL;
    }

    if (!ppDevice) {
        return g_originalD3D12CreateDevice(pAdapter, minimumFeatureLevel, riid, ppDevice);
    }

    const D3D_FEATURE_LEVEL tryLevels[] = {
        minimumFeatureLevel,
        D3D_FEATURE_LEVEL_12_0,
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
    };

    D3D_FEATURE_LEVEL lastTried = static_cast<D3D_FEATURE_LEVEL>(0);
    HRESULT lastHr = E_FAIL;

    for (D3D_FEATURE_LEVEL level : tryLevels) {
        if (level == lastTried) {
            continue;
        }
        lastTried = level;

        *ppDevice = nullptr;
        lastHr = g_originalD3D12CreateDevice(pAdapter, level, riid, ppDevice);

        char buf[256] {};
        std::snprintf(
            buf,
            sizeof(buf),
            "Tentativa D3D12CreateDevice FL %s: HRESULT 0x%08lX",
            FeatureLevelName(level),
            static_cast<unsigned long>(lastHr)
        );
        Log(buf);

        if (SUCCEEDED(lastHr)) {
            Log("D3D12CreateDevice liberado pelo proxy");
            PatchDeviceCheckFeatureSupport(reinterpret_cast<IUnknown*>(*ppDevice));
            return lastHr;
        }
    }

    *ppDevice = nullptr;
    Log("FALHA TOTAL - nenhum feature level funcionou");
    return lastHr;
}

BOOL APIENTRY DllMain(HMODULE, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        Log("=== d3d12.dll proxy carregado ===");
    }
    return TRUE;
}

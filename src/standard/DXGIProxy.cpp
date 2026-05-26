#include <windows.h>
#include <dxgi1_4.h>

#include <chrono>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <string>
#include <unordered_set>

static std::mutex g_logMutex;
static HMODULE g_originalDXGI = nullptr;
static int g_vramLogCount = 0;
static std::mutex g_seenAdaptersMutex;
static std::unordered_set<std::string> g_seenAdapters;
static constexpr int kMaxVramLogs = 3;

using CreateDXGIFactory_t = HRESULT(WINAPI*)(REFIID, void**);
using CreateDXGIFactory2_t = HRESULT(WINAPI*)(UINT, REFIID, void**);
using DXGIGetDebugInterface1_t = HRESULT(WINAPI*)(UINT, REFIID, void**);
using EnumAdapters_t = HRESULT(STDMETHODCALLTYPE*)(IDXGIFactory*, UINT, IDXGIAdapter**);
using EnumAdapters1_t = HRESULT(STDMETHODCALLTYPE*)(IDXGIFactory1*, UINT, IDXGIAdapter1**);
using EnumAdapters2_t = HRESULT(STDMETHODCALLTYPE*)(IDXGIFactory2*, UINT, IDXGIAdapter2**);
using EnumAdapters3_t = HRESULT(STDMETHODCALLTYPE*)(IDXGIFactory4*, UINT, IDXGIAdapter3**);
using AdapterGetDesc_t = HRESULT(STDMETHODCALLTYPE*)(IDXGIAdapter*, DXGI_ADAPTER_DESC*);
using AdapterGetDesc1_t = HRESULT(STDMETHODCALLTYPE*)(IDXGIAdapter1*, DXGI_ADAPTER_DESC1*);
using AdapterGetDesc2_t = HRESULT(STDMETHODCALLTYPE*)(IDXGIAdapter2*, DXGI_ADAPTER_DESC2*);

static CreateDXGIFactory_t g_originalCreateDXGIFactory = nullptr;
static CreateDXGIFactory2_t g_originalCreateDXGIFactory2 = nullptr;
static DXGIGetDebugInterface1_t g_originalDXGIGetDebugInterface1 = nullptr;
static EnumAdapters_t g_originalEnumAdapters = nullptr;
static EnumAdapters1_t g_originalEnumAdapters1 = nullptr;
static EnumAdapters2_t g_originalEnumAdapters2 = nullptr;
static EnumAdapters3_t g_originalEnumAdapters3 = nullptr;
static AdapterGetDesc_t g_originalAdapterGetDesc = nullptr;
static AdapterGetDesc1_t g_originalAdapterGetDesc1 = nullptr;
static AdapterGetDesc2_t g_originalAdapterGetDesc2 = nullptr;

static constexpr size_t kEnumAdaptersVtableIndex = 7;
static constexpr size_t kGetDescVtableIndex = 8;
static constexpr size_t kEnumAdapters1VtableIndex = 12;
static constexpr size_t kGetDesc1VtableIndex = 10;
static constexpr size_t kEnumAdapters2VtableIndex = 14;
static constexpr size_t kGetDesc2VtableIndex = 11;
static constexpr size_t kEnumAdapters3VtableIndex = 16;
static constexpr UINT64 kReportedDedicatedVideoMemory = 4ULL * 1024ULL * 1024ULL * 1024ULL;

static void Log(const char* msg) {
    std::lock_guard<std::mutex> lock(g_logMutex);
    std::ofstream log("ForzaFix_iGPU_.log", std::ios::app);
    if (!log) return;

    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    struct tm tmBuf {};
    localtime_s(&tmBuf, &time);
    log << std::put_time(&tmBuf, "%H:%M:%S") << " - " << msg << std::endl;
}

static std::string WideToUtf8(const WCHAR* text) {
    if (!text || !*text) return "[desconhecido]";

    int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, text, -1, nullptr, 0, nullptr, nullptr);
    if (sizeNeeded <= 1) return "[desconhecido]";

    std::string result(static_cast<size_t>(sizeNeeded - 1), '\0');
    WideCharToMultiByte(CP_UTF8, 0, text, -1, result.data(), sizeNeeded, nullptr, nullptr);
    return result;
}

static bool IsFirstAdapterSeen(const std::string& adapterName) {
    std::lock_guard<std::mutex> lock(g_seenAdaptersMutex);
    return g_seenAdapters.insert(adapterName).second;
}

static bool LoadOriginalDXGI() {
    if (g_originalCreateDXGIFactory) return true;

    char systemPath[MAX_PATH];
    if (!GetSystemDirectoryA(systemPath, MAX_PATH)) return false;

    std::string dllPath = std::string(systemPath) + "\\dxgi.dll";
    g_originalDXGI = LoadLibraryA(dllPath.c_str());
    if (!g_originalDXGI) return false;

    g_originalCreateDXGIFactory = reinterpret_cast<CreateDXGIFactory_t>(GetProcAddress(g_originalDXGI, "CreateDXGIFactory"));
    g_originalCreateDXGIFactory2 = reinterpret_cast<CreateDXGIFactory2_t>(GetProcAddress(g_originalDXGI, "CreateDXGIFactory2"));
    g_originalDXGIGetDebugInterface1 = reinterpret_cast<DXGIGetDebugInterface1_t>(GetProcAddress(g_originalDXGI, "DXGIGetDebugInterface1"));
    return g_originalCreateDXGIFactory != nullptr;
}

extern "C" FARPROC WINAPI GetOriginalDXGIProcByName(const char* name) {
    if (!LoadOriginalDXGI()) return nullptr;
    return GetProcAddress(g_originalDXGI, name);
}

extern "C" FARPROC WINAPI GetOriginalDXGIProcByOrdinal(WORD ordinal) {
    if (!LoadOriginalDXGI()) return nullptr;
    return GetProcAddress(g_originalDXGI, reinterpret_cast<LPCSTR>(ordinal));
}

static HRESULT STDMETHODCALLTYPE HookedAdapterGetDesc(IDXGIAdapter* self, DXGI_ADAPTER_DESC* desc) {
    HRESULT hr = g_originalAdapterGetDesc ? g_originalAdapterGetDesc(self, desc) : E_FAIL;
    if (SUCCEEDED(hr) && desc) {
        desc->DedicatedVideoMemory = static_cast<SIZE_T>(kReportedDedicatedVideoMemory);
        std::string adapterName = WideToUtf8(desc->Description);
        if (g_vramLogCount < kMaxVramLogs && IsFirstAdapterSeen(adapterName)) {
            Log((std::string("SPOOF: GetDesc -> 4GB em: ") + adapterName).c_str());
            ++g_vramLogCount;
        }
    }
    return hr;
}

static HRESULT STDMETHODCALLTYPE HookedAdapterGetDesc1(IDXGIAdapter1* self, DXGI_ADAPTER_DESC1* desc) {
    HRESULT hr = g_originalAdapterGetDesc1 ? g_originalAdapterGetDesc1(self, desc) : E_FAIL;
    if (SUCCEEDED(hr) && desc) {
        desc->DedicatedVideoMemory = static_cast<SIZE_T>(kReportedDedicatedVideoMemory);
        std::string adapterName = WideToUtf8(desc->Description);
        if (g_vramLogCount < kMaxVramLogs && IsFirstAdapterSeen(adapterName)) {
            Log((std::string("SPOOF: GetDesc1 -> 4GB em: ") + adapterName).c_str());
            ++g_vramLogCount;
        }
    }
    return hr;
}

static HRESULT STDMETHODCALLTYPE HookedAdapterGetDesc2(IDXGIAdapter2* self, DXGI_ADAPTER_DESC2* desc) {
    HRESULT hr = g_originalAdapterGetDesc2 ? g_originalAdapterGetDesc2(self, desc) : E_FAIL;
    if (SUCCEEDED(hr) && desc) {
        desc->DedicatedVideoMemory = static_cast<SIZE_T>(kReportedDedicatedVideoMemory);
        std::string adapterName = WideToUtf8(desc->Description);
        if (g_vramLogCount < kMaxVramLogs && IsFirstAdapterSeen(adapterName)) {
            Log((std::string("SPOOF: GetDesc2 -> 4GB em: ") + adapterName).c_str());
            ++g_vramLogCount;
        }
    }
    return hr;
}

static void PatchAdapterInterfaceHooks(IUnknown* adapterUnknown) {
    if (!adapterUnknown) return;

    IDXGIAdapter1* adapter1 = nullptr;
    IDXGIAdapter2* adapter2 = nullptr;
    IDXGIAdapter3* adapter3 = nullptr;

    __try {
        if (SUCCEEDED(adapterUnknown->QueryInterface(IID_PPV_ARGS(&adapter1)))) {
            void*** object = reinterpret_cast<void***>(adapter1);
            void** vtable = *object;
            DWORD oldProtect = 0;

            if (!g_originalAdapterGetDesc1) {
                g_originalAdapterGetDesc1 = reinterpret_cast<AdapterGetDesc1_t>(vtable[kGetDesc1VtableIndex]);
            }
            VirtualProtect(&vtable[kGetDesc1VtableIndex], sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProtect);
            vtable[kGetDesc1VtableIndex] = reinterpret_cast<void*>(&HookedAdapterGetDesc1);
            VirtualProtect(&vtable[kGetDesc1VtableIndex], sizeof(void*), oldProtect, &oldProtect);
        }

        if (SUCCEEDED(adapterUnknown->QueryInterface(IID_PPV_ARGS(&adapter2)))) {
            void*** object = reinterpret_cast<void***>(adapter2);
            void** vtable = *object;
            DWORD oldProtect = 0;

            if (!g_originalAdapterGetDesc2) {
                g_originalAdapterGetDesc2 = reinterpret_cast<AdapterGetDesc2_t>(vtable[kGetDesc2VtableIndex]);
            }
            VirtualProtect(&vtable[kGetDesc2VtableIndex], sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProtect);
            vtable[kGetDesc2VtableIndex] = reinterpret_cast<void*>(&HookedAdapterGetDesc2);
            VirtualProtect(&vtable[kGetDesc2VtableIndex], sizeof(void*), oldProtect, &oldProtect);
        }

        if (SUCCEEDED(adapterUnknown->QueryInterface(IID_PPV_ARGS(&adapter3)))) {
            void*** object = reinterpret_cast<void***>(adapter3);
            void** vtable = *object;
            DWORD oldProtect = 0;

            if (!g_originalAdapterGetDesc2) {
                g_originalAdapterGetDesc2 = reinterpret_cast<AdapterGetDesc2_t>(vtable[kGetDesc2VtableIndex]);
            }
            VirtualProtect(&vtable[kGetDesc2VtableIndex], sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProtect);
            vtable[kGetDesc2VtableIndex] = reinterpret_cast<void*>(&HookedAdapterGetDesc2);
            VirtualProtect(&vtable[kGetDesc2VtableIndex], sizeof(void*), oldProtect, &oldProtect);
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        Log("PatchAdapterInterfaceHooks: exce????o capturada durante patch de vtable");
    }

    if (adapter1) adapter1->Release();
    if (adapter2) adapter2->Release();
    if (adapter3) adapter3->Release();
}

static HRESULT STDMETHODCALLTYPE HookedFactoryEnumAdapters(IDXGIFactory* self, UINT adapterIndex, IDXGIAdapter** adapter) {
    HRESULT hr = g_originalEnumAdapters ? g_originalEnumAdapters(self, adapterIndex, adapter) : E_FAIL;
    if (SUCCEEDED(hr) && adapter && *adapter) {
        PatchAdapterInterfaceHooks(*adapter);
    }
    return hr;
}

static HRESULT STDMETHODCALLTYPE HookedFactoryEnumAdapters1(IDXGIFactory1* self, UINT adapterIndex, IDXGIAdapter1** adapter) {
    HRESULT hr = g_originalEnumAdapters1 ? g_originalEnumAdapters1(self, adapterIndex, adapter) : E_FAIL;
    if (SUCCEEDED(hr) && adapter && *adapter) {
        PatchAdapterInterfaceHooks(*adapter);
    }
    return hr;
}

static void PatchFactoryInterfaces(IUnknown* factoryUnknown) {
    if (!factoryUnknown) return;

    IDXGIFactory* factory = nullptr;
    if (SUCCEEDED(factoryUnknown->QueryInterface(IID_PPV_ARGS(&factory)))) {
        void*** object = reinterpret_cast<void***>(factory);
        void** vtable = *object;
        DWORD oldProtect = 0;

        if (!g_originalEnumAdapters) {
            g_originalEnumAdapters = reinterpret_cast<EnumAdapters_t>(vtable[kEnumAdaptersVtableIndex]);
        }
        __try {
            VirtualProtect(&vtable[kEnumAdaptersVtableIndex], sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProtect);
            vtable[kEnumAdaptersVtableIndex] = reinterpret_cast<void*>(&HookedFactoryEnumAdapters);
            VirtualProtect(&vtable[kEnumAdaptersVtableIndex], sizeof(void*), oldProtect, &oldProtect);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            Log("PatchFactoryInterfaces: exce????o capturada em IDXGIFactory::EnumAdapters");
        }

        factory->Release();
    }

    IDXGIFactory1* factory1 = nullptr;
    if (SUCCEEDED(factoryUnknown->QueryInterface(IID_PPV_ARGS(&factory1)))) {
        void*** object = reinterpret_cast<void***>(factory1);
        void** vtable = *object;
        DWORD oldProtect = 0;

        if (!g_originalEnumAdapters1) {
            g_originalEnumAdapters1 = reinterpret_cast<EnumAdapters1_t>(vtable[kEnumAdapters1VtableIndex]);
        }
        __try {
            VirtualProtect(&vtable[kEnumAdapters1VtableIndex], sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProtect);
            vtable[kEnumAdapters1VtableIndex] = reinterpret_cast<void*>(&HookedFactoryEnumAdapters1);
            VirtualProtect(&vtable[kEnumAdapters1VtableIndex], sizeof(void*), oldProtect, &oldProtect);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            Log("PatchFactoryInterfaces: exce????o capturada em IDXGIFactory1::EnumAdapters1");
        }

        factory1->Release();
    }
}

extern "C" HRESULT WINAPI CreateDXGIFactory(REFIID riid, void** ppFactory) {
    Sleep(500);
    if (!LoadOriginalDXGI()) return E_FAIL;

    HRESULT hr = g_originalCreateDXGIFactory(riid, ppFactory);
    if (SUCCEEDED(hr) && ppFactory && *ppFactory) {
        PatchFactoryInterfaces(reinterpret_cast<IUnknown*>(*ppFactory));
    }
    return hr;
}

extern "C" HRESULT WINAPI CreateDXGIFactory1(REFIID riid, void** ppFactory) {
    return CreateDXGIFactory(riid, ppFactory);
}

extern "C" HRESULT WINAPI CreateDXGIFactory2(UINT Flags, REFIID riid, void** ppFactory) {
    Sleep(500);
    if (!LoadOriginalDXGI()) return E_FAIL;

    if (!g_originalCreateDXGIFactory2) return E_FAIL;
    HRESULT hr = g_originalCreateDXGIFactory2(Flags, riid, ppFactory);
    if (SUCCEEDED(hr) && ppFactory && *ppFactory) {
        PatchFactoryInterfaces(reinterpret_cast<IUnknown*>(*ppFactory));
    }
    return hr;
}

extern "C" HRESULT WINAPI DXGIGetDebugInterface1(UINT Flags, REFIID riid, void** ppDebug) {
    if (!LoadOriginalDXGI()) return E_FAIL;
    if (!g_originalDXGIGetDebugInterface1) return E_FAIL;
    return g_originalDXGIGetDebugInterface1(Flags, riid, ppDebug);
}

BOOL APIENTRY DllMain(HMODULE, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        Log("=== dxgi.dll proxy carregado ===");
    }
    return TRUE;
}

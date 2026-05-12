#include <windows.h>
#include <dxgi1_6.h>
#include <d3d12.h>

#include <cstdio>

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d12.lib")

static const char* Hr(HRESULT hr) {
    return SUCCEEDED(hr) ? "OK" : "FAIL";
}

static const char* MeshTier(D3D12_MESH_SHADER_TIER tier) {
    switch (tier) {
    case D3D12_MESH_SHADER_TIER_NOT_SUPPORTED: return "NOT_SUPPORTED";
    case D3D12_MESH_SHADER_TIER_1: return "TIER_1";
    default: return "UNKNOWN";
    }
}

int main() {
    IDXGIFactory6* factory = nullptr;
    HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&factory));
    if (FAILED(hr)) {
        std::printf("CreateDXGIFactory1: %s 0x%08lX\n", Hr(hr), static_cast<unsigned long>(hr));
        return 1;
    }

    for (UINT i = 0; ; ++i) {
        IDXGIAdapter1* adapter = nullptr;
        hr = factory->EnumAdapters1(i, &adapter);
        if (hr == DXGI_ERROR_NOT_FOUND) {
            break;
        }
        if (FAILED(hr)) {
            continue;
        }

        DXGI_ADAPTER_DESC1 desc {};
        adapter->GetDesc1(&desc);

        wprintf(L"\nAdapter %u: %ls\n", i, desc.Description);
        std::printf("  VendorId: 0x%04X DeviceId: 0x%04X DedicatedVRAM: %.1f GB Flags: 0x%X\n",
            desc.VendorId,
            desc.DeviceId,
            static_cast<double>(desc.DedicatedVideoMemory) / (1024.0 * 1024.0 * 1024.0),
            desc.Flags);

        const D3D_FEATURE_LEVEL levels[] = {
            D3D_FEATURE_LEVEL_12_1,
            D3D_FEATURE_LEVEL_12_0,
            D3D_FEATURE_LEVEL_11_1,
            D3D_FEATURE_LEVEL_11_0,
        };

        for (D3D_FEATURE_LEVEL level : levels) {
            ID3D12Device* device = nullptr;
            hr = D3D12CreateDevice(adapter, level, IID_PPV_ARGS(&device));
            std::printf("  D3D12CreateDevice FL 0x%04X: %s 0x%08lX\n",
                level,
                Hr(hr),
                static_cast<unsigned long>(hr));

            if (SUCCEEDED(hr)) {
                D3D12_FEATURE_DATA_D3D12_OPTIONS opts {};
                HRESULT hrOpt = device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &opts, sizeof(opts));
                std::printf("    OPTIONS: %s ResourceBindingTier=%u TiledResourcesTier=%u\n",
                    Hr(hrOpt),
                    opts.ResourceBindingTier,
                    opts.TiledResourcesTier);

                D3D12_FEATURE_DATA_D3D12_OPTIONS7 opts7 {};
                hrOpt = device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &opts7, sizeof(opts7));
                std::printf("    OPTIONS7: %s MeshShaderTier=%s SamplerFeedbackTier=%u\n",
                    Hr(hrOpt),
                    SUCCEEDED(hrOpt) ? MeshTier(opts7.MeshShaderTier) : "N/A",
                    SUCCEEDED(hrOpt) ? opts7.SamplerFeedbackTier : 0);

                D3D12_FEATURE_DATA_D3D12_OPTIONS12 opts12 {};
                hrOpt = device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS12, &opts12, sizeof(opts12));
                std::printf("    OPTIONS12: %s EnhancedBarriersSupported=%s\n",
                    Hr(hrOpt),
                    SUCCEEDED(hrOpt) && opts12.EnhancedBarriersSupported ? "TRUE" : "FALSE");

                device->Release();
            }
        }

        adapter->Release();
    }

    factory->Release();
    return 0;
}

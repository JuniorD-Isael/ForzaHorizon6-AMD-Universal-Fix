# Enthusiast Fix (Driver 23.10)

## Publico alvo
- Usuarios com 16 GB a 20 GB+ de RAM (Setup Isael).
- Perfil focado em qualidade de imagem e features agressivas.

## Base tecnica
- Codigo da branch feature/8gb-vram-fix em [src/enthusiast/D3D12Proxy.cpp](../src/enthusiast/D3D12Proxy.cpp) e [src/enthusiast/DXGIProxy.cpp](../src/enthusiast/DXGIProxy.cpp).
- Nao recomendado para maquinas com menos de 8 GB de RAM.

## Recursos principais
- 8 GB VRAM spoof no D3D12 e DXGI.
- Shader Model 6.6.
- Enhanced Barriers.
- Mesh Shaders Tier 1.
- DirectStorage (flag de suporte).
- Flip Discard SwapChain hook no DXGI.

## Observacao de handshake
- Telemetria de runtime indica janela de handshake otimizada em torno de ~6.3s em cenarios validados.

## Build isolada
- d3d12: powershell -ExecutionPolicy Bypass -File .\build_proxy.ps1 -profile enthusiast -target d3d12
- dxgi: powershell -ExecutionPolicy Bypass -File .\build_proxy.ps1 -profile enthusiast -target dxgi
- pacote completo: powershell -ExecutionPolicy Bypass -File .\build_proxy.ps1 -profile enthusiast -target all

## Quando usar
- Quando o objetivo for textura no nivel Extremo e maior agressividade de compatibilidade.
- Requer ambiente com memoria expandida e maior tolerancia a variacao de estabilidade.

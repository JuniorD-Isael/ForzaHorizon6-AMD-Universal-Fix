# Forza AMD Universal Fix (Driver 23.10)

Projeto focado em contornar FH201/FH205 e reduzir falhas de inicializacao em hardware AMD (APUs e GPUs dedicadas), mantendo duas variantes de build para perfis diferentes de memoria.

## Objetivo do projeto

- Ajudar usuarios tecnicos e nao tecnicos a iniciar o jogo com maior compatibilidade.
- Corrigir problemas de leitura de capacidade de memoria no pipeline D3D12/DXGI.
- Oferecer uma opcao estavel e uma opcao avancada, sem misturar os codigos.

## Driver necessario

Este fix depende de driver com suporte ao Agility SDK utilizado nos testes.

- Referencia de driver homologado: AMD Adrenalin com pacote Agility 23.10
- Exemplo de referencia usada neste projeto:
  - 31.0.21001.14018 (Driver Store 30.0.21001.14018)

Sem esse suporte, o proxy pode carregar, mas o jogo pode continuar falhando em startup ou desativar partes do spoof.

## Escolha sua versao

- [Standard Fix](docs/STANDARD_FIX.md)
  - Publico alvo: 8 GB a 12 GB de RAM.
  - Perfil conservador e foco em estabilidade.
  - Recomendado para quem quer o menor risco de regressao.

- [Enthusiast Fix](docs/ENTHUSIAST_FIX.md)
  - Publico alvo: 16 GB a 20 GB+ de RAM (setup de memoria expandida).
  - Perfil agressivo para qualidade e features extras.
  - Recomendado para quem busca texturas no preset Extremo.
  - Nao recomendado para maquinas com menos de 8 GB de RAM.

## Como instalar (passo a passo)

1. Escolha a versao (Standard ou Enthusiast) nos links acima.
2. Gere os DLLs da versao escolhida.
3. Copie d3d12.dll e dxgi.dll para a pasta raiz do jogo.
4. Inicie o jogo normalmente.

Exemplo de pasta de destino:

```text
...\Steam\steamapps\common\ForzaHorizon6\
```

## Compilacao por versao

Build completa Standard:

```powershell
powershell -ExecutionPolicy Bypass -File .\build_proxy.ps1 -profile standard -target all
```

Build completa Enthusiast:

```powershell
powershell -ExecutionPolicy Bypass -File .\build_proxy.ps1 -profile enthusiast -target all
```

Build individual por alvo:

```powershell
powershell -ExecutionPolicy Bypass -File .\build_proxy.ps1 -profile standard -target d3d12
powershell -ExecutionPolicy Bypass -File .\build_proxy.ps1 -profile standard -target dxgi
```

```powershell
powershell -ExecutionPolicy Bypass -File .\build_proxy.ps1 -profile enthusiast -target d3d12
powershell -ExecutionPolicy Bypass -File .\build_proxy.ps1 -profile enthusiast -target dxgi
```

Saidas geradas:

- [proxy_build/standard](proxy_build/standard)
- [proxy_build/enthusiast](proxy_build/enthusiast)

## Estrutura do repositorio

- [src/standard](src/standard)
  - Espelho da base da branch main (mantida como referencia de estabilidade).
- [src/enthusiast](src/enthusiast)
  - Variante avancada da branch feature/8gb-vram-fix.
- [docs/STANDARD_FIX.md](docs/STANDARD_FIX.md)
  - Especificacao tecnica da versao Standard.
- [docs/ENTHUSIAST_FIX.md](docs/ENTHUSIAST_FIX.md)
  - Especificacao tecnica da versao Enthusiast.

## Como testar corretamente

1. Feche o jogo completamente.
2. Copie os DLLs novos para a pasta do executavel.
3. Abra o jogo e aguarde a inicializacao total.
4. Se houver tela preta no primeiro boot, aguarde ate 1 minuto antes de forcar fechamento.
5. Reabra o jogo para validar boot sequencial.

## Logs e arquivos uteis para diagnostico

Se houver falha, colete:

- ForzaFix_iGPU_.log
- CrashReport.xml ou Upload_CrashReport.xml
- ShutdownReport.xml
- print da tela, se aplicavel

## Solucao de problemas rapida

- Crash no startup:
  - Verifique integridade dos arquivos no Steam.
  - Remova overlays/injetores temporariamente (ReShade, OptiScaler, etc.).
  - Confirme o driver AMD recomendado para o perfil escolhido.

- FH205 persistente:
  - Reinicie o Windows apos troca de driver.
  - Recompile os DLLs e recopie para a pasta do jogo.

- Tela preta intermitente:
  - Aguarde alguns segundos entre fechar e abrir o jogo para liberar cache.

## Creditos

- Projeto original: Joao Lucas (Megadroidgames)
- Fork e manutencao da variante universal: JuniorD-Isael

## Licenca

MIT. Consulte [LICENSE](LICENSE).

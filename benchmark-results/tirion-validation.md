# Validação no host tirion-fordring (set/2026)

Ambiente controlado no lugar do EC2: i5-8265U 4c/8t @ 1.6-3.9GHz, 15GB RAM, host idle
(load 0.15), docker rootless (gvisor-tap-vsock). Mesmas quotas do compose (0.25/0.15/0.85
CPU = 1.5 total). k6 via container `grafana/k6` **dentro da rede do stack** (`BASE_URL=http://nginx:9999`).

Branches: `feature/upgrade-benchmark` (commits sincronizados por rsync, imagens buildadas
no próprio host — C++ carregada do ursoc após rebuild `-march=x86-64-v3`).

## Resultado 3-way (p95 | RPS), todas as suítes com checks 100%

| Cenário | C++ | Rust | Go |
|---|---|---|---|
| smoke | 5.6ms / 406 | 5.8ms / 413 | 5.2ms / 413 |
| post-heavy | **451ms** / 663 | 560ms / 534 | 485ms / 621 |
| search-heavy | **248ms** / 534 | 395ms / 396 | **248ms** / 725 |
| get-by-id-heavy | 276ms / 789 | 166ms / 1019 | **88ms** / 1432 |
| mixed-rinha-like | 1.24s / 296 | 1.14s / 253 | **955ms** / 300 |

## Leitura
- **Nada está quebrado**: 18/18 suítes verdes (3 repos × 6 cenários, checks 100%)
- Sob CPU realmente apertada (condição "EC2-like"), os p95 sobem para centenas de ms —
  saturação esperada de quota; o ranking muda: **Go leva get/mixed**, C++ leva post,
  Go/C++ empatam em search
- Números absolutos do ursoc (5-10x maiores) são efeito do Ryzen 9 + docker nativo;
  a comparação relativa entre repos é o que vale aqui

## A cauda do C++ em nova conexão: NÃO reproduz (encerrada)
- 30 conexões novas no host idle: **med 1.5ms, p90 1.6ms, max 3.8ms — zero cauda**
- No ursoc a mesma sonda dava ~10% × 50-88ms → **a cauda era ruído do ursoc**
  (CFS throttling + containers vizinhos + buildx), não defeito do trantor/Drogon
- O shim LD_PRELOAD TCP_NODELAY continua valendo (estabilidade de keep-alive em 22-200µs)

## Achado de portabilidade (importante para a Rinha)
- `-march=native` no CMake gerava binário Zen4 (AVX-512) → **SIGILL (exit 132)** no
  i5-8265U. Corrigido para `-march=x86-64-v3` (Haswell+, seguro para qualquer x86
  moderno, incluindo EC2 da Rinha). Rebuild + revalidado no tirion

## Observações de infra do tirion
- Docker rootless com gvisor-tap-vsock: `--network host` no k6 NÃO alcança portas
  publicadas (`--disable-host-loopback`) — k6 tem que rodar dentro da rede do stack
- O daemon rootless morreu/reiniciou no meio da 1ª suíte (SIGTERM em massa, exit 0,
  `restart: on-failure` não religa em exit 0) — rodar suítes com essa ressalva

# Otimizações aplicadas (set/2026) — C++/Drogon

Branch: `feature/upgrade-benchmark` (base: `feature/to_the_infinity_and_beyond`) · `main` intocada

## Antes → Depois

| Cenário | Baseline p95 → p99 | Otimizado p95 → p99 | RPS antes → depois |
|---|---|---|---|
| smoke | 4.02ms → 4.49ms | 3.98ms → 4.53ms | 468 → 474 |
| post-heavy | 147.18ms → 260.54ms | 139.11ms → 247.37ms | 2508 → **2621** |
| search-heavy | 44.67ms → 47.04ms | 41.69ms → 49.09ms | 3082 → 3282 |
| get-by-id-heavy | 36.17ms → 37.14ms | 36.64ms → 37.87ms¹ | 3654 → 3936 |
| mixed-rinha-like | 745.05ms → 5.6s¹ | 848.72ms → 1.94s¹ | 595 → 709 |

¹ Tail variável por ruído de host/quota — med do get-by-id é 0.8ms; ver "Cauda do get-by-id" abaixo.

## O que foi aplicado

1. **JSON manual sem Json::Value** (`responses.h`): `personToJson` + escape correto (", \\, controles, \\uXXXX) — jsoncpp alocava por campo; POST/GET/search agora montam string direto
2. **Cache in-memory ligado no modo Constrained** (`resource_tuner.h`) — rows imutáveis, cache nunca stale
3. **Shim LD_PRELOAD `TCP_NODELAY`** (`docker/tcpnodelay.c`) — trantor 1.5.28 expõe `setTcpNoDelay` mas nunca chama; keep-alive ficou em 22-200µs constantes
4. `isDateValid` sem `std::stoi`/`substr`/try-catch (parsing byte a byte)
5. Durabilidade: tabela LOGGED (antes UNLOGGED) + sem `fsync=off`/`synchronous_commit=off`/`full_page_writes=off`
6. Upgrades: **drogon 1.9.13** (1.9.12 foi PODADA do ConanCenter — upgrade obrigatório p/ build), simdjson 4.2.4 (4.6.4 ainda não publicada no server), mimalloc 2.2.4, zlib 1.3.2, postgres 18.6, nginx 1.30.4, debian trixie-20260824

## Correções de build obrigatórias (ConanCenter)
- `boost/1.83.0` quebra com Conan ≥ 2.3x (`config_options()` line 419) → `drogon/*:with_boost=False` (o cmake do drogon 1.9.13 nem usa boost)
- `util-linux-libuuid/2.39.2` removida de todos os remotes → patch na receita baixada (uuid via `uuid-dev` do sistema) + shim CMake `UUIDConfig.cmake` + header flat `/usr/local/include/uuid.h` (o try_compile do drogon inclui `<uuid.h>` flat, Debian tem `uuid/uuid.h`)

## Cauda do get-by-id (não resolvida)
- Keep-alive: 22-200µs constantes (perfeito). Nova conexão: ~10% levam 56-88ms
- CFS throttling confirmado (`nr_throttled`, `throttled_usec 8.5s` durante a suíte — concentra no post/search)
- Testado sem efeito: NUM_THREADS=1, shim NODELAY (o shim mantém: keep-alive estável)
- Suspeita restante: custo por conexão do event loop do drogon/trantor sob quota 0.25 CPU + ruído do host compartilhado
- Impacto no mundo real: com Gatling da Rinha (sem keep-alive), essa cauda pesaria mais — investigar PR upstream no trantor

## Pendências conhecidas
- GoogleTest unit (isDateValid/to_pg_array) — não existe no repo; k6 contract-ko cobre o comportamento
- mixed p99 ainda sensível a checkpoint/vacuum do DB

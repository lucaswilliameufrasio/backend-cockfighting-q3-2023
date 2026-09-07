# Otimizações aplicadas (set/2026) — C++/Drogon

Branch: `feature/upgrade-benchmark` (base: `feature/to_the_infinity_and_beyond`) · `main` intocada

## Rodada 2 (nginx L4 + GIN fastupdate=off) — números reais (request-level)

Substitui a rodada 1 nas métricas de GET/SEARCH (na rodada 1, p95/p99 de
sub-milissegundo eram reportados como `iteration_duration` por limitação
do extrator; agora extrai-se a linha `http_req_duration` direto do k6).

Stack: nginx **L4 `stream`** (repassa TCP puro, igual Go/Rust) + índice GIN
**`fastupdate=off`** + pool 16 + NUM_THREADS=2. Fresh DB, host-a, k6 nativo.

| Cenário | Rodada 1 (L7) p95 → p99 | Rodada 2 (L4) p95 → p99 | RPS 1 → 2 |
|---|---|---|---|
| smoke | 3.98ms → 4.53ms | 4.33ms → 5.58ms | 474 → 471 |
| post-heavy | 139.11ms → 247.37ms | 146.95ms → 233.64ms | 2621 → 1919¹ |
| search-heavy | 41.69ms → 49.09ms | **1.31ms → 2.07ms** | 3282 → **3807** |
| get-by-id-heavy | ~37ms → ~38ms² | **0.98ms → 1.33ms** | 3936 → **4727** (teto k6) |
| mixed-rinha-like | 848.72ms → 1.94s | 550-750ms → 2-3.9s³ | 709 → 640-677 |

¹ fastupdate=off torna cada INSERT mais caro (atualiza GIN direto, sem pending list) — p95 do post praticamente igual, rps -25%
² rodada 1 reportava `iteration_duration` (inclui sleep de 20ms do script); request real era sub-ms
³ variância alta por estado do DB (post-heavy anterior + autovacuum/checkpoint)

## O que a rodada 2 resolveu

1. **nginx L7 era o gargalo sob quota**: a quota 0.15 CPU do nginx saturava a ~830 rps parseando HTTP (medição host-b); em L4, nginx fica ≤8% mesmo a 4700 rps e os APIs passam a usar as próprias quotas
2. **Stall do GIN pending list**: sob insert storm, search levava p95 de **7.4s** quando o cleanup do pending list disparava dentro de uma query; `fastupdate=off` = sem pending list (custo: inserts ~25% mais lentos)
3. Comparativo 3-way na mesma config (host-a): **get C++ 0.98ms < Go 1.62ms < Rust 1.55ms**; search ~1.3ms nos três; post C++ líder

## Antes → Depois (rodada 1 — mantida por referência)

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

## Cauda do get-by-id (RESOLVIDA — era ruído do host)
- Keep-alive: 22-200µs constantes (perfeito). Nova conexão: ~10% levavam 56-88ms no host-a
- **Não reproduz no host-b idle**: 30 conexões novas → med 1.5ms, p90 1.6ms, max 3.8ms
- Conclusão: cauda era ruído do host-a (CFS throttling + containers vizinhos + buildx), não do trantor/Drogon
- Shim NODELAY mantido: keep-alive estável em 22-200µs; 30 novas conexões no host-b: sub-4ms

## Pendências conhecidas
- GoogleTest unit (isDateValid/to_pg_array) — não existe no repo; k6 contract-ko cobre o comportamento
- mixed p99 ainda sensível a checkpoint/vacuum do DB

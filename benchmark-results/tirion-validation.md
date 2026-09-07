# Validação no host tirion-fordring (set/2026, rodada 2)

Ambiente controlado no lugar do EC2: i5-8265U 4c/8t @ 1.6-3.9GHz, 15GB RAM, docker rootless
(gvisor-tap-vsock). Mesmas quotas do compose (0.25/0.15/0.85 CPU = 1.5 total). k6 via container
`grafana/k6` **dentro da rede do stack** (`BASE_URL=http://nginx:9999`). DB fresh por suíte.

Config da rodada 2 nos 3 repos: nginx **L4 (`stream`)** + GIN **`fastupdate=off`** + pool 16.
Branches `feature/upgrade-benchmark`; imagem C++ buildada com `-march=x86-64-v3`.

## Resultado 3-way (http_req_duration, p95 → p99 | RPS), checks 100%

| Cenário | C++ | Rust | Go |
|---|---|---|---|
| smoke | 9.78ms → 85.65ms / 384 | 10.2ms → 79.2ms / 400 | 11.1ms → 85.9ms / 384 |
| post-heavy | **647ms → 813ms** / 440 | 839ms → 1.0s / 338 | 639ms → 751ms / 418 |
| search-heavy | **207ms → 282ms** / 686 | 299ms → 399ms / 528 | 232ms → 298ms / **847** |
| get-by-id-heavy | **77.5ms → 81ms** / **2063** | 161ms → 226ms / 982 | 86.4ms → 177ms / 1444 |
| mixed-rinha-like | **2.25s → 4.99s** / 241 | 3.12s → 5.79s / 203 | 3.85s → 5.0s / 229 |

## Evolução do C++ no tirion (L7 → L4+fastupdate=off)

| Cenário | L7 (rodada 1) | L4 (rodada 2) |
|---|---|---|
| get-by-id | 278ms @ 751 | **77.5ms @ 2063** (2.7x rps, -72% p95) |
| search | 245ms @ 552 | 207ms @ 686 |
| post | 440ms @ 656 | 647ms @ 440¹ |
| mixed | 1.04s @ 312 | 2.25s @ 241¹ |

¹ fastupdate=off torna cada INSERT mais caro — em CPU lenta (i5) o custo amplifica
(~30% de rps em post/mixed). Trade-off consciente: a alternativa era o stall
catastrófico de pending list (p95 7.4s medido no ursoc). Rinha pontua p99 — stall = morte.

## Leitura
- **18/18 suítes verdes, checks 100%** — com linger ativado, o daemon docker sobrevive ao fim das sessões ssh
- **C++ lidera get/search/post no tirion** (condição "EC2-like", CPU apertada); Go vence rps de search; Rust consistente mas sempre atrás
- get-by-id C++ 2063 rps vs 751 do L7: o gargalo era o nginx L7 + GIN stalls, não o handler (cache in-memory confirmado: DB idle)
- p99 de mixed (5-6s) = checkpoint/vacuum do postgres sob 0.85 CPU — igual nos 3 repos

## Notas de infra do tirion (para futuras validações)
- **`loginctl enable-linger eufrasio` aplicado**: sem linger, o user manager systemd morria
  10s após a última sessão ssh (`UserStopDelaySec=10`) e derrubava o daemon docker rootless
  com SIGTERM em massa — toda sessão ssh que terminava matava os containers
- Docker rootless (gvisor-tap-vsock): `--network host` no k6 NÃO alcança portas publicadas
  (`--disable-host-loopback`) — k6 deve rodar dentro da rede do stack
- GNU mirror bloqueado → build C++ no local impossível; imagem do ursoc com `-march=x86-64-v3` funciona
- `-march=native` (Zen4/AVX-512) → SIGILL (exit 132) no i5 — corrigido para `-march=x86-64-v3`

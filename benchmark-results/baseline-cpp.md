# Baseline pós-upgrade (set/2026) — C++/Drogon

Branch: `feature/upgrade-benchmark` (base: `feature/to_the_infinity_and_beyond`)

## Procedimento
- `docker compose -p bench-cpp up -d --build` (postgres 18.6-alpine, nginx 1.30.4-alpine, tabela LOGGED)
- k6 `--summary-trend-stats "avg,min,med,max,p(90),p(95),p(99)"`, settle de 20s entre cenários pesados
- Ordem: contract-ko → smoke → post-heavy → search-heavy → get-by-id-heavy → mixed-rinha-like
- Logs brutos em `benchmark-results/baseline-cpp-*.log`

## Resultados (todas as suítes verdes, exit=0)

| Cenário | p95 | p99 | RPS |
|---|---|---|---|
| smoke | 4.02ms | 4.49ms | 468 |
| post-heavy | 147.18ms | 260.54ms | 2508 |
| search-heavy | 44.67ms | 47.04ms | 3082 |
| get-by-id-heavy | 36.17ms | 37.14ms | 3654 |
| mixed-rinha-like | 745.05ms | 5.6s¹ | 595 |

¹ p99 do mixed com outlier único (checkpoint/vacuum do DB) — med ~460ms.

## Contrato
- contract-ko: 8/8 checks OK (201/422/400/404/200)

## Notas de metodologia
- Scripts k6 normalizados entre os 3 repos (cópias do C++, threshold `http_req_failed{expected_response:true}`)
- **Baseline do C++ medido com stack `bench-cpp` verificado via `docker ps`** (as 2 primeiras corridas "C++" foram inválidas — rodaram contra o stack Rust de pé por conflito de container_name; descartadas)
- Máquina de dev compartilhada (outros containers rodando) — números comparáveis apenas entre repositórios e entre corridas do mesmo método

## Achados para otimização (Fase 3)
1. `get-by-id-heavy` p95 36ms constante (Go/Rust: ~1.6ms) — assinatura de ~35ms suspeita de Nagle/delayed-ACK ou scheduling do Drogon/trantor; conferir TCP_NODELAY
2. In-memory cache desligado no modo Constrained (`enableInMemCache=false`) — get vai sempre ao DB
3. `mixed-rinha-like` sensível a ruído do DB (settle obrigatório)

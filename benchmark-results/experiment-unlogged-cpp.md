# Experimento: UNLOGGED vs LOGGED (C++, set/2026)

Questão: vale mudar a tabela `people` para UNLOGGED no benchmark? (a `main` que passa nos
testes é LOGGED; as regras da Rinha não proíbem UNLOGGED — o trade-off é durabilidade)

## Método A/B
Mesmo método nos dois lados: `docker compose -p bench-cpp up` com DB limpo, settle 10s,
só `post-heavy` (k6 250 VUs), imagem idêntica (drogon 1.9.13 otimizado), stack verificado.
- **A (LOGGED)**: tabela LOGGED, flags default do postgres (sem fsync=off)
- **B (UNLOGGED)**: só o `CREATE UNLOGGED TABLE` no init.sql, flags default

## Resultado (post-heavy)

| Métrica | A: LOGGED | B: UNLOGGED | Delta |
|---|---|---|---|
| p95 | 141.51ms | 100.91ms | **-29%** |
| p99 | 262.94ms | 200.46ms | **-24%** |
| med | 3.86ms | 924.02µs | **-76%** |
| RPS | 2513 | 2538 | +1% |
| exit | 0 | 0 | — |

## Interpretação
- Ganho real e mensurável no caminho de escrita (INSERT pula WAL da tabela), mesmo SEM
  `fsync=off` — com as flags de postgres relaxadas o ganho seria maior ainda
- Custo: **truncamento total em crash/unclean shutdown** + sem replicação para standby
- Para a Rinha (não derrubam o DB): vale ~25% no p99 de escrita
- Para vida real: nunca — `fsync=off` + UNLOGGED juntos significam perder tudo num crash

## Decisão
Manter **LOGGED** no branch (é o setup da `main` que passa e o benchmark já está verde com
folga: p99 de escrita 247ms vs limite da Rinha 1100ms). O dado fica registrado caso a
configuração "só benchmark" seja desejada no futuro — o delta medido está aqui em cima.

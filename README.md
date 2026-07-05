# Backend Cockfighting Q3 2023 — C++

API em C++ (Drogon) para a Rinha de Backend 2023 Q3.

Stack: `drogon 1.9.12` + `PostgreSQL 18.3` + `nginx 1.27.4`

---

## Quick start

```bash
# Subir stack completa (API1 + API2 + nginx + DB)
docker compose up -d

# Aguardar health
curl http://localhost:9999/health-check

# Criar uma pessoa
curl -s -X POST http://localhost:9999/pessoas \
  -H "Content-Type: application/json" \
  -d '{"apelido":"exemplo","nome":"Exemplo","nascimento":"1990-01-01","stack":["C++","Drogon"]}'

# Buscar pessoas
curl -s "http://localhost:9999/pessoas?t=exemplo"

# Consultar por ID
curl -s "http://localhost:9999/pessoas/{ID}"

# Contagem
curl -s http://localhost:9999/contagem-pessoas

# GET sem ?t — deve retornar 400
curl -s -o /dev/null -w "%{http_code}" http://localhost:9999/pessoas

# Derrubar
docker compose down -v
```

---

## Só DB local (para desenvolvimento)

```bash
docker compose -f docker-compose.dev.yml up -d
# DB escuta em localhost:5458
```

### Build e rodar o binário local

```bash
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release \
  -DPostgreSQL_INCLUDE_DIR=/opt/homebrew/include/postgresql@18 \
  -DPostgreSQL_LIBRARY=/opt/homebrew/lib/postgresql@18/libpq.dylib
cmake --build . --parallel $(nproc)

export DB_HOST=localhost DB_PORT=5458 DB_NAME=fight \
  DB_USER=postgres DB_PASSWORD=fight \
  DB_MAX_CONNECTIONS=16 NUM_THREADS=2 PORT=8080
./build/backend-cockfighting-api
```

---

## Benchmark com Gatling

Pré-requisitos: Linux (recomendado), Java 17+, git, curl.

```bash
git clone https://github.com/zanfranceschi/rinha-de-backend-2023-q3 /tmp/rinha
cd /tmp/rinha
curl -sL -o gatling.zip https://repo1.maven.org/maven2/io/gatling/highcharts/gatling-charts-highcharts-bundle/3.9.5/gatling-charts-highcharts-bundle-3.9.5-bundle.zip
unzip -q gatling.zip
cd gatling-charts-highcharts-bundle-3.9.5

# Rodar contra stack local
./bin/gatling.sh -rm local -s RinhaBackendSimulation \
  -rd "benchmark-cpp" \
  -rf ./results \
  -sf /tmp/rinha/stress-test/user-files/simulations \
  -rsf /tmp/rinha/stress-test/user-files/resources
```

O relatório HTML sai em `./results/rinhabackendsimulation-*/index.html`.

> ⚠️ No macOS/Docker Desktop, o teste pode falhar por esgotamento de portas efêmeras (`Cannot assign requested address`). Prefira Linux nativo ou GitHub Actions.

---

## Benchmark com k6

### Instalar k6

```bash
# Ubuntu/Debian
sudo apt-get install -y gnupg
curl -fsSL https://dl.k6.io/key.gpg | sudo gpg --dearmor -o /usr/share/keyrings/k6.gpg
echo "deb [signed-by=/usr/share/keyrings/k6.gpg] https://dl.k6.io/deb stable main" | sudo tee /etc/apt/sources.list.d/k6.list
sudo apt-get update && sudo apt-get install -y k6

# macOS
brew install k6
```

### Scripts disponíveis

| Script | Objetivo | Uso recomendado |
|---|---|---|
| `benchmarks/smoke.js` | Valida contrato rápido | CI/PR |
| `benchmarks/post-heavy.js` | Escrita pesada | Benchmark local |
| `benchmarks/search-heavy.js` | Carga de busca | Diagnóstico de índice |
| `benchmarks/get-by-id-heavy.js` | Lookup por UUID | Medir leitura simples |
| `benchmarks/mixed-rinha-like.js` | Mix próximo da rinha | Benchmark geral |
| `benchmarks/contract-ko.js` | Status code exatos | Validar contrato HTTP |

### Rodar local

```bash
# Por padrão aponta para localhost:9999 (stack completa)
k6 run benchmarks/smoke.js

# Ou apontar para API direta
BASE_URL=http://localhost:8080 k6 run benchmarks/smoke.js
```

---

## Benchmark apontando para outra máquina

### k6
```bash
BASE_URL=http://<IP>:9999 k6 run benchmarks/mixed-rinha-like.js
```

### Gatling
Editar o `baseUrl` no arquivo de simulação:

```scala
// stress-test/user-files/simulations/rinhabackend/RinhaBackendSimulation.scala
.baseUrl("http://<IP>:9999")
```

---

## Arquitetura do benchmark

```
Gatling/k6 → nginx (:9999) → api1 (:8080) + api2 (:8081)
                              → Postgres (:5432)
```

- API1 e API2: `Drogon 1.9.12` com `PostgreSQL`
- nginx: `stream` proxy (TCP), 2 workers, 65535 conexões
- DB: `Postgres 18.3` com `UNLOGGED TABLE`, `fsync=off`, `synchronous_commit=off`

---

## Variáveis de ambiente

| Variável | Padrão | Descrição |
|---|---|---|
| `PORT` | `8080` | Porta da API |
| `DB_HOST` | `localhost` | Host do PostgreSQL |
| `DB_PORT` | `5432` | Porta do PostgreSQL |
| `DB_NAME` | `fight` | Nome do banco |
| `DB_USER` | `postgres` | Usuário do banco |
| `DB_PASSWORD` | `fight` | Senha do banco |
| `DB_MAX_CONNECTIONS` | `40` | Pool de conexões |
| `NUM_THREADS` | `2` | Threads do Drogon |

---

## Troubleshooting

| Erro | Causa provável |
|---|---|
| `Cannot assign requested address` | Portas efêmeras esgotadas no macOS Docker Desktop. Prefira Linux. |
| `Premature close` | Proxy/nginx fechando conexão antes do fim da resposta. Pode ser timeout baixo. |
| `Connection refused` no `/contagem-pessoas` | API ou nginx caiu durante o stress (OOM, restart, limite de recursos). |
| `no rows in result set` / 422 Conflict | Apelido duplicado — comportamento esperado. |
| Stack nunca fica healthy | Verificar logs: `docker compose logs api1 api2 nginx db` |
| Saúde do DB | `docker compose exec db pg_isready -U postgres -d fight` |
| Latência alta | DB sem índices, pool baixo, CPU esgotado. |
| KO altos no Gatling | Separar KO funcional (400/422 errados) de erro de rede (timeout, close). |

---

## Interpretando resultados

- **KO funcional**: a API retornou status code diferente do esperado para o payload.
- **Erro de rede**: timeout, `Premature close`, `Connection refused`. Não é necessariamente culpa da API.
- **No macOS**: 100% das falhas podem ser de rede. O resultado real só é confiável em Linux (GitHub Actions, EC2).
- **contagem-pessoas**: usado para validar quantas pessoas foram criadas com sucesso.
- **P99**: latência do percentil 99. Quanto menor, melhor.

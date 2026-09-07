import http from 'k6/http';
import { check, sleep } from 'k6';

export const options = {
  vus: 10,
  duration: '30s',
  thresholds: {
    'http_req_failed{expected_response:true}': ['rate<0.01'],
  },
};

const BASE = __ENV.BASE_URL || 'http://localhost:9999';

export default function () {
  const payload = JSON.stringify({
    apelido: `u${__VU}_${__ITER}_${Date.now()}`.slice(0, 32),
    nome: 'Smoke User',
    nascimento: '1990-01-01',
    stack: ['cpp', 'drogon'],
  });

  const create = http.post(`${BASE}/pessoas`, payload, {
    headers: { 'Content-Type': 'application/json' },
    tags: { name: 'create-person' },
  });
  check(create, { 'create 201': (r) => r.status === 201 });

  let id = null;
  if (create.status === 201) {
    try { id = create.json('id'); } catch (_) {}
  }

  if (id) {
    const get = http.get(`${BASE}/pessoas/${id}`, { tags: { name: 'get-person' } });
    check(get, { 'get 200': (r) => r.status === 200 });
  }

  const search = http.get(`${BASE}/pessoas?t=u`, { tags: { name: 'search-person' } });
  check(search, { 'search 200': (r) => r.status === 200 });

  const invalidSearch = http.get(`${BASE}/pessoas`, { tags: { name: 'invalid-search' } });
  check(invalidSearch, { 'invalid search 400': (r) => r.status === 400 });

  const count = http.get(`${BASE}/contagem-pessoas`, { tags: { name: 'count-person' } });
  check(count, { 'count 200': (r) => r.status === 200 });

  sleep(0.1);
}

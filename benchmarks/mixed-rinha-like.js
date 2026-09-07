import http from 'k6/http';
import { check, sleep } from 'k6';

export const options = {
  stages: [
    { duration: '10s', target: 20 },
    { duration: '20s', target: 100 },
    { duration: '60s', target: 250 },
    { duration: '20s', target: 0 },
  ],
  thresholds: {
    'http_req_failed{expected_response:true}': ['rate<0.05'],
  },
};

const BASE = __ENV.BASE_URL || 'http://localhost:9999';

function createPayload() {
  const apelido = `mix${__VU}_${__ITER}_${Date.now()}`.slice(0, 32);
  return {
    apelido,
    payload: JSON.stringify({
      apelido,
      nome: 'Mixed',
      nascimento: '1990-01-01',
      stack: ['mix', 'node'],
    }),
  };
}

export default function () {
  const mod = __ITER % 20;

  if (mod < 13) {
    const { apelido, payload } = createPayload();
    const create = http.post(`${BASE}/pessoas`, payload, {
      headers: { 'Content-Type': 'application/json' },
      tags: { name: 'create-person' },
    });
    check(create, { 'create 201': (r) => r.status === 201 });

    if (create.status === 201) {
      let id = null;
      try { id = create.json('id'); } catch (_) {}
      if (id) {
        const get = http.get(`${BASE}/pessoas/${id}`, { tags: { name: 'get-person' } });
        check(get, { 'get 200': (r) => r.status === 200 });
      }
    }
  } else if (mod < 17) {
    const search = http.get(`${BASE}/pessoas?t=mix`, { tags: { name: 'search-person' } });
    check(search, { 'search 200': (r) => r.status === 200 });
  } else if (mod < 19) {
    const invalid = http.get(`${BASE}/pessoas`, { tags: { name: 'invalid-search' } });
    check(invalid, { 'invalid 400': (r) => r.status === 400 });
  } else {
    const count = http.get(`${BASE}/contagem-pessoas`, { tags: { name: 'count-person' } });
    check(count, { 'count 200': (r) => r.status === 200 });
  }

  sleep(0.05);
}

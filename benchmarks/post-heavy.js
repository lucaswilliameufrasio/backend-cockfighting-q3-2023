import http from 'k6/http';
import { check, sleep } from 'k6';

export const options = {
  stages: [
    { duration: '10s', target: 20 },
    { duration: '20s', target: 80 },
    { duration: '40s', target: 150 },
    { duration: '40s', target: 250 },
    { duration: '20s', target: 0 },
  ],
  thresholds: {
    http_req_failed: ['rate<0.05'],
  },
};

const BASE = __ENV.BASE_URL || 'http://localhost:9999';

function nick() {
  return `u${__VU}${__ITER}${Date.now()}`.slice(0, 32);
}

export default function () {
  const apelido = nick();
  const payload = JSON.stringify({
    apelido,
    nome: 'Post Heavy',
    nascimento: '1990-01-01',
    stack: ['cpp'],
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

  if (__ITER % 20 === 0) {
    const count = http.get(`${BASE}/contagem-pessoas`, { tags: { name: 'count-person' } });
    check(count, { 'count 200': (r) => r.status === 200 });
  }

  sleep(0.05);
}

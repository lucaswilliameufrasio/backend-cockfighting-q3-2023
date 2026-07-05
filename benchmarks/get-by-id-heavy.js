import http from 'k6/http';
import { check, sleep } from 'k6';

export const options = {
  vus: 100,
  duration: '60s',
  thresholds: {
    http_req_failed: ['rate<0.01'],
  },
};

const BASE = __ENV.BASE_URL || 'http://localhost:9999';

export function setup() {
  const ids = [];
  for (let i = 0; i < 300; i++) {
    const payload = JSON.stringify({
      apelido: `id${i}_${Date.now()}`.slice(0, 32),
      nome: `GetById ${i}`,
      nascimento: '1990-01-01',
      stack: ['id'],
    });
    const resp = http.post(`${BASE}/pessoas`, payload, {
      headers: { 'Content-Type': 'application/json' },
      tags: { name: 'seed-create' },
    });
    if (resp.status === 201) {
      try { ids.push(resp.json('id')); } catch (_) {}
    }
  }
  return { ids };
}

export default function (data) {
  const id = data.ids[__ITER % data.ids.length];
  const resp = http.get(`${BASE}/pessoas/${id}`, { tags: { name: 'get-person' } });
  check(resp, {
    'get 200': (r) => r.status === 200,
    'body has id': (r) => r.body.includes(id),
  });
  sleep(0.02);
}

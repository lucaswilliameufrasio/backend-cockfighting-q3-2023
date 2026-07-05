import http from 'k6/http';
import { check, sleep } from 'k6';

export const options = {
  vus: 100,
  duration: '60s',
  thresholds: {
    http_req_failed: ['rate<0.02'],
  },
};

const BASE = __ENV.BASE_URL || 'http://localhost:9999';

export function setup() {
  const terms = [];
  for (let i = 0; i < 200; i++) {
    const apelido = `seed${i}_${Date.now()}`.slice(0, 32);
    const payload = JSON.stringify({
      apelido,
      nome: `Search User ${i}`,
      nascimento: '1990-01-01',
      stack: ['node', 'postgres', `tag${i}`],
    });
    http.post(`${BASE}/pessoas`, payload, {
      headers: { 'Content-Type': 'application/json' },
      tags: { name: 'seed-create' },
    });
    terms.push(`tag${i}`);
  }
  return { terms };
}

export default function (data) {
  const hit = data.terms[__ITER % data.terms.length];
  const miss = `nohit_${__VU}_${__ITER}`;

  const hitResp = http.get(`${BASE}/pessoas?t=${hit}`, { tags: { name: 'search-hit' } });
  check(hitResp, { 'search hit 200': (r) => r.status === 200 });

  const missResp = http.get(`${BASE}/pessoas?t=${miss}`, { tags: { name: 'search-miss' } });
  check(missResp, { 'search miss 200': (r) => r.status === 200 });

  sleep(0.05);
}

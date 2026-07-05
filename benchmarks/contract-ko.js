import http from 'k6/http';
import { check } from 'k6';

export const options = {
  vus: 1,
  iterations: 1,
  thresholds: {
    http_req_failed: ['rate<0.01'],
  },
};

const BASE = __ENV.BASE_URL || 'http://localhost:9999';

export default function () {
  const validNick = `ko_${Date.now()}`.slice(0, 32);
  const validPayload = JSON.stringify({
    apelido: validNick,
    nome: 'KO Test',
    nascimento: '1990-01-01',
    stack: ['ok'],
  });

  const created = http.post(`${BASE}/pessoas`, validPayload, {
    headers: { 'Content-Type': 'application/json' },
    tags: { name: 'valid-create' },
  });
  check(created, { 'valid create 201': (r) => r.status === 201 });

  const duplicate = http.post(`${BASE}/pessoas`, validPayload, {
    headers: { 'Content-Type': 'application/json' },
    tags: { name: 'duplicate-create' },
  });
  check(duplicate, { 'duplicate 422': (r) => r.status === 422 });

  const invalidJson = http.post(`${BASE}/pessoas`, 'not json', {
    headers: { 'Content-Type': 'application/json' },
    tags: { name: 'invalid-json' },
  });
  check(invalidJson, { 'invalid json 400': (r) => r.status === 400 });

  const emptyBody = http.post(`${BASE}/pessoas`, '', {
    headers: { 'Content-Type': 'application/json' },
    tags: { name: 'empty-body' },
  });
  check(emptyBody, { 'empty body 400 or 422': (r) => r.status === 400 || r.status === 422 });

  const invalidField = http.post(`${BASE}/pessoas`, JSON.stringify({
    apelido: '',
    nome: 'Bad',
    nascimento: '1990-01-01',
  }), {
    headers: { 'Content-Type': 'application/json' },
    tags: { name: 'invalid-field' },
  });
  check(invalidField, { 'invalid field 422': (r) => r.status === 422 });

  const missingTerm = http.get(`${BASE}/pessoas`, { tags: { name: 'missing-term' } });
  check(missingTerm, { 'missing term 400': (r) => r.status === 400 });

  const notFound = http.get(`${BASE}/pessoas/00000000-0000-0000-0000-000000000000`, {
    tags: { name: 'not-found-id' },
  });
  check(notFound, { 'not found 404': (r) => r.status === 404 });

  const count = http.get(`${BASE}/contagem-pessoas`, { tags: { name: 'count-person' } });
  check(count, { 'count 200': (r) => r.status === 200 });
}

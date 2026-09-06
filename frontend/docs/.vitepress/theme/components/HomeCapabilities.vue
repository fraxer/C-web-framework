<script setup>
// Bento capability grid: HTTP/3 gets a featured double tile with a small
// "many streams → one QUIC connection" animation; the rest are compact tiles.
// Trimmed to the essentials on purpose — the linked doc page carries the rest.
import { computed } from 'vue'
import { useData } from 'vitepress'

const { lang } = useData()
const isEn = computed(() => lang.value === 'en')
const pref = computed(() => (isEn.value ? '/en' : ''))
const lc = (obj) => (isEn.value ? obj.en : obj.ru)

// Line-art glyphs (Lucide-style, stroke = currentColor) rendered inside gradient tiles.
const icons = {
  http: `<circle cx="12" cy="12" r="10"/><path d="M2 12h20"/><path d="M12 2a15.3 15.3 0 0 1 4 10 15.3 15.3 0 0 1-4 10 15.3 15.3 0 0 1-4-10 15.3 15.3 0 0 1 4-10z"/>`,
  http2: `<path d="m12 2 9 5-9 5-9-5 9-5z"/><path d="m3 12 9 5 9-5"/><path d="m3 17 9 5 9-5"/>`,
  http3: `<path d="M13 2 3 14h9l-1 8 10-12h-9l1-8z"/>`,
  ws: `<path d="M8 3 4 7l4 4"/><path d="M4 7h16"/><path d="m16 21 4-4-4-4"/><path d="M20 17H4"/>`,
  db: `<ellipse cx="12" cy="5" rx="9" ry="3"/><path d="M3 5v14a9 3 0 0 0 18 0V5"/><path d="M3 12a9 3 0 0 0 18 0"/>`,
  security: `<path d="M20 13c0 5-3.5 7.5-7.66 8.95a1 1 0 0 1-.67-.01C7.5 20.5 4 18 4 13V6a1 1 0 0 1 1-1c2 0 4.5-1.2 6.24-2.72a1.17 1.17 0 0 1 1.52 0C14.51 3.81 17 5 19 5a1 1 0 0 1 1 1z"/><path d="m9 12 2 2 4-4"/>`,
  storage: `<path d="M17.5 19H9a7 7 0 1 1 6.71-9h1.79a4.5 4.5 0 1 1 0 9Z"/>`,
  tools: `<path d="M14.7 6.3a1 1 0 0 0 0 1.4l1.6 1.6a1 1 0 0 0 1.4 0l3.77-3.77a6 6 0 0 1-7.94 7.94l-6.91 6.91a2.12 2.12 0 0 1-3-3l6.91-6.91a6 6 0 0 1 7.94-7.94l-3.76 3.76z"/>`
}
const tileSvg = (id) =>
  `<svg class="feat-icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round" aria-hidden="true">${icons[id]}</svg>`

const checkSvg = `<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.4" stroke-linecap="round" stroke-linejoin="round" aria-hidden="true"><path d="M20 6 9 17l-5-5"/></svg>`

const categories = [
  {
    id: 'http3',
    color: '#d946ef',
    color2: '#a21caf',
    xl: true,
    title: { ru: 'HTTP/3 · QUIC', en: 'HTTP/3 · QUIC' },
    desc: {
      ru: 'Собственный стек QUIC (RFC 9000) поверх UDP: рукопожатие TLS 1.3 за один RTT, нет блокировки головы очереди, миграция соединений между сетями — телефон перешёл с Wi-Fi на LTE, соединение живо.',
      en: 'A hand-written QUIC stack (RFC 9000) over UDP: TLS 1.3 handshake in one RTT, no head-of-line blocking, connection migration across networks — switch from Wi-Fi to LTE and the connection survives.'
    },
    link: '/http3',
    items: {
      ru: [
        'Параллельные потоки запросов в одном соединении',
        'QPACK-сжатие заголовков',
        'Трейлеры, 103 Early Hints, 100 Continue',
        'Автоанонс через Alt-Svc — клиенты переходят сами'
      ],
      en: [
        'Parallel request streams over one connection',
        'QPACK header compression',
        'Trailers, 103 Early Hints, 100 Continue',
        'Auto-advertised via Alt-Svc — clients upgrade themselves'
      ]
    }
  },
  {
    id: 'http2',
    color: '#0ea5e9',
    color2: '#0284c7',
    title: { ru: 'HTTP/2', en: 'HTTP/2' },
    desc: {
      ru: 'Мультиплексирование до 100 потоков, HPACK и защита от Rapid Reset.',
      en: 'Up to 100 multiplexed streams, HPACK and Rapid Reset protection.'
    },
    link: '/http2',
    items: {
      ru: ['WebSocket поверх h2 (Extended CONNECT)', 'Трейлеры и 103 Early Hints', 'h2c upgrade для plaintext'],
      en: ['WebSocket over h2 (Extended CONNECT)', 'Trailers and 103 Early Hints', 'h2c upgrade for plaintext']
    }
  },
  {
    id: 'ws',
    color: '#8b5cf6',
    color2: '#6d28d9',
    title: { ru: 'WebSocket', en: 'WebSocket' },
    desc: {
      ru: 'Каналы реального времени с broadcasting и именованными группами.',
      en: 'Real-time channels with broadcasting and named groups.'
    },
    link: '/wsrequests',
    items: {
      ru: ['Именованные каналы и фильтрация получателей', 'permessage-deflate сжатие', 'Работает поверх HTTP/1.1 и HTTP/2 (Extended CONNECT)'],
      en: ['Named channels and recipient filtering', 'permessage-deflate compression', 'Runs over HTTP/1.1 and HTTP/2 (Extended CONNECT)']
    }
  },
  {
    id: 'http',
    color: '#3b82f6',
    color2: '#2563eb',
    title: { ru: 'HTTP/1.1', en: 'HTTP/1.1' },
    desc: {
      ru: 'Полный сервер и клиент: маршрутизация, виртуальные хосты, TLS.',
      en: 'Full server and client: routing, vhosts, TLS.'
    },
    link: '/routing',
    items: {
      ru: ['Маршруты с динамическими параметрами', 'Виртуальные хосты, regex-домены, IDN', 'multipart/form-data и загрузка файлов'],
      en: ['Routes with dynamic parameters', 'Virtual hosts, regex domains, IDN', 'multipart/form-data and file uploads']
    }
  },
  {
    id: 'db',
    color: '#10b981',
    color2: '#047857',
    chips: ['PostgreSQL', 'MySQL', 'Redis', 'SQLite'],
    title: { ru: 'Базы данных', en: 'Databases' },
    desc: {
      ru: 'Четыре драйвера за единой API с ORM и миграциями.',
      en: 'Four drivers behind one unified API with ORM and migrations.'
    },
    link: '/db',
    items: {
      ru: ['ORM-модели и prepared statements', 'Миграции схемы из коробки', 'Транзакции и пулы соединений'],
      en: ['ORM models and prepared statements', 'Schema migrations out of the box', 'Transactions and connection pools']
    }
  },
  {
    id: 'security',
    color: '#f43f5e',
    color2: '#e11d48',
    title: { ru: 'Безопасность', en: 'Security' },
    desc: {
      ru: 'Аутентификация, сессии и RBAC без внешних сервисов.',
      en: 'Authentication, sessions and RBAC without external services.'
    },
    link: '/auth',
    items: {
      ru: ['PBKDF2-HMAC-SHA256 для паролей', 'Сессии: файлы, Redis, БД (AES-256-GCM)', 'Rate limiting и валидаторы данных'],
      en: ['PBKDF2-HMAC-SHA256 password hashing', 'Sessions: files, Redis, DB (AES-256-GCM)', 'Rate limiting and data validators']
    }
  },
  {
    id: 'storage',
    color: '#f59e0b',
    color2: '#ea580c',
    title: { ru: 'Хранилище и Email', en: 'Storage & Email' },
    desc: {
      ru: 'Локальный FS и S3 плюс транзакционная почта с DKIM.',
      en: 'Local FS and S3 plus transactional email with DKIM.'
    },
    link: '/storage',
    items: {
      ru: ['S3-совместимые сервисы', 'multipart-загрузка файлов в хранилище', 'SMTP-клиент и шаблоны писем'],
      en: ['S3-compatible services', 'multipart uploads into storage', 'SMTP client and email templates']
    }
  },
  {
    id: 'tools',
    color: '#06b6d4',
    color2: '#0891b2',
    title: { ru: 'Инструменты', en: 'Tooling' },
    desc: {
      ru: 'Шаблонизатор, i18n, JSON, JWT и планировщик задач.',
      en: 'Template engine, i18n, JSON, JWT and the task scheduler.'
    },
    link: '/view',
    items: {
      ru: ['i18n на gettext: плюрализм, fallback', 'Планировщик: interval, daily, weekly', 'str_t с SSO, HashMap, JSON-парсер'],
      en: ['gettext i18n: plurals, fallback', 'Scheduler: interval, daily, weekly', 'str_t with SSO, HashMap, JSON parser']
    }
  }
]

const lanes = [
  { label: '/api/users', color: '#e879f9' },
  { label: '/static/app.js', color: '#38bdf8' },
  { label: '/chat · ws', color: '#a78bfa' }
]

const moreText = computed(() => (isEn.value ? 'Learn more' : 'Подробнее'))
const kicker = computed(() => (isEn.value ? 'Capabilities' : 'Возможности'))
const title = computed(() =>
  isEn.value ? 'Everything you need in one framework' : 'Всё необходимое в одном фреймворке'
)
const desc = computed(() =>
  isEn.value
    ? 'A complete toolkit for modern web services — protocols, databases, security and utilities out of the box.'
    : 'Полный набор инструментов для современных веб-сервисов — протоколы, базы данных, безопасность и утилиты из коробки.'
)
const laneCaption = computed(() =>
  isEn.value ? 'dozens of streams → one UDP connection' : 'десятки потоков → одно UDP-соединение'
)
const allTitle = computed(() => (isEn.value ? 'And that is not all' : 'И это ещё не всё'))
const allDesc = computed(() =>
  isEn.value
    ? 'Hot reload of handlers, logging, the HTTP client, scheduled tasks and more — in the docs.'
    : 'Горячая перезагрузка обработчиков, логирование, HTTP-клиент, планировщик задач и другое — в документации.'
)
const allLinkText = computed(() => (isEn.value ? 'Open the docs' : 'Открыть документацию'))
</script>

<template>
  <div class="home-extras">
    <section class="home-section">
      <div class="home-head">
        <span class="home-kicker">{{ kicker }}</span>
        <h2 class="home-title">{{ title }}</h2>
        <p class="home-desc">{{ desc }}</p>
      </div>

      <div class="bento">
        <a
          v-for="c in categories"
          :key="c.id"
          class="feat-card"
          :class="{ 'feat-card--xl': c.xl }"
          :href="`${pref}${c.link}`"
          :style="{ '--feat': c.color, '--feat-2': c.color2 }"
        >
          <span class="feat-tile" v-html="tileSvg(c.id)" />
          <h3 class="feat-title">{{ lc(c.title) }}</h3>
          <p class="feat-desc">{{ lc(c.desc) }}</p>

          <span v-if="c.chips" class="feat-chips">
            <span v-for="chip in c.chips" :key="chip" class="feat-chip">{{ chip }}</span>
          </span>

          <ul class="feat-list">
            <li v-for="(item, idx) in lc(c.items)" :key="idx" class="feat-item">
              <span v-html="checkSvg" />
              <span>{{ item }}</span>
            </li>
          </ul>

          <!-- Featured tile: packets from several streams merge into one QUIC pipe -->
          <div v-if="c.xl" class="quic-viz" aria-hidden="true">
            <div v-for="lane in lanes" :key="lane.label" class="quic-lane">
              <span class="quic-lane-label">{{ lane.label }}</span>
              <span class="quic-lane-track">
                <i class="quic-packet" :style="{ '--pc': lane.color }" />
                <i class="quic-packet d2" :style="{ '--pc': lane.color }" />
                <i class="quic-packet d3" :style="{ '--pc': lane.color }" />
              </span>
            </div>
            <div class="quic-merge">
              <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" aria-hidden="true"><path d="M13 2 3 14h9l-1 8 10-12h-9l1-8z"/></svg>
              <span>{{ laneCaption }}</span>
            </div>
          </div>

          <span class="feat-more">{{ moreText }} <span class="arrow">→</span></span>
        </a>

        <a class="feat-card feat-card--more" :href="`${pref}/introduction`">
          <h3 class="feat-title">{{ allTitle }}</h3>
          <p class="feat-desc">{{ allDesc }}</p>
          <span class="feat-more">{{ allLinkText }} <span class="arrow">→</span></span>
        </a>
      </div>
    </section>
  </div>
</template>

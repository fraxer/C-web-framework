<script setup>
import { computed } from 'vue'
import { useData } from 'vitepress'

const { lang } = useData()
const isEn = computed(() => lang.value === 'en')
const pref = computed(() => (isEn.value ? '/en' : ''))
const lc = (obj) => (isEn.value ? obj.en : obj.ru)

// Line-art glyphs (Lucide-style, stroke = currentColor) rendered inside gradient tiles.
const icons = {
  http: `<circle cx="12" cy="12" r="10"/><path d="M2 12h20"/><path d="M12 2a15.3 15.3 0 0 1 4 10 15.3 15.3 0 0 1-4 10 15.3 15.3 0 0 1-4-10 15.3 15.3 0 0 1 4-10z"/>`,
  ws: `<path d="M8 3 4 7l4 4"/><path d="M4 7h16"/><path d="m16 21 4-4-4-4"/><path d="M20 17H4"/>`,
  db: `<ellipse cx="12" cy="5" rx="9" ry="3"/><path d="M3 5v14a9 3 0 0 0 18 0V5"/><path d="M3 12a9 3 0 0 0 18 0"/>`,
  security: `<path d="M20 13c0 5-3.5 7.5-7.66 8.95a1 1 0 0 1-.67-.01C7.5 20.5 4 18 4 13V6a1 1 0 0 1 1-1c2 0 4.5-1.2 6.24-2.72a1.17 1.17 0 0 1 1.52 0C14.51 3.81 17 5 19 5a1 1 0 0 1 1 1z"/><path d="m9 12 2 2 4-4"/>`,
  storage: `<path d="M17.5 19H9a7 7 0 1 1 6.71-9h1.79a4.5 4.5 0 1 1 0 9Z"/>`,
  tools: `<path d="M14.7 6.3a1 1 0 0 0 0 1.4l1.6 1.6a1 1 0 0 0 1.4 0l3.77-3.77a6 6 0 0 1-7.94 7.94l-6.91 6.91a2.12 2.12 0 0 1-3-3l6.91-6.91a6 6 0 0 1 7.94-7.94l-3.76 3.76z"/>`
}
const tileSvg = (id) =>
  `<svg class="feat-icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round" aria-hidden="true">${icons[id]}</svg>`

const categories = [
  {
    id: 'http',
    color: '#3b82f6',
    color2: '#2563eb',
    title: { ru: 'HTTP/1.1', en: 'HTTP/1.1' },
    desc: {
      ru: 'Полный HTTP/1.1 сервер и клиент — маршрутизация, виртуальные хосты, middleware и TLS.',
      en: 'A complete HTTP/1.1 server and client — routing, virtual hosts, middleware and TLS.'
    },
    link: '/routing',
    items: {
      ru: [
        'Полная поддержка HTTP/1.1',
        'Гибкая маршрутизация с динамическими параметрами',
        'Виртуальные хосты с regex-доменами и поддержкой IDN',
        'Middleware и фильтры: gzip, range, chunked, cache control',
        'Cookie с secure, httpOnly, sameSite',
        'Обработка multipart/form-data и загрузка файлов',
        'Автоматическое сжатие gzip для поддерживаемых типов',
        'Редиректы с регулярными выражениями и группами захвата',
        'Встроенный HTTP-клиент: TLS 1.2+, keep-alive pool, редиректы'
      ],
      en: [
        'Full HTTP/1.1 support',
        'Flexible routing with dynamic parameters',
        'Virtual hosts with regex domains and IDN support',
        'Middleware and filters: gzip, range, chunked, cache control',
        'Cookies with secure, httpOnly, sameSite',
        'multipart/form-data parsing and file uploads',
        'Automatic gzip compression for supported content types',
        'Redirects with regular expressions and capture groups',
        'Built-in HTTP client: TLS 1.2+, keep-alive pool, redirects'
      ]
    }
  },
  {
    id: 'ws',
    color: '#8b5cf6',
    color2: '#6d28d9',
    title: { ru: 'WebSocket', en: 'WebSocket' },
    desc: {
      ru: 'Двунаправленные каналы реального времени с broadcasting и именованными группами.',
      en: 'Bidirectional real-time channels with broadcasting and named recipient groups.'
    },
    link: '/wsrequests',
    items: {
      ru: [
        'Полная поддержка протокола WebSocket',
        'Система broadcasting для групп клиентов',
        'Именованные каналы с фильтрацией получателей',
        'Встроенная поддержка JSON-сообщений',
        'Middleware для WebSocket-запросов'
      ],
      en: [
        'Full WebSocket protocol support',
        'Broadcasting system for groups of clients',
        'Named channels with recipient filtering',
        'Built-in JSON message support',
        'Middleware for WebSocket requests'
      ]
    }
  },
  {
    id: 'db',
    color: '#10b981',
    color2: '#047857',
    title: { ru: 'Базы данных', en: 'Databases' },
    desc: {
      ru: 'PostgreSQL, MySQL, SQLite и Redis за единой API с ORM и миграциями.',
      en: 'PostgreSQL, MySQL, SQLite and Redis behind one unified API with ORM and migrations.'
    },
    link: '/db',
    items: {
      ru: [
        'PostgreSQL — нативная поддержка с prepared statements',
        'MySQL — нативная поддержка с защитой от SQL-инъекций',
        'SQLite — встраиваемая БД без отдельного сервера',
        'Redis — для кеширования и сессий',
        'ORM-модели для работы с таблицами',
        'Миграции — версионирование схемы базы данных',
        'Query Builder — безопасное построение SQL-запросов',
        'Транзакции и connection pool'
      ],
      en: [
        'PostgreSQL — native support with prepared statements',
        'MySQL — native support with SQL injection protection',
        'SQLite — embedded database without a separate server',
        'Redis — for caching and sessions',
        'ORM models for working with tables',
        'Migrations — database schema versioning',
        'Query Builder — safe SQL query construction',
        'Transactions and connection pool'
      ]
    }
  },
  {
    id: 'security',
    color: '#f43f5e',
    color2: '#e11d48',
    title: { ru: 'Безопасность', en: 'Security' },
    desc: {
      ru: 'Аутентификация, сессии, RBAC, rate limiting и современное хеширование паролей.',
      en: 'Authentication, sessions, RBAC, rate limiting and modern password hashing.'
    },
    link: '/auth',
    items: {
      ru: [
        'Встроенная система регистрации и авторизации',
        'Сессии на файлах, в Redis и в базе данных',
        'Секреты сессий защищены через AES-256-GCM',
        'Хеширование паролей PBKDF2-HMAC-SHA256',
        'Валидаторы email, паролей и других данных',
        'RBAC — система ролевого доступа',
        'Rate Limiting — защита от DDoS'
      ],
      en: [
        'Built-in registration and authorization system',
        'Sessions in files, Redis and the database',
        'Session secrets protected with AES-256-GCM',
        'Password hashing with PBKDF2-HMAC-SHA256',
        'Validators for email, passwords and other data',
        'RBAC — role-based access control',
        'Rate Limiting — DDoS protection'
      ]
    }
  },
  {
    id: 'storage',
    color: '#f59e0b',
    color2: '#ea580c',
    title: { ru: 'Хранилище и Email', en: 'Storage & Email' },
    desc: {
      ru: 'Локальное и S3-хранилище плюс транзакционная почта с DKIM.',
      en: 'Local and S3 storage plus transactional email with DKIM.'
    },
    link: '/storage',
    items: {
      ru: [
        'Локальное файловое хранилище',
        'S3-совместимые сервисы',
        'CRUD-операции над файлами',
        'multipart-загрузка с сохранением в хранилище',
        'SMTP-клиент для отправки писем',
        'DKIM-подписи для аутентификации отправителя',
        'Шаблоны писем'
      ],
      en: [
        'Local file storage',
        'S3-compatible services',
        'CRUD operations on files',
        'multipart uploads saved to storage',
        'SMTP client for sending email',
        'DKIM signatures for sender authentication',
        'Email templates'
      ]
    }
  },
  {
    id: 'tools',
    color: '#06b6d4',
    color2: '#0891b2',
    title: { ru: 'Инструменты', en: 'Tooling' },
    desc: {
      ru: 'Шаблонизатор, i18n, JSON, JWT, планировщик задач и набор str_t / HashMap.',
      en: 'Template engine, i18n, JSON, JWT, task scheduler and the str_t / HashMap toolkit.'
    },
    link: '/view',
    items: {
      ru: [
        'Шаблонизатор: переменные, циклы, интеграция с моделями',
        'i18n на базе gettext: плюрализм, автоопределение языка, fallback',
        'Высокопроизводительный JSON-парсер и сериализация',
        'JWT, UUID, Base64, SHA-1/256',
        'Планировщик задач: interval, daily, weekly, monthly',
        'AES-256-GCM, генератор случайных чисел',
        'str_t с SSO, Array, HashMap/Map, упорядоченная очередь'
      ],
      en: [
        'Template engine: variables, loops, model integration',
        'i18n on gettext: pluralization, language auto-detection, fallback',
        'High-performance JSON parser and serialization',
        'JWT, UUID, Base64, SHA-1/256',
        'Task scheduler: interval, daily, weekly, monthly',
        'AES-256-GCM, random number generator',
        'str_t with SSO, Array, HashMap/Map, ordered queue'
      ]
    }
  }
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
</script>

<template>
  <div class="home-extras">
    <section class="home-section">
      <div class="home-head">
        <span class="home-kicker">{{ kicker }}</span>
        <h2 class="home-title">{{ title }}</h2>
        <p class="home-desc">{{ desc }}</p>
      </div>

      <div class="feat-grid">
        <a
          v-for="c in categories"
          :key="c.id"
          class="feat-card"
          :href="`${pref}${c.link}`"
          :style="{ '--feat': c.color, '--feat-2': c.color2 }"
        >
          <span class="feat-tile" v-html="tileSvg(c.id)" />
          <h3 class="feat-title">{{ lc(c.title) }}</h3>
          <p class="feat-desc">{{ lc(c.desc) }}</p>

          <ul class="feat-list">
            <li v-for="(item, idx) in lc(c.items)" :key="idx" class="feat-item">
              <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.4" stroke-linecap="round" stroke-linejoin="round" aria-hidden="true"><path d="M20 6 9 17l-5-5"/></svg>
              <span>{{ item }}</span>
            </li>
          </ul>

          <span class="feat-more">{{ moreText }} <span class="arrow">→</span></span>
        </a>
      </div>
    </section>
  </div>
</template>

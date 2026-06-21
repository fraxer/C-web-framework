<script setup>
import { computed } from 'vue'
import { useData } from 'vitepress'

const { lang } = useData()
const isEn = computed(() => lang.value === 'en')
const pref = computed(() => (isEn.value ? '/en' : ''))
const gh = 'https://github.com/fraxer/C-web-framework'

// Lucide-style line glyphs (stroke = currentColor), consistent with the
// capability tiles so the whole page shares one icon language.
const icons = {
  api: '<path d="M8 3H7a2 2 0 0 0-2 2v5a2 2 0 0 1-2 2 2 2 0 0 1 2 2v5c0 1.1.9 2 2 2h1"/><path d="M16 21h1a2 2 0 0 0 2-2v-5c0-1.1.9-2 2-2a2 2 0 0 1-2-2V5a2 2 0 0 0-2-2h-1"/>',
  realtime: '<polygon points="13 2 3 14 12 14 11 22 21 10 12 10 13 2"/>',
  layers: '<path d="m12.83 2.18a2 2 0 0 0-1.66 0L2.6 6.08a1 1 0 0 0 0 1.83l8.58 3.91a2 2 0 0 0 1.66 0l8.58-3.9a1 1 0 0 0 0-1.83Z"/><path d="m22 17.65-9.17 4.16a2 2 0 0 1-1.66 0L2 17.65"/><path d="m22 12.65-9.17 4.16a2 2 0 0 1-1.66 0L2 12.65"/>',
  gateway: '<path d="m18 14 4 4-4 4"/><path d="m18 2 4 4-4 4"/><path d="M2 18h1.97a4 4 0 0 0 3.3-1.7l5.45-8.6a4 4 0 0 1 3.3-1.7H22"/><path d="M2 6h1.97a4 4 0 0 1 3.6 2.2"/><path d="M22 18h-1.97a4 4 0 0 1-3.6-2.2"/>',
  shield: '<path d="M20 13c0 5-3.5 7.5-7.66 8.95a1 1 0 0 1-.67-.01C7.5 20.5 4 18 4 13V6a1 1 0 0 1 1-1c2 0 4.5-1.2 6.24-2.72a1.17 1.17 0 0 1 1.52 0C14.51 3.81 17 5 19 5a1 1 0 0 1 1 1z"/>',
  upload: '<path d="M21 15v4a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-4"/><path d="M17 8l-5-5-5 5"/><path d="M12 3v12"/>',
  mail: '<rect width="20" height="16" x="2" y="4" rx="2"/><path d="m22 7-8.97 5.7a1.94 1.94 0 0 1-2.06 0L2 7"/>',
  globe: '<circle cx="12" cy="12" r="10"/><path d="M2 12h20"/><path d="M12 2a15.3 15.3 0 0 1 4 10 15.3 15.3 0 0 1-4 10 15.3 15.3 0 0 1-4-10 15.3 15.3 0 0 1 4-10z"/>'
}
const iconSvg = (id) =>
  `<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round" aria-hidden="true">${icons[id]}</svg>`

const useCases = computed(() =>
  isEn.value
    ? [
        { icon: 'api', title: 'REST API', text: 'High-performance APIs for mobile and web applications' },
        { icon: 'realtime', title: 'Real-time', text: 'Chats, notifications and live updates via WebSocket' },
        { icon: 'layers', title: 'Microservices', text: 'Microservices and monolithic applications' },
        { icon: 'gateway', title: 'API Gateway', text: 'Request routing and proxying between services' },
        { icon: 'shield', title: 'Admin panels', text: 'Authentication, RBAC and CRUD operations' },
        { icon: 'upload', title: 'File uploads', text: 'Local and S3-compatible cloud storage' },
        { icon: 'mail', title: 'Email services', text: 'Transactional email sending with DKIM' },
        { icon: 'globe', title: 'i18n', text: 'Multilingual content with gettext and pluralization' }
      ]
    : [
        { icon: 'api', title: 'REST API', text: 'Высокопроизводительные API для мобильных и веб-приложений' },
        { icon: 'realtime', title: 'Real-time', text: 'Чаты, уведомления и live-обновления через WebSocket' },
        { icon: 'layers', title: 'Микросервисы', text: 'Микросервисы и монолитные приложения' },
        { icon: 'gateway', title: 'API Gateway', text: 'Маршрутизация и проксирование запросов между сервисами' },
        { icon: 'shield', title: 'Админ-панели', text: 'Аутентификация, RBAC и CRUD-операции' },
        { icon: 'upload', title: 'Загрузка файлов', text: 'Локальное и S3-совместимое облачное хранилище' },
        { icon: 'mail', title: 'Email-сервисы', text: 'Транзакционная отправка писем с DKIM' },
        { icon: 'globe', title: 'i18n', text: 'Многоязычный контент на базе gettext и плюрализма' }
      ]
)

const kicker = computed(() => (isEn.value ? 'Use Cases' : 'Области применения'))
const title = computed(() => (isEn.value ? 'What you can build' : 'Где применяется фреймворк'))

const ctaKicker = computed(() => (isEn.value ? 'Get started' : 'Начните прямо сейчас'))
const ctaTitle = computed(() => (isEn.value ? 'Build fast web services in C' : 'Создавайте быстрые веб-сервисы на Си'))
const ctaDesc = computed(() =>
  isEn.value
    ? 'Read the introduction, browse the examples, or open the source on GitHub.'
    : 'Изучите введение, посмотрите примеры или откройте исходный код на GitHub.'
)
const docsText = computed(() => (isEn.value ? 'Documentation' : 'Документация'))
const examplesText = computed(() => (isEn.value ? 'Examples' : 'Примеры'))
</script>

<template>
  <div class="home-extras">
    <section class="home-section">
      <div class="home-head">
        <span class="home-kicker">{{ kicker }}</span>
        <h2 class="home-title">{{ title }}</h2>
      </div>

      <div class="home-grid">
        <div class="home-card" v-for="u in useCases" :key="u.title">
          <span class="home-card-icon" v-html="iconSvg(u.icon)" />
          <h3 class="home-card-title">{{ u.title }}</h3>
          <p class="home-card-text">{{ u.text }}</p>
        </div>
      </div>
    </section>

    <section class="home-cta">
      <span class="home-kicker">{{ ctaKicker }}</span>
      <h2>{{ ctaTitle }}</h2>
      <p>{{ ctaDesc }}</p>
      <div class="home-cta-actions">
        <a class="home-btn home-btn--brand" :href="`${pref}/introduction`">{{ docsText }}</a>
        <a class="home-btn home-btn--ghost" :href="`${pref}/examples-req-res`">{{ examplesText }}</a>
        <a class="home-btn home-btn--ghost" :href="gh" target="_blank" rel="noopener">GitHub</a>
      </div>
    </section>
  </div>
</template>

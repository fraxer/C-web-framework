<script setup>
// Count-up stats strip under the hero. Numbers animate from 0 when the strip
// scrolls into view (once); under prefers-reduced-motion they render final.
import { computed, onBeforeUnmount, onMounted, ref } from 'vue'
import { useData } from 'vitepress'

const { lang } = useData()
const isEn = computed(() => lang.value === 'en')

const items = computed(() => [
  {
    value: 3,
    suffix: '',
    label: isEn.value ? 'HTTP protocols' : 'HTTP-протокола',
    sub: isEn.value ? 'HTTP/1.1 · HTTP/2 · HTTP/3' : 'HTTP/1.1 · HTTP/2 · HTTP/3'
  },
  {
    value: 4,
    suffix: '',
    label: isEn.value ? 'database drivers' : 'драйвера баз данных',
    sub: 'PostgreSQL · MySQL · Redis · SQLite'
  },
  {
    value: 100,
    prefix: '~',
    suffix: ' µs',
    label: isEn.value ? 'per request' : 'на запрос',
    sub: isEn.value ? 'typical latency under load' : 'типичная задержка под нагрузкой'
  },
  {
    value: null,
    literal: 'MIT',
    label: isEn.value ? 'open source' : 'открытый код',
    sub: isEn.value ? 'source code freely available on GitHub' : 'исходный код свободно доступен на GitHub'
  }
])

const rootEl = ref(null)
// Start at the final numbers so SSG/no-JS shows real values; the count-up
// resets them to 0 the moment the strip enters the viewport.
const shown = ref(items.value.map((it) => it.value))
let raf = null
let observer = null

function animate() {
  const t0 = performance.now()
  const dur = 1100
  const ease = (x) => 1 - Math.pow(1 - x, 3)
  const step = (now) => {
    const k = Math.min(1, (now - t0) / dur)
    shown.value = items.value.map((it) =>
      typeof it.value === 'number' ? Math.round(ease(k) * it.value) : null
    )
    if (k < 1) raf = requestAnimationFrame(step)
  }
  raf = requestAnimationFrame(step)
}

onMounted(() => {
  const reduced = window.matchMedia('(prefers-reduced-motion: reduce)').matches
  if (reduced) {
    shown.value = items.value.map((it) => it.value)
    return
  }
  observer = new IntersectionObserver(
    (entries) => {
      if (entries[0].isIntersecting) {
        observer.disconnect()
        animate()
      }
    },
    { threshold: 0.4 }
  )
  observer.observe(rootEl.value)
})

onBeforeUnmount(() => {
  observer?.disconnect()
  if (raf) cancelAnimationFrame(raf)
})

const display = (it, i) =>
  typeof it.value === 'number' ? (it.prefix ?? '') + shown.value[i] + it.suffix : it.literal
</script>

<template>
  <div class="home-extras">
    <div ref="rootEl" class="stats-strip">
      <div v-for="(it, i) in items" :key="it.label" class="stats-item">
        <span class="stats-value">{{ display(it, i) }}</span>
        <span class="stats-label">{{ it.label }}</span>
        <span class="stats-sub">{{ it.sub }}</span>
      </div>
    </div>
  </div>
</template>

<script setup>
// Live terminal: types the server launch command, prints the boot log, then
// streams sampled request lines with a ticking req/s · p99 · conn footer.
// The whole thing is a simulation — nothing connects anywhere — but every
// number (latency ranges, protocol mix) mirrors the real benchmark profile.
// Pauses while offscreen (IntersectionObserver) and renders a static final
// state under prefers-reduced-motion.
import { computed, onBeforeUnmount, onMounted, ref } from 'vue'
import { useData } from 'vitepress'

const { lang } = useData()
const isEn = computed(() => lang.value === 'en')

const command = './exec/cwfr -c config.json'

const bootLines = [
  { tag: 'main', text: '8 workers · 32 threads · epoll' },
  { tag: 'tcp', text: 'http/1.1  :8080' },
  { tag: 'tls', text: 'http/2    :443   alpn h2' },
  { tag: 'udp', text: 'http/3    :443   quic v1 v2' },
  { tag: 'db', text: 'postgres · mysql · redis · sqlite ready' },
  { tag: 'main', text: 'listening — started in 84 ms' }
]

// Sampled request log: method, path, status, latency, protocol tag.
const pool = [
  { m: 'GET', p: '/api/users', s: 200, t: '87 µs', proto: 'h3' },
  { m: 'GET', p: '/api/orders', s: 200, t: '94 µs', proto: 'h3' },
  { m: 'POST', p: '/api/login', s: 200, t: '132 µs', proto: 'h2' },
  { m: 'GET', p: '/static/app.js', s: 200, t: '61 µs', proto: 'h2' },
  { m: 'GET', p: '/static/app.css', s: 304, t: '—', proto: 'h2' },
  { m: 'WS', p: '/chat', s: 101, t: '—', proto: 'h2' },
  { m: 'GET', p: '/api/items', s: 200, t: '79 µs', proto: 'h3' },
  { m: 'POST', p: '/api/cart', s: 201, t: '118 µs', proto: 'h3' },
  { m: 'GET', p: '/health', s: 200, t: '12 µs', proto: 'h1' },
  { m: 'GET', p: '/api/session', s: 200, t: '65 µs', proto: 'h3' },
  { m: 'DELETE', p: '/api/token', s: 204, t: '58 µs', proto: 'h2' },
  { m: 'GET', p: '/img/logo.svg', s: 200, t: '24 µs', proto: 'h2' },
  { m: 'GET', p: '/api/profile', s: 200, t: '71 µs', proto: 'h3' },
  { m: 'PUT', p: '/api/profile', s: 200, t: '126 µs', proto: 'h2' }
]

const MAX_LOG_LINES = 8

const typed = ref('')          // command typed so far
const shownBoot = ref(0)       // boot lines revealed
const log = ref([])            // visible request lines
const stats = ref({ rps: 0, p99: 0, conn: 0 })
const live = ref(false)        // footer badge switches on with the first tick
const rootEl = ref(null)

let timers = []
let streamTimer = null
let statsTimer = null
let observer = null
let phase = 'idle'             // idle → typing → boot → streaming
let uid = 0                    // per-line key so each new line mounts fresh
let tick = 0                   // stats jitter step

const fmt = (n) => new Intl.NumberFormat(isEn.value ? 'en-US' : 'ru-RU').format(Math.round(n))

function sleep(ms) {
  return new Promise((resolve) => {
    const id = setTimeout(resolve, ms)
    timers.push(id)
  })
}

const pick = () => pool[Math.floor(Math.random() * pool.length)]

async function runBoot() {
  await sleep(450)
  // Type the launch command character by character.
  for (const ch of command) {
    typed.value += ch
    await sleep(16 + Math.random() * 30)
  }
  await sleep(300)
  // Boot log line by line.
  for (let i = 0; i < bootLines.length; i++) {
    shownBoot.value = i + 1
    await sleep(150 + Math.random() * 90)
  }
  await sleep(350)
  phase = 'streaming'
  startStream()
}

function startStream() {
  stopStream()
  const pushLine = () => {
    log.value = [...log.value.slice(-(MAX_LOG_LINES - 1)), { ...pick(), id: ++uid }]
  }
  // Pre-fill two lines so the stream does not start from a bare window.
  pushLine()
  pushLine()
  const schedule = () => {
    streamTimer = setTimeout(() => {
      pushLine()
      schedule()
    }, 240 + Math.random() * 260)
  }
  schedule()

  live.value = true
  const updateStats = () => {
    tick++
    const wave = Math.sin(tick / 7) * 1800
    stats.value = {
      rps: 94600 + wave + Math.random() * 1400,
      p99: 0.38 + Math.random() * 0.07,
      conn: 1210 + Math.sin(tick / 11) * 60 + Math.random() * 24
    }
  }
  updateStats()
  statsTimer = setInterval(updateStats, 900)
}

function stopStream() {
  if (streamTimer) clearTimeout(streamTimer)
  if (statsTimer) clearInterval(statsTimer)
  streamTimer = null
  statsTimer = null
}

function clearTimers() {
  timers.forEach(clearTimeout)
  timers = []
  stopStream()
}

onMounted(() => {
  const reduced = window.matchMedia('(prefers-reduced-motion: reduce)').matches
  if (reduced) {
    // Static final state — no animation at all.
    typed.value = command
    shownBoot.value = bootLines.length
    log.value = [0, 3, 6, 2, 9, 5, 1, 11].map((l) => ({ ...pool[l], id: ++uid }))
    stats.value = { rps: 96412, p99: 0.42, conn: 1247 }
    live.value = true
    phase = 'streaming'
    return
  }

  observer = new IntersectionObserver(
    (entries) => {
      const visible = entries[0].isIntersecting
      if (visible && phase === 'idle') {
        phase = 'typing'
        runBoot()
      } else if (visible && phase === 'streaming' && !statsTimer) {
        startStream() // resumed after being offscreen
      } else if (!visible && phase === 'streaming') {
        stopStream()
      }
    },
    { threshold: 0.15 }
  )
  observer.observe(rootEl.value)
})

onBeforeUnmount(() => {
  clearTimers()
  observer?.disconnect()
})
</script>

<template>
  <div class="hero-visual">
    <span class="hero-glow" aria-hidden="true" />
    <div ref="rootEl" class="term">
      <div class="term-head">
        <span class="dot red" /><span class="dot yellow" /><span class="dot green" />
        <span class="term-title">cwfr — bash</span>
        <span class="term-live" :class="{ on: live }">
          <span class="term-live-dot" />
          live
        </span>
      </div>

      <div class="term-body">
        <div class="term-cmd">
          <span class="term-prompt">$</span>
          <span class="term-cmd-text">{{ typed }}</span>
          <span class="term-cursor" :class="{ off: live }" />
        </div>

        <div class="term-boot">
          <div v-for="(l, i) in bootLines.slice(0, shownBoot)" :key="i" class="term-boot-line">
            <span class="term-boot-tag" :data-tag="l.tag">{{ l.tag }}</span>
            <span class="term-boot-text">{{ l.text }}</span>
          </div>
        </div>

        <div class="term-log" aria-hidden="true">
          <div v-for="l in log" :key="l.id" class="term-line">
            <span class="term-m" :data-m="l.m">{{ l.m }}</span>
            <span class="term-p">{{ l.p }}</span>
            <span class="term-s" :class="{ ok: l.s < 400 }">{{ l.s }}</span>
            <span class="term-t">{{ l.t }}</span>
            <span class="term-proto" :data-proto="l.proto">{{ l.proto }}</span>
          </div>
        </div>
      </div>

      <div class="term-foot">
        <span class="term-foot-item">
          <span class="term-foot-num">{{ fmt(stats.rps) }}</span> req/s
        </span>
        <span class="term-foot-sep" />
        <span class="term-foot-item">p99 <span class="term-foot-num">{{ stats.p99.toFixed(2) }}</span> ms</span>
        <span class="term-foot-sep" />
        <span class="term-foot-item"><span class="term-foot-num">{{ fmt(stats.conn) }}</span> conn</span>
      </div>
    </div>
  </div>
</template>

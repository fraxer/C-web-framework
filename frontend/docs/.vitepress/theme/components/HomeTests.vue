<script setup>
// Testing section: the test layers as cards, then three comparisons — line
// coverage of the core per subsystem (tests vs fuzzing), how the two
// layers overlap, and how much test code stands behind the core.
//
// The coverage numbers are a measurement, not a target. They come from a
// --coverage build of backend/core (Debug + ASan, gcov): `runner`, then
// `db_runner` against PostgreSQL, then every fuzz target for 60 s through
// tests/fuzz/run.sh. A line counts as covered when any object that compiles
// it executed it — `runner` builds some core sources into itself. The
// integration suites drive a separate live server and are not in the numbers,
// which is why the server runtime reads low. Remeasure and replace the
// constants below when the suite changes noticeably.
import { computed, ref } from 'vue'
import { useData } from 'vitepress'

const { lang } = useData()
const isEn = computed(() => lang.value === 'en')
const lc = (obj) => (isEn.value ? obj.en : obj.ru)
const testsRepo = 'https://github.com/fraxer/C-web-framework-core/tree/master/tests'

const MEASURED = {
  date: { ru: '1 октября 2026', en: 'October 1, 2026' },
  lines: 46007,
  tests: 65.1,
  fuzz: 51.0,
  union: 70.0
}

// Share of the core's executable lines by who reaches them.
const SPLIT = [
  { cls: 's1', value: 19.0, label: { ru: 'Только тесты', en: 'Tests only' } },
  { cls: 's3', value: 46.0, label: { ru: 'Тесты и фаззинг', en: 'Tests and fuzzing' } },
  { cls: 's2', value: 5.0, label: { ru: 'Только фаззинг', en: 'Fuzzing only' } },
  { cls: 's0', value: 30.0, label: { ru: 'Не достигнуто', en: 'Not reached' } }
]

// v: [tests (unit + DB), fuzzing] — percent of the subsystem's lines, each
// on its own. Sorted by test coverage.
const SUBSYSTEMS = [
  { id: 'quic', lines: 5236, v: [88.2, 78.6], name: { ru: 'QUIC', en: 'QUIC' },
    hint: { ru: 'транспорт, криптография, восстановление потерь', en: 'transport, crypto, loss recovery' } },
  { id: 'ws', lines: 1640, v: [87.7, 77.9], name: { ru: 'WebSocket', en: 'WebSocket' },
    hint: { ru: 'парсер кадров, permessage-deflate, рассылка', en: 'frame parser, permessage-deflate, broadcasting' } },
  { id: 'h3', lines: 2725, v: [85.2, 71.0], name: { ru: 'HTTP/3 · QPACK', en: 'HTTP/3 · QPACK' },
    hint: { ru: 'сессии, кадры, сжатие заголовков', en: 'sessions, frames, header compression' } },
  { id: 'smtp', lines: 2000, v: [73.8, 50.8], name: { ru: 'SMTP · почта', en: 'SMTP · mail' },
    hint: { ru: 'клиент, письма, DKIM', en: 'client, messages, DKIM' } },
  { id: 'h1', lines: 7820, v: [72.9, 54.8], name: { ru: 'HTTP/1.1 · клиент', en: 'HTTP/1.1 · client' },
    hint: { ru: 'парсеры, фильтры ответа, HTTP-клиент', en: 'parsers, response filters, HTTP client' } },
  { id: 'misc', lines: 6079, v: [70.9, 68.9], name: { ru: 'Утилиты', en: 'Utilities' },
    hint: { ru: 'строки, JSON, JWT, base64, SHA, gzip, UTF-8', en: 'strings, JSON, JWT, base64, SHA, gzip, UTF-8' } },
  { id: 'fw', lines: 4396, v: [59.3, 35.4], name: { ru: 'Шаблоны, формы, сессии', en: 'Views, forms, sessions' },
    hint: { ru: 'а также хранилища, i18n, middleware, планировщик', en: 'plus storage, i18n, middleware, scheduler' } },
  { id: 'db', lines: 5612, v: [51.5, 27.2], name: { ru: 'БД и ORM', en: 'Databases & ORM' },
    hint: { ru: 'драйверы и модели; БД-тесты — на PostgreSQL', en: 'drivers and models; DB tests ran on PostgreSQL' } },
  { id: 'h2', lines: 2559, v: [50.7, 73.6], name: { ru: 'HTTP/2 · HPACK', en: 'HTTP/2 · HPACK' },
    hint: { ru: 'сессии, кадры, сжатие заголовков', en: 'sessions, frames, header compression' } },
  { id: 'rt', lines: 7940, v: [41.3, 21.2], name: { ru: 'Рантайм сервера', en: 'Server runtime' },
    hint: { ru: 'event loop, воркеры, конфиг, загрузка модулей — их гоняют интеграционные сценарии вне замера', en: 'event loop, workers, config, module loading — driven by the integration suites outside this measurement' } }
]

// Lines of code, blank lines and comments included, counted the same way on
// both sides: backend/core/**/*.{c,h} without tests/ and apps/, and tests/.
const CODE = {
  core: 106213,
  tests: [
    { cls: 's1', value: 81396, label: { ru: 'Unit-тесты', en: 'Unit tests' } },
    { cls: 's2', value: 14886, label: { ru: 'Фаззинг', en: 'Fuzzing' } },
    { cls: 's3', value: 13595, label: { ru: 'БД, интеграция, стенды', en: 'DB, integration, harnesses' } }
  ]
}
const testsTotal = CODE.tests.reduce((s, t) => s + t.value, 0)
const codeMax = Math.max(CODE.core, testsTotal)

const nf = computed(() => new Intl.NumberFormat(isEn.value ? 'en-US' : 'ru-RU'))
const fmt = (n) => nf.value.format(n)
const pct = (n) => (isEn.value ? n.toFixed(1) : n.toFixed(1).replace('.', ',')) + '%'

const icons = {
  unit: '<path d="M9 11l3 3L22 4"/><path d="M21 12v7a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h11"/>',
  db: '<ellipse cx="12" cy="5" rx="9" ry="3"/><path d="M3 5v14a9 3 0 0 0 18 0V5"/><path d="M3 12a9 3 0 0 0 18 0"/>',
  fuzz: '<path d="m8 2 1.88 1.88"/><path d="M14.12 3.88 16 2"/><path d="M9 7.13v-1a3.003 3.003 0 1 1 6 0v1"/><path d="M12 20c-3.3 0-6-2.7-6-6v-3a4 4 0 0 1 4-4h4a4 4 0 0 1 4 4v3c0 3.3-2.7 6-6 6"/><path d="M12 20v-9"/><path d="M6.53 9C4.6 8.8 3 7.1 3 5"/><path d="M6 13H2"/><path d="M3 21c0-2.1 1.7-3.9 3.8-4"/><path d="M20.97 5c0 2.1-1.6 3.8-3.5 4"/><path d="M22 13h-4"/><path d="M17.2 17c2.1.1 3.8 1.9 3.8 4"/>',
  live: '<path d="M22 12h-4l-3 9L9 3l-3 9H2"/>',
  rfc: '<path d="M14.5 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V7.5L14.5 2z"/><path d="M14 2v6h6"/><path d="m9 15 2 2 4-4"/>',
  shield: '<path d="M20 13c0 5-3.5 7.5-7.66 8.95a1 1 0 0 1-.67-.01C7.5 20.5 4 18 4 13V6a1 1 0 0 1 1-1c2 0 4.5-1.2 6.24-2.72a1.17 1.17 0 0 1 1.52 0C14.51 3.81 17 5 19 5a1 1 0 0 1 1 1z"/><path d="m9 12 2 2 4-4"/>'
}
const iconSvg = (id) =>
  `<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round" aria-hidden="true">${icons[id]}</svg>`

const layers = computed(() =>
  isEn.value
    ? [
        { icon: 'unit', value: fmt(2872), label: 'unit tests', text: '108,104 checks in 137 files, about 5 s under AddressSanitizer; separate runners keep HTTP/2 and HTTP/3 visible.' },
        { icon: 'db', value: '295', label: 'checks on a real database', text: 'Real SQL against PostgreSQL, each run in its own schema; suites for MySQL, Redis and SQLite too.' },
        { icon: 'fuzz', value: '47', label: 'fuzz targets', text: 'HTTP/1–3, HPACK, QPACK, QUIC, WebSocket, JSON, JWT and templates. The last audit ran ~183M inputs.' },
        { icon: 'live', value: '20', label: 'live-server suites', text: 'Hard and soft reload, QUIC migration, 0-RTT, IPv6, memory limits and soak runs against a real process.' },
        { icon: 'rfc', value: '147/147 · 49/49', label: 'h2spec · h3spec', text: 'Every HTTP/2 and HTTP/3 conformance case passes; h2load pushed 300K requests without an error.' },
        { icon: 'shield', value: 'ASan · TSan', label: 'plus UBSan and LSan', text: 'Not one memory error or data race reported under a 400K-request load.' }
      ]
    : [
        { icon: 'unit', value: fmt(2872), label: 'unit-тестов', text: '108 104 проверки в 137 файлах, прогон ≈ 5 с под AddressSanitizer; HTTP/2 и HTTP/3 ещё и в отдельных раннерах.' },
        { icon: 'db', value: '295', label: 'проверок на живой БД', text: 'Настоящий SQL против PostgreSQL, у каждого прогона своя схема; есть наборы для MySQL, Redis и SQLite.' },
        { icon: 'fuzz', value: '47', label: 'фазз-целей', text: 'HTTP/1–3, HPACK, QPACK, QUIC, WebSocket, JSON, JWT и шаблоны. В последнем аудите — ≈ 183 млн входов.' },
        { icon: 'live', value: '20', label: 'сценариев на живом сервере', text: 'Жёсткая и мягкая перезагрузка, миграция QUIC, 0-RTT, IPv6, лимиты памяти и длительные прогоны.' },
        { icon: 'rfc', value: '147/147 · 49/49', label: 'h2spec · h3spec', text: 'Все проверки соответствия HTTP/2 и HTTP/3 пройдены; h2load дал 300 тыс. запросов без единой ошибки.' },
        { icon: 'shield', value: 'ASan · TSan', label: 'а также UBSan и LSan', text: 'Ни одной ошибки памяти и ни одной гонки под нагрузкой в 400 тыс. запросов.' }
      ]
)

const series = [
  { cls: 's1', label: { ru: 'Тесты (unit + БД)', en: 'Tests (unit + DB)' } },
  { cls: 's2', label: { ru: 'Фаззинг', en: 'Fuzzing' } }
]
const ticks = [0, 25, 50, 75, 100]

// Hover or keyboard focus on a row opens its tooltip.
const active = ref(null)
const showTable = ref(false)
</script>

<template>
  <div class="home-extras">
    <section class="home-section">
      <div class="home-head">
        <span class="home-kicker">{{ isEn ? 'Testing' : 'Тесты' }}</span>
        <h2 class="home-title">
          {{ isEn ? 'Tested, fuzzed and checked against the RFCs' : 'Проверен тестами, фаззингом и сверен с RFC' }}
        </h2>
        <p class="home-desc">
          {{ isEn
            ? 'There is more test code than framework code. Each layer catches what the others miss: unit tests pin down behaviour, fuzzers throw garbage at every parser, and integration suites restart, reload and overload a live server.'
            : 'Кода тестов больше, чем кода ядра. Каждый слой ловит то, что пропускают другие: unit-тесты фиксируют поведение, фаззеры забрасывают мусором каждый парсер, а интеграционные сценарии перезапускают, перезагружают и нагружают живой сервер.' }}
        </p>
      </div>

      <div class="home-grid tests-layers">
        <div v-for="l in layers" :key="l.icon" class="home-card tests-layer">
          <span class="home-card-icon" v-html="iconSvg(l.icon)" />
          <span class="tests-layer-value">{{ l.value }}</span>
          <span class="tests-layer-label">{{ l.label }}</span>
          <p class="home-card-text">{{ l.text }}</p>
        </div>
      </div>

      <!-- Coverage per subsystem -->
      <div class="tests-panel">
        <div class="tests-panel-head">
          <div>
            <h3 class="tests-panel-title">
              {{ isEn ? 'Line coverage of the core by subsystem' : 'Покрытие строк ядра по подсистемам' }}
            </h3>
            <p class="tests-panel-sub">
              {{ isEn
                ? `${fmt(MEASURED.lines)} executable lines, gcov. Sorted by test coverage.`
                : `${fmt(MEASURED.lines)} исполняемых строк, gcov. По убыванию покрытия тестами.` }}
            </p>
          </div>
          <div class="tests-legend" role="list">
            <span v-for="s in series" :key="s.cls" class="tests-legend-item" role="listitem">
              <i class="tests-swatch" :class="s.cls" />{{ lc(s.label) }}
            </span>
          </div>
        </div>

        <div class="tests-chart" @mouseleave="active = null">
          <div class="tests-axis" aria-hidden="true">
            <span />
            <span class="tests-axis-scale">
              <span v-for="t in ticks" :key="t" class="tests-tick" :class="{ minor: t % 50 }" :style="{ left: t + '%' }">{{ t }}%</span>
            </span>
            <span />
          </div>
          <div
            v-for="(r, i) in SUBSYSTEMS"
            :key="r.id"
            class="tests-row"
            :class="{ 'is-active': active === i }"
            tabindex="0"
            :aria-label="`${lc(r.name)}: ${lc(series[0].label)} ${pct(r.v[0])}, ${lc(series[1].label)} ${pct(r.v[1])}`"
            @mouseenter="active = i"
            @focus="active = i"
            @blur="active = null"
          >
            <span class="tests-row-name">{{ lc(r.name) }}</span>
            <span class="tests-row-bars">
              <i v-for="t in ticks" :key="t" class="tests-grid" :style="{ left: t + '%' }" />
              <span
                v-for="(s, k) in series"
                :key="s.cls"
                class="tests-bar"
                :class="s.cls"
                :style="{ width: r.v[k] + '%' }"
              />
            </span>
            <span class="tests-row-value">
              <span v-for="(s, k) in series" :key="s.cls">{{ pct(r.v[k]) }}</span>
            </span>

            <div v-if="active === i" class="tests-tip" role="tooltip">
              <strong>{{ lc(r.name) }}</strong>
              <span class="tests-tip-hint">{{ lc(r.hint) }}</span>
              <span v-for="(s, k) in series" :key="s.cls" class="tests-tip-line">
                <i class="tests-swatch" :class="s.cls" />{{ lc(s.label) }}
                <b>{{ pct(r.v[k]) }}</b>
              </span>
              <span class="tests-tip-hint">
                {{ isEn ? `${fmt(r.lines)} lines` : `${fmt(r.lines)} строк` }}
              </span>
            </div>
          </div>
        </div>

        <button class="tests-table-toggle" :aria-expanded="showTable" @click="showTable = !showTable">
          {{ showTable ? (isEn ? 'Hide table' : 'Скрыть таблицу') : (isEn ? 'Show as a table' : 'Показать таблицей') }}
        </button>
        <div v-if="showTable" class="tests-table-wrap">
        <table class="tests-table">
          <thead>
            <tr>
              <th>{{ isEn ? 'Subsystem' : 'Подсистема' }}</th>
              <th>{{ isEn ? 'Lines' : 'Строк' }}</th>
              <th v-for="s in series" :key="s.cls">{{ lc(s.label) }}</th>
            </tr>
          </thead>
          <tbody>
            <tr v-for="r in SUBSYSTEMS" :key="r.id">
              <td>{{ lc(r.name) }}</td>
              <td>{{ fmt(r.lines) }}</td>
              <td v-for="(v, k) in r.v" :key="k">{{ pct(v) }}</td>
            </tr>
            <tr class="tests-table-total">
              <td>{{ isEn ? 'Whole core' : 'Всё ядро' }}</td>
              <td>{{ fmt(MEASURED.lines) }}</td>
              <td>{{ pct(MEASURED.tests) }}</td>
              <td>{{ pct(MEASURED.fuzz) }}</td>
            </tr>
          </tbody>
        </table>
        </div>
      </div>

      <div class="tests-duo">
        <!-- How the two layers overlap -->
        <div class="tests-panel">
          <h3 class="tests-panel-title">{{ isEn ? 'Who reaches which lines' : 'Кто какие строки достаёт' }}</h3>
          <p class="tests-panel-sub">
            {{ isEn
              ? 'Fuzzing alone reaches lines no test touches; together the layers cover more than either one.'
              : 'Фаззинг достаёт строки, до которых не дошёл ни один тест; вместе слои покрывают больше, чем каждый по отдельности.' }}
          </p>
          <div class="tests-hero">
            <span class="tests-hero-value">{{ pct(MEASURED.union) }}</span>
            <span class="tests-hero-label">
              {{ isEn
                ? `of the core together · tests ${pct(MEASURED.tests)} · fuzzing ${pct(MEASURED.fuzz)}`
                : `ядра вместе · тесты ${pct(MEASURED.tests)} · фаззинг ${pct(MEASURED.fuzz)}` }}
            </span>
          </div>
          <div class="tests-stack" role="img"
               :aria-label="SPLIT.map((s) => `${lc(s.label)} ${pct(s.value)}`).join(', ')">
            <span
              v-for="s in SPLIT"
              :key="s.cls"
              class="tests-seg"
              :class="s.cls"
              :style="{ flexGrow: s.value }"
              :title="`${lc(s.label)}: ${pct(s.value)}`"
            >
              <span v-if="s.value >= 12" class="tests-seg-label">{{ pct(s.value) }}</span>
            </span>
          </div>
          <ul class="tests-split-legend">
            <li v-for="s in SPLIT" :key="s.cls">
              <i class="tests-swatch" :class="s.cls" />
              <span>{{ lc(s.label) }}</span>
              <b>{{ pct(s.value) }}</b>
            </li>
          </ul>
        </div>

        <!-- Test code vs core code -->
        <div class="tests-panel">
          <h3 class="tests-panel-title">{{ isEn ? 'Test code vs core code' : 'Код тестов и код ядра' }}</h3>
          <p class="tests-panel-sub">
            {{ isEn
              ? 'Lines of C, shell and Python, counted the same way on both sides.'
              : 'Строки на Си, shell и Python, посчитанные одинаково для обеих сторон.' }}
          </p>
          <div class="tests-code">
            <div class="tests-code-row">
              <span class="tests-code-name">{{ isEn ? 'Core' : 'Ядро' }}</span>
              <span class="tests-code-track">
                <span class="tests-code-bar s0" :style="{ width: (100 * CODE.core) / codeMax + '%' }" />
              </span>
              <span class="tests-code-value">{{ fmt(CODE.core) }}</span>
            </div>
            <div class="tests-code-row">
              <span class="tests-code-name">{{ isEn ? 'Tests' : 'Тесты' }}</span>
              <span class="tests-code-track">
                <span class="tests-code-stack" :style="{ width: (100 * testsTotal) / codeMax + '%' }">
                  <span
                    v-for="t in CODE.tests"
                    :key="t.cls"
                    class="tests-seg"
                    :class="t.cls"
                    :style="{ flexGrow: t.value }"
                    :title="`${lc(t.label)}: ${fmt(t.value)}`"
                  />
                </span>
              </span>
              <span class="tests-code-value">{{ fmt(testsTotal) }}</span>
            </div>
          </div>
          <ul class="tests-split-legend">
            <li v-for="t in CODE.tests" :key="t.cls">
              <i class="tests-swatch" :class="t.cls" />
              <span>{{ lc(t.label) }}</span>
              <b>{{ fmt(t.value) }}</b>
            </li>
          </ul>
        </div>
      </div>

      <div class="home-note">
        <p>
          <strong>{{ isEn ? 'How it was measured' : 'Как измерено' }}</strong> —
          {{ isEn
            ? `${lc(MEASURED.date)}: a --coverage build of the core, the unit and DB runners, then every fuzz target for 60 s. One script, ci.sh, runs all 24 stages — from the ASan and TSan builds to h3spec.`
            : `${lc(MEASURED.date)}: сборка ядра с --coverage, unit- и БД-раннеры, затем каждая фазз-цель по 60 с. Все 24 стадии — от сборок под ASan и TSan до h3spec — запускает один скрипт ci.sh.` }}
        </p>
        <a class="home-link" :href="testsRepo" target="_blank" rel="noopener">
          {{ isEn ? 'The test suite on GitHub' : 'Тесты на GitHub' }} →
        </a>
      </div>
    </section>
  </div>
</template>

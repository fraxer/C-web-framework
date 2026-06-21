<script setup>
import { computed } from 'vue'
import { useData } from 'vitepress'

const { lang } = useData()
const isEn = computed(() => lang.value === 'en')
const pref = computed(() => (isEn.value ? '/en' : ''))
const gh = 'https://github.com/fraxer/C-web-framework'

const kicker = computed(() => (isEn.value ? 'Quick Start' : 'Быстрый старт'))
const title = computed(() =>
  isEn.value ? 'Run the server in a couple of commands' : 'Запустите сервер за пару команд'
)
const desc = computed(() =>
  isEn.value
    ? 'Clone the repository, build the project with the database drivers you need and launch the server with a single command.'
    : 'Склонируйте репозиторий, соберите проект с нужными драйверами баз данных и запустите сервер одной командой.'
)

const terminal = `git clone ${gh}.git
cd C-web-framework/backend

mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release \\
  -DINCLUDE_POSTGRESQL=yes \\
  -DINCLUDE_MYSQL=yes \\
  -DINCLUDE_REDIS=yes \\
  -DINCLUDE_SQLITE=yes
cmake --build . -j\$(nproc)

./exec/cpdy -c /path/to/config.json`

const route = `{
  "/": {
    "GET": {
      "file": "handlers/libindexpage.so",
      "function": "index"
    }
  }
}`

const handler = `#include "http.h"

void index(httpctx_t* ctx) {
  ctx->response->send_data(
    ctx->response,
    "Hello world!"
  );
}`

const routeLabel = computed(() => (isEn.value ? 'Route — config.json' : 'Маршрут — config.json'))
const handlerLabel = computed(() =>
  isEn.value ? 'Handler — handlers/indexpage.c' : 'Обработчик — handlers/indexpage.c'
)
const codeKicker = computed(() => (isEn.value ? 'A handler is just a function' : 'Обработчик — это просто функция'))
const codeDesc = computed(() =>
  isEn.value
    ? 'Each route loads a handler from a .so library. Read the request, build the response — nothing else to wire up.'
    : 'Каждый маршрут загружает обработчик из .so-библиотеки. Прочитайте запрос, сформируйте ответ — больше ничего настраивать не нужно.'
)
const guideLink = computed(() => `${pref.value}/build-and-run`)
const guideText = computed(() => (isEn.value ? 'Build & run guide' : 'Руководство по сборке'))

/* --- Minimal, dependency-free syntax highlighting ----------------------- */
// Each language is a list of [pattern, className] rules. Patterns must use
// only non-capturing groups ((?:…)) and lookarounds — they get wrapped in a
// capture group per rule so the classifier can map a match back to a class.
const esc = (s) => s.replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;')

function highlight(code, rules) {
  const re = new RegExp(rules.map((r) => `(${r[0]})`).join('|'), 'g')
  const classes = rules.map((r) => r[1])
  let out = ''
  let last = 0
  let m
  while ((m = re.exec(code)) !== null) {
    if (m.index > last) out += esc(code.slice(last, m.index))
    let cls = ''
    for (let i = 0; i < classes.length; i++) {
      if (m[i + 1] !== undefined) {
        cls = classes[i] || ''
        break
      }
    }
    out += cls ? `<span class="${cls}">${esc(m[0])}</span>` : esc(m[0])
    last = m.index + m[0].length
    if (m[0].length === 0) re.lastIndex++ // guard against zero-width loops
  }
  out += esc(code.slice(last))
  return out
}

const BASH = [
  ['#[^\\n]*', 'tok-c'],
  ['https?:\\/\\/[^\\s]+', 'tok-s'],
  ['"(?:[^"\\\\]|\\\\.)*"|\'[^\']*\'', 'tok-s'],
  ['\\$\\([^)]*\\)|\\$\\w+', 'tok-n'],
  ['(?<=\\s)-{1,2}[A-Za-z][\\w-]*', 'tok-p'],
  ['(?:\\.\\.?\\/|\\/)[^\\s]*', 'tok-path'],
  ['\\b(?:git|cd|mkdir|cmake|make)\\b', 'tok-t'],
  ['[A-Za-z0-9_.\\/-]+\\.(?:so|json|c|h)\\b', 'tok-path']
]

const JSON_RULES = [
  ['"(?:[^"\\\\]|\\\\.)*"(?=\\s*:)', 'tok-t'],
  ['"(?:[^"\\\\]|\\\\.)*"', 'tok-s'],
  ['-?\\d+(?:\\.\\d+)?', 'tok-n'],
  ['\\b(?:true|false|null)\\b', 'tok-k']
]

const C = [
  ['\\/\\/[^\\n]*|\\/\\*[\\s\\S]*?\\*\\/', 'tok-c'],
  ['#[A-Za-z]+', 'tok-p'],
  ['"(?:[^"\\\\]|\\\\.)*"', 'tok-s'],
  ['\\b(?:return|if|else|for|while|struct|sizeof)\\b', 'tok-k'],
  ['\\b(?:void|int|char|short|long|float|double|unsigned|signed|static|const|size_t|httpctx_t|wsctx_t)\\b', 'tok-t'],
  ['\\b[A-Za-z_]\\w*(?=\\s*\\()', 'tok-f'],
  ['\\b\\d+\\b', 'tok-n']
]

const terminalHtml = computed(() => highlight(terminal, BASH))
const routeHtml = computed(() => highlight(route, JSON_RULES))
const handlerHtml = computed(() => highlight(handler, C))
</script>

<template>
  <div class="home-extras">
    <section class="home-section">
      <div class="home-head">
        <span class="home-kicker">{{ kicker }}</span>
        <h2 class="home-title">{{ title }}</h2>
        <p class="home-desc">{{ desc }}</p>
      </div>

      <div class="qs-grid">
        <div class="code-card">
          <div class="code-head">
            <span class="dot red" /><span class="dot yellow" /><span class="dot green" />
            <span class="code-name">bash</span>
          </div>
          <pre class="code-body"><code v-html="terminalHtml" /></pre>
        </div>

        <div class="code-card">
          <div class="code-head">
            <span class="code-name">{{ routeLabel }}</span>
          </div>
          <pre class="code-body"><code v-html="routeHtml" /></pre>
          <div class="code-head code-head--sub">
            <span class="code-name">{{ handlerLabel }}</span>
          </div>
          <pre class="code-body"><code v-html="handlerHtml" /></pre>
        </div>
      </div>

      <div class="home-note">
        <p><strong>{{ codeKicker }}</strong> — {{ codeDesc }}</p>
        <a class="home-link" :href="guideLink">{{ guideText }} →</a>
      </div>
    </section>
  </div>
</template>

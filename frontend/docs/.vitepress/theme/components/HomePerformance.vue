<script setup>
import { computed, ref } from 'vue'
import { useData, withBase } from 'vitepress'
import data from '../performance.json'
const { lang } = useData()
const en = computed(() => lang.value === 'en')
const section = ref('json')
const connections = ref(64)
const range = ref('full')
const staticConnections = ref(64)
const dbConnections = ref(64)
const dbRows = computed(() => (data.postgres || []).filter(r => r.connections === dbConnections.value).slice().sort((a,b) => b.req_s-a.req_s))
const dbMax = computed(() => Math.max(...dbRows.value.map(r => r.req_s),1))
const fmt = n => new Intl.NumberFormat(en.value ? 'en-US' : 'ru-RU', { maximumFractionDigits: 0 }).format(n)
const decimals = n => new Intl.NumberFormat(en.value ? 'en-US' : 'ru-RU', { maximumFractionDigits: 2, minimumFractionDigits: 2 }).format(n)
const rows = computed(() => data.json.filter(r => r.connections === connections.value).slice().sort((a,b) => b.req_s-a.req_s))
const max = computed(() => Math.max(...rows.value.map(r => r.req_s),1))
const staticRows = computed(() => data.static.filter(r => r.connections === staticConnections.value && r.case === range.value))
const ranges = computed(() => [
 {value:'full',text:en.value?'Whole file':'Целый файл'},
 {value:'prefix_half',text:en.value?'Prefix ½':'Начало ½'},
 {value:'suffix_quarter',text:en.value?'Suffix ¼':'Конец ¼'},
 {value:'multipart_quarters',text:'Multipart ¼+¼'}
])
const measurementDate = computed(() => section.value === 'json' ? data.jsonSettings.latest_date : data.date)
const details = computed(() => withBase(en.value ? '/en/performance.html' : '/performance.html'))
</script>

<template>
  <div class="home-extras">
    <section class="home-section performance" aria-labelledby="performance-title">
      <div class="home-head">
        <span class="home-kicker">{{ en ? 'Performance' : 'Производительность' }}</span>
        <h2 id="performance-title" class="home-title">{{ en ? 'Measured on the same machine' : 'Замеры на одной машине' }}</h2>
        <p class="home-desc">{{ en ? 'Real container measurements: JSON processing, PostgreSQL queries and static file serving. Choose a workload to see its results and conditions.' : 'Результаты реальных замеров в контейнерах: обработка JSON, запросы PostgreSQL и отдача статических файлов. Выберите нагрузку, чтобы увидеть результаты и условия.' }}</p>
      </div>
      <div class="perf-controls" :aria-label="en ? 'Workload' : 'Нагрузка'">
        <button type="button" :aria-pressed="section === 'json'" @click="section = 'json'">JSON · 1 {{ en ? 'MiB' : 'МиБ' }}</button>
        <button v-if="data.postgres" type="button" :aria-pressed="section === 'postgres'" @click="section = 'postgres'">PostgreSQL · SELECT</button>
        <button type="button" :aria-pressed="section === 'static'" @click="section = 'static'">{{ en ? 'Static files · cwfr / nginx' : 'Статика · cwfr / nginx' }}</button>
      </div>
      <div v-if="section === 'json'" class="perf-panel">
        <h3>{{ en ? 'Parse JSON and count top-level keys' : 'Разобрать JSON и посчитать ключи первого уровня' }}</h3>
        <p>{{ en ? 'POST /count · 1,048,576 bytes · 4,097 keys · nested objects and mixed types. One application worker, eight execution threads, identical nginx gateway and resource limits.' : 'POST /count · 1 048 576 байт · 4097 ключей · вложенные объекты и разные типы. Один рабочий процесс приложения, восемь потоков выполнения, одинаковый nginx-прокси и лимиты ресурсов.' }}</p>
        <label class="perf-select">{{ en ? 'Connections' : 'Соединения' }}
          <select v-model.number="connections"><option :value="16">16</option><option :value="64">64</option></select>
        </label>
        <div class="perf-scroll">
          <table>
            <caption>{{ en ? `Medians of five 15-second runs.` : `Медианы пяти прогонов по 15 секунд.` }}</caption>
            <thead><tr><th>{{ en ? 'Framework' : 'Фреймворк' }}</th><th>req/s</th><th>p99, {{ en ? 'ms' : 'мс' }}</th><th>CPU/{{ en ? 'request, µs' : 'запрос, мкс' }}</th><th>{{ en ? 'RSS median, MiB' : 'RSS медиана, МиБ' }}</th></tr></thead>
            <tbody><tr v-for="row in rows" :key="row.framework" :class="{ 'perf-cwfr': row.framework === 'cwfr' }">
              <th scope="row">{{ row.name }}</th>
              <td><span class="perf-bar" :style="{ width: `${100 * row.req_s / max}%` }" aria-hidden="true" /><span class="perf-number">{{ fmt(row.req_s) }}</span></td>
              <td>{{ decimals(row.p99_us/1000) }}</td><td>{{ fmt(row.app_cpu_us_per_request) }}</td><td>{{ decimals(row.app_rss_median_bytes/1048576) }}</td>
            </tr></tbody>
          </table>
        </div>
        <p class="perf-note">{{ en ? 'p99 is the 99th percentile of request latency as seen by the load client: 99% of responses arrived faster. It covers the full round trip through the gateway, including queueing, not only application CPU time.' : 'p99 — 99-й перцентиль времени ответа с точки зрения клиента нагрузки: 99% ответов приходят быстрее. Это сквозное время через прокси, включая ожидание в очередях, а не только работа CPU приложения.' }}</p>
        <p class="perf-note">{{ en ? 'CPU covers application containers; gateway CPU is reported separately. Server: 8 logical CPUs on 4 physical cores, 2 GiB per container. Client: separate cores, 8 threads.' : 'CPU относится к контейнерам приложений; расход прокси приведён отдельно в отчёте. Сервер: 8 логических CPU на 4 физических ядрах, 2 ГиБ на контейнер. Клиент: отдельные ядра, 8 потоков.' }}</p>
        <p class="perf-note">{{ en ? 'RSS is the sum of resident memory of application processes, including supervisors, sampled every 200 ms during load. The table shows the median of five run medians.' : 'RSS — сумма резидентной памяти процессов приложения, включая управляющие процессы; снимается каждые 200 мс под нагрузкой. В таблице медиана медиан пяти прогонов.' }}</p>
      </div>
      <div v-else-if="section === 'postgres' && data.postgres" class="perf-panel">
        <h3>{{ en ? 'Select 20 rows from one million records' : 'Выбрать 20 строк из миллиона записей' }}</h3>
        <p>{{ en ? 'POST /db · two bound parameters · index on (category, id) · raw SQL · no ORM or response cache. One app worker with eight execution threads and up to eight reused DB connections. All selected fields are serialized to JSON.' : 'POST /db · два bind-параметра · индекс (category, id) · raw SQL · без ORM и кеша ответа. Один рабочий процесс приложения, восемь потоков выполнения и до восьми повторно используемых соединений БД. Все выбранные поля сериализуются в JSON.' }}</p>
        <label class="perf-select">{{ en ? 'Connections' : 'Соединения' }}<select v-model.number="dbConnections"><option :value="16">16</option><option :value="64">64</option></select></label>
        <div class="perf-scroll"><table>
          <caption>{{ en ? `Medians of ${data.postgresSettings.repeats} runs of ${data.postgresSettings.seconds} seconds. Memory includes the app worker and supervisors.` : `Медианы ${data.postgresSettings.repeats} прогонов по ${data.postgresSettings.seconds} секунд. Память включает worker и управляющие процессы.` }}</caption>
          <thead>
            <tr>
              <th>{{ en ? 'Framework' : 'Фреймворк' }}</th>
              <th>req/s</th><th>p99, {{ en ? 'ms' : 'мс' }}</th>
              <th>{{ en ? 'RSS median, MiB' : 'RSS медиана, МиБ' }}</th>
            </tr>
          </thead>
          <tbody><tr v-for="row in dbRows" :key="row.framework" :class="{ 'perf-cwfr': row.framework === 'cwfr' }">
            <th scope="row">{{ row.name }}</th>
            <td><span class="perf-bar" :style="{ width: `${100 * row.req_s / dbMax}%` }" aria-hidden="true" /><span class="perf-number">{{ fmt(row.req_s) }}</span></td>
            <td>{{ decimals(row.p99_us/1000) }}</td>
            <td>{{ decimals(row.rss_median_bytes/1048576) }}</td>
          </tr></tbody>
        </table></div>
        <p class="perf-note">{{ en ? 'Warm indexed reads, eight parameter pairs. Apps: CPU 0–7; PostgreSQL: separate P cores 8–15; client: E cores 16–19 with eight threads. App and DB containers: 2 GiB each. Cgroup RAM and process RSS sampled every 100 ms; RSS includes supervisors and may count shared pages repeatedly. Brief peaks may be missed. DB/gateway RAM and CPU are reported separately.' : 'Прогретое индексное чтение, восемь пар параметров. Приложения: CPU 0–7; PostgreSQL: отдельные P-ядра 8–15; клиент: E-ядра 16–19, восемь потоков. Контейнеры приложения и БД: по 2 ГиБ. Память cgroup и RSS процессов снимаются каждые 100 мс; RSS включает управляющие процессы и может повторно учитывать общие страницы. Короткие пики могут быть пропущены. RAM и CPU БД/прокси приведены отдельно в отчёте.' }}</p>
      </div>
      <div v-else class="perf-panel">
        <h3>{{ en ? 'Static files and byte ranges' : 'Статические файлы и диапазоны байтов' }}</h3>
        <p>{{ en ? 'HTTP/1.1 keep-alive · 4 workers · 4 physical CPU cores · 512 MiB per container · page cache · no gzip or access logs. Direct requests to cwfr and nginx.' : 'HTTP/1.1 keep-alive · 4 воркера · 4 физических ядра CPU · 512 МиБ на контейнер · page cache · без gzip и access log. Прямые запросы к cwfr и nginx.' }}</p>
        <div class="perf-controls">
          <label class="perf-select">{{ en ? 'Connections' : 'Соединения' }}<select v-model.number="staticConnections"><option v-for="c in [1,16,64,256]" :key="c" :value="c">{{ c }}</option></select></label>
          <label class="perf-select">{{ en ? 'Response' : 'Ответ' }}<select v-model="range"><option v-for="r in ranges" :key="r.value" :value="r.value">{{ r.text }}</option></select></label>
        </div>
        <div class="perf-scroll"><table>
          <caption>{{ en ? 'Medians of five 15-second runs.' : 'Медианы пяти прогонов по 15 секунд.' }}</caption>
          <thead><tr><th>{{ en ? 'File' : 'Файл' }}</th><th>cwfr, req/s</th><th>nginx, req/s</th><th>cwfr / nginx</th></tr></thead>
          <tbody><tr v-for="row in staticRows" :key="row.file"><th scope="row">{{ en ? row.size : row.size.replace("KiB", "КиБ").replace("MiB", "МиБ").replace(" B", " Б") }}</th><td>{{ fmt(row.cwfr.rps) }}</td><td>{{ fmt(row.nginx.rps) }}</td><td>{{ decimals(100*row.cwfr.rps/row.nginx.rps) }}%</td></tr></tbody>
        </table></div>
      </div>
      <p class="perf-links"><a :href="details">{{ en ? 'Methodology, versions and results →' : 'Методика, версии и результаты →' }}</a><span> · {{ measurementDate }}</span></p>
    </section>
  </div>
</template>

<style scoped>
.perf-controls { display:flex; flex-wrap:wrap; gap:12px; align-items:center; margin:20px 0; }
.perf-controls button, .perf-select select { border:1px solid var(--vp-c-divider); border-radius:8px; padding:8px 14px; background:var(--vp-c-bg-soft); color:var(--vp-c-text-1); font:inherit; cursor:pointer; }
.perf-controls button[aria-pressed="true"] { border-color:var(--vp-c-brand-1); color:var(--vp-c-brand-1); }
.perf-panel { padding:24px; border:1px solid var(--vp-c-divider); border-radius:16px; background:var(--vp-c-bg-soft); }
.perf-panel h3 { margin:0 0 10px; font-size:22px; font-weight:600; }
.perf-panel p { color:var(--vp-c-text-2); line-height:1.7; }
.perf-select { display:inline-flex; gap:10px; align-items:center; margin:12px 0; }
.perf-scroll { overflow-x:auto; }
table { width:100%; border-collapse:collapse; margin:16px 0; text-align:left; font-variant-numeric:tabular-nums; }
caption { text-align:left; color:var(--vp-c-text-2); font-size:13px; margin-bottom:10px; }
th, td { padding:12px; border-bottom:1px solid var(--vp-c-divider); white-space:nowrap; }
td { position:relative; min-width:110px; }
th { font-weight:600; }
.perf-cwfr { color:var(--vp-c-brand-1); }
.perf-bar { position:absolute; height:26px; left:0; top:50%; transform:translateY(-50%); background:var(--vp-c-brand-soft); border-radius:4px; }
.perf-number { position:relative; }
.perf-note { font-size:13px; margin-top:16px; }
.perf-links { margin-top:18px; color:var(--vp-c-text-2); }
.perf-links a { color:var(--vp-c-brand-1); text-decoration:underline; }
@media(max-width:640px) { .perf-panel { padding:16px; } .perf-panel h3 { font-size:19px; } th,td { padding:10px 8px; } }
</style>

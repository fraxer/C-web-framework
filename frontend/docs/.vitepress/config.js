import { defineConfig, createContentLoader } from 'vitepress'
import { SitemapStream } from 'sitemap'
import { createWriteStream } from 'node:fs'
import { resolve } from 'node:path'

// https://vitepress.dev/reference/site-config
export default defineConfig({
  title: "C Web Framework — HTTP и WebSocket сервер на C",
  description: "C Web Framework — легковесный веб-фреймворк на языке C для создания HTTP и WebSocket серверов. Поддержка баз данных, ORM, миграций, шаблонизатора и SSL.",
  head: [
    [
      'script',
      { id: 'mtrk' },
      `
        (function(m,e,t,r,i,k,a){m[i]=m[i]||function(){(m[i].a=m[i].a||[]).push(arguments)};
        m[i].l=1*new Date();
        for (var j = 0; j < document.scripts.length; j++) {if (document.scripts[j].src === r) { return; }}
        k=e.createElement(t),a=e.getElementsByTagName(t)[0],k.async=1,k.src=r,a.parentNode.insertBefore(k,a)})
        (window, document, "script", "https://mc.yandex.ru/metrika/tag.js", "ym");

        ym(94252971, "init", {
              clickmap:true,
              trackLinks:true,
              accurateTrackBounce:true,
              webvisor:true
        });
      `
    ],
    [
      'div',
      {},
      `<img src="https://mc.yandex.ru/watch/94252971" style="position:absolute; left:-9999px;" alt="" />`
    ],
    [
      'link', { rel: "icon", type: "image/x-icon", href: "/favicon.ico"}
    ]
  ],
  locales: {
    root: {
      label: 'Русский',
      lang: 'ru',
      titleTemplate: ':title | C Web Framework — веб-фреймворк на C',
      themeConfig: {
        logo: '/favicon.png',
        siteTitle: 'C Web Framework',
        outlineTitle: 'На этой странице',

        nav: [
          { text: 'Главная', link: '/' },
          { text: 'Документация', link: '/introduction' },
          { text: 'Примеры', link: '/examples-req-res' },
        ],

        darkModeSwitchLabel: 'Тема',

        sidebar: [
          {
            items: [
              { text: 'Введение', link: '/introduction' },
              { text: 'Установка зависимостей', link: '/install' },
              { text: 'Сборка и запуск', link: '/build-and-run' },
              { text: 'Файл конфигурации', link: '/config' },
              { text: 'HTTP', collapsed: true,
                items: [
                  { text: 'Запросы', link: '/requests' },
                  { text: 'Ответы', link: '/responses' },
                  { text: 'Cookie', link: '/cookie' },
                  { text: 'Получение данных от клиента', link: '/payload' },
                  { text: 'Query-параметры', link: '/query-params' },
                  { text: 'Коды состояния', link: '/http-codes' },
                ]
              },
              { text: 'WebSockets', collapsed: true,
                items: [
                  { text: 'Запросы', link: '/wsrequests' },
                  { text: 'Ответы', link: '/wsresponses' },
                  { text: 'Получение данных от клиента', link: '/wspayload' },
                  { text: 'Широковещание', link: '/wsbroadcast' },
                ]
              },
              { text: 'Базы данных', collapsed: true,
                items: [
                  { text: 'Доступ к данным', link: '/db' },
                  { text: 'ORM-модели', link: '/model' },
                  { text: 'Миграции', link: '/migrations' },
                  { text: 'Подготовленные запросы', link: '/prepared-statements' },
                ]
              },
              { text: 'Middleware', link: '/middleware' },
              { text: 'Аутентификация', link: '/auth' },
              { text: 'Сессии', link: '/session' },
              { text: 'Домены', link: '/domains' },
              { text: 'Маршрутизация', link: '/routing' },
              { text: 'Шаблонизатор', link: '/view' },
              { text: 'Файловое хранилище', link: '/storage' },
              { text: 'HTTP-клиент', link: '/http-client' },
              { text: 'Отправка Email', link: '/mail' },
              { text: 'Логирование', link: '/logging' },
              { text: 'SSL-сертификаты', link: '/ssl-certs' },
              { text: 'Горячая перезагрузка', link: '/hot-reload' },
            ]
          },
          {
            text: 'Библиотеки',
            items: [
              { text: 'Json', link: '/json' },
              { text: 'Base64', link: '/base64' },
            ]
          },
          {
            text: 'Примеры',
            items: [
              { text: 'Запросы и ответы', link: '/examples-req-res' },
              { text: 'Базы данных', link: '/examples-db' },
              { text: 'Json', link: '/examples-json' },
            ]
          },
        ],

        socialLinks: [
          { icon: 'github', link: 'https://github.com/fraxer/C-web-framework' },
          {
            icon: {
              svg: '<svg role="img" viewBox="0 0 600 510" xmlns="http://www.w3.org/2000/svg"><title>Email</title><path class="st0" d="M440.917,67.925H71.083C31.827,67.925,0,99.752,0,139.008v233.984c0,39.256,31.827,71.083,71.083,71.083 h369.834c39.255,0,71.083-31.827,71.083-71.083V139.008C512,99.752,480.172,67.925,440.917,67.925z M178.166,321.72l-99.54,84.92 c-7.021,5.992-17.576,5.159-23.567-1.869c-5.992-7.021-5.159-17.576,1.87-23.567l99.54-84.92c7.02-5.992,17.574-5.159,23.566,1.87 C186.027,305.174,185.194,315.729,178.166,321.72z M256,289.436c-13.314-0.033-26.22-4.457-36.31-13.183l0.008,0.008l-0.032-0.024 c0.008,0.008,0.017,0.008,0.024,0.016L66.962,143.694c-6.98-6.058-7.723-16.612-1.674-23.583c6.057-6.98,16.612-7.723,23.582-1.674 l152.771,132.592c3.265,2.906,8.645,5.004,14.359,4.971c5.706,0.017,10.995-2.024,14.44-5.028l0.074-0.065l152.615-132.469 c6.971-6.049,17.526-5.306,23.583,1.674c6.048,6.97,5.306,17.525-1.674,23.583l-152.77,132.599 C282.211,284.929,269.322,289.419,256,289.436z M456.948,404.771c-5.992,7.028-16.547,7.861-23.566,1.869l-99.54-84.92 c-7.028-5.992-7.861-16.546-1.869-23.566c5.991-7.029,16.546-7.861,23.566-1.87l99.54,84.92 C462.107,387.195,462.94,397.75,456.948,404.771z"/></svg>'
            },
            link: 'mailto:fraxer13@gmail.com'
          }
        ],

        footer: {
          message: 'Выпущено под лицензией MIT.',
          copyright: 'Copyright © 2023 Александр Корчагин'
        },

        docFooter: {
          prev: 'Назад',
          next: 'Дальше'
        }
      }
    },
    en: {
      label: 'English',
      lang: 'en',
      titleTemplate: ':title | C Web Framework',
      description: 'C Web Framework — lightweight web framework in C for building HTTP and WebSocket servers. Database support, ORM, migrations, templating, and SSL.',
      themeConfig: {
        logo: '/favicon.png',
        siteTitle: 'C Web Framework',

        nav: [
          { text: 'Home', link: '/en/' },
          { text: 'Docs', link: '/en/introduction' },
          { text: 'Examples', link: '/en/examples-req-res' },
        ],

        sidebar: [
          {
            items: [
              { text: 'Intro', link: '/en/introduction' },
              { text: 'Install', link: '/en/install' },
              { text: 'Build and run', link: '/en/build-and-run' },
              { text: 'Configuration file', link: '/en/config' },
              { text: 'HTTP', collapsed: true,
                items: [
                  { text: 'Requests', link: '/en/requests' },
                  { text: 'Responses', link: '/en/responses' },
                  { text: 'Cookie', link: '/en/cookie' },
                  { text: 'Receiving data from the client', link: '/en/payload' },
                  { text: 'Query parameters', link: '/en/query-params' },
                  { text: 'HTTP codes', link: '/en/http-codes' },
                ]
              },
              { text: 'WebSockets', collapsed: true,
                items: [
                  { text: 'Requests', link: '/en/wsrequests' },
                  { text: 'Responses', link: '/en/wsresponses' },
                  { text: 'Receiving data from the client', link: '/en/wspayload' },
                  { text: 'Broadcasting', link: '/en/wsbroadcast' },
                ]
              },
              { text: 'Databases', collapsed: true,
                items: [
                  { text: 'Data access', link: '/en/db' },
                  { text: 'ORM models', link: '/en/model' },
                  { text: 'Migrations', link: '/en/migrations' },
                  { text: 'Prepared statements', link: '/en/prepared-statements' },
                ]
              },
              { text: 'Middleware', link: '/en/middleware' },
              { text: 'Authentication', link: '/en/auth' },
              { text: 'Sessions', link: '/en/session' },
              { text: 'Domains', link: '/en/domains' },
              { text: 'Routes', link: '/en/routing' },
              { text: 'Templates', link: '/en/view' },
              { text: 'File storage', link: '/en/storage' },
              { text: 'HTTP client', link: '/en/http-client' },
              { text: 'Sending Email', link: '/en/mail' },
              { text: 'Logging', link: '/en/logging' },
              { text: 'SSL-certificates', link: '/en/ssl-certs' },
              { text: 'Hot reload', link: '/en/hot-reload' },
            ]
          },
          {
            text: 'Libraries',
            items: [
              { text: 'Json', link: '/en/json' },
              { text: 'Base64', link: '/en/base64' },
            ]
          },
          {
            text: 'Examples',
            items: [
              { text: 'Requests and responses', link: '/en/examples-req-res' },
              { text: 'Databases', link: '/en/examples-db' },
              { text: 'Json', link: '/en/examples-json' },
            ]
          },
        ],

        socialLinks: [
          { icon: 'github', link: 'https://github.com/fraxer/C-web-framework' },
          {
            icon: {
              svg: '<svg role="img" viewBox="0 0 600 510" xmlns="http://www.w3.org/2000/svg"><title>Email</title><path class="st0" d="M440.917,67.925H71.083C31.827,67.925,0,99.752,0,139.008v233.984c0,39.256,31.827,71.083,71.083,71.083 h369.834c39.255,0,71.083-31.827,71.083-71.083V139.008C512,99.752,480.172,67.925,440.917,67.925z M178.166,321.72l-99.54,84.92 c-7.021,5.992-17.576,5.159-23.567-1.869c-5.992-7.021-5.159-17.576,1.87-23.567l99.54-84.92c7.02-5.992,17.574-5.159,23.566,1.87 C186.027,305.174,185.194,315.729,178.166,321.72z M256,289.436c-13.314-0.033-26.22-4.457-36.31-13.183l0.008,0.008l-0.032-0.024 c0.008,0.008,0.017,0.008,0.024,0.016L66.962,143.694c-6.98-6.058-7.723-16.612-1.674-23.583c6.057-6.98,16.612-7.723,23.582-1.674 l152.771,132.592c3.265,2.906,8.645,5.004,14.359,4.971c5.706,0.017,10.995-2.024,14.44-5.028l0.074-0.065l152.615-132.469 c6.971-6.049,17.526-5.306,23.583,1.674c6.048,6.97,5.306,17.525-1.674,23.583l-152.77,132.599 C282.211,284.929,269.322,289.419,256,289.436z M456.948,404.771c-5.992,7.028-16.547,7.861-23.566,1.869l-99.54-84.92 c-7.028-5.992-7.861-16.546-1.869-23.566c5.991-7.029,16.546-7.861,23.566-1.87l99.54,84.92 C462.107,387.195,462.94,397.75,456.948,404.771z"/></svg>'
            },
            link: 'mailto:fraxer13@gmail.com'
          }
        ],

        footer: {
          message: 'Released under the MIT License.',
          copyright: 'Copyright © 2025 Alexander Korchagin'
        }
      }
    }
  },
  async buildEnd(siteConfig) {
    const sitemap = new SitemapStream({ hostname: 'https://cwebframework.tech/' })
    const pages = await createContentLoader(['*.md', 'en/*.md']).load()
    const writeStream = createWriteStream(resolve(siteConfig.outDir, 'sitemap.xml'))

    sitemap.pipe(writeStream)
    pages.forEach((page) => sitemap.write(
      page.url
        // Strip `index.html` from URL
        .replace(/index.html$/g, '')
        // Optional: if Markdown files are located in a subfolder
        .replace(/^\/docs/, '')
      ))
    sitemap.end()

    await new Promise((r) => writeStream.on('finish', r))
  }
})

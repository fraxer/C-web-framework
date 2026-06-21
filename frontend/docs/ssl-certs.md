---
outline: deep
description: Настройка HTTPS в C Web Framework. Подключение SSL/TLS сертификатов, Let's Encrypt, самоподписанные сертификаты, cipher suites.
---

# SSL-сертификаты

SSL/TLS-сертификаты обеспечивают шифрованное HTTPS-соединение между клиентом и сервером, защищая передаваемые данные от перехвата.

## Конфигурация

Для включения HTTPS добавьте секцию `tls` в конфигурацию сервера:

```json
{
    "servers": {
        "s1": {
            "domains": ["example.com", "www.example.com"],
            "ip": "0.0.0.0",
            "port": 443,
            "root": "/var/www/example.com/web",
            "tls": {
                "fullchain": "/etc/ssl/certs/fullchain.pem",
                "private": "/etc/ssl/private/privkey.pem",
                "ciphers": "TLS_AES_256_GCM_SHA384 TLS_CHACHA20_POLY1305_SHA256 TLS_AES_128_GCM_SHA256 ECDHE-ECDSA-AES256-GCM-SHA384:ECDHE-RSA-AES256-GCM-SHA384:ECDHE-ECDSA-CHACHA20-POLY1305:ECDHE-RSA-CHACHA20-POLY1305:ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES128-GCM-SHA256"
            },
            "http": {
                "routes": { ... }
            }
        }
    }
}
```

### Параметры

Все три поля обязательны — при отсутствии или пустом значении любого из них сервер не запустится.

| Параметр | Описание |
|----------|----------|
| `fullchain` | Путь к файлу сертификата в формате PEM (включая промежуточные сертификаты) |
| `private` | Путь к файлу приватного ключа в формате PEM |
| `ciphers` | Список поддерживаемых шифров, передаётся в OpenSSL как есть |

::: tip Протокол и шифры
Сервер использует `TLS_server_method()`: протоколы SSLv2 и SSLv3 отключены, ренеготиация запрещена. Строка `ciphers` задаётся одной строкой, где имена с префиксом `TLS_` (`TLS_AES_256_GCM_SHA384`, …) — это наборы TLS 1.3, а остальные (`ECDHE-RSA-AES256-GCM-SHA384`, …) — наборы TLS 1.2. Разделителем может быть пробел или двоеточие.
:::

## Получение сертификата

### Let's Encrypt (бесплатный)

[Let's Encrypt](https://letsencrypt.org/) предоставляет бесплатные SSL-сертификаты сроком на 90 дней с автоматическим продлением.

#### Установка Certbot

```bash
# Ubuntu/Debian
sudo apt update
sudo apt install certbot

# CentOS/RHEL
sudo yum install certbot
```

#### Получение сертификата

```bash
# Standalone режим (сервер должен быть остановлен)
sudo certbot certonly --standalone -d example.com -d www.example.com

# Webroot режим (сервер работает)
sudo certbot certonly --webroot -w /var/www/example.com/web -d example.com -d www.example.com
```

После успешного получения сертификаты будут расположены в:
- `/etc/letsencrypt/live/example.com/fullchain.pem` — сертификат
- `/etc/letsencrypt/live/example.com/privkey.pem` — приватный ключ

#### Конфигурация с Let's Encrypt

```json
{
    "tls": {
        "fullchain": "/etc/letsencrypt/live/example.com/fullchain.pem",
        "private": "/etc/letsencrypt/live/example.com/privkey.pem",
        "ciphers": "TLS_AES_256_GCM_SHA384 TLS_CHACHA20_POLY1305_SHA256 TLS_AES_128_GCM_SHA256 ECDHE-ECDSA-AES256-GCM-SHA384:ECDHE-RSA-AES256-GCM-SHA384"
    }
}
```

#### Автоматическое продление

```bash
# Проверка продления
sudo certbot renew --dry-run

# Добавление в cron (выполняется дважды в день)
echo "0 0,12 * * * root certbot renew --quiet" | sudo tee /etc/cron.d/certbot-renew
```

### Самоподписанный сертификат

Для разработки и тестирования можно использовать самоподписанный сертификат:

```bash
# Создание директории
sudo mkdir -p /etc/ssl/private

# Генерация приватного ключа и сертификата
sudo openssl req -x509 -nodes -days 365 -newkey rsa:2048 \
    -keyout /etc/ssl/private/selfsigned.key \
    -out /etc/ssl/certs/selfsigned.crt \
    -subj "/C=RU/ST=Moscow/L=Moscow/O=Development/CN=localhost"

# Установка прав доступа
sudo chmod 600 /etc/ssl/private/selfsigned.key
```

#### Конфигурация

```json
{
    "tls": {
        "fullchain": "/etc/ssl/certs/selfsigned.crt",
        "private": "/etc/ssl/private/selfsigned.key",
        "ciphers": "TLS_AES_256_GCM_SHA384 TLS_AES_128_GCM_SHA256"
    }
}
```

::: warning Внимание
Самоподписанные сертификаты вызывают предупреждение в браузерах. Используйте их только для разработки.
:::

### Коммерческий сертификат

При покупке сертификата у центра сертификации (CA) вы получите:
- Сертификат домена (`.crt`)
- Промежуточные сертификаты (intermediate/chain)
- Приватный ключ (`.key`)

#### Объединение сертификатов

```bash
# Создание fullchain из сертификата и промежуточных
cat domain.crt intermediate.crt > fullchain.pem
```

## Рекомендуемые шифры

### Современная конфигурация (TLS 1.3 + TLS 1.2)

```
TLS_AES_256_GCM_SHA384 TLS_CHACHA20_POLY1305_SHA256 TLS_AES_128_GCM_SHA256 ECDHE-ECDSA-AES256-GCM-SHA384:ECDHE-RSA-AES256-GCM-SHA384:ECDHE-ECDSA-CHACHA20-POLY1305:ECDHE-RSA-CHACHA20-POLY1305:ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES128-GCM-SHA256
```

### Максимальная совместимость

```
TLS_AES_256_GCM_SHA384 TLS_CHACHA20_POLY1305_SHA256 TLS_AES_128_GCM_SHA256 TLS_AES_128_CCM_8_SHA256 TLS_AES_128_CCM_SHA256 ECDHE-ECDSA-AES256-GCM-SHA384:ECDHE-RSA-AES256-GCM-SHA384:ECDHE-ECDSA-CHACHA20-POLY1305:ECDHE-RSA-CHACHA20-POLY1305:ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES256-SHA384:ECDHE-RSA-AES256-SHA384:ECDHE-ECDSA-AES128-SHA256:ECDHE-RSA-AES128-SHA256
```

## Несколько сертификатов на одном IP (SNI)

Фреймворк поддерживает SNI (Server Name Indication). Если несколько серверов слушают один и тот же `ip` и `port`, во время TLS-рукопожатия выбирается сертификат того сервера, чей шаблон домена совпадает с именем хоста, переданным клиентом в SNI. Это позволяет раздавать разные сертификаты для разных доменов на одном адресе.

```json
{
    "servers": {
        "site_a": {
            "domains": ["a.example.com"],
            "ip": "0.0.0.0",
            "port": 443,
            "tls": {
                "fullchain": "/etc/ssl/a/fullchain.pem",
                "private": "/etc/ssl/a/privkey.pem",
                "ciphers": "..."
            },
            "http": { "routes": { ... } }
        },
        "site_b": {
            "domains": ["b.example.com"],
            "ip": "0.0.0.0",
            "port": 443,
            "tls": {
                "fullchain": "/etc/ssl/b/fullchain.pem",
                "private": "/etc/ssl/b/privkey.pem",
                "ciphers": "..."
            },
            "http": { "routes": { ... } }
        }
    }
}
```

::: tip Сопоставление доменов
Сопоставление SNI выполняется по тем же регулярным выражениям PCRE, что и домены сервера. IDN-домены автоматически приводятся к Punycode, а IP-адреса в качестве SNI игнорируются согласно RFC 6066. Если совпадений нет — используется сертификат сервера по умолчанию (тот, что принял соединение).
:::

## HTTP и HTTPS на одном сервере

Для поддержки обоих протоколов создайте два сервера:

```json
{
    "servers": {
        "http": {
            "domains": ["example.com"],
            "ip": "0.0.0.0",
            "port": 80,
            "http": {
                "redirects": {
                    "/(.*)": "https://example.com/{1}"
                }
            }
        },
        "https": {
            "domains": ["example.com"],
            "ip": "0.0.0.0",
            "port": 443,
            "tls": {
                "fullchain": "/etc/ssl/certs/fullchain.pem",
                "private": "/etc/ssl/private/privkey.pem",
                "ciphers": "..."
            },
            "http": {
                "routes": { ... }
            }
        }
    }
}
```

## Проверка настройки

### Проверка сертификата

```bash
# Просмотр информации о сертификате
openssl x509 -in /etc/ssl/certs/fullchain.pem -text -noout

# Проверка срока действия
openssl x509 -in /etc/ssl/certs/fullchain.pem -enddate -noout

# Проверка соответствия ключа и сертификата
openssl x509 -noout -modulus -in fullchain.pem | openssl md5
openssl rsa -noout -modulus -in privkey.pem | openssl md5
# Хеши должны совпадать
```

### Проверка подключения

```bash
# Проверка SSL-соединения
openssl s_client -connect example.com:443 -servername example.com

# Проверка поддерживаемых протоколов
openssl s_client -connect example.com:443 -tls1_3
openssl s_client -connect example.com:443 -tls1_2
```

### Онлайн-проверка

- [SSL Labs](https://www.ssllabs.com/ssltest/) — детальный анализ SSL-конфигурации
- [SSL Checker](https://www.sslshopper.com/ssl-checker.html) — быстрая проверка сертификата

## Безопасность

### Права доступа к файлам

```bash
# Сертификат (может быть публичным)
sudo chmod 644 /etc/ssl/certs/fullchain.pem

# Приватный ключ (только root)
sudo chmod 600 /etc/ssl/private/privkey.pem
sudo chown root:root /etc/ssl/private/privkey.pem
```

### Заголовки безопасности

Добавьте заголовки безопасности через middleware:

```c
int middleware_security_headers(httpctx_t* ctx) {
    httpresponse_t* res = ctx->response;

    // Принудительное использование HTTPS
    res->add_header(res, "Strict-Transport-Security", "max-age=31536000; includeSubDomains");

    // Защита от clickjacking
    res->add_header(res, "X-Frame-Options", "SAMEORIGIN");

    // Защита от XSS
    res->add_header(res, "X-Content-Type-Options", "nosniff");

    return 1;
}
```

## Устранение неполадок

### Ошибка "certificate verify failed"

Убедитесь, что файл `fullchain.pem` содержит полную цепочку сертификатов (сертификат домена + промежуточные).

### Ошибка "key values mismatch"

Приватный ключ не соответствует сертификату. Проверьте соответствие командой выше.

### Ошибка "permission denied"

Проверьте права доступа к файлам сертификата и ключа. Процесс сервера должен иметь доступ на чтение.

### Сертификат не обновляется

После замены файлов сертификата выполните горячую перезагрузку — она перечитывает `config.json` и создаёт новые SSL-контуры, не перезапуская процесс:

```bash
kill -SIGUSR1 $(pidof cpdy)
```

Поведение перезагрузки задаётся параметром `main.reload` в `config.json`:
- `"soft"` — новые соединения используют обновлённую конфигурацию, текущие соединения продолжают работать;
- `"hard"` — дополнительно принудительно закрываются текущие сокеты, клиенты переподключаются.

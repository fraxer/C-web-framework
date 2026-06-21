<script setup>
// Floating capability chips around the emblem — same accent colours as the
// capability tiles below, so the hero reads as a preview of the page.
const chips = [
  { t: 'HTTP/1.1', c: '#3b82f6' },
  { t: 'WebSocket', c: '#8b5cf6' },
  { t: 'PostgreSQL', c: '#10b981' }
]
</script>

<template>
  <div class="hero-visual">
    <span class="hero-glow" aria-hidden="true" />
    <div class="hero-emblem">
      <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" aria-hidden="true">
        <path d="m16 18 6-6-6-6" />
        <path d="m8 6-6 6 6 6" />
      </svg>
    </div>
    <span v-for="(chip, i) in chips" :key="i" :class="['hero-chip', 'c' + (i + 1)]">
      <span class="hero-chip-dot" :style="{ background: chip.c }" />
      {{ chip.t }}
    </span>
  </div>
</template>

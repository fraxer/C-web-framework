<script setup>
import { computed } from 'vue'
import { useData } from 'vitepress'

const { lang } = useData()
const isEn = computed(() => lang.value === 'en')
const text = computed(() => (isEn.value ? 'Open source · MIT License' : 'Открытый исходный код · MIT'))
</script>

<template>
  <span class="hero-badge">{{ text }}</span>
</template>

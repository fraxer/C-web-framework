// https://vitepress.dev/guide/custom-theme
import { h } from 'vue'
import Theme from 'vitepress/theme'
import './style.css'
import HomeHeroBadge from './components/HomeHeroBadge.vue'
import HomeHeroVisual from './components/HomeHeroVisual.vue'
import HomeHeroExtras from './components/HomeHeroExtras.vue'
import HomeCapabilities from './components/HomeCapabilities.vue'
import HomeFooterExtras from './components/HomeFooterExtras.vue'

export default {
  extends: Theme,
  Layout: () => {
    return h(Theme.Layout, null, {
      // https://vitepress.dev/guide/extending-default-theme#layout-slots
      'home-hero-info-before': () => h(HomeHeroBadge),
      'home-hero-image': () => h(HomeHeroVisual),
      'home-features-before': () => h(HomeHeroExtras),
      'home-features-after': () => [h(HomeCapabilities), h(HomeFooterExtras)]
    })
  },
  enhanceApp({ app, router, siteData }) {
    // ...
  }
}

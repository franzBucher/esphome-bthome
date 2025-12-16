import { defineConfig } from 'astro/config';
import starlight from '@astrojs/starlight';

export default defineConfig({
  site: 'https://dz0ny.github.io',
  base: '/esphome-bthome',
  integrations: [
    starlight({
      title: 'ESPHome BTHome',
      description: 'BTHome v2 BLE Protocol Component for ESPHome',
      social: {
        github: 'https://github.com/dz0ny/esphome-bthome',
      },
      expressiveCode: {
        themes: ['github-dark', 'github-light'],
        defaultProps: {
          wrap: true,
        },
        styleOverrides: {
          borderRadius: '0.5rem',
          codePaddingBlock: '0.75rem',
          codePaddingInline: '1rem',
        },
      },
      sidebar: [
        {
          label: 'Getting Started',
          autogenerate: { directory: 'getting-started' },
        },
        {
          label: 'Configuration',
          autogenerate: { directory: 'configuration' },
        },
        {
          label: 'Platforms',
          autogenerate: { directory: 'platforms' },
        },
        {
          label: 'Devices',
          autogenerate: { directory: 'devices' },
        },
        {
          label: 'Reference',
          autogenerate: { directory: 'reference' },
        },
      ],
      editLink: {
        baseUrl: 'https://github.com/dz0ny/esphome-bthome/edit/main/docs/',
      },
      customCss: ['./src/styles/custom.css'],
      head: [
        {
          tag: 'script',
          attrs: {
            type: 'module',
          },
          content: `
            import mermaid from 'https://cdn.jsdelivr.net/npm/mermaid@11/dist/mermaid.esm.min.mjs';
            mermaid.initialize({ startOnLoad: true, theme: 'neutral' });
          `,
        },
      ],
    }),
  ],
});

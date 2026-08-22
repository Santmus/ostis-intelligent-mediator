# docs-site

Публикация `agents_vault/` (Obsidian-vault документации по агентам) как
сайта на [Quartz 4](https://quartz.jzhao.xyz/) — тот же вид (граф связей,
переходы по `[[wikilinks]]`, теги, обратные ссылки), что и в Obsidian, но
в браузере, без установки.

Контент не копируется сюда — Quartz собирает сайт прямо из
`../agents_vault` (флаг `-d`). Деплой на GitHub Pages автоматический,
через `.github/workflows/deploy-docs.yml` при пуше в `main`.

## Локальный просмотр

```bash
cd docs-site
npm install
npx quartz build -d ../agents_vault --serve
```

Vendored copy of [Quartz](https://github.com/jackyzha0/quartz) (MIT,
see `LICENSE.txt`), config in `quartz.config.ts` / `quartz.layout.ts`.

# docs-site

Publishes `agents_vault/` (the Obsidian vault of agent documentation) as a
site using [Quartz 4](https://quartz.jzhao.xyz/) — the same view (graph of
links, `[[wikilinks]]` navigation, tags, backlinks) as in Obsidian, but in
a browser and without installation.

The content is not copied here — Quartz builds the site directly from
`../agents_vault` (the `-d` flag). Deployment to GitHub Pages is automatic,
via `.github/workflows/deploy-docs.yml` on push to `main`.

## Local preview

```bash
cd docs-site
npm install
npx quartz build -d ../agents_vault --serve
```

Vendored copy of [Quartz](https://github.com/jackyzha0/quartz) (MIT,
see `LICENSE.txt`), configured in `quartz.config.ts` / `quartz.layout.ts`.

# SLAB demo (`demo/slab`)

SLAB is a tiny static-site generator demo built on top of:

- **STDX** (filesystem, strings, INI parsing, arena allocator, logging, etc.)
- **Minima** (embedded scripting language for templates)
- **`markdown.h`** (Markdown → HTML conversion for `.md` content)

At a high level, `slab` scans your content and theme folders, collects frontmatter metadata, builds a list of pages/categories, and renders each page by running Minima scripts embedded in HTML templates.

---

## How SLAB works

### 1) Input layout and config

SLAB expects a site root directory passed on the command line:

```bash
slab <site_root>
```

Inside `<site_root>`, it looks for `_site.ini`, then resolves:

- `content_dir`
- `template_dir`
- `output_dir`
- plus `name` and `url`

from the `[site]` section.

The sample site (`sample_site`) uses:

- `content/` for source content
- `themes/default/` for templates and static theme assets
- `docs/` as output

### 2) Metadata pass

SLAB recursively scans both `template_dir` and `content_dir`.

- Paths beginning with `_` are ignored during the recursive scan.
- Files ending in `.md`, `.htm`, or `.html` are treated as **processable pages**.
- Everything else is copied to the output directory as static assets (images/css/etc).

For each processable file:

- It tries to parse frontmatter delimited by `--- ... ---`.
- If frontmatter is missing, the file is copied as-is (not templated).
- If frontmatter exists, SLAB builds a `SlabPage` entry with metadata and output path.

After all pages are collected, SLAB derives a unique `all_categories` list from page metadata.

### 3) Render pass (template execution)

For each collected page:

1. Read source content.
2. If source is `.md`, convert body (after frontmatter) to HTML.
3. Load the selected layout from `template_dir/_layout/<template>.html`.
4. Expand template tags (`<% ... %>` and `<%= ... %>`) into Minima code.
5. Execute that Minima code with variables injected into the global scope.
6. Write generated HTML to `output_path`.

---

## Frontmatter format

Frontmatter is optional, but required if you want SLAB to treat a file as a templated page.

Expected format:

```text
---
title: Hello
template: post
slug: posts/hello
date: 2025-03-17
category: post
tags: demo c
author: sailor
---

Markdown or HTML body starts here...
```

Recognized keys:

- `title`
- `slug`
- `template`
- `date` (parsed as `year/month/day`)
- `author`
- `tags`
- `category`

Notes:

- `slug` determines output file path (`<output_dir>/<slug>.html`).
- If `date` is missing, file modification date is used.
- Unknown keys are currently ignored (with debug print output).

---

## Template language in SLAB

SLAB templates are HTML files with embedded Minima code blocks:

- `<% ... %>` → statement block
- `<%= ... %>` → expression block (auto-printed)

Examples from sample layouts:

```html
<%= $post_title %>
<% foreach p $all_pages { print (page title $p) } %>
<% template (format "${template_dir}\\_include\\header.html") %>
```

SLAB registers two custom Minima commands:

- `template PATH` → load and execute another template file
- `page PROPERTY PAGE_OBJ` → read properties from `SlabPage`

Everything else comes from Minima builtins.

---

## Variables available to templates

### Site-level

- `site_url`
- `site_name`
- `output_dir`
- `content_dir`
- `template_dir`

### Current page (`post_*`)

- `post_title`
- `post_date`
- `post_year`
- `post_month`
- `post_day`
- `post_slug`
- `post_tags`
- `post_author`
- `post_body`

### Collections

- `all_pages` (list of page objects)
- `all_categories` (list of category strings)

Use `page` command to inspect each item in `all_pages`, e.g.:

```minima
(page title $p)
(page url $p)
(page category $p)
```

---

## Using the sample project

From repository root:

1. Build `slab`.
2. Run it against the sample site:

```bash
./demo/slab/bin/slab ./demo/slab/sample_site
```

Expected result:

- Generated HTML in `demo/slab/sample_site/docs`
- Copied static assets from `content/assets` and `themes/default/assets`

You can then serve the `docs/` folder with any static HTTP server.

---

## Important behavior and caveats

- Files/folders starting with `_` are excluded from directory scanning.
- Pages without frontmatter are copied as static files.
- Layout file must exist at `_layout/<template>.html` under `template_dir`.
- Markdown conversion only happens for `.md` pages.
- Date parsing is forgiving; invalid month/day values are clamped to defaults with warnings.
- The `draft` property exists on `SlabPage`/`page` command but is not populated by frontmatter yet.

---

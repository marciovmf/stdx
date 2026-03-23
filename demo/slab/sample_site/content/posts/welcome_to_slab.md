---
title:    Welcome to Slab
template: post
tags:     programming tutorual
date:     2026-03-27
category: post
slug:     welcome_to_slab
---

Slab is a tiny static-site generator built on top of [STDX](stdx_my_standard_c_library) and powered by [Minima](minima_a_tiny_command_language_you_can_embedd_anywhere.html).

Since slab is mostly a thin wrapper on top of minima, templates are programs.
HTML files are treated as a mix of static markup and embedded Minima code:
```
  <%= $post_title %>
  <% foreach p $all_pages { print (page title $p) } %>

```
There are only two constructs:

- `<% ... %>` → execute code  
- `<%= ... %>` → evaluate and print result  

Everything else is just Minima. In Minima, everything is a command—templates are just another place where commands run.

---

## How Slab works

At a high level, Slab is a two-pass system: first it collects information, then it executes templates.

Slab scans two directories:

- content (your pages)
- templates (your layouts and includes)

It walks the directory tree, ignoring paths that start with `_`, and classifies files:

- `.md`, `.html`, `.htm` → processable pages  
- everything else → static assets (copied as-is)

For each page, it looks for a frontmatter block:

```
---
title: Hello
template: post
slug: posts_hello
date: 2025-03-17
category: post
---
```
    
If frontmatter exists, Slab builds a page object with metadata and output path. If not, the file is treated as a static asset.

Once all pages are collected, Slab injects global structures into the Minima runtime, such as the list of all pages, the list of all categories, and metadata for the current page.

Additionally, it registers only a couple of custom commands:

- `template PATH` → include and execute another template  
- `page PROPERTY PAGE_OBJ` → access page metadata  

Everything else—loops, conditionals, string formatting—comes from Minima.

---

### Render pass

For each page:

1. Read source content  
2. Convert Markdown to HTML (if needed)  
3. Load the selected layout  
4. Expand template tags into Minima code  
5. Execute that code  
6. Write the result to disk  

Because templates are compiled into Minima and executed by the runtime templates are either flexible and simple.

---

## The data model

Before executing templates, Slab injects a set of global variables into the Minima runtime. These variables are then used by the script to generate the final page output.

At the site level:

- `site_url`
- `site_name`
- `content_dir`
- `template_dir`
- `output_dir`

For the current page:

- `post_title`
- `post_date`
- `post_slug`
- `post_body`
- and other derived fields

Slab also provides a couple of collections:

- `all_pages`
- `all_categories`

Page objects are opaque values and are accessed through a command:

```
  print (page title $p)
  print (page url $p)
  print (page category $p)

```

This keeps the language simple while still allowing structured data access.
---

## What now ?

  Check the other pages in this sample site for more details and have fun :)

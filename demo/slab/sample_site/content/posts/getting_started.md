---
title:    Getting Started with Slab and Minima
template: post
tags:     programming tutorial
date:     2026-03-23
category: post
slug:     getting_started_with_slab_and_minima
---

Welcome! This short guide will teach you enough **Minima** to understand and modify this site.

By the end, you’ll be able to:
- Include templates
- Render page content
- Iterate over pages
- Filter and sort content
- Access page metadata

---

## 1. Templates

Slab uses Minima commands embedded in HTML using:

```
<% ... %>
```

To include another template:

```
template (format "${template_dir}\\_include\\header.html")
```

### What happens here?

- `format` builds a string
- `${template_dir}` is a variable
- `template` loads and executes the file

---

## 2. Outputting Values

To print values into HTML:

```
<%= expression %>
```

Example:

```
<%= $post_body %>
```

- `$post_body` is the rendered content of the current page

---

## 3. Variables

Use `set` to assign values:

```
set sorted_pages (page_sort newer $all_pages)
```

- `$all_pages` is a global list of all pages
- `page_sort` returns a sorted list

---

## 4. Looping

To iterate over a list:

```
foreach p $sorted_pages {
  ...
}
```

- `p` is each page in the list

---

## 5. Accessing Page Properties

Use the `page` command:

```
page PROPERTY PAGE
```

Examples:

```
page title $p
page url $p
page category $p
page year $p
page month $p
page day $p
```

---

## 6. Sorting Pages

Use:

```
page_sort MODE LIST
```

Example:

```
set sorted_pages (page_sort newer $all_pages)
```

Available modes:

- `newer` → newest first
- `older` → oldest first
- `name` → title ascending
- `name_desc` → title descending
- `category` → category ascending
- `category_desc` → category descending

---

## 7. Filtering

Example: only posts

```
set is_post (str equals_i (page category $p) "post")

if (expr not $is_post) {
  continue;
}
```

### What’s happening:

- `str equals_i` → case-insensitive string compare
- `expr` → evaluates expressions
- `continue` → skips current loop iteration

---

## 8. Expressions

Expressions are wrapped in:

```
expr ...
```

Example:

```
expr not $is_post
```

---

## 9. Formatting Strings

Use:

```
format "..."
```

Example:

```
set date (format "$(page year $p)-$(page month $p)-$(page day $p)")
```

- `$(...)` evaluates expressions inside strings

---

## 10. Putting It Together

This pattern:

```
foreach p $sorted_pages {
  set is_post (str equals_i (page category $p) "post")

  if (expr not $is_post) {
    continue;
  }

  set date (format "$(page year $p)-$(page month $p)-$(page day $p)")
}
```

Means:

1. Loop all pages
2. Keep only posts
3. Build a date string

---

## 11. Mental Model

Minima is:

- Data-first (lists, values)
- Command-based (`page`, `template`, `format`)
- Explicit (no hidden magic)
- Embedded inside HTML

---

## 12. Next Steps

Try:

- Changing sort order (`older`)
- Filtering by another category
- Adding new page fields
- Creating new templates

---

That’s it. You now understand everything used in this site.

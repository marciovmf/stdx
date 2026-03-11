# doxter

**doxter** is a small documentation generator for C projects originally written as a **demo application for the STDX library**.  
It scans `.c` and `.h` files, extracts documentation comments, and generates **static HTML documentation**.

Its purpose was to exercise and test the library in a real program while producing something genuinely useful.

---

# Command Line Usage

```
doxter [options] input_files
```

## Options

```
-f <path-to-doxter.ini>
```

Path to the configuration file.

```
-d <path-to-output-dir>
```

Directory where the generated documentation files will be written.

```
-p <project-name>
```

Override the project name defined in `doxter.ini`.

```
--skip-static
```

Skip documentation for `static` functions.

```
--skip-undocumented
```

Skip symbols that do not have documentation comments.

```
--skip-empty-defines
```

Skip `#define` macros that have no assigned value.

---

# Example

Generate documentation for all headers in `include/` and source files in `src/`:

```
doxter -f doxter.ini -d docs include/*.h src/*.c
```

This will:

1. Read configuration from `doxter.ini`
2. Scan all specified source files
3. Extract documentation comments
4. Generate static HTML files inside `docs/`

---

# Documentation Comments

`doxter` extracts documentation from comment blocks using the following format:

```
/**
 * Documentation text
 */
```

These comment blocks are associated with the symbol that immediately follows them.

The contents of the comment block become the documentation text in the generated HTML.

---

# Configuration File

`doxter` uses a single configuration file written in **INI format**.

Example configuration (from the STDX project):

```
[project]
name = stdx
url = http://github.com/marciovmf/stdx
brief = "STDX is a minimalist, modular C99 utility library inspired by the STB style.
It provides a set of dependency-free, single-header components that extend common programming functionality in C"

skip_static_functions = 1
skip_undocumented = 0
skip_empty_defines = 1
markdown_gobal_comments = 1
markdown_index_page = 1

[theme]
color_page_background = ffffff
color_sidebar_background = f8f9fb
color_main_text = 1b1b1b
color_secondary_text = 5f6368
color_highlight = 0067b8
color_light_borders = e1e4e8
color_code_blocks = f6f8fa
color_code_block_border = d0d7de
border_radius = 10

color_tok_pp  = 6f42c1
color_tok_kw  = 005cc5
color_tok_id  = 24292f
color_tok_num = b31d28
color_tok_str = 032f62
color_tok_crt = 032f62
color_tok_pun = 6a737d
color_tok_doc = 22863a

font_code    = "Cascadia Mono"
font_ui      = "Helvetica", "Verdana", "Calibri"
font_heading = "Helvetica"
font_symbol  = "Segoe UI"
```

---

# `[project]` Section

Project metadata and documentation behavior.

### name

Name of the project shown in the generated documentation.

### url

Project URL.

### brief

Short project description displayed in the documentation.

### skip_static_functions

Controls whether `static` functions are documented.

```
0 = include static functions
1 = skip static functions
```

### skip_undocumented

Controls whether undocumented symbols appear in the output.

```
0 = include undocumented symbols
1 = skip undocumented symbols
```

### skip_empty_defines

Controls whether empty `#define` macros appear in the output.

```
0 = include empty defines
1 = skip empty defines
```

### markdown_gobal_comments

If enabled, global file comments are interpreted as Markdown.

### markdown_index_page

If enabled, the generated index page is interpreted as Markdown.

---

# `[theme]` Section

The theme section controls the appearance of the generated HTML documentation.

Most color fields use **hex values without the `#` prefix**.

## Page Colors

```
color_page_background
color_sidebar_background
color_main_text
color_secondary_text
color_highlight
color_light_borders
color_code_blocks
color_code_block_border
```

### border_radius

Controls the rounding of UI elements in the generated documentation.

---

# Syntax Highlighting

The following settings control token colors in code blocks:

```
color_tok_pp
color_tok_kw
color_tok_id
color_tok_num
color_tok_str
color_tok_crt
color_tok_pun
color_tok_doc
```

---

# Fonts

Fonts used in the generated documentation.

```
font_code
font_ui
font_heading
font_symbol
```

Each field accepts a comma-separated list of fonts that will be used by the browser.

---

# Typical Workflow

1. Create a `doxter.ini` configuration file.
2. Add documentation comments to your `.c` and `.h` files.
3. Run:

```
doxter -f doxter.ini -d docs src/*.c include/*.h
```

4. Open the generated HTML documentation in a browser.

---

# License

Same license as the STDX project.

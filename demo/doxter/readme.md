# doxter

**doxter** is a small documentation generator for C projects originally written as a **demo application for the STDX library**.  
It scans `.c` and `.h` files, extracts documentation comments, and generates **static HTML documentation**.

Its purpose was to exercise and test the library in a real program while producing something genuinely useful.

---

# Command Line Usage

```
Usage:

  doxter [options] input_files

Options

  -f <path-to-doxter.init>         = Path to doxter.ini.
  -d <path-to-output-dir>          = Specify output directory for generated files.
  -p <project-name>                = Project name.
  --skip-static                    = Skip static functions.
  --skip-undocumented              = Skip undocumented symbols.
  --skip-empty-defines             = Skip empty defines.
```

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

# Typical Workflow

1. Create a `doxter.ini` configuration file.
2. Add documentation comments to your `.c` and `.h` files.
3. Run:

```
doxter -f doxter.ini -d docs file1.h file2.c file3.h 
```

4. Open the generated HTML documentation in a browser.

---


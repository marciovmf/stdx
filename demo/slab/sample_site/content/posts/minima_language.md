---
title:    The Minima language
template: post
tags:     programming c project
date:     2026-03-22
category: post
slug:     the_minima_language
---


Minima is a small, command-driven DSL built to be embedded directly into applications. The same core runs seamlessly in contexts like an interactive console or a static site generator because everything is expressed as explicit commands defined by the host, making it predictable, lightweight, and easy to adapt to different domains.


## 1. What a Minima program looks like

Minima is a very simple language. A program is just a list of commands, one after another.

Each line is usually one command:

```
print "hello"
set x 10
```

You can also put multiple commands on the same line using `;`:

```
print "hello"; print "Sailor!\n"
```

If you understand this, you already understand the core idea:

> A Minima program is just a sequence of commands being executed in order.

---

## 2. Comments

Anything after `#` is ignored until the end of the line:

```
# this is a comment

```

---


## 3. When does a command continue onto the next line?

Most of the time, a command ends when the line ends.

There are only a few exceptions.

A newline may continue the same command only in these cases:

- immediately after `(`
- immediately after `{`
- immediately before `)`
- immediately before `}`

So `()` and `{}` do **not** allow line breaks freely anywhere inside them. They only allow line breaks at the edge of the construct: right after it opens, or right before it closes.

Examples:

```minima
print (
  format "hello"
)

if cond {
  print "a"
  print "b"
}
```

In contrast, strings are different. Inside a string literal delimited by `"..."`, line breaks may appear freely anywhere, and they become part of the string content.

Example:

```minima
print "Hello
Sailor"
```

That is valid, and the newline belongs to the string itself.

Outside of those specific cases, a newline ends the command. So this is invalid:

```minima
print
"hello"
```

because it is parsed as two separate commands.

A good mental model is:

> A newline ends the command unless it appears right after an opening `(` or `{`, right before a closing `)` or `}`, or inside a quoted string.

---

### Invalid example

```
print
"hello"
```

This becomes two separate commands:

```
print
"hello"
```

The first line is incomplete → error.

---

### Simple mental model

> A new line ends the command, unless something is still “open”.

---

## 3. The basic building blocks

Everything in Minima is made from a few simple pieces.

---

### 3.1 RAW tokens (plain words)

These are just words without quotes:

```
print
abc
123
+
```

They are used for things like:

- command names (`print`, `set`)
- variable names (`x`)
- simple values

---

### 3.2 Strings

Strings are always inside quotes:

```
"hello"
"line\nbreak"
```

They can contain:

- line breaks
- escape sequences like `\\n`, `\\t`, `\\"`

---

### 3.3 Variables and the value (`$`)

You read a variable with the value operator `$`:

```
set msg "Hello, Sailor!"
print $msg                # $msg is replaced by the value of msg variable
```

Think of `$msg` as “give me the value of msg”.

---

### 3.4 Subcommands (`()`)

Parentheses run **ONE** command and give you a result:

```
(expr 2 + 3)
```

You can use that result anywhere:

```
set x (expr 2 + 3)
print (expr 2 * $x)
```

Think of `()` as:

> “run this and use the result here”

---

### 3.5 Blocks (`{}`)

Blocks group multiple commands together:

```
{
  print "hello"
  print "world"
}
```

They do not run by themselves. They are given to commands that might run the
block at some point. Notable exemple of commands that takes in blocks as
arguments are `if` and `while`.

Blocks also create a new **scope** (a local space for variables).

```
set msg "Hello, Sailor!"

if (str length $msg) {
  print $msg
}
```

Inside the block, you can still read variables from outside.

---

## 4. What is a command?

Every command looks like this:

```
name arg1 arg2 arg3 ...
```

For example:

```
print "hello"
set x 10
```

The first word is the command name. Everything after that are arguments.

Important idea:

> Commands decide how their arguments are used and evaluated.

---

## 5. How values are combined

Minima does not have operators like `+` or `*` built into the language.

Instead, you combine things using:

- `$` to read values
- `()` to compute values

Example:

```
set x (expr 2 * (expr 3 + 1))
```

Everything is explicit. Nothing happens “automatically”.

---

## 6. Strings with substitution (`format`)

The `format` command lets you insert values inside strings.

There are two special forms:

```
"${x}"        # variable
"$(expr ...)" # execute code
```

Example:

```
format "x = ${x}\n"
format "result = $(expr 2 * $x)"
```

---


## 7. Full example

```
set a "hello"
set b "Hello"

if (str equals_i $a $b) {
  print "match\n"
} else {
  print "does not match\n"
}
```

---

# Built-in Commands

## set

```
set name value
```

Stores a value in a variable.

```
set x 10
```

---

## print

```
print arg1 arg2 ...
```

Prints values to the output.

```
print "Hello" $x
```

---

## format

```
format "template"
```

Creates a string with substitutions.

---

## if

```
if cond { block }
elseif cond { block }
else { block }
```

Runs the first block whose condition is true.

---

## while

```
while cond { block }
```

Repeats while the condition is true.

---

## break

```
break
```

Stops a loop.

---

## continue

```
continue
```

Skips to the next loop iteration.

---

## expr

```
expr <expression>
```

Performs math and logic:

```
expr 2 + 3 * 4
```

---

## list

```
list <subcommand> ...
```

Works with lists:

```
set xs (list create a b c)
list push $xs d
list get $xs 0
```

---

## foreach

```
foreach var list { block }
```

Loops over a list:

```
foreach item $xs {
  print $item
}
```

---

## include

```
include "file"
```

Runs another file.

---

## eval

```
eval "code"
```

Runs code stored in a string.

---

## str

```
str <subcommand> ...
```

String operations:

```
str length "hello"
str equals "a" "b"
```

---

## Final idea

Minima is intentionally very simple.
It does not try to do much by itself.
Instead, it gives you a small set of rules: variables, blocks, commands and subcommands.
And everything else is built on top of that.

If you understand those pieces, you understand the language.
For mor details, please refer to the project's [README](https://github.com/marciovmf/stdx/blob/main/demo/minima/readme.md) file.

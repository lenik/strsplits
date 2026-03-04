# strsplit

Split input string(s) by delimiter and print each item as a separate line.

## Synopsis

```bash
strsplit [OPTIONS] ARGS...
```

If `ARGS` are provided, each argument is treated as one input line (one run).

If no argument is specified, input is read from `stdin`, line by line (each line is one run).

## Description

For each run:

1. Take input string
2. Optionally case-fold
3. Split by delimiter (default: whitespace `\s+`)
4. Optionally sort
5. Print items according to format rules

A **run** is defined as:
- One command-line argument, OR
- One line read from stdin

## Installation

### Building from Source

```bash
# Install build dependencies
sudo apt install meson ninja-build gcc

# Configure and build
meson setup builddir
cd builddir
ninja

# Install (optional)
sudo ninja install
```

### Debian Package

```bash
# Build Debian package
dpkg-buildpackage -us -uc

# Install the package
sudo dpkg -i ../strsplit_*.deb
```

## Options

### Delimiter

```
-d, --delim REGEX
```

Use POSIX Extended Regular Expression as delimiter.  
**Default:** `\s+`

Delimiter matches are **not included** in output.

### Line Numbering

```
-n, --linenum
```

Print line number before each split item.

**Default numbering base:** 1

```
-0, --0-based
```

Use 0-based numbering.

```
-z, --reset
```

Reset numbering between runs.

**Default behavior:**
- If multiple runs and `-z` not specified → numbering continues across runs
- If `-z` specified → numbering restarts per run

### Output Formatting

```
-c, --format FMT
```

Custom output format.

If `-n` is enabled:
- **Default format:** `"%d. %s"`

If `-n` is not enabled:
- **Default format:** `"%s"`

**Format placeholders:**

| Placeholder | Meaning     |
| ----------- | ----------- |
| `%d`        | item index  |
| `%s`        | item string |

Undefined placeholders → implementation-defined (may pass to printf).

### Run Separation

```
-N, --insert-nl
```

Insert one blank line between runs.

May be specified repeatedly:
- `-N` → 1 blank line
- `-NN` → 2 blank lines
- etc.

No trailing blank lines after last run.

### Indentation

```
-i, --indented
```

Indent output by 2 spaces per level.

Repeatable:
- `-i` → 2 spaces
- `-ii` → 4 spaces

Indentation applies to each output line.

### Sorting

Sorting applies **after splitting** and before printing.

```
-r, --reversed
```

Reverse final ordering.

#### Sort Modes (mutually exclusive)

```
-f, --ignore-case
    Case-insensitive lexicographic sort

-s, --numeric-sort
    Compare using strtol()

-g, --general-numeric-sort
    Compare using strtod()

-H, --human-numeric-sort
    Human numeric (e.g. 2K < 10K)

-D, --dictionary-sort
    Compare ignoring non-alphanumeric chars

-V, --version-sort
    Version-aware sort (e.g. 1.9 < 1.10)
```

If multiple sort modes specified → last one wins.

If no sort option specified → preserve original order.

## Processing Order

For each run:

1. Read input
2. If `-f` specified → fold case (implementation: `toupper()`)
3. Split using delimiter regex
4. Preserve empty items (if delimiter produces empty tokens)
5. Apply selected sort (if any)
6. Apply reverse (if `-r`)
7. Output with formatting
8. Insert run-separator newlines (if `-N` and not last run)

## Exit Status

| Code | Meaning           |
| ---- | ----------------- |
| 0    | success           |
| 1    | invalid arguments |
| 2    | runtime error     |

## Examples

### Basic split

```bash
strsplit "foo bar baz"
```

**Output:**
```
foo
bar
baz
```

### With numbering (0-based) and reset

```bash
strsplit -z0niN 'foo bar' 'cat dog'
```

**Output:**
```
  0. foo
  1. bar

  0. cat
  1. dog
```

### Numeric sort

```bash
strsplit -sn "10 2 30 4"
```

**Output:**
```
1. 2
2. 4
3. 10
4. 30
```

### Version sort

```bash
strsplit -V "1.2 1.10 1.3"
```

**Output:**
```
1.2
1.3
1.10
```

### Custom delimiter

```bash
strsplit -d "," "foo,bar,baz"
```

**Output:**
```
foo
bar
baz
```

### Read from stdin

```bash
echo "foo bar baz" | strsplit
```

**Output:**
```
foo
bar
baz
```

### Edge Cases

- Empty input line → no output
- Delimiter matching entire string → no output
- Large input → dynamic realloc growth
- UTF-8 handling → byte-wise unless extended

## Bash Completion

Source the completion script or install it to your bash completion directory:

```bash
source bash-completion/strsplit
```

Or copy to system completion directory:

```bash
sudo cp bash-completion/strsplit /etc/bash_completion.d/
```

## License

Copyright (C) 2026 Lenik <strsplit@bodz.net>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.

## Author

**Lenik** <strsplit@bodz.net>

## Bug Reports

Please report bugs and feature requests at:
https://github.com/lenik/strsplit/issues

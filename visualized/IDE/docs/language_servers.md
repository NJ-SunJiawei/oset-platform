# Setup of tested language servers

- [JavaScript/TypeScript](#javascripttypescript)
- [Python3](#python3)
- [Rust](#rust)
- [Go](#go)
- [Julia](#julia)
- [GLSL](#glsl)

## JavaScript/TypeScript

### JavaScript with Flow static type checker

- Prerequisites:
  - Node.js
- Recommended:
  - [Prettier](https://github.com/prettier/prettier) (installed globally: `install i -g prettier`)

Install language server, and create executable to enable server in juCi++:

```sh
npm install -g flow-bin

# Usually as root:
echo '#!/bin/sh
flow lsp' > /usr/local/bin/javascript-language-server
chmod 755 /usr/local/bin/javascript-language-server
```

- Additional setup within a JavaScript project:
  - Add a `.prettierrc` file to enable style format on save

### TypeScript or JavaScript without Flow

- Prerequisites:
  - Node.js
- Recommended:
  - [Prettier](https://github.com/prettier/prettier) (installed globally: `install i -g prettier`)

Install language server, and create executable to enable server in juCi++:

```sh
npm install -g typescript-language-server typescript

# Usually as root:
echo '#!/bin/sh
`npm root -g`/typescript-language-server/lib/cli.js --stdio' > /usr/local/bin/javascript-language-server
chmod 755 /usr/local/bin/javascript-language-server
rm -f /usr/local/bin/typescript-language-server
cp /usr/local/bin/javascript-language-server /usr/local/bin/typescript-language-server
cp /usr/local/bin/javascript-language-server /usr/local/bin/typescriptreact-language-server
```

- Additional setup within a JavaScript project:
  - Add a `.prettierrc` file to enable style format on save

## Python3

- Prerequisites:
  - Python3
  - In juCi++ preferences, set `project.python_command` to `python3 -u`

### Python LSP Server

Install language server, and create symbolic link to enable server in juCi++:

```sh
pip3 install python-lsp-server[pycodestyle,yapf]

# Usually as root:
ln -s `which pylsp` /usr/local/bin/python-language-server
```

- Additional setup within a Python project:
  - Add a setup file, for instance:
    `printf '[pycodestyle]\nmax-line-length = 120\n\n[yapf]\nCOLUMN_LIMIT = 120\n' > setup.cfg`
  - Add an empty `.python-format` file to enable style format on save

### Pyright

- Prerequisite:
  - Node.js
- Recommended:
  - [YAPF](https://github.com/google/yapf) for style format (`pip3 install yapf`)

Install language server, and create executable to enable server in juCi++:

```sh
npm install -g pyright

# Usually as root:
echo '#!/bin/sh
pyright-langserver --stdio' > /usr/local/bin/python-language-server
chmod 755 /usr/local/bin/python-language-server
```

- Additional setup within a Python project:
  - Add Pyright configuration file: `echo '{}' > pyrightconfig.json`
  - Add style format file to enable format on save, for instance:
    `printf '[style]\nCOLUMN_LIMIT = 120\n' > .style.yapf`

## Rust

- Prerequisites:
  - [Rust](https://www.rust-lang.org/tools/install)

Install language server:

```sh
rustup toolchain install nightly --component rust-analyzer-preview
```

- Additional setup within a Rust project:
  - Add an empty `.rustfmt.toml` file to enable style format on save

## Go

- Prerequisites:
  - [Go](https://golang.org/doc/install)
  - [gopls](https://github.com/golang/tools/blob/master/gopls/README.md#installation) (must be
    installed)

Create symbolic link to enable language server in juCi++:

```sh
# Usually as root:
ln -s `which gopls` /usr/local/bin/go-language-server
```

- Additional setup within a Go project:
  - Add an empty `.go-format` file to enable style format on save

## Julia

- Prerequisites:
  - [Julia](https://julialang.org/downloads/)

Install language server, and create executable to enable server in juCi++:

```sh
julia -e 'using Pkg;Pkg.add("LanguageServer");Pkg.add("SymbolServer");Pkg.add("StaticLint");'

# Usually as root:
echo '#!/bin/sh
julia --startup-file=no --history-file=no -e '\''
using LanguageServer;
using Pkg;
import StaticLint;
import SymbolServer;
env_path = dirname(Pkg.Types.Context().env.project_file);
server = LanguageServer.LanguageServerInstance(stdin, stdout, env_path, "");
server.runlinter = true;
run(server);
'\''' > /usr/local/bin/julia-language-server
chmod 755 /usr/local/bin/julia-language-server
```

- Additional setup within a Julia project:
  - Add an empty `.julia-format` file to enable style format on save

## GLSL

Install language server, and create a script to enable server in juCi++:

```sh
git clone https://github.com/svenstaro/glsl-language-server --recursive
cd glsl-language-server
mkdir build
cd build
cmake ..
make
# Usually as root:
make install
echo '#!/bin/sh
/usr/local/bin/glslls --stdin' > /usr/local/bin/glsl-language-server
chmod 755 /usr/local/bin/glsl-language-server
```

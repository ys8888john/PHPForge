# PHPForge

A PHP compiler built with LLVM 15 that compiles PHP source code to LLVM IR and executes it via JIT.

## Architecture

```
PHP Source → Lexer → Parser → Semantic Analyzer → Code Generator (LLVM IR) → JIT Execution
```

| Stage | Source | Description |
|-------|--------|-------------|
| Lexer | `src/lexer/` | Tokenizes PHP source code |
| Parser | `src/parser/` | Builds AST from tokens |
| AST | `src/ast/` | Abstract syntax tree definitions |
| Semantic | `src/semantic/` | Symbol table & type checking |
| Codegen | `src/codegen/` | Emits LLVM IR via LLVM 15 |
| Interpreter | `src/interpreter/` | Tree-walking interpreter (alternative backend) |

## Prerequisites

- **CMake** >= 3.10
- **C++17** compiler (GCC 9+ / Clang 12+)
- **LLVM 15** (with development headers)

### Installing LLVM 15 (macOS)

```bash
wget https://github.com/llvm/llvm-project/releases/download/llvmorg-15.0.7/clang+llvm-15.0.7-x86_64-apple-darwin21.0.tar.xz
sudo tar xf ./clang+llvm-15.0.7-x86_64-apple-darwin21.0.tar.xz -C /usr/local
sudo mv /usr/local/clang+llvm-15.0.7-x86_64-apple-darwin21.0 /usr/local/llvm-15

# Verify
/usr/local/llvm-15/bin/llvm-config --version

# Add to PATH (optional)
echo 'export LLVM_HOME=/usr/local/llvm-15' >> ~/.zshrc
echo 'export PATH=$LLVM_HOME/bin:$PATH' >> ~/.zshrc
source ~/.zshrc
```

Then configure CMake with:

```bash
cmake .. -DLLVM_DIR=/usr/local/llvm-15/lib/cmake/llvm
```

### Installing LLVM 15 (Ubuntu)

```bash
wget https://apt.llvm.org/llvm.sh
chmod +x llvm.sh
sudo ./llvm.sh 15
sudo apt-get install -y llvm-15-dev libclang-15-dev
```

## Build

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release   # or Debug
cmake --build . -j$(nproc)
```

> If LLVM 15 is installed at a non-default path, pass `-DLLVM_DIR=/path/to/llvm/lib/cmake/llvm`.

## Usage

```bash
./build/PHPForge <php_file>
```

### Example

```php
<?php
declare(strict_types=1);

function add(int $lhs, int $rhs): int {
    return $lhs + $rhs;
}

$ret = add(1, 1);
echo $ret;
?>
```

```bash
$ ./build/PHPForge test/add.php
2
```

The compiler also writes the generated LLVM IR to a `.ll` file alongside the input:

```bash
$ cat test/add.ll
; ModuleID = 'PHPForge'
...
```

## CI

GitHub Actions runs all tests on every push across both Release and Debug builds. See `.github/workflows/ci.yml`.

## License

MIT

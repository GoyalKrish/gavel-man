# THE JUDGE - Web Frontend

A modern, responsive web interface for **THE JUDGE** empirical algorithmic complexity analyzer.

## Features
- **Live Empirical Profiler**: Type or paste any C or C++ program, customize $N$ input values, and run the profiler.
- **Interactive Instruction Scaling Chart**: Visualizes instruction counts ($Ir$) vs. input size ($N$) with Linear and Logarithmic scale toggling.
- **Automated Benchmark Suite**: Direct access to 27 benchmark programs across 9 complexity classes ($O(1)$, $O(\log N)$, $O(\sqrt{N})$, $O(N)$, $O(N \log N)$, $O(N^2)$, $O(N^3)$, $O(2^N)$, $O(N!)$).
- **Test Suite & Reports Inspector**: View aggregate benchmark summaries (`summary.txt`), results tables (`results.csv`), and per-program Callgrind diagnostic traces.
- **Architecture Explainer**: Visual overview of Callgrind collection mechanics, base-case stripping, and ordinary least squares (OLS) regression in data-space.

---

## Quick Start

### 1. Launch the Frontend Server
From the project root:
```bash
python3 frontend/server.py
```
*(Uses Python's standard library — zero external dependencies or pip packages required)*

### 2. Open in Browser
Visit:
```
http://localhost:8080
```

---

## How It Works Under the Hood
1. **Frontend**: Pure Vanilla HTML5, CSS3, and JavaScript with custom Canvas chart rendering.
2. **Server (`server.py`)**: An asynchronous HTTP server that handles API requests without modifying any C++ code.
3. **Execution**:
   - Compiles user code using `g++` with flags `-O0 -g -fno-omit-frame-pointer`.
   - Invokes `./complexity_profiler` which executes the target inside `valgrind --tool=callgrind --collect-atstart=no --toggle-collect=main`.
   - Parses the instruction counts (`Ir`), doubling ratio $I(2N)/I(N)$, power-law exponent $k$, and $R^2$.

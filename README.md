<p align="center">
  <img
    src="https://github.com/user-attachments/assets/64ac3b02-9134-4644-b9b3-65b15d71c786"
    alt="gavel-man logo"
    width="100%"
  />
</p>



# gavel-man

gavel-man is an empirical time-complexity profiler for C/C++ programs. It runs a program with different values of `N`, measures the number of executed instructions using Valgrind Callgrind, and fits the measurements against several mathematical complexity models.

The program is not trying to prove the Big-O mathematically. It checks which model best matches the observed instruction growth.

## Models

For each model, gavel-man compares the measured instruction count `I(N)` against:

| Complexity   | Model                     |
| ------------ | ------------------------- |
| `O(1)`       | `I(N) = c`                |
| `O(log N)`   | `I(N) = c · log(N) + b`   |
| `O(sqrt N)`  | `I(N) = c · sqrt(N) + b`  |
| `O(N)`       | `I(N) = c · N + b`        |
| `O(N log N)` | `I(N) = c · N log(N) + b` |
| `O(N²)`      | `I(N) = c · N² + b`       |
| `O(N³)`      | `I(N) = c · N³ + b`       |
| `O(2^N)`     | `I(N) = c · 2^N + b`      |
| `O(N!)`      | `I(N) = c · N! + b`       |

Here:

* `I(N)` is the measured number of executed instructions.
* `c` represents how much work the program performs per unit of the model.
* `b` represents fixed overhead that does not depend on `N`.

The analyzer fits each model to the measured data and selects the model with the best fit.

## Example

For a linear program, the profiler might produce:

```text
N          Instructions
--------------------------------
128        125420
256        250817
512        501612
1024       1003202
2048       2006381
4096       4012740
```

The instruction count approximately doubles whenever `N` doubles, so the measured behaviour closely matches `O(N)`.

A report for a test program looks like:

```text
============================================================
PROGRAM PROFILE REPORT
============================================================

Program:
    programs/n/0.cc

Expected Complexity:
    O(N)

------------------------------------------------------------
INSTRUCTION SCALING
------------------------------------------------------------

BASE CASE

N = 0
IR = 132481

N          IR                  IR - Base
----------------------------------------------------------------------
0          132481              0
64         133202              721
128        134001              1520
256        135601              3120
512        138801              6320
1024       145201              12720
2048       158001              25520
4096       183601              51120

------------------------------------------------------------
SCALING ANALYSIS
------------------------------------------------------------

Observed Complexity:
    O(N)

Expected Complexity:
    O(N)

============================================================
RESULT: CORRECT
============================================================
```

The test suite runs programs from `programs/<complexity>/`, compares the profiler's answer with the known complexity, and generates individual reports plus an overall accuracy score.

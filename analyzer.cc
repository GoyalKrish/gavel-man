#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <numeric>
#include <sstream>
#include <string>
#include <vector>
#include <unistd.h>
#include <iostream>
#include <cerrno>
#include <cstring>
#include <sys/stat.h>

using namespace std;
namespace fs = std::filesystem;

struct Measurement {
    long long n;
    long long instructions;
};

struct FitResult {
    string name;
    long double rmse = numeric_limits<long double>::infinity();
};

static string shellQuote(const string& s) {
    // Safely quote a string for POSIX shell usage.
    string result = "'";
    for (char c : s) {
        if (c == '\'') {
            result += "'\\''";
        } else {
            result += c;
        }
    }
    result += "'";
    return result;
}

static bool commandExists(const string& command) {
    string cmd = "command -v " + shellQuote(command) + " > /dev/null 2>&1";
    return system(cmd.c_str()) == 0;
}

static bool runCommand(const string& command) {
    cout << "  $ " << command << '\n';
    int rc = system(command.c_str());
    return rc == 0;
}

static long long parseCallgrindSummary(const string& filename) {
    ifstream file(filename);

    if (!file) {
        throw runtime_error("Could not open Callgrind output: " + filename);
    }

    string line;

    while (getline(file, line)) {
        // We only have one event: Ir
        if (line.rfind("summary:", 0) == 0) {
            string value = line.substr(string("summary:").size());

            try {
                return stoll(value);
            } catch (...) {
                throw runtime_error(
                    "Could not parse Callgrind summary in: " + filename
                );
            }
        }
    }

    throw runtime_error("No 'summary:' line found in: " + filename);
}

static long double mean(const vector<long double>& values) {
    if (values.empty()) {
        return 0.0L;
    }

    return accumulate(values.begin(), values.end(), 0.0L) /
           static_cast<long double>(values.size());
}

/*
 * Compute the model-specific transform f(N) for a given N.
 *
 * For each complexity class, f(N) is defined so that
 *     I(N)  ≈  c · f(N) + b
 * becomes a simple linear relationship in (f(N), I(N)) space.
 *
 * The additive constant b captures fixed overhead (argument
 * parsing, stack setup, allocation bookkeeping, etc.) that does
 * not scale with N.
 */
static long double modelTransform(const string& model, long double N) {
    if (model == "O(1)")        return 1.0L;
    if (model == "O(log N)")    return logl(N);
    if (model == "O(sqrt N)")   return sqrtl(N);
    if (model == "O(N)")        return N;
    if (model == "O(N log N)")  return N * logl(N);
    if (model == "O(N^2)")      return N * N;
    if (model == "O(N^3)")      return N * N * N;
    if (model == "O(2^N)")      return expl(N * logl(2.0L));
    if (model == "O(N!)")       return expl(lgammal(N + 1.0L));
    throw runtime_error("Unknown model: " + model);
}

/*
 * Result of fitting I(N) = c · f(N) + b  via OLS linear regression.
 */
struct ModelFit {
    string     name;
    long double r2     = -1.0L;   // R^2: fraction of variance explained
    long double slope  = 0.0L;    // c (must be > 0 for a valid fit)
    long double offset = 0.0L;    // b (additive constant / intercept)
    long double nrmse  = 1e18L;   // normalized RMSE (RMSE / range of I)
};

/*
 * Fit the model   I(N) = c · f(N) + b   using ordinary least squares
 * in data-space (NOT log-space).
 *
 * This correctly handles the additive constant overhead that the
 * old log-space fit could not:
 *     log(c·f(N) + b) ≠ log(c) + log(f(N))  when b is significant.
 *
 * Returns a ModelFit with R², slope c, intercept b, and normalized RMSE.
 */
static ModelFit fitModel(
    const vector<Measurement>& data,
    const string& model
) {
    ModelFit result;
    result.name = model;

    vector<long double> x;   // f(N_i)
    vector<long double> y;   // I(N_i)

    for (const auto& p : data) {
        if (p.n <= 1 || p.instructions <= 0) {
            continue;
        }

        long double N = static_cast<long double>(p.n);
        long double fN = modelTransform(model, N);

        // Guard against overflow for super-polynomial models at large N.
        if (!isfinite(fN) || fN > 1e30L) {
            continue;
        }

        x.push_back(fN);
        y.push_back(static_cast<long double>(p.instructions));
    }

    size_t n = x.size();
    if (n < 2) {
        return result;  // r2 stays -1, nrmse stays huge
    }

    long double xMean = mean(x);
    long double yMean = mean(y);

    /*
     * OLS:   y = slope * x + offset
     *
     * slope = Σ(x_i - x̄)(y_i - ȳ) / Σ(x_i - x̄)²
     */
    long double sxx = 0.0L, sxy = 0.0L;
    for (size_t i = 0; i < n; ++i) {
        long double dx = x[i] - xMean;
        long double dy = y[i] - yMean;
        sxx += dx * dx;
        sxy += dx * dy;
    }

    // For O(1), all x values are 1.0 so sxx ≈ 0.  Handle specially.
    if (model == "O(1)") {
        // The model is I(N) = constant.  Best fit: predicted = mean(y).
        result.slope  = 0.0L;
        result.offset = yMean;

        long double ssTotal    = 0.0L;
        for (size_t i = 0; i < n; ++i) {
            ssTotal += (y[i] - yMean) * (y[i] - yMean);
        }

        // Use coefficient of variation as the score.
        // If CV is small, the data is effectively constant.
        long double stddev = sqrtl(ssTotal / static_cast<long double>(n));
        long double cv = (yMean > 0) ? (stddev / yMean) : 0.0L;

        // Map CV to R²: perfectly constant → R²=1, high CV → R²=0.
        // This makes O(1) comparable with other models on the same scale.
        result.r2 = 1.0L - cv * cv * 100.0L;  // amplify CV to get meaningful R²
        if (result.r2 < -1.0L) result.r2 = -1.0L;

        result.nrmse = cv;  // normalized by mean, so comparable
        return result;
    }

    if (fabsl(sxx) < 1e-18L) {
        return result;  // degenerate: all x values identical
    }

    long double slope = sxy / sxx;
    long double offset = yMean - slope * xMean;

    result.slope  = slope;
    result.offset = offset;

    // Compute R² and NRMSE.
    long double ssTotal    = 0.0L;
    long double ssResidual = 0.0L;
    long double yMin = y[0], yMax = y[0];

    for (size_t i = 0; i < n; ++i) {
        long double predicted = slope * x[i] + offset;
        long double residual  = y[i] - predicted;

        ssTotal    += (y[i] - yMean) * (y[i] - yMean);
        ssResidual += residual * residual;
        if (y[i] < yMin) yMin = y[i];
        if (y[i] > yMax) yMax = y[i];
    }

    if (ssTotal > 1e-12L) {
        result.r2 = 1.0L - ssResidual / ssTotal;

        // Apply adjusted R² penalty: models with 2 params (slope+intercept)
        // are penalized vs simpler models.  adj_R² = 1 - (1-R²)·(n-1)/(n-2)
        if (n > 2) {
            result.r2 = 1.0L - (1.0L - result.r2) *
                        static_cast<long double>(n - 1) / static_cast<long double>(n - 2);
        }
    } else {
        result.r2 = (ssResidual < 1e-12L) ? 1.0L : 0.0L;
    }

    // NRMSE: use RMSE / mean(y) to match CV-based scoring of O(1).
    if (yMean > 0) {
        result.nrmse = sqrtl(ssResidual / static_cast<long double>(n)) / yMean;
    } else {
        result.nrmse = 0.0L;
    }

    // Penalise models where the slope is negative (c < 0 is non-physical:
    // more work at larger N should never cost fewer instructions).
    if (slope < 0) {
        result.r2    = -1.0L;
        result.nrmse = 1e18L;
    }

    return result;
}

/*
 * Power-law fit:  log I = log c + k · log N
 *
 * This is kept as a diagnostic / reporting tool.  The main
 * model selection now uses fitModel() above.
 */
static pair<long double, long double> fitPowerLaw(
    const vector<Measurement>& data
) {
    vector<long double> x;
    vector<long double> y;

    for (const auto& p : data) {
        if (p.n <= 1 || p.instructions <= 0) {
            continue;
        }

        x.push_back(log(static_cast<long double>(p.n)));
        y.push_back(log(static_cast<long double>(p.instructions)));
    }

    if (x.size() < 2) {
        return {0.0L, 0.0L};
    }

    long double xMean = mean(x);
    long double yMean = mean(y);

    long double numerator = 0.0L;
    long double denominator = 0.0L;

    for (size_t i = 0; i < x.size(); ++i) {
        long double dx = x[i] - xMean;
        long double dy = y[i] - yMean;

        numerator += dx * dy;
        denominator += dx * dx;
    }

    if (fabsl(denominator) < 1e-18L) {
        return {0.0L, 0.0L};
    }

    long double k = numerator / denominator;
    long double intercept = yMean - k * xMean;

    // Calculate R^2
    long double ssTotal = 0.0L;
    long double ssResidual = 0.0L;

    for (size_t i = 0; i < x.size(); ++i) {
        long double predicted = intercept + k * x[i];

        ssTotal += (y[i] - yMean) * (y[i] - yMean);
        ssResidual += (y[i] - predicted) * (y[i] - predicted);
    }

    long double r2 = 0.0L;

    if (ssTotal > 1e-18L) {
        r2 = 1.0L - ssResidual / ssTotal;
    }

    return {k, r2};
}

/*
 * Infer the complexity class by fitting every candidate model
 * using data-space OLS regression:
 *
 *     I(N) = c · f(N) + b
 *
 * The model with the highest R² wins.  Ties are broken by
 * normalised RMSE.
 *
 * Special handling:
 *   - If the power-law exponent k ≈ 0, return O(1) immediately.
 *   - Models with negative slope (c < 0) are rejected.
 *   - Super-polynomial models (2^N, N!) that overflow for the
 *     measured N values are gracefully skipped.
 */
static string inferComplexity(
    const vector<Measurement>& data,
    long double& exponent,
    long double& r2
) {
    auto [k, powerR2] = fitPowerLaw(data);

    exponent = k;
    r2 = powerR2;

    // Fast path: if power-law exponent is near zero, the data is
    // essentially constant.  No model selection needed.
    if (fabsl(k) < 0.10L && powerR2 > 0.5L) {
        return "O(1)";
    }

    vector<string> models = {
        "O(1)",
        "O(log N)",
        "O(sqrt N)",
        "O(N)",
        "O(N log N)",
        "O(N^2)",
        "O(N^3)",
        "O(2^N)",
        "O(N!)"
    };

    vector<ModelFit> fits;

    for (const string& model : models) {
        fits.push_back(fitModel(data, model));
    }

    // Sort by R² descending, then by NRMSE ascending to break ties.
    sort(
        fits.begin(),
        fits.end(),
        [](const ModelFit& a, const ModelFit& b) {
            // Higher R² is better.  If within 0.005 of each other,
            // break tie by lower NRMSE.
            if (fabsl(a.r2 - b.r2) > 0.005L) {
                return a.r2 > b.r2;
            }
            return a.nrmse < b.nrmse;
        }
    );

    return fits.front().name;
}

static string compilerFor(const string& source) {
    fs::path path(source);

    string ext = path.extension().string();

    if (ext == ".c") {
        return "gcc";
    }

    if (ext == ".cpp" ||
        ext == ".cc" ||
        ext == ".cxx") {
        return "g++";
    }

    throw runtime_error(
        "Unsupported source extension: " + ext +
        ". Use .c, .cpp, .cc, or .cxx."
    );
}

int main(int argc, char* argv[]) {

    if (argc < 2) {
        cerr << R"(
Usage:
    ./complexity_profiler <source.c|source.cpp> [N1 N2 N3 ...]

Example:
    ./complexity_profiler code.cpp
    ./complexity_profiler code.cpp 10 20 40 80 160 320

Assumption:
    Your program accepts N as the first command-line argument.

Example target program:
    ./code 100
)";
        return 1;
    }

    try {
        string source = fs::absolute(argv[1]).string();

        if (!fs::exists(source)) {
            throw runtime_error("Source file does not exist: " + source);
        }

        if (!commandExists("valgrind")) {
            throw runtime_error(
                "Valgrind is not installed.\n"
                "On Fedora:\n"
                "    sudo dnf install valgrind"
            );
        }

        string compiler = compilerFor(source);

        if (!commandExists(compiler)) {
            throw runtime_error(
                "Required compiler not found: " + compiler
            );
        }

        /*
         * Default N values.
         *
         * They are powers of two so we can easily examine
         * growth when the input size doubles.
         */
        vector<long long> nValues = {128, 256, 512, 1024, 2048, 4096, 8192};


        // User supplied N values override defaults.
        if (argc >= 3) {
            nValues.clear();

            for (int i = 2; i < argc; ++i) {
                long long n = stoll(argv[i]);

                if (n <= 0) {
                    throw runtime_error(
                        "N must be positive: " + to_string(n)
                    );
                }

                nValues.push_back(n);
            }
        }

        sort(nValues.begin(), nValues.end());

        nValues.erase(
            unique(nValues.begin(), nValues.end()),
            nValues.end()
        );

        /*
         * Temporary working directory.
         *
         * Use mkdtemp() instead of a PID-based path so we always get a
         * fresh, unique directory.  This is critical on AWS Lambda where
         * containers are re-used across invocations: the old PID-named
         * directory may already exist (or fail to create) causing every
         * subsequent custom-code run to silently fall back to an error
         * with no Callgrind output.
         */
        char tempDirTemplate[] = "/tmp/complexity_profiler_XXXXXX";
        char* tempDirPtr = mkdtemp(tempDirTemplate);
        if (tempDirPtr == nullptr) {
            throw runtime_error(
                string("Failed to create temporary directory in /tmp: ") +
                strerror(errno) +
                ". Ensure /tmp is writable (required on AWS Lambda)."
            );
        }
        string tempDir(tempDirPtr);

        string executable = tempDir + "/target";

        cout << "\n";
        cout << "========================================\n";
        cout << "     Empirical Complexity Profiler\n";
        cout << "========================================\n\n";

        cout << "Source:\n  " << source << "\n\n";

        cout << "Compiler:\n  " << compiler << "\n\n";

        cout << "Measurement:\n";
        cout << "  Callgrind event: Ir (executed instructions)\n";
        cout << "  Collection starts at main()\n";
        cout << "  Startup/runtime overhead is excluded as much as Callgrind permits\n";
        cout << "  Optimization: -O0\n\n";

        /*
         * Compile target program.
         *
         * -O0:
         *   Prevents the compiler from completely optimizing away
         *   the computation.
         *
         * -g:
         *   Adds debug information.
         */
        string compileCommand =
            compiler +
            " -O0 -g -fno-omit-frame-pointer " +
            shellQuote(source) +
            " -o " +
            shellQuote(executable) +
            " 2>&1";

        cout << "Compiling target...\n";

        if (!runCommand(compileCommand)) {
            throw runtime_error("Compilation failed.");
        }

        // Ensure the compiled binary is executable.
        // On some Lambda environments the umask or filesystem flags
        // may strip the execute bit after compilation.
        if (chmod(executable.c_str(), 0755) != 0) {
            cerr << "  WARNING: chmod +x failed for compiled binary: "
                 << strerror(errno) << "\n";
        }

        cout << "\n";
        cout << "Running measurements...\n\n";

        vector<Measurement> results;

        for (long long N : nValues) {

            string callgrindFile =
                tempDir +
                "/callgrind_" +
                to_string(N) +
                ".out";

            /*
             * IMPORTANT:
             *
             * --collect-atstart=no
             *
             * means Callgrind doesn't collect startup code.
             *
             * --toggle-collect=main
             *
             * turns collection ON when main() starts.
             *
             * Therefore we mostly measure the work caused by
             * the user's program rather than:
             *
             *   _dl_start
             *   dynamic linker
             *   libc startup
             *   relocation
             *   etc.
             */
            string runCommandString =
                "valgrind "
                "--tool=callgrind "
                "--collect-atstart=no "
                "--toggle-collect=main "
                "--callgrind-out-file=" +
                shellQuote(callgrindFile) +
                " " +
                shellQuote(executable) +
                " " +
                to_string(N) +
                " > /dev/null 2>&1";

            cout << "N = " << N << "\n";

            if (!runCommand(runCommandString)) {
                cerr << "  WARNING: execution failed for N = "
                     << N << "\n";

                continue;
            }

            if (!fs::exists(callgrindFile)) {
                cerr << "  WARNING: Callgrind output missing for N = "
                     << N << "\n";

                continue;
            }

            long long instructions =
                parseCallgrindSummary(callgrindFile);

            cout << "  Instructions = "
                 << instructions
                 << "\n\n";

            results.push_back({
                N,
                instructions
            });
        }

        if (results.size() < 3) {
            throw runtime_error(
                "Not enough successful measurements to infer complexity."
            );
        }

        /*
         * Print collected data.
         */
        cout << "\n";
        cout << "========================================\n";
        cout << "              Measurements\n";
        cout << "========================================\n\n";

        cout << left
             << setw(15) << "N"
             << setw(20) << "Instructions"
             << setw(20) << "I(2N)/I(N)"
             << "\n";

        cout << string(55, '-') << "\n";

        for (size_t i = 0; i < results.size(); ++i) {

            string ratio = "-";

            if (i > 0 &&
                results[i].n == 2 * results[i - 1].n &&
                results[i - 1].instructions > 0) {

                long double r =
                    static_cast<long double>(
                        results[i].instructions
                    ) /
                    static_cast<long double>(
                        results[i - 1].instructions
                    );

                ostringstream ss;
                ss << fixed << setprecision(3) << r;
                ratio = ss.str();
            }

            cout << left
                 << setw(15) << results[i].n
                 << setw(20) << results[i].instructions
                 << setw(20) << ratio
                 << "\n";
        }

        /*
         * Infer complexity.
         */
        long double exponent = 0.0L;
        long double powerR2 = 0.0L;

        string complexity =
            inferComplexity(
                results,
                exponent,
                powerR2
            );

        cout << "\n";
        cout << "========================================\n";
        cout << "                Analysis\n";
        cout << "========================================\n\n";

        cout << fixed << setprecision(4);

        cout << "Estimated power-law exponent:\n";
        cout << "  k = " << exponent << "\n\n";

        cout << "Power-law R^2:\n";
        cout << "  " << powerR2 << "\n\n";

        cout << "Likely time complexity:\n";
        cout << "  " << complexity << "\n";

        /*
         * Give an interpretation of the exponent.
         */
        cout << "\nInterpretation:\n";

        if (fabsl(exponent) < 0.15L) {
            cout << "  Instruction count is approximately constant.\n";
        }
        else if (fabsl(exponent - 1.0L) < 0.20L) {
            cout << "  Growth is approximately linear.\n";
        }
        else if (fabsl(exponent - 2.0L) < 0.25L) {
            cout << "  Growth is approximately quadratic.\n";
        }
        else if (fabsl(exponent - 3.0L) < 0.35L) {
            cout << "  Growth is approximately cubic.\n";
        }
        else if (exponent > 3.0L) {
            cout << "  Growth is faster than cubic polynomial behavior.\n";
        }
        else {
            cout << "  Growth does not closely match a simple integer-degree polynomial.\n";
        }

        cout << "\n";
        cout << "Temporary files:\n";
        cout << "  " << tempDir << "\n";

        cout << "\n";
        cout << "NOTE:\n";
        cout << "  This is an empirical estimate, not a mathematical proof.\n";
        cout << "  The program must actually use N in a meaningful way.\n";
        cout << "  Library calls made by the program are included while main()\n";
        cout << "  is being profiled, which is important for operations such as sort().\n";

        cout << "\n";
        cout << "Done.\n";

        return 0;

    } catch (const exception& e) {

        cerr << "\nERROR:\n";
        cerr << "  " << e.what() << "\n";

        return 1;
    }
}
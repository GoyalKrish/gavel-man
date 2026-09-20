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
 * Fits:
 *
 *     log(I) = a + b * x
 *
 * where x is a transformed version of N.
 *
 * This is useful because:
 *
 *   I ~ c
 *   I ~ c log(N)
 *   I ~ c N
 *   I ~ c N log(N)
 *   I ~ c N^2
 *   I ~ c N^3
 *   I ~ c 2^N
 *   I ~ c N!
 *
 * all become approximately linear after the appropriate transform.
 */
static long double fitRMSE(
    const vector<Measurement>& data,
    const string& model
) {
    vector<long double> x;
    vector<long double> y;

    for (const auto& p : data) {
        if (p.n <= 1 || p.instructions <= 0) {
            continue;
        }

        long double N = static_cast<long double>(p.n);
        long double I = static_cast<long double>(p.instructions);

        long double transformed = 0.0L;

        if (model == "O(1)") {
            transformed = 0.0L;
        }
        else if (model == "O(log N)") {
            transformed = log(log(N));
        }
        else if (model == "O(sqrt N)") {
            transformed = 0.5L * log(N);
        }
        else if (model == "O(N)") {
            transformed = log(N);
        }
        else if (model == "O(N log N)") {
            transformed = log(N) + log(log(N));
        }
        else if (model == "O(N^2)") {
            transformed = 2.0L * log(N);
        }
        else if (model == "O(N^3)") {
            transformed = 3.0L * log(N);
        }
        else if (model == "O(2^N)") {
            transformed = N * log(2.0L);
        }
        else if (model == "O(N!)") {
            transformed = lgammal(N + 1.0L);
        }
        else {
            throw runtime_error("Unknown model: " + model);
        }

        x.push_back(transformed);
        y.push_back(log(I));
    }

    if (x.size() < 2) {
        return numeric_limits<long double>::infinity();
    }

    long double xMean = mean(x);
    long double yMean = mean(y);

    /*
     * Since the multiplicative constant c is unknown:
     *
     * log(I) = log(c) + transformed(N)
     *
     * so we only fit an intercept here.
     */
    long double intercept = yMean;

    long double squaredError = 0.0L;

    for (size_t i = 0; i < x.size(); ++i) {
        long double predicted = intercept + x[i] - xMean;
        long double error = y[i] - predicted;
        squaredError += error * error;
    }

    return sqrtl(squaredError / static_cast<long double>(x.size()));
}

static pair<long double, long double> fitPowerLaw(
    const vector<Measurement>& data
) {
    /*
     * Assume:
     *
     *     I(N) ~ c * N^k
     *
     * Taking logs:
     *
     *     log I = log c + k log N
     *
     * So linear regression of log(I) against log(N)
     * estimates k.
     */

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

static string inferComplexity(
    const vector<Measurement>& data,
    long double& exponent,
    long double& r2
) {
    auto [k, powerR2] = fitPowerLaw(data);

    exponent = k;
    r2 = powerR2;

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

    vector<FitResult> fits;

    for (const string& model : models) {
        fits.push_back({
            model,
            fitRMSE(data, model)
        });
    }

    sort(
        fits.begin(),
        fits.end(),
        [](const FitResult& a, const FitResult& b) {
            return a.rmse < b.rmse;
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
         */
        string tempDir =
            "/tmp/complexity_profiler_" +
            to_string(static_cast<long long>(getpid()));

        fs::create_directories(tempDir);

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
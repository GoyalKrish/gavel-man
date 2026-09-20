// ============================================================================
// tester.cc  --  test harness for the empirical complexity analyzer
// ============================================================================
//
// WHAT IT DOES
//   For every program under  programs/<class>/<n>.cc  it:
//     1. compiles the program and measures the BASE CASE (N = 0) with
//        Callgrind, using exactly the same flags as analyzer.cc,
//     2. runs your analyzer binary on the program for N = 64 ... 4096,
//     3. parses the analyzer's measurement table + its final answer
//        ("Likely time complexity"),
//     4. writes a per-program report showing how IR scales from the base
//        case through every N, and whether the final answer was correct,
//     5. at the end writes reports/summary.txt (score, accuracy, per-class
//        breakdown, confusion matrix, failures) and reports/results.csv.
//
// FOLDER LAYOUT (folder name = the TRUE complexity of everything inside it)
//   programs/
//       1/        0.cc 1.cc 2.cc ...      -> O(1)
//       logn/     ...                     -> O(log N)
//       sqrtn/    ...                     -> O(sqrt N)
//       n/        ...                     -> O(N)
//       nlogn/    ...                     -> O(N log N)
//       nn/       ...                     -> O(N^2)      ("n2" also accepted)
//       nnn/      ...                     -> O(N^3)      ("n3" also accepted)
//       2n/       ...                     -> O(2^N)
//       nfact/    ...                     -> O(N!)
//   (Add or rename aliases in classes() below if you use other folder names.)
//
//   Every test program must take N as argv[1] (same rule as analyzer.cc).
//
// BUILD
//   g++ -std=c++17 -O2 tester.cc -o tester
//   g++ -std=c++17 -O2 analyzer.cc -o complexity_profiler      (the analyzer)
//
// RUN
//   ./tester
//   ./tester --analyzer ./complexity_profiler --programs programs
//   ./tester --filter nlogn            # only run programs whose path has "nlogn"
//   ./tester --help
//
// EXIT CODE
//   0 if every program was classified correctly, 1 otherwise, 2 on bad usage.
// ============================================================================

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include <sys/wait.h>
#include <unistd.h>

using namespace std;
namespace fs = std::filesystem;

static const long double NaN = numeric_limits<long double>::quiet_NaN();

// ----------------------------------------------------------------------------
// Configuration
// ----------------------------------------------------------------------------

struct Config {
    string programsDir;                        // auto-detected if empty
    string analyzer   = "./complexity_profiler";
    string reportsDir = "reports";
    string valgrind   = "valgrind";            // prefix used for the N=0 run
    string filter;                             // substring filter on "class/file"
    int    timeoutSec         = 900;           // per program, analyzer run
    int    baselineTimeoutSec = 120;           // per program, N=0 run

    // N values handed to the analyzer (N = 0 is always measured separately,
    // because analyzer.cc rejects N <= 0).  Same list as profiler.cc, minus 0.
    vector<long long> ns = {64, 128, 256, 512, 1024, 2048, 4096};
};

// ----------------------------------------------------------------------------
// Complexity classes: folder-name aliases -> label used by analyzer.cc
// The labels MUST match the model names in analyzer.cc exactly.
// Order here = order used in the reports (slowest growth first).
// ----------------------------------------------------------------------------

struct ClassInfo {
    vector<string> aliases;   // normalized: lowercase, no spaces/underscores/hyphens
    string label;
};

static const vector<ClassInfo>& classes() {
    static const vector<ClassInfo> c = {
        {{"1", "o1", "const", "constant"},                       "O(1)"},
        {{"logn", "log", "olog"},                                "O(log N)"},
        {{"sqrtn", "sqrt", "rootn"},                             "O(sqrt N)"},
        {{"n", "on", "linear"},                                  "O(N)"},
        {{"nlogn", "nlog"},                                      "O(N log N)"},
        {{"nn", "n2", "n^2", "nsquared", "quadratic"},           "O(N^2)"},
        {{"nnn", "n3", "n^3", "ncubed", "cubic"},                "O(N^3)"},
        {{"2n", "2^n", "exp", "exponential"},                    "O(2^N)"},
        {{"nfact", "n!", "fact", "factorial"},                   "O(N!)"},
    };
    return c;
}

// ----------------------------------------------------------------------------
// Small helpers
// ----------------------------------------------------------------------------

static string shellQuote(const string& s) {
    string r = "'";
    for (char c : s) {
        if (c == '\'') r += "'\\''";
        else r += c;
    }
    r += "'";
    return r;
}

static string trim(const string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

static string normalizeKey(const string& s) {
    string out;
    for (unsigned char c : s) {
        if (c == ' ' || c == '_' || c == '-') continue;
        out += static_cast<char>(tolower(c));
    }
    return out;
}

static int classify(const string& dirName) {
    string key = normalizeKey(dirName);
    const auto& cs = classes();
    for (size_t i = 0; i < cs.size(); ++i)
        for (const auto& a : cs[i].aliases)
            if (key == a) return static_cast<int>(i);
    return -1;
}

// "2.cc" < "10.cc"
static bool naturalLess(const string& a, const string& b) {
    size_t i = 0, j = 0;
    while (i < a.size() && j < b.size()) {
        if (isdigit(static_cast<unsigned char>(a[i])) &&
            isdigit(static_cast<unsigned char>(b[j]))) {
            size_t i2 = i, j2 = j;
            while (i2 < a.size() && isdigit(static_cast<unsigned char>(a[i2]))) ++i2;
            while (j2 < b.size() && isdigit(static_cast<unsigned char>(b[j2]))) ++j2;
            string na = a.substr(i, i2 - i), nb = b.substr(j, j2 - j);
            na.erase(0, min(na.find_first_not_of('0'), na.size()));
            nb.erase(0, min(nb.find_first_not_of('0'), nb.size()));
            if (na.size() != nb.size()) return na.size() < nb.size();
            if (na != nb) return na < nb;
            i = i2;
            j = j2;
        } else {
            if (a[i] != b[j]) return a[i] < b[j];
            ++i;
            ++j;
        }
    }
    return (a.size() - i) < (b.size() - j);
}

static string fmtInt(long long v) {
    string s = to_string(v < 0 ? -v : v);
    string out;
    int cnt = 0;
    for (int i = static_cast<int>(s.size()) - 1; i >= 0; --i) {
        out += s[i];
        if (++cnt % 3 == 0 && i > 0) out += ',';
    }
    reverse(out.begin(), out.end());
    return (v < 0 ? "-" : "") + out;
}

static string fmtFixed(long double v, int prec) {
    if (isnan(v)) return "-";
    ostringstream ss;
    ss << fixed << setprecision(prec) << static_cast<double>(v);
    return ss.str();
}

static string fmtSig(long double v, int sig = 5) {
    if (!isfinite(v)) return "-";
    ostringstream ss;
    ss << setprecision(sig) << static_cast<double>(v);
    return ss.str();
}

static string fmtRatio(long double r) {
    if (isnan(r)) return "-";
    if (isinf(r) || r > 1e12L) return "huge";
    return fmtFixed(r, 3);
}

static string joinNs(const vector<long long>& ns, const string& sep) {
    string s;
    for (size_t i = 0; i < ns.size(); ++i) {
        if (i) s += sep;
        s += to_string(ns[i]);
    }
    return s;
}

static string csvEscape(const string& s) {
    if (s.find_first_of(",\"\n") == string::npos) return s;
    string r = "\"";
    for (char c : s) {
        if (c == '"') r += "\"\"";
        else r += c;
    }
    return r + "\"";
}

static string tailLines(const string& text, int count) {
    vector<string> lines;
    istringstream in(text);
    string l;
    while (getline(in, l))
        if (!trim(l).empty()) lines.push_back(l);
    string out;
    size_t start = lines.size() > static_cast<size_t>(count) ? lines.size() - count : 0;
    for (size_t i = start; i < lines.size(); ++i) out += "    " + lines[i] + "\n";
    return out;
}

static string headLines(const string& text, int count) {
    string out;
    istringstream in(text);
    string l;
    int shown = 0;
    while (shown < count && getline(in, l)) {
        if (trim(l).empty()) continue;
        out += "    " + l + "\n";
        ++shown;
    }
    return out;
}

// ----------------------------------------------------------------------------
// Process helpers
// ----------------------------------------------------------------------------

static bool g_haveTimeout = false;

// Runs a shell command, captures stdout+stderr, returns exit code (-1 if the
// command was killed by a signal / could not be started).
static int runCapture(const string& cmd, string& out) {
    out.clear();
    FILE* pipe = popen((cmd + " 2>&1").c_str(), "r");
    if (!pipe) return -1;
    char buf[4096];
    size_t n;
    while ((n = fread(buf, 1, sizeof buf, pipe)) > 0) out.append(buf, n);
    int st = pclose(pipe);
    if (WIFEXITED(st)) return WEXITSTATUS(st);
    return -1;
}

static bool commandExists(const string& command) {
    string out;
    return runCapture("command -v " + shellQuote(command) + " > /dev/null", out) == 0;
}

// Wrap a command in `timeout` (kills the whole process group, incl. valgrind).
static string withTimeout(const string& cmd, int seconds) {
    if (!g_haveTimeout || seconds <= 0) return cmd;
    return "timeout --kill-after=5 " + to_string(seconds) + " sh -c " + shellQuote(cmd);
}

static string compilerFor(const string& source) {
    string ext = fs::path(source).extension().string();
    return ext == ".c" ? "gcc" : "g++";
}

static bool isSourceFile(const fs::path& p) {
    string e = p.extension().string();
    return e == ".cc" || e == ".cpp" || e == ".cxx" || e == ".c";
}

// ----------------------------------------------------------------------------
// Data model
// ----------------------------------------------------------------------------

enum class Status { PASS, FAIL, ERROR };

static const char* statusName(Status s) {
    switch (s) {
        case Status::PASS: return "PASS";
        case Status::FAIL: return "FAIL";
        default:           return "ERROR";
    }
}

struct Point {
    long long n;
    long long ir;
};

struct TestCase {
    string   category;   // folder name, e.g. "nlogn"
    string   expected;   // e.g. "O(N log N)"
    int      rank = 0;   // index into classes()
    fs::path source;     // absolute path
    string   id;         // "nlogn/2.cc"
};

struct Result {
    TestCase       tc;
    Status         status = Status::ERROR;
    string         predicted;               // analyzer's final answer ("" if none)
    string         error;                   // why it is an ERROR
    long long      baseIR = -1;             // IR at N = 0 (-1 = not measured)
    string         baseNote;                // why base case is missing
    vector<Point>  points;                  // N > 0, as reported by the analyzer
    long double    k  = NaN;                // analyzer's power-law exponent
    long double    r2 = NaN;                // analyzer's power-law R^2
    string         interpretation;          // analyzer's one-line interpretation
    string         rawOutput;               // full analyzer output (for debugging)
    string         rawLabel = "RAW ANALYZER OUTPUT";
    double         seconds = 0;
};

// ----------------------------------------------------------------------------
// Model functions (used only for the "expected ratio" / normalisation columns)
// Returns log f(N) for the true class, or NaN if not defined at this N.
// ----------------------------------------------------------------------------

static long double logModel(const string& label, long long n) {
    if (n < 2) return NaN;
    long double N = static_cast<long double>(n);
    if (label == "O(1)")        return 0.0L;
    if (label == "O(log N)")    return logl(logl(N));
    if (label == "O(sqrt N)")   return 0.5L * logl(N);
    if (label == "O(N)")        return logl(N);
    if (label == "O(N log N)")  return logl(N) + logl(logl(N));
    if (label == "O(N^2)")      return 2.0L * logl(N);
    if (label == "O(N^3)")      return 3.0L * logl(N);
    if (label == "O(2^N)")      return N * logl(2.0L);
    if (label == "O(N!)")       return lgammal(N + 1.0L);
    return NaN;
}

// f(N_cur) / f(N_prev) for the expected class
static long double expectedRatio(const string& label, long long nPrev, long long nCur) {
    long double a = logModel(label, nPrev), b = logModel(label, nCur);
    if (isnan(a) || isnan(b)) return NaN;
    return expl(b - a);
}

// ----------------------------------------------------------------------------
// Callgrind base-case (N = 0) measurement
// ----------------------------------------------------------------------------

static long long parseCallgrindSummary(const string& filename) {
    ifstream file(filename);
    if (!file) return -1;
    string line;
    while (getline(file, line)) {
        if (line.rfind("summary:", 0) == 0) {
            try {
                return stoll(line.substr(8));
            } catch (...) {
                return -1;
            }
        }
    }
    return -1;
}

static bool compileProgram(const string& src, const string& exe, string& log) {
    // Same flags as analyzer.cc so N = 0 is measured on an identical binary.
    string cmd = compilerFor(src) + " -O0 -g -fno-omit-frame-pointer " +
                 shellQuote(src) + " -o " + shellQuote(exe);
    return runCapture(cmd, log) == 0;
}

static long long measureBaseline(const Config& cfg, const string& exe,
                                 const string& cgFile, string& note) {
    // Same Callgrind flags as analyzer.cc (count only what happens inside main()).
    string cmd = cfg.valgrind +
                 " --tool=callgrind --collect-atstart=no --toggle-collect=main"
                 " --callgrind-out-file=" + shellQuote(cgFile) +
                 " " + shellQuote(exe) + " 0 > /dev/null 2>&1";
    string out;
    int rc = runCapture(withTimeout(cmd, cfg.baselineTimeoutSec), out);
    if (rc == 124) {
        note = "N = 0 run timed out after " + to_string(cfg.baselineTimeoutSec) + " s";
        return -1;
    }

    // A program may legitimately exit early / non-zero at N = 0 (e.g. "if (n == 0)
    // return 1;"), so use the Callgrind data whenever it exists.
    long long ir = parseCallgrindSummary(cgFile);
    if (ir >= 0) {
        if (rc != 0)
            note = "program exited with code " + to_string(rc) +
                   " at N = 0 (IR up to that point is still used)";
        return ir;
    }
    note = "N = 0 run produced no Callgrind data (exit code " + to_string(rc) +
           "); the program may not accept N = 0, or valgrind is unavailable";
    return -1;
}

// ----------------------------------------------------------------------------
// Parsing the analyzer's stdout
// ----------------------------------------------------------------------------

struct ParsedOutput {
    vector<Point> points;
    string        complexity;
    string        interpretation;
    string        tempDir;
    long double   k  = NaN;
    long double   r2 = NaN;
};

static ParsedOutput parseAnalyzerOutput(const string& out) {
    ParsedOutput p;
    istringstream in(out);
    string line;
    bool inTable = false;
    string expecting;   // which value the next non-empty line holds

    while (getline(in, line)) {
        string t = trim(line);

        if (t == "Measurements") { inTable = true;  expecting.clear(); continue; }
        if (t == "Analysis")     { inTable = false; continue; }

        if (inTable) {
            // Rows look like:  "128   4567890   2.003".  Header / dashes fail to parse.
            istringstream ls(t);
            long long n, ir;
            if (ls >> n >> ir) p.points.push_back({n, ir});
            continue;
        }

        if (t.empty()) continue;

        if (!expecting.empty()) {
            if      (expecting == "r2")         p.r2 = strtold(t.c_str(), nullptr);
            else if (expecting == "complexity") p.complexity = t;
            else if (expecting == "interp")     p.interpretation = t;
            else if (expecting == "temp")       p.tempDir = t;
            expecting.clear();
            continue;
        }

        if      (t.rfind("k = ", 0) == 0)          p.k = strtold(t.c_str() + 4, nullptr);
        else if (t == "Power-law R^2:")            expecting = "r2";
        else if (t == "Likely time complexity:")   expecting = "complexity";
        else if (t == "Interpretation:")           expecting = "interp";
        else if (t == "Temporary files:")          expecting = "temp";
    }
    return p;
}

// ----------------------------------------------------------------------------
// Running one test
// ----------------------------------------------------------------------------

static Result runOne(const Config& cfg, const TestCase& tc, const string& workDir) {
    Result r;
    r.tc = tc;
    auto t0 = chrono::steady_clock::now();
    auto finish = [&]() {
        r.seconds = chrono::duration<double>(chrono::steady_clock::now() - t0).count();
        return r;
    };

    string exe = workDir + "/target";
    string cg  = workDir + "/base.out";
    string src = tc.source.string();

    // 1. compile (also validates the test program itself)
    string compileLog;
    if (!compileProgram(src, exe, compileLog)) {
        r.error = "Test program failed to compile\n" + headLines(compileLog, 8);
        r.rawOutput = compileLog;
        r.rawLabel = "COMPILER OUTPUT";
        r.baseNote = "skipped (test program did not compile)";
        return finish();
    }

    // 2. base case, N = 0
    r.baseIR = measureBaseline(cfg, exe, cg, r.baseNote);
    error_code ec;
    fs::remove(exe, ec);
    fs::remove(cg, ec);

    // 3. the analyzer under test
    string cmd = shellQuote(cfg.analyzer) + " " + shellQuote(src) + " " + joinNs(cfg.ns, " ");
    string out;
    int rc = runCapture(withTimeout(cmd, cfg.timeoutSec), out);
    r.rawOutput = out;

    ParsedOutput parsed = parseAnalyzerOutput(out);

    // Remove the analyzer's leftover /tmp/complexity_profiler_<pid> directory.
    const string prefix = "/tmp/complexity_profiler_";
    if (parsed.tempDir.rfind(prefix, 0) == 0) fs::remove_all(parsed.tempDir, ec);

    r.points         = parsed.points;
    r.k              = parsed.k;
    r.r2             = parsed.r2;
    r.interpretation = parsed.interpretation;

    if (rc == 124) {
        r.error = "Analyzer timed out after " + to_string(cfg.timeoutSec) + " s";
        return finish();
    }
    if (rc != 0) {
        r.error = "Analyzer exited with code " + to_string(rc) + "\n" + tailLines(out, 6);
        return finish();
    }
    if (parsed.complexity.empty()) {
        r.error = "No 'Likely time complexity' found in analyzer output (format changed?)\n" +
                  tailLines(out, 6);
        return finish();
    }

    r.predicted = parsed.complexity;
    r.status = (r.predicted == tc.expected) ? Status::PASS : Status::FAIL;
    return finish();
}

// ----------------------------------------------------------------------------
// Per-program report
// ----------------------------------------------------------------------------

static string scalingTable(const Result& r) {
    ostringstream o;
    auto row = [&](const string& n, const string& ir, const string& net, const string& d,
                   const string& ratio, const string& expect, const string& dev,
                   const string& lk) {
        o << "  " << setw(9) << n << ' ' << setw(16) << ir << ' ' << setw(16) << net
          << ' ' << setw(16) << d << ' ' << setw(9) << ratio << ' ' << setw(9) << expect
          << ' ' << setw(9) << dev << ' ' << setw(8) << lk << '\n';
    };

    row("N", "IR", "IR - IR(0)", "delta IR", "ratio", "expected", "dev %", "local k");
    o << "  " << string(101, '-') << '\n';

    bool haveBase = r.baseIR >= 0;
    if (haveBase) row("0 (base)", fmtInt(r.baseIR), "0", "-", "-", "-", "-", "-");
    else          row("0 (base)", "n/a", "-", "-", "-", "-", "-", "-");

    for (size_t i = 0; i < r.points.size(); ++i) {
        const Point& p = r.points[i];
        string net = haveBase ? fmtInt(p.ir - r.baseIR) : "-";

        if (i == 0) {
            // First step goes from the base case (N = 0) to the first N.
            row(to_string(p.n), fmtInt(p.ir), net,
                haveBase ? fmtInt(p.ir - r.baseIR) : "-", "-", "-", "-", "-");
            continue;
        }

        const Point& q = r.points[i - 1];
        long double ratio = q.ir > 0 ? static_cast<long double>(p.ir) / q.ir : NaN;
        long double exp   = expectedRatio(r.tc.expected, q.n, p.n);
        string dev = "-";
        if (!isnan(ratio) && !isnan(exp) && !isinf(exp) && exp > 0)
            dev = fmtFixed((ratio / exp - 1.0L) * 100.0L, 1);
        string lk = "-";
        if (!isnan(ratio) && ratio > 0 && p.n > q.n)
            lk = fmtFixed(logl(ratio) / logl(static_cast<long double>(p.n) / q.n), 3);

        row(to_string(p.n), fmtInt(p.ir), net, fmtInt(p.ir - q.ir),
            fmtRatio(ratio), fmtRatio(exp), dev, lk);
    }

    o << "\n"
      << "  Legend:\n"
      << "    IR - IR(0) : instructions above the N = 0 base case (fixed overhead removed)\n"
      << "    delta IR   : increase over the previous row (first row: increase over base case)\n"
      << "    ratio      : IR(N) / IR(previous N)\n"
      << "    expected   : the ratio a pure " << r.tc.expected << " program would show for that step\n"
      << "    dev %      : how far the observed ratio is from the expected one\n"
      << "    local k    : log(ratio) / log(N / previous N)  (local growth exponent:\n"
      << "                 ~0 constant, ~1 linear, ~2 quadratic, ~3 cubic)\n";
    return o.str();
}

static string normalizedTable(const Result& r) {
    ostringstream o;
    const string& label = r.tc.expected;
    string fname = label.size() > 3 ? label.substr(2, label.size() - 3) : label;
    string lastCol = "IR/[" + fname + "]";

    o << "  " << setw(9) << "N" << ' ' << setw(14) << "IR/N" << ' ' << setw(14)
      << "IR/(N*log2N)" << ' ' << setw(14) << "IR/N^2" << ' ' << setw(18) << lastCol << '\n';
    o << "  " << string(74, '-') << '\n';

    vector<long double> flat;
    for (const Point& p : r.points) {
        long double N = static_cast<long double>(p.n);
        long double ir = static_cast<long double>(p.ir);
        string a = fmtSig(ir / N);
        string b = p.n >= 2 ? fmtSig(ir / (N * log2l(N))) : "-";
        string c = fmtSig(ir / (N * N));
        string d = "-";
        long double lf = logModel(label, p.n);
        if (!isnan(lf)) {
            long double f = expl(lf);
            if (isfinite(f) && f > 0) {
                d = fmtSig(ir / f);
                flat.push_back(ir / f);
            }
        }
        o << "  " << setw(9) << p.n << ' ' << setw(14) << a << ' ' << setw(14) << b
          << ' ' << setw(14) << c << ' ' << setw(18) << d << '\n';
    }

    if (flat.size() >= 2) {
        size_t cnt = min<size_t>(4, flat.size());
        auto first = flat.end() - cnt;
        long double mn = *min_element(first, flat.end());
        long double mx = *max_element(first, flat.end());
        if (mn > 0)
            o << "\n  Flatness of " << lastCol << " over the last " << cnt
              << " points: max/min = " << fmtFixed(mx / mn, 3)
              << "   (1.000 = perfectly flat = the expected class fits well)\n";
    }
    return o.str();
}

static string growthSummary(const Result& r) {
    ostringstream o;
    if (r.baseIR >= 0)
        o << "  Base case IR(0)                     : " << fmtInt(r.baseIR) << "\n";
    else
        o << "  Base case IR(0)                     : not measured"
          << (r.baseNote.empty() ? string() : " (" + r.baseNote + ")") << "\n";

    if (r.points.size() >= 2) {
        const Point& a = r.points.front();
        const Point& b = r.points.back();
        long double nGrow  = static_cast<long double>(b.n) / a.n;
        long double irGrow = a.ir > 0 ? static_cast<long double>(b.ir) / a.ir : NaN;

        vector<long double> logs;
        for (size_t i = 1; i < r.points.size(); ++i)
            if (r.points[i - 1].ir > 0 && r.points[i].ir > 0)
                logs.push_back(logl(static_cast<long double>(r.points[i].ir) / r.points[i - 1].ir));
        long double geo = NaN;
        if (!logs.empty()) {
            long double s = 0;
            for (long double v : logs) s += v;
            geo = expl(s / logs.size());
        }

        o << "  N range                             : " << a.n << " -> " << b.n
          << "  (x" << fmtFixed(nGrow, 1) << ")\n";
        o << "  IR growth over that range           : x" << fmtFixed(irGrow, 3) << "\n";
        long double expGrow = expectedRatio(r.tc.expected, a.n, b.n);
        string growthLabel = "  Growth a pure " + r.tc.expected + " would show";
        o << growthLabel
          << string(max<int>(1, 38 - static_cast<int>(growthLabel.size())), ' ')
          << ": x" << fmtRatio(expGrow) << "\n";
        o << "  Geometric-mean step ratio           : " << fmtFixed(geo, 3) << "\n";
        if (!isnan(irGrow) && irGrow > 0 && nGrow > 1)
            o << "  Overall log-log slope               : "
              << fmtFixed(logl(irGrow) / logl(nGrow), 3) << "\n";
    }

    if (!isnan(r.k))
        o << "  Analyzer power-law exponent k       : " << fmtFixed(r.k, 4) << "\n";
    if (!isnan(r.r2))
        o << "  Analyzer power-law R^2              : " << fmtFixed(r.r2, 4) << "\n";
    if (!r.interpretation.empty())
        o << "  Analyzer interpretation             : " << r.interpretation << "\n";
    return o.str();
}

static string buildProgramReport(const Config& cfg, const Result& r) {
    ostringstream o;
    const string bar(78, '=');
    const string sep(78, '-');

    o << bar << "\n PROGRAM REPORT: " << r.tc.id << "\n" << bar << "\n\n";
    o << "  Source file        : " << r.tc.source.string() << "\n";
    o << "  Folder             : " << r.tc.category << "\n";
    o << "  Expected (truth)   : " << r.tc.expected << "\n";
    o << "  Analyzer answered  : " << (r.predicted.empty() ? "(no answer)" : r.predicted) << "\n";
    o << "  Result             : " << statusName(r.status) << "\n";
    o << "  Time taken         : " << fmtFixed(r.seconds, 1) << " s\n";
    o << "  N values requested : 0 (base case), " << joinNs(cfg.ns, ", ") << "\n\n";

    o << sep << "\n 1. BASE CASE (N = 0)\n" << sep << "\n";
    if (r.baseIR >= 0) {
        o << "  IR(0) = " << fmtInt(r.baseIR)
          << "   <- fixed cost of the program at N = 0 (setup, argument parsing, ...)\n";
        if (!r.baseNote.empty()) o << "  Note: " << r.baseNote << "\n";
        o << "\n";
    } else
        o << "  Not measured: " << (r.baseNote.empty() ? "unavailable" : r.baseNote) << "\n\n";

    o << sep << "\n 2. HOW IR SCALES ACROSS N\n" << sep << "\n";
    if (r.points.empty()) o << "  No measurements available.\n\n";
    else                  o << scalingTable(r) << "\n";

    if (!r.points.empty()) {
        o << sep << "\n 3. NORMALISED IR (flat column = matching growth rate)\n" << sep << "\n";
        o << normalizedTable(r) << "\n";
    }

    o << sep << "\n 4. GROWTH SUMMARY\n" << sep << "\n";
    o << growthSummary(r) << "\n";

    o << sep << "\n 5. FINAL VERDICT\n" << sep << "\n";
    if (r.status == Status::ERROR) {
        o << "  ERROR - no verdict could be produced.\n";
        o << "  " << r.error << "\n";
    } else {
        o << "  Expected : " << r.tc.expected << "\n";
        o << "  Got      : " << r.predicted << "\n";
        o << "  Verdict  : " << (r.status == Status::PASS ? "CORRECT" : "WRONG") << "\n";
    }
    o << "\n";

    if (!r.rawOutput.empty()) {
        o << sep << "\n " << r.rawLabel << "\n" << sep << "\n" << r.rawOutput;
        if (r.rawOutput.back() != '\n') o << '\n';
    }
    return o.str();
}

// ----------------------------------------------------------------------------
// Summary report + CSV
// ----------------------------------------------------------------------------

struct ClassStat {
    int total = 0, pass = 0, fail = 0, err = 0;
};

static string buildSummary(const Config& cfg, const vector<Result>& results,
                           double totalSeconds, const vector<string>& warnings) {
    ostringstream o;
    const string bar(78, '=');
    const string sep(78, '-');

    int total = static_cast<int>(results.size()), pass = 0, fail = 0, err = 0;
    for (const auto& r : results) {
        if (r.status == Status::PASS) ++pass;
        else if (r.status == Status::FAIL) ++fail;
        else ++err;
    }
    double acc = total ? 100.0 * pass / total : 0.0;

    char when[64];
    time_t now = time(nullptr);
    strftime(when, sizeof when, "%Y-%m-%d %H:%M:%S", localtime(&now));

    o << bar << "\n COMPLEXITY ANALYZER - TEST SUMMARY\n" << bar << "\n\n";
    o << "  Date          : " << when << "\n";
    o << "  Programs dir  : " << cfg.programsDir << "\n";
    o << "  Analyzer      : " << cfg.analyzer << "\n";
    o << "  N values      : 0 (base case), " << joinNs(cfg.ns, ", ") << "\n";
    o << "  Total runtime : " << fmtFixed(totalSeconds, 1) << " s\n\n";

    o << sep << "\n SCORE\n" << sep << "\n";
    o << "  Correct  : " << pass << " / " << total << "\n";
    o << "  Wrong    : " << fail << "\n";
    o << "  Errors   : " << err << "   (compile failure, analyzer crash, timeout, unparsable output)\n";
    o << "  Accuracy : " << fmtFixed(acc, 2) << " %\n\n";

    // ---- per class ----
    map<int, ClassStat> stats;
    for (const auto& r : results) {
        ClassStat& s = stats[r.tc.rank];
        ++s.total;
        if (r.status == Status::PASS) ++s.pass;
        else if (r.status == Status::FAIL) ++s.fail;
        else ++s.err;
    }

    o << sep << "\n PER COMPLEXITY CLASS\n" << sep << "\n";
    o << "  " << left << setw(14) << "Expected" << right << setw(9) << "Correct"
      << setw(8) << "Wrong" << setw(8) << "Errors" << setw(8) << "Total" << setw(11) << "Accuracy" << "\n";
    for (const auto& [rank, s] : stats) {
        double a = s.total ? 100.0 * s.pass / s.total : 0.0;
        o << "  " << left << setw(14) << classes()[rank].label << right << setw(9) << s.pass
          << setw(8) << s.fail << setw(8) << s.err << setw(8) << s.total
          << setw(10) << fmtFixed(a, 1) << "%\n";
    }
    o << "\n";

    // ---- confusion matrix ----
    vector<string> cols;
    auto addCol = [&](const string& c) {
        if (find(cols.begin(), cols.end(), c) == cols.end()) cols.push_back(c);
    };
    for (const auto& [rank, s] : stats) addCol(classes()[rank].label);
    for (const auto& c : classes())
        for (const auto& r : results)
            if (r.predicted == c.label) addCol(c.label);
    for (const auto& r : results)
        if (!r.predicted.empty()) addCol(r.predicted);
    if (err > 0) addCol("ERROR");

    map<pair<string, string>, int> cm;
    for (const auto& r : results)
        ++cm[{r.tc.expected, r.status == Status::ERROR ? "ERROR" : r.predicted}];

    size_t w = 8;
    for (const auto& c : cols) w = max(w, c.size() + 2);

    o << sep << "\n CONFUSION MATRIX  (rows = expected, columns = analyzer's answer)\n" << sep << "\n";
    o << "  " << left << setw(14) << "expected \\ got" << right;
    for (const auto& c : cols) o << setw(static_cast<int>(w)) << c;
    o << "\n";
    for (const auto& [rank, s] : stats) {
        (void)s;
        const string& rowLabel = classes()[rank].label;
        o << "  " << left << setw(14) << rowLabel << right;
        for (const auto& c : cols) {
            auto it = cm.find({rowLabel, c});
            o << setw(static_cast<int>(w)) << (it == cm.end() ? string(".") : to_string(it->second));
        }
        o << "\n";
    }
    o << "\n";

    // ---- failures / errors ----
    o << sep << "\n WRONG ANSWERS\n" << sep << "\n";
    if (fail == 0) o << "  (none)\n";
    for (const auto& r : results)
        if (r.status == Status::FAIL)
            o << "  " << left << setw(20) << r.tc.id << right << " expected "
              << r.tc.expected << ", analyzer said " << r.predicted << "\n";
    o << "\n";

    o << sep << "\n ERRORS\n" << sep << "\n";
    if (err == 0) o << "  (none)\n";
    for (const auto& r : results)
        if (r.status == Status::ERROR) {
            string first = r.error.substr(0, r.error.find('\n'));
            o << "  " << left << setw(20) << r.tc.id << right << " " << first << "\n";
        }
    o << "\n";

    // ---- everything ----
    o << sep << "\n ALL RESULTS\n" << sep << "\n";
    for (const auto& r : results)
        o << "  [" << left << setw(5) << statusName(r.status) << right << "] "
          << left << setw(20) << r.tc.id << " expected " << setw(11) << r.tc.expected
          << " got " << (r.predicted.empty() ? "-" : r.predicted) << right << "\n";
    o << "\n";

    if (!warnings.empty()) {
        o << sep << "\n WARNINGS\n" << sep << "\n";
        for (const auto& w2 : warnings) o << "  " << w2 << "\n";
        o << "\n";
    }
    return o.str();
}

static string buildCsv(const Config& cfg, const vector<Result>& results) {
    ostringstream o;
    o << "folder,program,expected,analyzer_answer,status,base_ir_n0";
    for (long long n : cfg.ns) o << ",ir_n" << n;
    o << ",exponent_k,r2,seconds\n";

    for (const auto& r : results) {
        o << csvEscape(r.tc.category) << ',' << csvEscape(r.tc.id) << ','
          << csvEscape(r.tc.expected) << ',' << csvEscape(r.predicted) << ','
          << statusName(r.status) << ',';
        if (r.baseIR >= 0) o << r.baseIR;
        for (long long n : cfg.ns) {
            o << ',';
            for (const Point& p : r.points)
                if (p.n == n) { o << p.ir; break; }
        }
        o << ',' << (isnan(r.k) ? "" : fmtFixed(r.k, 4))
          << ',' << (isnan(r.r2) ? "" : fmtFixed(r.r2, 4))
          << ',' << fmtFixed(r.seconds, 1) << '\n';
    }
    return o.str();
}

// ----------------------------------------------------------------------------
// Discovering the test programs
// ----------------------------------------------------------------------------

static vector<TestCase> discoverTests(const Config& cfg, vector<string>& warnings) {
    struct Dir { int rank; fs::path path; };
    vector<Dir> dirs;

    for (const auto& e : fs::directory_iterator(cfg.programsDir)) {
        if (!e.is_directory()) continue;
        string name = e.path().filename().string();
        if (!name.empty() && name[0] == '.') continue;
        int rank = classify(name);
        if (rank < 0) {
            warnings.push_back("Skipped folder '" + name +
                               "': not a known complexity class (see classes() in tester.cc)");
            continue;
        }
        dirs.push_back({rank, e.path()});
    }

    sort(dirs.begin(), dirs.end(), [](const Dir& a, const Dir& b) {
        if (a.rank != b.rank) return a.rank < b.rank;
        return naturalLess(a.path.filename().string(), b.path.filename().string());
    });

    vector<TestCase> tests;
    for (const auto& d : dirs) {
        vector<fs::path> files;
        for (const auto& e : fs::directory_iterator(d.path))
            if (e.is_regular_file() && isSourceFile(e.path())) files.push_back(e.path());
        sort(files.begin(), files.end(), [](const fs::path& a, const fs::path& b) {
            return naturalLess(a.filename().string(), b.filename().string());
        });

        if (files.empty())
            warnings.push_back("Folder '" + d.path.filename().string() + "' has no .cc/.cpp/.cxx/.c files");

        for (const auto& f : files) {
            TestCase tc;
            tc.category = d.path.filename().string();
            tc.rank     = d.rank;
            tc.expected = classes()[d.rank].label;
            tc.source   = fs::absolute(f);
            tc.id       = tc.category + "/" + f.filename().string();
            if (!cfg.filter.empty() && tc.id.find(cfg.filter) == string::npos) continue;
            tests.push_back(tc);
        }
    }
    return tests;
}

// ----------------------------------------------------------------------------
// main
// ----------------------------------------------------------------------------

static void printUsage() {
    cout << R"(
Usage: ./tester [options]

Runs every program in programs/<class>/ through the complexity analyzer and
reports whether the analyzer's final answer matches the folder's true class.

Options:
  --programs DIR     programs directory                (default: programs, then Programs)
  --analyzer PATH    analyzer executable               (default: ./complexity_profiler)
  --reports DIR      where reports are written         (default: reports)
  --ns LIST          comma-separated N values          (default: 64,128,256,512,1024,2048,4096)
                     N = 0 is always measured as the base case
  --filter TEXT      only run programs whose "folder/file" contains TEXT
  --timeout SEC      per-program analyzer timeout      (default: 900)
  --valgrind CMD     valgrind command used for the N=0 run (default: valgrind)
                     e.g. --valgrind "VALGRIND_LIB=./valgrind_local/libexec/valgrind ./valgrind_local/bin/valgrind"
  -h, --help         show this help

Folder names -> true complexity:
  1 -> O(1)   logn -> O(log N)   sqrtn -> O(sqrt N)   n -> O(N)   nlogn -> O(N log N)
  nn -> O(N^2)   nnn -> O(N^3)   2n -> O(2^N)   nfact -> O(N!)

Build the analyzer first:
  g++ -std=c++17 -O2 analyzer.cc -o complexity_profiler
)";
}

int main(int argc, char* argv[]) {
    Config cfg;

    for (int i = 1; i < argc; ++i) {
        string a = argv[i];
        auto need = [&]() -> string {
            if (i + 1 >= argc) {
                cerr << "Missing value for " << a << "\n";
                exit(2);
            }
            return argv[++i];
        };

        if (a == "-h" || a == "--help") { printUsage(); return 0; }
        else if (a == "--programs") cfg.programsDir = need();
        else if (a == "--analyzer") cfg.analyzer = need();
        else if (a == "--reports")  cfg.reportsDir = need();
        else if (a == "--valgrind") cfg.valgrind = need();
        else if (a == "--filter")   cfg.filter = need();
        else if (a == "--timeout")  cfg.timeoutSec = atoi(need().c_str());
        else if (a == "--ns") {
            string list = need();
            for (char& c : list) if (c == ',') c = ' ';
            istringstream in(list);
            set<long long> uniq;
            long long n;
            while (in >> n)
                if (n > 0) uniq.insert(n);
            cfg.ns.assign(uniq.begin(), uniq.end());
        } else {
            cerr << "Unknown option: " << a << "\n";
            printUsage();
            return 2;
        }
    }

    if (cfg.ns.size() < 3) {
        cerr << "Need at least 3 positive N values (the analyzer requires >= 3 measurements).\n";
        return 2;
    }

    g_haveTimeout = commandExists("timeout");

    // ---- locate programs dir ----
    if (cfg.programsDir.empty()) {
        if (fs::is_directory("programs"))      cfg.programsDir = "programs";
        else if (fs::is_directory("Programs")) cfg.programsDir = "Programs";
        else {
            cerr << "Could not find a 'programs' directory here. Expected layout:\n"
                    "  programs/1/0.cc  programs/n/0.cc  programs/nlogn/0.cc  programs/nn/0.cc ...\n"
                    "Use --programs DIR to point somewhere else.\n";
            return 2;
        }
    } else if (!fs::is_directory(cfg.programsDir)) {
        cerr << "Programs directory does not exist: " << cfg.programsDir << "\n";
        return 2;
    }

    // ---- locate analyzer ----
    bool analyzerOk = cfg.analyzer.find('/') != string::npos ? fs::exists(cfg.analyzer)
                                                             : commandExists(cfg.analyzer);
    if (!analyzerOk) {
        cerr << "Analyzer not found: " << cfg.analyzer << "\n"
                "Build it first:\n"
                "  g++ -std=c++17 -O2 analyzer.cc -o complexity_profiler\n"
                "or pass its location with --analyzer PATH.\n";
        return 2;
    }

    vector<string> warnings;
    vector<TestCase> tests = discoverTests(cfg, warnings);
    for (const auto& w : warnings) cerr << "WARNING: " << w << "\n";

    if (tests.empty()) {
        cerr << "No test programs found under " << cfg.programsDir << "\n";
        return 2;
    }

    error_code ec;
    fs::create_directories(cfg.reportsDir, ec);
    if (ec) {
        cerr << "Cannot create reports directory '" << cfg.reportsDir << "': " << ec.message() << "\n";
        return 2;
    }

    string workDir = "/tmp/tester_" + to_string(static_cast<long long>(getpid()));
    fs::create_directories(workDir);

    cout << "\nComplexity analyzer test run\n"
         << "  programs : " << cfg.programsDir << "  (" << tests.size() << " programs)\n"
         << "  analyzer : " << cfg.analyzer << "\n"
         << "  N values : 0, " << joinNs(cfg.ns, ", ") << "\n"
         << "  reports  : " << cfg.reportsDir << "/\n\n";

    auto tStart = chrono::steady_clock::now();
    vector<Result> results;

    for (size_t i = 0; i < tests.size(); ++i) {
        const TestCase& tc = tests[i];
        cout << "[" << setw(3) << (i + 1) << "/" << tests.size() << "] " << left << setw(22)
             << tc.id << right << " expected " << left << setw(11) << tc.expected << right
             << " ... " << flush;

        Result r = runOne(cfg, tc, workDir);

        if (r.status == Status::ERROR)
            cout << "ERROR (" << r.error.substr(0, r.error.find('\n')) << ")";
        else
            cout << statusName(r.status) << " (got " << r.predicted << ")";
        cout << "  [" << fmtFixed(r.seconds, 1) << "s]\n";

        // per-program report: reports/<folder>/<file>.txt
        fs::path reportPath = fs::path(cfg.reportsDir) / tc.category / (tc.source.filename().string() + ".txt");
        fs::create_directories(reportPath.parent_path(), ec);
        ofstream(reportPath) << buildProgramReport(cfg, r);

        results.push_back(std::move(r));
    }

    double totalSeconds = chrono::duration<double>(chrono::steady_clock::now() - tStart).count();

    string summary = buildSummary(cfg, results, totalSeconds, warnings);
    ofstream((fs::path(cfg.reportsDir) / "summary.txt").string()) << summary;
    ofstream((fs::path(cfg.reportsDir) / "results.csv").string()) << buildCsv(cfg, results);

    fs::remove_all(workDir, ec);

    cout << "\n" << summary;
    cout << "Reports written to " << cfg.reportsDir << "/\n"
         << "  " << cfg.reportsDir << "/summary.txt\n"
         << "  " << cfg.reportsDir << "/results.csv\n"
         << "  " << cfg.reportsDir << "/<folder>/<file>.txt   (one per program)\n\n";

    for (const auto& r : results)
        if (r.status != Status::PASS) return 1;
    return 0;
}
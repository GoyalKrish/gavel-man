#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

using namespace std;
namespace fs = std::filesystem;


// ============================================================
// CONFIGURATION
// ============================================================

const vector<int> N_VALUES = {
    0,
    64,
    128,
    256,
    512,
    1024,
    2048,
    4096
};


// Directory containing complexity classes.
const string PROGRAMS_DIR = "programs";

// Where all generated reports go.
const string REPORTS_DIR = "reports";

// Your profiler executable.
const string PROFILER = "./profiler";


// ============================================================
// COMPLEXITY REPRESENTATION
// ============================================================

struct Complexity {
    string name;
    double exponent;
};


// Expected complexity of each directory.
//
// You can add more here later.
//
// For example:
//
// "n3" -> 3.0
//
// "nlogn" is handled specially because its exponent is
// approximately 1 but with an additional log factor.
//

Complexity get_complexity(const string& directory)
{
    if (directory == "1")
        return {"O(1)", 0.0};

    if (directory == "logn")
        return {"O(log N)", 0.0};

    if (directory == "sqrt_n")
        return {"O(sqrt N)", 0.5};

    if (directory == "n")
        return {"O(N)", 1.0};

    if (directory == "nlogn")
        return {"O(N log N)", 1.0};

    if (directory == "n15")
        return {"O(N^1.5)", 1.5};

    if (directory == "nn")
        return {"O(N^2)", 2.0};

    return {"UNKNOWN", -1.0};
}


// ============================================================
// PROGRAM RESULT
// ============================================================

struct ProgramResult {

    string program;
    string expected;

    map<int, long long> ir;

    string observed;

    bool correct = false;

    double exponent = 0.0;

    string report_file;
};


// ============================================================
// CREATE DIRECTORY
// ============================================================

void ensure_directory(const fs::path& path)
{
    if (!fs::exists(path))
        fs::create_directories(path);
}


// ============================================================
// RUN YOUR PROFILER
// ============================================================

bool run_profiler(
    const fs::path& source,
    const string& executable,
    const fs::path& output
)
{
    /*
        Your current profiler is hardcoded to:

            helloworld.c
            helloworld

        Therefore we temporarily create a wrapper source file
        and executable names expected by your profiler.

        This assumes your profiler accepts C++ source through
        gcc/g++ appropriately.

        If your profiler is modified later to accept:

            ./profiler source.cc executable

        this function can be simplified considerably.
    */

    fs::path profiler_source =
        "helloworld.c";

    fs::path profiler_executable =
        "helloworld";


    // Copy test source to the name expected by profiler.
    fs::copy_file(
        source,
        profiler_source,
        fs::copy_options::overwrite_existing
    );


    // Run profiler.
    string command =
        PROFILER +
        " > \"" +
        output.string() +
        "\" 2>&1";


    int result =
        system(command.c_str());


    return result == 0;
}


// ============================================================
// PARSE:
//
// Total Instructions (IR) for N=64: 123456
// ============================================================

map<int, long long> parse_ir(
    const fs::path& output
)
{
    map<int, long long> result;

    ifstream file(output);

    if (!file)
        return result;


    regex pattern(
        R"(Total Instructions \(IR\) for N=([0-9]+):\s*([0-9]+))"
    );


    string line;

    smatch match;


    while (getline(file, line)) {

        if (regex_search(line, match, pattern)) {

            int n =
                stoi(match[1].str());

            long long ir =
                stoll(match[2].str());


            result[n] = ir;
        }
    }


    return result;
}


// ============================================================
// CALCULATE LOG-LOG EXPONENT
//
// We subtract the N=0 baseline.
//
// This is important.
//
// If:
//
//     IR(N) = startup + 20N
//
// then looking directly at IR makes the curve look almost
// constant because startup may be huge.
//
// Instead:
//
//     Work(N) = IR(N) - IR(0)
//
// gives:
//
//     Work(N) ≈ 20N
//
// ============================================================

double calculate_exponent(
    const map<int, long long>& ir
)
{
    if (!ir.count(0))
        return NAN;


    long long base =
        ir.at(0);


    vector<double> x;
    vector<double> y;


    for (const auto& [n, instructions] : ir) {

        if (n <= 0)
            continue;


        long long work =
            instructions - base;


        if (work <= 0)
            continue;


        x.push_back(
            log(static_cast<double>(n))
        );


        y.push_back(
            log(static_cast<double>(work))
        );
    }


    if (x.size() < 2)
        return NAN;


    double x_mean = 0;
    double y_mean = 0;


    for (double v : x)
        x_mean += v;


    for (double v : y)
        y_mean += v;


    x_mean /= x.size();
    y_mean /= y.size();


    double numerator = 0;
    double denominator = 0;


    for (size_t i = 0; i < x.size(); ++i) {

        numerator +=
            (x[i] - x_mean) *
            (y[i] - y_mean);


        denominator +=
            (x[i] - x_mean) *
            (x[i] - x_mean);
    }


    if (denominator == 0)
        return NAN;


    return numerator / denominator;
}


// ============================================================
// DETERMINE OBSERVED COMPLEXITY
// ============================================================

string classify_complexity(
    const map<int, long long>& ir
)
{
    if (!ir.count(0))
        return "UNKNOWN";


    long long base =
        ir.at(0);


    vector<double> n_values;
    vector<double> work_values;


    for (const auto& [n, instructions] : ir) {

        if (n <= 0)
            continue;


        long long work =
            instructions - base;


        if (work <= 0)
            continue;


        n_values.push_back(
            static_cast<double>(n)
        );


        work_values.push_back(
            static_cast<double>(work)
        );
    }


    if (n_values.size() < 3)
        return "UNKNOWN";


    // --------------------------------------------------------
    // Calculate exponent.
    // --------------------------------------------------------

    double exponent =
        calculate_exponent(ir);


    if (isnan(exponent))
        return "UNKNOWN";


    // --------------------------------------------------------
    // O(1)
    //
    // If the amount of work above the baseline is basically
    // constant.
    // --------------------------------------------------------

    double min_work =
        *min_element(
            work_values.begin(),
            work_values.end()
        );


    double max_work =
        *max_element(
            work_values.begin(),
            work_values.end()
        );


    if (min_work > 0 &&
        max_work / min_work < 2.0)
    {
        return "O(1)";
    }


    // --------------------------------------------------------
    // O(log N)
    //
    // Logarithmic growth has an apparent exponent close to 0,
    // but unlike O(1), the work increases with N.
    // --------------------------------------------------------

    if (exponent >= 0.05 &&
        exponent < 0.30)
    {
        return "O(log N)";
    }


    // --------------------------------------------------------
    // O(sqrt N)
    // --------------------------------------------------------

    if (exponent >= 0.30 &&
        exponent < 0.75)
    {
        return "O(sqrt N)";
    }


    // --------------------------------------------------------
    // O(N)
    //
    // Give N log N some room because finite measurements can
    // look almost linear.
    // --------------------------------------------------------

    if (exponent >= 0.75 &&
        exponent < 1.15)
    {
        // Check whether the growth has a visible logarithmic
        // factor.
        //
        // Compare:
        //
        // work / N
        //
        // If this is roughly constant -> N
        //
        // If it keeps increasing -> N log N

        vector<double> ratio;


        for (size_t i = 0;
             i < n_values.size();
             ++i)
        {
            ratio.push_back(
                work_values[i] /
                n_values[i]
            );
        }


        double first =
            ratio.front();


        double last =
            ratio.back();


        if (last / first > 1.7)
            return "O(N log N)";


        return "O(N)";
    }


    // --------------------------------------------------------
    // O(N log N)
    // --------------------------------------------------------

    if (exponent >= 1.15 &&
        exponent < 1.35)
    {
        return "O(N log N)";
    }


    // --------------------------------------------------------
    // O(N^1.5)
    // --------------------------------------------------------

    if (exponent >= 1.35 &&
        exponent < 1.75)
    {
        return "O(N^1.5)";
    }


    // --------------------------------------------------------
    // O(N^2)
    // --------------------------------------------------------

    if (exponent >= 1.75)
    {
        return "O(N^2)";
    }


    return "UNKNOWN";
}


// ============================================================
// GENERATE INDIVIDUAL REPORT
// ============================================================

void generate_program_report(
    const ProgramResult& result,
    const fs::path& report_file
)
{
    ofstream report(report_file);


    if (!report)
        return;


    report
        << "============================================================\n";

    report
        << "PROGRAM COMPLEXITY REPORT\n";

    report
        << "============================================================\n\n";


    report
        << "Program:\n"
        << "    "
        << result.program
        << "\n\n";


    report
        << "Expected Complexity:\n"
        << "    "
        << result.expected
        << "\n\n";


    report
        << "------------------------------------------------------------\n";

    report
        << "INSTRUCTION SCALING\n";

    report
        << "------------------------------------------------------------\n\n";


    if (result.ir.count(0)) {

        report
            << "BASE CASE\n\n";

        report
            << "N = 0"
            << "\n";

        report
            << "IR = "
            << result.ir.at(0)
            << "\n\n";


        report
            << "The N=0 instruction count is treated as the\n"
            << "fixed profiler/program startup baseline.\n\n";
    }


    report
        << "INCREMENTAL MEASUREMENTS\n\n";


    long long base =
        result.ir.count(0)
        ? result.ir.at(0)
        : 0;


    report
        << left
        << setw(10)
        << "N"
        << setw(20)
        << "IR"
        << setw(20)
        << "IR - Base"
        << setw(20)
        << "Ratio to Previous"
        << "\n";


    report
        << string(70, '-')
        << "\n";


    long long previous_work = -1;


    for (int n : N_VALUES) {

        if (!result.ir.count(n))
            continue;


        long long current =
            result.ir.at(n);


        long long work =
            current - base;


        report
            << left
            << setw(10)
            << n
            << setw(20)
            << current
            << setw(20)
            << work;


        if (n == 0 || previous_work <= 0) {

            report
                << setw(20)
                << "-";

        }
        else {

            double ratio =
                static_cast<double>(work) /
                previous_work;


            report
                << setw(20)
                << fixed
                << setprecision(3)
                << ratio;
        }


        report
            << "\n";


        if (n > 0)
            previous_work = work;
    }


    report
        << "\n";


    report
        << "------------------------------------------------------------\n";

    report
        << "SCALING ANALYSIS\n";

    report
        << "------------------------------------------------------------\n\n";


    if (!isnan(result.exponent)) {

        report
            << "Observed log-log exponent:\n"
            << "    "
            << fixed
            << setprecision(4)
            << result.exponent
            << "\n\n";
    }


    report
        << "Observed Complexity:\n"
        << "    "
        << result.observed
        << "\n\n";


    report
        << "Expected Complexity:\n"
        << "    "
        << result.expected
        << "\n\n";


    report
        << "============================================================\n";


    if (result.correct) {

        report
            << "RESULT: CORRECT\n";

    }
    else {

        report
            << "RESULT: WRONG\n";
    }


    report
        << "============================================================\n";


    report.close();
}


// ============================================================
// MAIN
// ============================================================

int main()
{
    cout
        << "\n"
        << "============================================================\n"
        << "                PROFILER TEST SUITE\n"
        << "============================================================\n\n";


    // --------------------------------------------------------
    // Check programs directory.
    // --------------------------------------------------------

    if (!fs::exists(PROGRAMS_DIR)) {

        cerr
            << "ERROR: "
            << PROGRAMS_DIR
            << "/ directory does not exist.\n";

        return 1;
    }


    // --------------------------------------------------------
    // Create reports directory.
    // --------------------------------------------------------

    ensure_directory(REPORTS_DIR);


    // --------------------------------------------------------
    // Find complexity directories.
    // --------------------------------------------------------

    vector<fs::path> complexity_dirs;


    for (const auto& entry :
         fs::directory_iterator(PROGRAMS_DIR))
    {
        if (!entry.is_directory())
            continue;


        Complexity complexity =
            get_complexity(
                entry.path().filename().string()
            );


        if (complexity.exponent < 0) {

            cout
                << "WARNING: Unknown complexity directory: "
                << entry.path()
                << "\n";

            continue;
        }


        complexity_dirs.push_back(
            entry.path()
        );
    }


    sort(
        complexity_dirs.begin(),
        complexity_dirs.end()
    );


    // --------------------------------------------------------
    // Results.
    // --------------------------------------------------------

    vector<ProgramResult> all_results;


    // --------------------------------------------------------
    // Run every program.
    // --------------------------------------------------------

    for (const fs::path& complexity_dir :
         complexity_dirs)
    {
        string complexity_name_dir =
            complexity_dir.filename().string();


        Complexity expected =
            get_complexity(
                complexity_name_dir
            );


        cout
            << "\n\n"
            << "############################################################\n"
            << "COMPLEXITY: "
            << expected.name
            << "\n"
            << "DIRECTORY: "
            << complexity_dir
            << "\n"
            << "############################################################\n";


        vector<fs::path> programs;


        for (const auto& entry :
             fs::directory_iterator(complexity_dir))
        {
            if (!entry.is_regular_file())
                continue;


            if (entry.path().extension() != ".cc")
                continue;


            programs.push_back(
                entry.path()
            );
        }


        sort(
            programs.begin(),
            programs.end()
        );


        for (const fs::path& program :
             programs)
        {
            ProgramResult result;


            result.program =
                program.string();


            result.expected =
                expected.name;


            cout
                << "\n------------------------------------------------------------\n"
                << "Testing: "
                << program
                << "\n"
                << "Expected: "
                << expected.name
                << "\n"
                << "------------------------------------------------------------\n";


            // ------------------------------------------------
            // Run profiler for this program.
            // ------------------------------------------------

            fs::path raw_output =
                fs::path(REPORTS_DIR) /
                (
                    complexity_name_dir +
                    "_" +
                    program.stem().string() +
                    ".raw.txt"
                );


            bool profiler_ok =
                run_profiler(
                    program,
                    "helloworld",
                    raw_output
                );


            if (!profiler_ok) {

                cout
                    << "Profiler FAILED.\n";


                result.observed =
                    "PROFILER FAILED";


                result.correct =
                    false;

            }
            else {

                // --------------------------------------------
                // Parse IR.
                // --------------------------------------------

                result.ir =
                    parse_ir(
                        raw_output
                    );


                // --------------------------------------------
                // Analyze.
                // --------------------------------------------

                result.exponent =
                    calculate_exponent(
                        result.ir
                    );


                result.observed =
                    classify_complexity(
                        result.ir
                    );


                result.correct =
                    result.observed ==
                    result.expected;


                // --------------------------------------------
                // Print measurements immediately.
                // --------------------------------------------

                cout
                    << "\nInstruction scaling:\n\n";


                long long base =
                    result.ir.count(0)
                    ? result.ir.at(0)
                    : 0;


                cout
                    << left
                    << setw(10)
                    << "N"
                    << setw(20)
                    << "IR"
                    << setw(20)
                    << "IR - Base"
                    << setw(20)
                    << "Growth"
                    << "\n";


                cout
                    << string(70, '-')
                    << "\n";


                long long previous =
                    -1;


                for (int n : N_VALUES) {

                    if (!result.ir.count(n))
                        continue;


                    long long ir =
                        result.ir.at(n);


                    long long work =
                        ir - base;


                    cout
                        << left
                        << setw(10)
                        << n
                        << setw(20)
                        << ir
                        << setw(20)
                        << work;


                    if (n == 0 ||
                        previous <= 0)
                    {
                        cout
                            << setw(20)
                            << "-";
                    }
                    else {

                        double ratio =
                            static_cast<double>(
                                work
                            ) / previous;


                        cout
                            << setw(20)
                            << fixed
                            << setprecision(3)
                            << ratio;
                    }


                    cout
                        << "\n";


                    if (n > 0)
                        previous = work;
                }


                cout
                    << "\nObserved: "
                    << result.observed
                    << "\n";


                cout
                    << "Expected: "
                    << result.expected
                    << "\n";


                if (!isnan(result.exponent)) {

                    cout
                        << "Exponent: "
                        << fixed
                        << setprecision(4)
                        << result.exponent
                        << "\n";
                }


                cout
                    << "\nRESULT: "
                    << (
                        result.correct
                        ? "CORRECT"
                        : "WRONG"
                    )
                    << "\n";
            }


            // ------------------------------------------------
            // Individual report.
            // ------------------------------------------------

            fs::path individual_dir =
                fs::path(REPORTS_DIR) /
                complexity_name_dir;


            ensure_directory(
                individual_dir
            );


            fs::path report_file =
                individual_dir /
                (
                    program.stem().string() +
                    ".report.txt"
                );


            result.report_file =
                report_file.string();


            generate_program_report(
                result,
                report_file
            );


            all_results.push_back(
                result
            );
        }
    }


    // ========================================================
    // FINAL REPORT
    // ========================================================

    int correct = 0;


    for (const auto& result :
         all_results)
    {
        if (result.correct)
            ++correct;
    }


    int total =
        static_cast<int>(
            all_results.size()
        );


    double accuracy =
        total == 0
        ? 0.0
        : 100.0 *
          static_cast<double>(correct) /
          total;


    fs::path final_report =
        fs::path(REPORTS_DIR) /
        "FINAL_REPORT.txt";


    ofstream final(final_report);


    final
        << "============================================================\n"
        << "                 PROFILER FINAL REPORT\n"
        << "============================================================\n\n";


    final
        << "Total Programs : "
        << total
        << "\n";


    final
        << "Correct        : "
        << correct
        << "\n";


    final
        << "Wrong          : "
        << total - correct
        << "\n";


    final
        << "Accuracy       : "
        << fixed
        << setprecision(2)
        << accuracy
        << "%\n\n";


    final
        << "============================================================\n"
        << "RESULTS BY PROGRAM\n"
        << "============================================================\n\n";


    final
        << left
        << setw(30)
        << "PROGRAM"
        << setw(18)
        << "EXPECTED"
        << setw(18)
        << "OBSERVED"
        << setw(12)
        << "RESULT"
        << "\n";


    final
        << string(78, '-')
        << "\n";


    for (const auto& result :
         all_results)
    {
        final
            << left
            << setw(30)
            << result.program
            << setw(18)
            << result.expected
            << setw(18)
            << result.observed
            << setw(12)
            << (
                result.correct
                ? "CORRECT"
                : "WRONG"
            )
            << "\n";
    }


    final
        << "\n\n"
        << "============================================================\n"
        << "SUMMARY BY COMPLEXITY\n"
        << "============================================================\n\n";


    // Group results by expected complexity.
    map<string, pair<int, int>> summary;


    for (const auto& result :
         all_results)
    {
        summary[result.expected].first++;


        if (result.correct)
            summary[result.expected].second++;
    }


    for (const auto& [complexity, counts] :
         summary)
    {
        int class_total =
            counts.first;


        int class_correct =
            counts.second;


        double class_accuracy =
            class_total == 0
            ? 0.0
            : 100.0 *
              class_correct /
              class_total;


        final
            << left
            << setw(18)
            << complexity
            << " "
            << class_correct
            << "/"
            << class_total
            << " correct"
            << "    "
            << fixed
            << setprecision(2)
            << class_accuracy
            << "%\n";
    }


    final
        << "\n\n"
        << "============================================================\n";


    final
        << "FINAL SCORE: "
        << correct
        << "/"
        << total
        << "\n";


    final
        << "FINAL ACCURACY: "
        << fixed
        << setprecision(2)
        << accuracy
        << "%\n";


    final
        << "============================================================\n";


    final.close();


    // ========================================================
    // PRINT FINAL RESULT
    // ========================================================

    cout
        << "\n\n"
        << "============================================================\n"
        << "                 FINAL TEST RESULTS\n"
        << "============================================================\n\n";


    cout
        << "Total Programs : "
        << total
        << "\n";


    cout
        << "Correct        : "
        << correct
        << "\n";


    cout
        << "Wrong          : "
        << total - correct
        << "\n";


    cout
        << "Accuracy       : "
        << fixed
        << setprecision(2)
        << accuracy
        << "%\n\n";


    cout
        << "Final report:\n"
        << "    "
        << final_report
        << "\n\n";


    cout
        << "Individual reports:\n"
        << "    "
        << REPORTS_DIR
        << "/<complexity>/<program>.report.txt\n\n";


    cout
        << "============================================================\n";


    return 0;
}
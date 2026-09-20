#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstdlib>
#include <cstdio>
using namespace std;

const vector<int>& n_values = {0,64,128,256,512,1024,2048,4096};

void profile_code(const string& source_file,
                  const string& executable)
                {

    string compile_cmd =
        "gcc -g " + source_file + " -o " + executable;

    if (system(compile_cmd.c_str()) != 0) {
        cerr << "Compilation failed.\n";
        return;
    }

    for (int n : n_values) {

        cout << "\n--- Profiling N=" << n << " ---\n";

        string out_file = "callgrind.out." + to_string(n);
        string valgrind_cmd =
            "VALGRIND_LIB=./valgrind_local/libexec/valgrind ./valgrind_local/bin/valgrind "
            "--tool=callgrind "
            "--callgrind-out-file=" + out_file +
            " ./" + executable + " " + to_string(n) +
            " > /dev/null 2>&1";

        int result = system(valgrind_cmd.c_str());

        if (result != 0) {
            cerr << "Valgrind execution failed for N="
                 << n << "\n";
            continue;
        }

        long long total_instructions = 0;

        ifstream file(out_file);

        if (!file.is_open()) {
            cerr << "Could not open " << out_file << "\n";
            continue;
        }

        string line;

        while (getline(file, line)) {

            if (line.rfind("summary:", 0) == 0) {

                size_t pos = line.find(':');

                if (pos != string::npos) {
                    total_instructions =
                        stoll(line.substr(pos + 1));
                }

                break;
            }
        }

        file.close();

        cout << "Total Instructions (IR) for N="
             << n << ": "
             << total_instructions
             << "\n";

        string annotation_file =
            "annotation.out." + to_string(n);

        string annotate_cmd =
            "./valgrind_local/bin/callgrind_annotate "
            "--auto=yes " +
            out_file + " " +
            source_file +
            " > " + annotation_file;

        result = system(annotate_cmd.c_str());

        if (result != 0) {
            cerr << "callgrind_annotate failed.\n";
            remove(out_file.c_str());
            continue;
        }

        ifstream annotation(annotation_file);

        cout << "\nPer-line Execution Instructions:\n\n";

        bool recording = false;

        while (getline(annotation, line)) {
            if (line.find("Ir ") != string::npos &&
                line.find("file:function") != string::npos) {

                recording = true;
                continue;
            }

            if (recording) {

                if (line.empty())
                    continue;

                cout << line << '\n';
            }
        }

        annotation.close();

        remove(out_file.c_str());
        remove(annotation_file.c_str());
    }
}

int main() {

    profile_code(
        "helloworld.c",
        "helloworld"
    );

    return 0;
}
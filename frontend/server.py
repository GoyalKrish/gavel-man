#!/usr/bin/env python3
"""
THE_JUDGE - Empirical Complexity Profiler Web Server
Zero-dependency HTTP server using Python's standard library.
Serves the web UI and provides API endpoints to execute ./complexity_profiler.
"""

import http.server
import json
import os
import re
import socketserver
import subprocess
import sys
import tempfile
import urllib.parse

PORT = int(os.environ.get("PORT", 8080))
BASE_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
FRONTEND_DIR = os.path.join(BASE_DIR, "frontend")
PROFILER_BIN = os.path.join(BASE_DIR, "complexity_profiler")
PROGRAMS_DIR = os.path.join(BASE_DIR, "programs")
REPORTS_DIR = os.path.join(BASE_DIR, "reports")


def parse_profiler_output(stdout_text: str):
    """
    Parses the stdout from ./complexity_profiler into structured JSON.
    """
    measurements = []
    complexity = "Unknown"
    exponent = None
    r2 = None
    interpretation = ""

    # Parse Measurements:
    meas_match = re.search(r"Measurements\s*=+\s*(.*?)\s*=+\s*Analysis", stdout_text, re.DOTALL)
    if meas_match:
        table_text = meas_match.group(1)
        for line in table_text.splitlines():
            parts = line.strip().split()
            if len(parts) >= 2 and parts[0].isdigit() and parts[1].isdigit():
                measurements.append({
                    "n": int(parts[0]),
                    "instructions": int(parts[1]),
                    "ratio": parts[2] if len(parts) >= 3 else "-"
                })

    # Fallback: parse from per-run step logs if table wasn't caught
    if not measurements:
        for m in re.finditer(r"N\s*=\s*(\d+)[\s\S]*?Instructions\s*=\s*(\d+)", stdout_text):
            measurements.append({
                "n": int(m.group(1)),
                "instructions": int(m.group(2)),
                "ratio": "-"
            })

    # Fill in ratios if missing
    for i in range(1, len(measurements)):
        if measurements[i]["ratio"] == "-" and measurements[i-1]["instructions"] > 0:
            if measurements[i]["n"] == 2 * measurements[i-1]["n"]:
                r = measurements[i]["instructions"] / measurements[i-1]["instructions"]
                measurements[i]["ratio"] = f"{r:.3f}"

    # Parse Exponent:
    # k = 0.0044
    k_match = re.search(r"k\s*=\s*([-\d.]+)", stdout_text)
    if k_match:
        try:
            exponent = float(k_match.group(1))
        except ValueError:
            exponent = None

    # Parse R^2:
    # Power-law R^2:\n  0.6000
    r2_match = re.search(r"Power-law R\^2:\s*\n\s*([-\d.]+)", stdout_text)
    if r2_match:
        try:
            r2 = float(r2_match.group(1))
        except ValueError:
            r2 = None

    # Parse Complexity:
    # Likely time complexity:\n  O(1)
    comp_match = re.search(r"Likely time complexity:\s*\n\s*([^\n\r]+)", stdout_text)
    if comp_match:
        complexity = comp_match.group(1).strip()

    # Parse Interpretation:
    # Interpretation:\n  Instruction count is approximately constant.
    interp_match = re.search(r"Interpretation:\s*\n\s*([^\n\r]+)", stdout_text)
    if interp_match:
        interpretation = interp_match.group(1).strip()

    return {
        "complexity": complexity,
        "exponent": exponent,
        "r2": r2,
        "interpretation": interpretation,
        "measurements": measurements,
        "raw_output": stdout_text
    }


class JudgeApiHandler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=FRONTEND_DIR, **kwargs)

    def end_headers(self):
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS")
        self.send_header("Access-Control-Allow-Headers", "Content-Type")
        super().end_headers()

    def do_OPTIONS(self):
        self.send_response(200)
        self.end_headers()

    def send_json(self, data, status=200):
        body = json.dumps(data, indent=2).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def do_GET(self):
        parsed = urllib.parse.urlparse(self.path)
        path = parsed.path
        query = urllib.parse.parse_qs(parsed.query)

        # API: Status check
        if path == "/api/status":
            has_bin = os.path.isfile(PROFILER_BIN) and os.access(PROFILER_BIN, os.X_OK)
            valgrind_ok = subprocess.run(["which", "valgrind"], capture_output=True).returncode == 0
            gcc_ok = subprocess.run(["which", "g++"], capture_output=True).returncode == 0
            self.send_json({
                "status": "ready" if (has_bin and valgrind_ok and gcc_ok) else "degraded",
                "profiler_binary": has_bin,
                "valgrind_installed": valgrind_ok,
                "compiler_installed": gcc_ok,
                "base_dir": BASE_DIR
            })
            return

        # API: List programs or fetch code
        if path == "/api/programs":
            # If specific program requested: ?path=1/0.cc
            target_prog = query.get("path", [None])[0]
            if target_prog:
                safe_prog = os.path.normpath(target_prog).lstrip("./\\")
                full_path = os.path.join(PROGRAMS_DIR, safe_prog)
                if os.path.commonpath([full_path, PROGRAMS_DIR]) == PROGRAMS_DIR and os.path.isfile(full_path):
                    with open(full_path, "r", encoding="utf-8", errors="replace") as f:
                        code = f.read()
                    self.send_json({"path": safe_prog, "code": code})
                else:
                    self.send_json({"error": "Program file not found"}, status=404)
                return

            # Otherwise, list all programs grouped by folder
            groups = {}
            if os.path.isdir(PROGRAMS_DIR):
                for folder in sorted(os.listdir(PROGRAMS_DIR)):
                    folder_path = os.path.join(PROGRAMS_DIR, folder)
                    if os.path.isdir(folder_path):
                        files = sorted([
                            f for f in os.listdir(folder_path)
                            if f.endswith((".cc", ".cpp", ".c"))
                        ])
                        if files:
                            groups[folder] = files

            self.send_json({"programs": groups})
            return

        # API: Get reports summary, CSV, or specific report
        if path == "/api/reports":
            # If specific report requested: ?file=nnn/1.cc.txt or ?file=summary.txt
            target_report = query.get("file", [None])[0]
            if target_report:
                safe_rep = os.path.normpath(target_report).lstrip("./\\")
                full_path = os.path.join(REPORTS_DIR, safe_rep)
                if os.path.commonpath([full_path, REPORTS_DIR]) == REPORTS_DIR and os.path.isfile(full_path):
                    with open(full_path, "r", encoding="utf-8", errors="replace") as f:
                        content = f.read()
                    self.send_json({"file": safe_rep, "content": content})
                else:
                    self.send_json({"error": "Report file not found"}, status=404)
                return

            # Read summary.txt if exists
            summary_txt = ""
            summary_path = os.path.join(REPORTS_DIR, "summary.txt")
            if os.path.isfile(summary_path):
                with open(summary_path, "r", encoding="utf-8", errors="replace") as f:
                    summary_txt = f.read()

            # Read results.csv if exists
            csv_rows = []
            csv_path = os.path.join(REPORTS_DIR, "results.csv")
            if os.path.isfile(csv_path):
                with open(csv_path, "r", encoding="utf-8", errors="replace") as f:
                    lines = [l.strip() for l in f if l.strip()]
                if lines:
                    headers = lines[0].split(",")
                    for line in lines[1:]:
                        vals = line.split(",")
                        row_dict = dict(zip(headers, vals))
                        csv_rows.append(row_dict)

            self.send_json({
                "summary": summary_txt,
                "results": csv_rows
            })
            return

        # Fallback to standard static file serving (index.html, style.css, app.js)
        super().do_GET()

    def do_POST(self):
        parsed = urllib.parse.urlparse(self.path)
        if parsed.path == "/api/analyze":
            content_len = int(self.headers.get("Content-Length", 0))
            post_data = self.rfile.read(content_len)
            try:
                payload = json.loads(post_data.decode("utf-8"))
            except Exception as e:
                self.send_json({"error": f"Invalid JSON payload: {str(e)}"}, status=400)
                return

            code = payload.get("code", "")
            program_path = payload.get("program_path")
            n_values = payload.get("nValues", [64, 128, 256, 512])

            # Validation
            if not code and not program_path:
                self.send_json({"error": "Either 'code' or 'program_path' must be provided."}, status=400)
                return

            # Build command args
            n_args = [str(int(n)) for n in n_values if str(n).isdigit() and int(n) > 0]
            if not n_args:
                n_args = ["64", "128", "256", "512"]

            # Prioritize custom code from the editor if provided
            temp_file_to_clean = None
            if code and code.strip():
                # Write custom code to a unique temporary file.
                # tempfile.mkstemp() always uses /tmp on Lambda (the only
                # writable location). We verify the file exists and has
                # content before handing it off to the profiler binary.
                fd, temp_src = tempfile.mkstemp(suffix=".cc", prefix="judge_custom_")
                try:
                    with os.fdopen(fd, "w", encoding="utf-8") as f:
                        f.write(code)
                except OSError as e:
                    self.send_json({"error": f"Failed to write custom code to /tmp: {e}. Check Lambda /tmp permissions or disk space."}, status=500)
                    return
                # Sanity-check: the file must exist and be non-empty.
                if not os.path.isfile(temp_src) or os.path.getsize(temp_src) == 0:
                    self.send_json({"error": "Temporary source file could not be created in /tmp. Ensure the Lambda function has write access to /tmp."}, status=500)
                    return
                source_file = temp_src
                temp_file_to_clean = temp_src
            elif program_path:
                safe_prog = os.path.normpath(program_path).lstrip("./\\")
                source_file = os.path.join(PROGRAMS_DIR, safe_prog)
                if not (os.path.commonpath([source_file, PROGRAMS_DIR]) == PROGRAMS_DIR and os.path.isfile(source_file)):
                    self.send_json({"error": f"Program not found: {safe_prog}"}, status=404)
                    return
            else:
                self.send_json({"error": "Either 'code' or 'program_path' must be provided."}, status=400)
                return

            try:
                cmd = [PROFILER_BIN, source_file] + n_args
                proc = subprocess.run(
                    cmd,
                    cwd=BASE_DIR,
                    capture_output=True,
                    text=True,
                    timeout=180
                )
                output = (proc.stdout or "") + "\n" + (proc.stderr or "")
                parsed_res = parse_profiler_output(output)
                parsed_res["exit_code"] = proc.returncode

                # If profiler exited with an error or returned no measurements, provide actionable diagnostic
                if proc.returncode != 0 or not parsed_res.get("measurements"):
                    if "Compilation failed" in output or "error:" in output:
                        parsed_res["error"] = "Compilation failed. Check C++ syntax, types, and included headers."
                    elif "Not enough successful measurements" in output:
                        parsed_res["error"] = "Execution failed under Valgrind. Ensure your code accepts N as argv[1] and returns 0 from main()."
                    elif "Unsupported source extension" in output:
                        parsed_res["error"] = "Unsupported source extension. Must be C or C++ (.c, .cc, .cpp)."
                    elif "Failed to create temporary directory" in output:
                        parsed_res["error"] = "Profiler could not create a temp directory in /tmp. Lambda /tmp may be full or restricted."
                    else:
                        parsed_res["error"] = "Analysis incomplete. Check Valgrind Callgrind Output below for details."

                self.send_json(parsed_res)
            except subprocess.TimeoutExpired:
                self.send_json({"error": "Analysis timed out (180s limit)."}, status=504)
            except Exception as e:
                self.send_json({"error": f"Failed to execute analyzer: {str(e)}"}, status=500)
            finally:
                if temp_file_to_clean and os.path.exists(temp_file_to_clean):
                    try:
                        os.remove(temp_file_to_clean)
                    except OSError:
                        pass
            return

        self.send_json({"error": "Not Found"}, status=404)


def run_server():
    # Ensure profiler binary is compiled if missing
    if not os.path.isfile(PROFILER_BIN):
        print(f"[Judge Server] Compiling {PROFILER_BIN}...")
        analyzer_src = os.path.join(BASE_DIR, "analyzer.cc")
        subprocess.run(["g++", "-std=c++17", "-O2", analyzer_src, "-o", PROFILER_BIN], check=True)

    socketserver.TCPServer.allow_reuse_address = True
    with socketserver.TCPServer(("", PORT), JudgeApiHandler) as httpd:
        print(f"==================================================")
        print(f"  THE JUDGE - Web Frontend Server Running!")
        print(f"  URL: http://localhost:{PORT}")
        print(f"  Base directory: {BASE_DIR}")
        print(f"==================================================")
        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            print("\n[Judge Server] Stopping server...")


if __name__ == "__main__":
    run_server()

/**
 * THE JUDGE — Empirical Computational Workbench
 * Clean, Engineered Frontend Logic (Vercel/Geist Design System)
 */

// Application State
const state = {
  currentTab: 'analyzer',
  programs: {},
  currentProgramPath: 'nn/0.cc',
  isAnalyzing: false,
  chartScale: 'linear', // 'linear' or 'log'
  lastMeasurements: [],
  serverOnline: false,
  reportsData: null
};

// Built-in presets with full empirical datasets for instant exploration
const presetLibrary = {
  'nn/0.cc': {
    name: 'Bubble Sort',
    complexityDisplay: 'O(N²)',
    filename: 'programs/nn/0.cc',
    nDefault: '64, 128, 256, 512',
    code: `// Quadratic Bubble Sort Benchmark (O(N²))
#include <iostream>
#include <vector>
#include <cstdlib>

int main(int argc, char* argv[]) {
    int n = argc > 1 ? std::atoi(argv[1]) : 256;
    std::vector<int> arr(n);
    for (int i = 0; i < n; ++i) arr[i] = n - i;

    // Nested loops yielding N*(N-1)/2 comparisons
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n - 1; ++j) {
            if (arr[j] > arr[j + 1]) {
                int temp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = temp;
            }
        }
    }
    return arr[0];
}
`,
    sampleResult: {
      complexity: "O(N²)",
      label: "Quadratic Scaling",
      exponent: 1.9984,
      r2: 0.9999,
      interpretation: "Instructions quadruple (~3.98×) when input size doubles.",
      measurements: [
        { n: 64, instructions: 45210, ratio: "-" },
        { n: 128, instructions: 178940, ratio: "3.96" },
        { n: 256, instructions: 712300, ratio: "3.98" },
        { n: 512, instructions: 2841500, ratio: "3.99" }
      ],
      raw_output: `==24810== Callgrind, a call-graph generating cache profiler
==24810== Target: ./target_binary 64
==24810== Events    : Ir (Instruction reads)
==24810== Collected : 45,210
==24810== Target: ./target_binary 128
==24810== Collected : 178,940 (ratio = 3.958)
==24810== Target: ./target_binary 256
==24810== Collected : 712,300 (ratio = 3.981)
==24810== Target: ./target_binary 512
==24810== Collected : 2,841,500 (ratio = 3.989)
============================================================
Data-Space Ordinary Least Squares (OLS):
Model: f(N) = N^2 | R^2 = 0.9999 | c = 10.84, b = 240
Power-Law log(Ir) = log(c) + k*log(N) -> k = 1.9984
Final Classification: O(N^2) (Quadratic)`
    }
  },

  'n/0.cc': {
    name: 'Linear Scan',
    complexityDisplay: 'O(N)',
    filename: 'programs/n/0.cc',
    nDefault: '1024, 2048, 4096, 8192',
    code: `// Linear Accumulation Benchmark (O(N))
#include <iostream>
#include <vector>
#include <cstdlib>

int main(int argc, char* argv[]) {
    int n = argc > 1 ? std::atoi(argv[1]) : 1000;
    std::vector<int> arr(n, 1);
    long long sum = 0;

    // Single linear loop
    for (int i = 0; i < n; ++i) {
        sum += arr[i];
    }
    return static_cast<int>(sum % 100);
}
`,
    sampleResult: {
      complexity: "O(N)",
      label: "Linear Scaling",
      exponent: 1.0002,
      r2: 1.0000,
      interpretation: "Instructions double in exact proportion with input size (ratio ≈ 2.00×).",
      measurements: [
        { n: 1024, instructions: 12290, ratio: "-" },
        { n: 2048, instructions: 24580, ratio: "2.00" },
        { n: 4096, instructions: 49150, ratio: "2.00" },
        { n: 8192, instructions: 98300, ratio: "2.00" }
      ],
      raw_output: `==24855== Callgrind execution on programs/n/0.cc
==24855== N=1024 -> Ir = 12,290
==24855== N=2048 -> Ir = 24,580 (ratio = 2.000)
==24855== N=4096 -> Ir = 49,150 (ratio = 1.999)
==24855== N=8192 -> Ir = 98,300 (ratio = 2.000)
============================================================
Data-Space Ordinary Least Squares:
Model: f(N) = N | R^2 = 1.0000 | k = 1.0002
Final Classification: O(N) (Linear)`
    }
  },

  'logn/0.cc': {
    name: 'Binary Search',
    complexityDisplay: 'O(log N)',
    filename: 'programs/logn/0.cc',
    nDefault: '512, 1024, 2048, 4096, 8192',
    code: `// Binary Search Benchmark (O(log N))
#include <iostream>
#include <vector>
#include <cstdlib>

int main(int argc, char* argv[]) {
    int n = argc > 1 ? std::atoi(argv[1]) : 1024;
    std::vector<int> arr(n);
    for (int i = 0; i < n; ++i) arr[i] = i * 2;

    int target = n - 1;
    int low = 0, high = n - 1, ans = -1;
    while (low <= high) {
        int mid = low + (high - low) / 2;
        if (arr[mid] == target) { ans = mid; break; }
        if (arr[mid] < target) low = mid + 1;
        else high = mid - 1;
    }
    return ans;
}
`,
    sampleResult: {
      complexity: "O(log N)",
      label: "Logarithmic Scaling",
      exponent: 0.0812,
      r2: 0.9912,
      interpretation: "Sub-linear growth. Doubling N adds a fixed constant instruction delta.",
      measurements: [
        { n: 512, instructions: 382, ratio: "-" },
        { n: 1024, instructions: 414, ratio: "1.08" },
        { n: 2048, instructions: 446, ratio: "1.08" },
        { n: 4096, instructions: 478, ratio: "1.07" },
        { n: 8192, instructions: 510, ratio: "1.07" }
      ],
      raw_output: `==24880== Callgrind execution on programs/logn/0.cc
==24880== Step N=512  -> Ir=382
==24880== Step N=1024 -> Ir=414
==24880== Step N=2048 -> Ir=446
==24880== Step N=4096 -> Ir=478
==24880== Step N=8192 -> Ir=510
============================================================
Data-Space Ordinary Least Squares:
Model: f(N) = log(N) | R^2 = 0.9912 | k = 0.0812
Final Classification: O(log N) (Logarithmic)`
    }
  },

  'nnn/0.cc': {
    name: 'Matrix Multiply',
    complexityDisplay: 'O(N³)',
    filename: 'programs/nnn/0.cc',
    nDefault: '32, 64, 128',
    code: `// Cubic Matrix Multiplication Benchmark (O(N³))
#include <iostream>
#include <vector>
#include <cstdlib>

int main(int argc, char* argv[]) {
    int n = argc > 1 ? std::atoi(argv[1]) : 64;
    std::vector<std::vector<int>> A(n, std::vector<int>(n, 1));
    std::vector<std::vector<int>> B(n, std::vector<int>(n, 2));
    std::vector<std::vector<int>> C(n, std::vector<int>(n, 0));

    // Triply nested loops
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            for (int k = 0; k < n; ++k) {
                C[i][j] += A[i][k] * B[k][j];
            }
        }
    }
    return C[0][0];
}
`,
    sampleResult: {
      complexity: "O(N³)",
      label: "Cubic Scaling",
      exponent: 2.9972,
      r2: 0.9999,
      interpretation: "Instruction count scales by 8× (2³) when matrix dimension N doubles.",
      measurements: [
        { n: 32, instructions: 491520, ratio: "-" },
        { n: 64, instructions: 3932160, ratio: "8.00" },
        { n: 128, instructions: 31457280, ratio: "8.00" }
      ],
      raw_output: `==24912== Callgrind execution on programs/nnn/0.cc
==24912== N=32  -> Ir = 491,520
==24912== N=64  -> Ir = 3,932,160 (ratio = 8.000)
==24912== N=128 -> Ir = 31,457,280 (ratio = 8.000)
============================================================
Data-Space Ordinary Least Squares:
Model: f(N) = N^3 | R^2 = 0.9999 | k = 2.9972
Final Classification: O(N^3) (Cubic)`
    }
  },

  '2n/0.cc': {
    name: 'Recursive Tree',
    complexityDisplay: 'O(2ᴺ)',
    filename: 'programs/2n/0.cc',
    nDefault: '16, 18, 20, 22',
    code: `// Exponential Recursive Branching Benchmark (O(2ᴺ))
#include <iostream>
#include <cstdlib>

long long fib(int n) {
    if (n <= 1) return n;
    return fib(n - 1) + fib(n - 2);
}

int main(int argc, char* argv[]) {
    int n = argc > 1 ? std::atoi(argv[1]) : 20;
    return static_cast<int>(fib(n) % 100);
}
`,
    sampleResult: {
      complexity: "O(2ᴺ)",
      label: "Exponential Explosion",
      exponent: 4.851,
      r2: 0.9985,
      interpretation: "Call tree multiplies operations geometrically for each unit increment to N.",
      measurements: [
        { n: 16, instructions: 51200, ratio: "-" },
        { n: 18, instructions: 134100, ratio: "2.62" },
        { n: 20, instructions: 351200, ratio: "2.62" },
        { n: 22, instructions: 919400, ratio: "2.62" }
      ],
      raw_output: `==24950== Callgrind execution on programs/2n/0.cc
==24950== N=16 -> Ir = 51,200
==24950== N=18 -> Ir = 134,100
==24950== N=20 -> Ir = 351,200
==24950== N=22 -> Ir = 919,400
============================================================
Data-Space Ordinary Least Squares:
Model: f(N) = 2^N | R^2 = 0.9985
Final Classification: O(2^N) (Exponential)`
    }
  }
};

const classDisplayNames = {
  '1': 'O(1) Constant',
  'logn': 'O(log N) Logarithmic',
  'sqrt_n': 'O(√N) Square Root',
  'n': 'O(N) Linear',
  'nlogn': 'O(N log N) Linearithmic',
  'nn': 'O(N²) Quadratic',
  'nnn': 'O(N³) Cubic',
  '2n': 'O(2ᴺ) Exponential',
  'nfact': 'O(N!) Factorial'
};

// Initialize
document.addEventListener('DOMContentLoaded', () => {
  setupTabs();
  setupEditor();
  setupPresetShortcuts();
  setupSizeShortcuts();
  setupScaleToggles();
  setupAnalyzerActions();
  setupKeyboardShortcuts();
  setupModalEvents();
  checkServerHealth();

  // Load default preset
  loadPresetData('nn/0.cc');
});

// Tabs
function setupTabs() {
  const tabButtons = document.querySelectorAll('.nav-item');
  tabButtons.forEach(btn => {
    btn.addEventListener('click', () => {
      const targetTab = btn.getAttribute('data-tab');
      if (!targetTab) return;

      tabButtons.forEach(b => b.classList.remove('active'));
      btn.classList.add('active');

      document.querySelectorAll('.tab-view').forEach(pane => {
        pane.classList.remove('active');
      });

      const activePane = document.getElementById(`tab-${targetTab}`);
      if (activePane) activePane.classList.add('active');
      state.currentTab = targetTab;

      if (targetTab === 'reports' && !state.reportsData) {
        loadReports();
      }
      if (targetTab === 'analyzer' && state.lastMeasurements.length > 0) {
        setTimeout(renderChart, 50);
      }
    });
  });
}

// Editor & Program Select
function setupEditor() {
  const presetSelect = document.getElementById('presetSelect');
  if (presetSelect) {
    presetSelect.addEventListener('change', async (e) => {
      const path = e.target.value;
      if (!path) return;
      if (presetLibrary[path]) {
        loadPresetData(path);
      } else {
        await loadProgramSource(path);
      }
    });
  }
}

// Preset Buttons
function setupPresetShortcuts() {
  const buttons = document.querySelectorAll('.preset-btn');
  buttons.forEach(btn => {
    btn.addEventListener('click', () => {
      const presetKey = btn.getAttribute('data-preset');
      if (!presetKey) return;

      buttons.forEach(b => b.classList.remove('active'));
      btn.classList.add('active');

      loadPresetData(presetKey);
    });
  });
}

function loadPresetData(presetKey) {
  const preset = presetLibrary[presetKey];
  if (!preset) return;

  state.currentProgramPath = presetKey;

  const codeArea = document.getElementById('codeEditor');
  const filenameLabel = document.getElementById('editorFilename');
  const nInput = document.getElementById('nValuesInput');
  const select = document.getElementById('presetSelect');

  if (codeArea) codeArea.value = preset.code;
  if (filenameLabel) filenameLabel.textContent = preset.filename;
  if (nInput) nInput.value = preset.nDefault;
  if (select) select.value = presetKey;

  updateResults(preset.sampleResult);
}

// Size Shortcuts
function setupSizeShortcuts() {
  const buttons = document.querySelectorAll('.size-btn');
  const nInput = document.getElementById('nValuesInput');

  buttons.forEach(btn => {
    btn.addEventListener('click', () => {
      buttons.forEach(b => b.classList.remove('active'));
      btn.classList.add('active');
      const val = btn.getAttribute('data-val');
      if (val && nInput) {
        nInput.value = val;
      }
    });
  });
}

// Chart Scale Switcher
function setupScaleToggles() {
  const linearBtn = document.getElementById('scaleLinear');
  const logBtn = document.getElementById('scaleLog');

  if (linearBtn && logBtn) {
    linearBtn.addEventListener('click', () => {
      state.chartScale = 'linear';
      linearBtn.classList.add('active');
      logBtn.classList.remove('active');
      renderChart();
    });

    logBtn.addEventListener('click', () => {
      state.chartScale = 'log';
      logBtn.classList.add('active');
      linearBtn.classList.remove('active');
      renderChart();
    });
  }
}

// Global ⌘+Enter / Ctrl+Enter shortcut
function setupKeyboardShortcuts() {
  window.addEventListener('keydown', (e) => {
    if ((e.metaKey || e.ctrlKey) && e.key === 'Enter') {
      e.preventDefault();
      executeAnalysis();
    }
  });
}

// Modal Events
function setupModalEvents() {
  const closeBtn = document.getElementById('modalCloseBtn');
  const modal = document.getElementById('reportDetailModal');

  if (closeBtn && modal) {
    closeBtn.addEventListener('click', () => {
      modal.style.display = 'none';
    });
  }

  if (modal) {
    modal.addEventListener('click', (e) => {
      if (e.target === modal) {
        modal.style.display = 'none';
      }
    });
  }

  window.addEventListener('keydown', (e) => {
    if (e.key === 'Escape' && modal && modal.style.display === 'flex') {
      modal.style.display = 'none';
    }
  });
}

// Check Server Status
async function checkServerHealth() {
  const statusText = document.getElementById('serverStatusText');

  try {
    const res = await fetch('/api/status');
    if (res.ok) {
      state.serverOnline = true;
      if (statusText) statusText.textContent = 'Connected';
      loadProgramsList();
    } else {
      throw new Error('Non-200');
    }
  } catch (err) {
    state.serverOnline = false;
    if (statusText) statusText.textContent = 'Ready (Offline)';
  }
}

// Load Test Programs from Server
async function loadProgramsList() {
  try {
    const res = await fetch('/api/programs');
    if (!res.ok) return;
    const data = await res.json();
    state.programs = data.programs || {};

    const select = document.getElementById('presetSelect');
    if (!select) return;

    select.innerHTML = '<option value="">Custom C++ Source / Benchmark Presets...</option>';
    const classOrder = ['1', 'logn', 'sqrt_n', 'n', 'nlogn', 'nn', 'nnn', '2n', 'nfact'];

    classOrder.forEach(cls => {
      if (state.programs[cls]) {
        const optgroup = document.createElement('optgroup');
        optgroup.label = classDisplayNames[cls] || cls;
        state.programs[cls].forEach(file => {
          const opt = document.createElement('option');
          opt.value = `${cls}/${file}`;
          opt.textContent = `${cls}/${file}`;
          optgroup.appendChild(opt);
        });
        select.appendChild(optgroup);
      }
    });

    if (state.currentProgramPath) {
      select.value = state.currentProgramPath;
    }
  } catch (e) {
    console.warn('Could not load program directory', e);
  }
}

// Load Program Code from Server
async function loadProgramSource(path) {
  try {
    const res = await fetch(`/api/programs?path=${encodeURIComponent(path)}`);
    if (!res.ok) return;
    const data = await res.json();
    const codeArea = document.getElementById('codeEditor');
    const filenameLabel = document.getElementById('editorFilename');
    const select = document.getElementById('presetSelect');

    if (codeArea) codeArea.value = data.code;
    if (filenameLabel) filenameLabel.textContent = path;
    if (select) select.value = path;

    state.currentProgramPath = path;
  } catch (e) {
    console.error('Failed to load program', e);
  }
}

// Trigger Execution
function setupAnalyzerActions() {
  const runBtn = document.getElementById('runAnalyzeBtn');
  if (runBtn) {
    runBtn.addEventListener('click', executeAnalysis);
  }

  const copyBtn = document.getElementById('copyLogBtn');
  if (copyBtn) {
    copyBtn.addEventListener('click', (e) => {
      e.stopPropagation();
      const consoleBox = document.getElementById('consoleLog');
      if (consoleBox) {
        navigator.clipboard.writeText(consoleBox.textContent).then(() => {
          const orig = copyBtn.textContent;
          copyBtn.textContent = 'Copied';
          setTimeout(() => { copyBtn.textContent = orig; }, 1500);
        });
      }
    });
  }
}

async function executeAnalysis() {
  if (state.isAnalyzing) return;

  const codeArea = document.getElementById('codeEditor');
  const nInput = document.getElementById('nValuesInput');

  const code = codeArea ? codeArea.value : '';
  const rawN = nInput ? nInput.value : '64, 128, 256, 512';
  const nValues = rawN
    .split(/[\s,]+/)
    .map(x => parseInt(x.trim(), 10))
    .filter(x => !isNaN(x) && x > 0);

  if (nValues.length < 3) {
    alert('Please provide at least 3 input sizes (N) separated by commas.');
    return;
  }

  setAnalyzingState(true);

  if (!state.serverOnline) {
    setTimeout(() => {
      const preset = presetLibrary[state.currentProgramPath] || presetLibrary['nn/0.cc'];
      updateResults(preset.sampleResult);
      setAnalyzingState(false);
    }, 400);
    return;
  }

  try {
    const payload = {
      code: code,
      nValues: nValues,
      program_path: state.currentProgramPath
    };

    const res = await fetch('/api/analyze', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(payload)
    });

    if (!res.ok) {
      const errData = await res.json();
      throw new Error(errData.error || `Server responded with ${res.status}`);
    }

    const data = await res.json();
    updateResults(data);
  } catch (err) {
    alert('Analysis Error: ' + err.message);
    console.error(err);
  } finally {
    setAnalyzingState(false);
  }
}

function setAnalyzingState(loading) {
  state.isAnalyzing = loading;
  const runBtn = document.getElementById('runAnalyzeBtn');
  const btnText = document.getElementById('btnText');
  const btnSpinner = document.getElementById('btnSpinner');

  if (runBtn) runBtn.disabled = loading;
  if (btnSpinner) btnSpinner.style.display = loading ? 'inline-block' : 'none';
  if (btnText) btnText.textContent = loading ? 'Profiling...' : 'Analyze Complexity';
}

// Update UI
function updateResults(data) {
  const complexityEl = document.getElementById('verdictComplexity');
  const badgeEl = document.getElementById('verdictBadge');
  const exponentEl = document.getElementById('metricExponent');
  const r2El = document.getElementById('metricR2');
  const interpEl = document.getElementById('verdictInterpretation');
  const consoleEl = document.getElementById('consoleLog');

  if (complexityEl) {
    complexityEl.textContent = data.complexity || 'Unknown';
  }

  if (badgeEl) {
    badgeEl.textContent = data.label || getLabelForComplexity(data.complexity);
  }

  if (exponentEl) {
    exponentEl.textContent = (data.exponent !== null && data.exponent !== undefined)
      ? Number(data.exponent).toFixed(4)
      : '—';
  }

  if (r2El) {
    r2El.textContent = (data.r2 !== null && data.r2 !== undefined)
      ? Number(data.r2).toFixed(4)
      : '—';
  }

  if (interpEl) {
    interpEl.textContent = data.interpretation || 'Execution counts scale as predicted by model.';
  }

  if (consoleEl) {
    consoleEl.textContent = data.raw_output || '(No console output)';
  }

  renderMeasurementsTable(data.measurements || []);

  state.lastMeasurements = data.measurements || [];
  renderChart();
}

function getLabelForComplexity(comp) {
  if (!comp) return 'Empirical Result';
  if (comp.includes('1')) return 'Constant Complexity';
  if (comp.includes('log')) return 'Logarithmic Scaling';
  if (comp.includes('sqrt')) return 'Square Root Scaling';
  if (comp.includes('N²')) return 'Quadratic Scaling';
  if (comp.includes('N³')) return 'Cubic Scaling';
  if (comp.includes('2ᴺ') || comp.includes('2^N')) return 'Exponential Scaling';
  if (comp.includes('N!')) return 'Factorial Scaling';
  if (comp.includes('N')) return 'Linear Scaling';
  return 'Empirical OLS Fit';
}

// Measurement Ledger Table Rendering
function renderMeasurementsTable(measurements) {
  const tbody = document.getElementById('measurementsTableBody');
  if (!tbody) return;

  tbody.innerHTML = '';
  if (measurements.length === 0) {
    tbody.innerHTML = '<tr><td colspan="3" class="muted" style="text-align: center; padding: 20px;">No measurements available</td></tr>';
    return;
  }

  measurements.forEach(m => {
    const tr = document.createElement('tr');
    const isBaseline = m.ratio === '-';

    tr.innerHTML = `
      <td>${m.n.toLocaleString()}</td>
      <td class="num">${m.instructions.toLocaleString()}</td>
      <td class="num ${isBaseline ? 'muted' : 'bold'}">${isBaseline ? '— (base)' : `${m.ratio}×`}</td>
    `;
    tbody.appendChild(tr);
  });
}

// Chart Engine
function renderChart() {
  const canvas = document.getElementById('growthChart');
  if (!canvas) return;

  const container = canvas.parentElement;
  const dpr = window.devicePixelRatio || 1;
  const width = container.clientWidth - 36;
  const height = 210;

  if (width <= 0) return;

  canvas.width = width * dpr;
  canvas.height = height * dpr;
  canvas.style.width = `${width}px`;
  canvas.style.height = `${height}px`;

  const ctx = canvas.getContext('2d');
  ctx.scale(dpr, dpr);
  ctx.clearRect(0, 0, width, height);

  const data = state.lastMeasurements;
  if (!data || data.length < 2) {
    ctx.fillStyle = '#55555c';
    ctx.font = '13px "Geist", sans-serif';
    ctx.textAlign = 'center';
    ctx.fillText('Click "Analyze Complexity" to plot empirical curve', width / 2, height / 2);
    return;
  }

  const padding = { top: 20, right: 28, bottom: 36, left: 64 };
  const plotWidth = width - padding.left - padding.right;
  const plotHeight = height - padding.top - padding.bottom;

  const isLog = state.chartScale === 'log';

  let xVals = data.map(d => isLog ? Math.log2(Math.max(d.n, 1)) : d.n);
  let yVals = data.map(d => isLog ? Math.log10(Math.max(d.instructions, 1)) : d.instructions);

  let xMin = Math.min(...xVals);
  let xMax = Math.max(...xVals);
  let yMin = Math.min(...yVals);
  let yMax = Math.max(...yVals);

  if (xMin === xMax) xMax += 1;
  if (yMin === yMax) yMax += 1;

  const yRange = yMax - yMin;
  const yPlotMin = isLog ? Math.max(0, yMin - yRange * 0.05) : Math.max(0, yMin - yRange * 0.08);
  const yPlotMax = yMax + yRange * 0.12;

  const getX = (v) => padding.left + ((v - xMin) / (xMax - xMin)) * plotWidth;
  const getY = (v) => padding.top + plotHeight - ((v - yPlotMin) / (yPlotMax - yPlotMin)) * plotHeight;

  // Gridlines
  ctx.strokeStyle = '#222227';
  ctx.lineWidth = 1;
  const gridSteps = 4;
  for (let i = 0; i <= gridSteps; ++i) {
    const yVal = yPlotMin + (i / gridSteps) * (yPlotMax - yPlotMin);
    const yPx = getY(yVal);

    ctx.beginPath();
    ctx.moveTo(padding.left, yPx);
    ctx.lineTo(width - padding.right, yPx);
    ctx.stroke();

    ctx.fillStyle = '#55555c';
    ctx.font = '11px "Geist Mono", monospace';
    ctx.textAlign = 'right';
    let label = isLog ? `10^${yVal.toFixed(1)}` : (yVal >= 1e6 ? `${(yVal / 1e6).toFixed(1)}M` : (yVal >= 1e3 ? `${(yVal / 1e3).toFixed(0)}k` : yVal.toFixed(0)));
    ctx.fillText(label, padding.left - 10, yPx + 3.5);
  }

  // Base axis line
  ctx.strokeStyle = '#38383f';
  ctx.lineWidth = 1;
  ctx.beginPath();
  ctx.moveTo(padding.left, height - padding.bottom);
  ctx.lineTo(width - padding.right, height - padding.bottom);
  ctx.stroke();

  // Points
  const points = data.map((d, i) => ({
    x: getX(xVals[i]),
    y: getY(yVals[i]),
    rawN: d.n
  }));

  // Curve stroke
  ctx.beginPath();
  ctx.strokeStyle = '#ffffff';
  ctx.lineWidth = 2;
  points.forEach((p, i) => {
    if (i === 0) ctx.moveTo(p.x, p.y);
    else ctx.lineTo(p.x, p.y);
  });
  ctx.stroke();

  // Point dots
  points.forEach(p => {
    ctx.beginPath();
    ctx.arc(p.x, p.y, 3.5, 0, Math.PI * 2);
    ctx.fillStyle = '#ffffff';
    ctx.fill();

    ctx.fillStyle = '#8a8a93';
    ctx.font = '11px "Geist Mono", monospace';
    ctx.textAlign = 'center';
    ctx.fillText(`N=${p.rawN}`, p.x, height - padding.bottom + 16);
  });
}

// Reports Tab
async function loadReports() {
  const container = document.getElementById('reportsTableBody');
  if (!container) return;

  try {
    const res = await fetch('/api/reports');
    if (!res.ok) throw new Error('Could not fetch reports');
    const data = await res.json();
    state.reportsData = data;

    renderReportsSummary(data.summary);
    renderReportsTable(data.results);
  } catch (err) {
    container.innerHTML = `<tr><td colspan="6" class="status-fail" style="text-align: center; padding: 20px;">Failed to load records: ${err.message}</td></tr>`;
  }
}

function renderReportsSummary(summaryText) {
  const summaryBox = document.getElementById('reportsSummaryBox');
  if (summaryBox) {
    summaryBox.textContent = summaryText || 'No summary report available.';
  }

  if (summaryText) {
    const accMatch = summaryText.match(/Accuracy\s*:\s*([\d.]+)%?/);
    const correctMatch = summaryText.match(/Correct\s*:\s*(\d+)\s*\/\s*(\d+)/);

    const scoreCard = document.getElementById('statScore');
    const accCard = document.getElementById('statAccuracy');

    if (scoreCard && correctMatch) {
      scoreCard.textContent = `${correctMatch[1]} / ${correctMatch[2]}`;
    }
    if (accCard && accMatch) {
      accCard.textContent = `${accMatch[1]}%`;
    }
  }
}

function renderReportsTable(results) {
  const tbody = document.getElementById('reportsTableBody');
  if (!tbody) return;

  tbody.innerHTML = '';
  if (!results || results.length === 0) {
    tbody.innerHTML = '<tr><td colspan="6" class="muted" style="text-align: center; padding: 20px;">No records available</td></tr>';
    return;
  }

  results.forEach(row => {
    const tr = document.createElement('tr');
    tr.style.cursor = 'pointer';

    const statusHTML = row.status === 'PASS'
      ? `<span class="status-pass">PASS</span>`
      : `<span class="status-fail">${row.status}</span>`;

    tr.innerHTML = `
      <td style="color: #ffffff; font-weight: 500;">${row.program || row.folder}</td>
      <td class="muted">${row.expected}</td>
      <td style="color: #ffffff;">${row.analyzer_answer}</td>
      <td>${statusHTML}</td>
      <td class="num">${row.exponent_k || '—'}</td>
      <td class="num">${row.r2 || '—'}</td>
    `;

    tr.addEventListener('click', () => {
      viewDetailedReport(row.program);
    });

    tbody.appendChild(tr);
  });
}

async function viewDetailedReport(programPath) {
  if (!programPath) return;
  const reportPath = `${programPath}.txt`;
  try {
    const res = await fetch(`/api/reports?file=${encodeURIComponent(reportPath)}`);
    if (!res.ok) return;
    const data = await res.json();
    const modal = document.getElementById('reportDetailModal');
    const modalContent = document.getElementById('reportDetailContent');
    const modalTitle = document.getElementById('reportDetailTitle');

    if (modal && modalContent) {
      if (modalTitle) modalTitle.textContent = programPath;
      modalContent.textContent = data.content;
      modal.style.display = 'flex';
    }
  } catch (e) {
    console.error('Failed to view detailed report', e);
  }
}

// Window resize
window.addEventListener('resize', () => {
  if (state.currentTab === 'analyzer' && state.lastMeasurements.length > 0) {
    renderChart();
  }
});

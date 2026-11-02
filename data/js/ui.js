// ===== KONFIGURASI THRESHOLD & WARNA UNTUK SETIAP SENSOR =====
const THRESHOLDS = {
  suhu: {
    min: 24.0,
    max: 30.0,
    label: "°C",
    color: "#FF8A65", // Coral Orange
    fillColor: "rgba(255, 138, 101, 0.1)",
  },
  hum: {
    min: 60.0,
    max: 85.0,
    label: "%",
    color: "#29B6F6", // Sky Blue
    fillColor: "rgba(41, 182, 246, 0.1)",
  },
  lux: {
    min: 10000,
    max: 50000,
    label: " Lux",
    color: "#FFF176", // Soft Gold
    fillColor: "rgba(255, 241, 118, 0.1)",
  },
};

// ===== STATE MANAGEMENT =====
let currentTab = "suhu";
let chartInstance = null;

// Data arrays untuk setiap sensor (20 titik data)
const chartData = {
  suhu: Array(20).fill(27),
  hum: Array(20).fill(70),
  lux: Array(20).fill(25000),
};

// ===== INISIALISASI CHART =====
function initChart() {
  const ctx = document.getElementById("dynamicChart").getContext("2d");

  chartInstance = new Chart(ctx, {
    type: "line",
    data: {
      labels: Array(20).fill(""),
      datasets: [
        {
          label: "Data Aktual",
          data: chartData.suhu,
          borderColor: THRESHOLDS.suhu.color,
          backgroundColor: THRESHOLDS.suhu.fillColor,
          borderWidth: 2,
          tension: 0.4,
          pointRadius: 0,
          pointHoverRadius: 4,
          fill: true,
        },
        {
          label: "Batas Atas",
          data: Array(20).fill(THRESHOLDS.suhu.max),
          borderColor: "#EF4444",
          borderWidth: 1,
          borderDash: [5, 5],
          pointRadius: 0,
          fill: false,
        },
        {
          label: "Batas Bawah",
          data: Array(20).fill(THRESHOLDS.suhu.min),
          borderColor: "#F59E0B",
          borderWidth: 1,
          borderDash: [5, 5],
          pointRadius: 0,
          fill: false,
        },
      ],
    },
    options: {
      responsive: true,
      maintainAspectRatio: false,
      interaction: { intersect: false, mode: "index" },
      plugins: {
        legend: { display: false },
        tooltip: {
          backgroundColor: "rgba(0, 0, 0, 0.8)",
          titleColor: "#fff",
          bodyColor: "#fff",
          borderColor: "#333",
          borderWidth: 1,
          callbacks: {
            label: function (context) {
              let label = context.dataset.label || "";
              if (label) label += ": ";
              if (context.parsed.y !== null) {
                label +=
                  context.parsed.y.toFixed(1) + THRESHOLDS[currentTab].label;
              }
              return label;
            },
          },
        },
      },
      scales: {
        x: { display: false },
        y: {
          display: true,
          min: getYMin("suhu"),
          max: getYMax("suhu"),
          grid: { color: "#222" },
          ticks: {
            color: "#666",
            font: { size: 10 },
            callback: function (value) {
              return value + THRESHOLDS.suhu.label;
            },
          },
        },
      },
      animation: { duration: 500 },
    },
  });
}

// ===== FUNGSI SWITCH TAB =====
function switchTab(tab, element) {
  currentTab = tab;

  // Update UI tombol aktif
  document
    .querySelectorAll(".tab-btn")
    .forEach((btn) => btn.classList.remove("active"));
  element.classList.add("active");

  // Update data grafik
  updateChartData(tab);
}

// ===== UPDATE CHART DATA PER TAB =====
function updateChartData(tab) {
  if (!chartInstance) return;

  const threshold = THRESHOLDS[tab];

  // Update dataset 0 (data aktual) dengan warna dinamis
  chartInstance.data.datasets[0].data = chartData[tab];
  chartInstance.data.datasets[0].borderColor = threshold.color;
  chartInstance.data.datasets[0].backgroundColor = threshold.fillColor;

  // Update threshold lines
  chartInstance.data.datasets[1].data = Array(20).fill(threshold.max);
  chartInstance.data.datasets[2].data = Array(20).fill(threshold.min);

  // Update Y-axis scale & label
  chartInstance.options.scales.y.min = getYMin(tab);
  chartInstance.options.scales.y.max = getYMax(tab);
  chartInstance.options.scales.y.ticks.callback = function (value) {
    return value + threshold.label;
  };

  chartInstance.update();
}

// ===== HELPER FUNCTIONS UNTUK SKALA Y-AXIS =====
function getYMin(tab) {
  if (tab === "suhu") return 20;
  if (tab === "hum") return 40;
  return 0; // lux
}

function getYMax(tab) {
  if (tab === "suhu") return 40;
  if (tab === "hum") return 100;
  return 60000; // lux
}

// ===== UPDATE CHART DENGAN DATA BARU =====
function updateChart(suhu, hum, lux) {
  chartData.suhu.push(suhu);
  chartData.suhu.shift();
  chartData.hum.push(hum);
  chartData.hum.shift();
  chartData.lux.push(lux);
  chartData.lux.shift();

  if (chartInstance) {
    chartInstance.data.datasets[0].data = chartData[currentTab];
    chartInstance.update();
  }
}

// ===== UPDATE UI KARTU SENSOR & STATUS =====
function updateUI(suhu, hum, lux) {
  const result = decisionTree(suhu, hum, lux);

  document.getElementById("suhuValue").textContent = suhu.toFixed(1);
  document.getElementById("humValue").textContent = Math.round(hum);
  document.getElementById("luxValue").textContent = Math.round(lux);

  const statusCard = document.getElementById("statusCard");
  statusCard.className = "status-card " + result.status;
  document.getElementById("statusTitle").textContent = result.title;
}

// Inisialisasi saat halaman dimuat
document.addEventListener("DOMContentLoaded", initChart);

// ===== MODE REAL-TIME: Ambil data dari ESP8266 =====
const API_URL = '/api/data';

async function ambilData() {
  try {
    const respon = await fetch(API_URL);
    if (!respon.ok) throw new Error('HTTP ' + respon.status);
    const d = await respon.json();

    updateUI(d.suhu, d.hum, d.lux);      // Update kartu & status (dari ui.js)
    updateChart(d.suhu, d.hum, d.lux);   // Update grafik (dari ui.js)
    setKoneksi(true);
  } catch (err) {
    setKoneksi(false);
  }
}

function setKoneksi(ok) {
  const el = document.querySelector('.header-subtitle');
  if (ok) {
    el.innerHTML = '<span class="status-dot"></span> ESP8266 - terhubung';
  } else {
    el.innerHTML = '<span class="status-dot" style="background:#EF4444; animation:none;"></span> ESP8266 - terputus';
  }
}

function updateTime() {
  const now = new Date();
  const hh = String(now.getHours()).padStart(2, '0');
  const mm = String(now.getMinutes()).padStart(2, '0');
  document.getElementById('timeDisplay').textContent = hh + ':' + mm;
}

// Inisialisasi
updateTime();
setInterval(updateTime, 60000);
ambilData();
setInterval(ambilData, 2000);  // Polling setiap 2 detik

console.log('Dashboard terhubung ke ESP8266 via /api/data');

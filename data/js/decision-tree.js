// Threshold Decision Tree
const THRESHOLD = {
  SUHU_MIN: 24.0,
  SUHU_MAX: 30.0,
  HUM_MIN: 60.0,
  HUM_MAX: 85.0,
  LUX_MIN: 10000,
  LUX_MAX: 50000,
};

function decisionTree(suhu, hum, lux) {
  let kelasSuhu =
    suhu < THRESHOLD.SUHU_MIN
      ? "TERLALU_DINGIN"
      : suhu > THRESHOLD.SUHU_MAX
        ? "TERLALU_PANAS"
        : "OPTIMAL";

  let kelasHum =
    hum < THRESHOLD.HUM_MIN
      ? "TERLALU_KERING"
      : hum > THRESHOLD.HUM_MAX
        ? "TERLALU_LEMBAP"
        : "OPTIMAL";

  let kelasCahaya =
    lux < THRESHOLD.LUX_MIN
      ? "TERLALU_GELAP"
      : lux > THRESHOLD.LUX_MAX
        ? "TERLALU_TERANG"
        : "OPTIMAL";

  let jumlahMasalah = 0;
  let daftarMasalah = [];

  if (kelasSuhu !== "OPTIMAL") {
    jumlahMasalah++;
    daftarMasalah.push(
      kelasSuhu === "TERLALU_PANAS"
        ? "Suhu terlalu tinggi"
        : "Suhu terlalu rendah",
    );
  }
  if (kelasHum !== "OPTIMAL") {
    jumlahMasalah++;
    daftarMasalah.push(
      kelasHum === "TERLALU_KERING" ? "Kelembapan rendah" : "Kelembapan tinggi",
    );
  }
  if (kelasCahaya !== "OPTIMAL") {
    jumlahMasalah++;
    daftarMasalah.push(
      kelasCahaya === "TERLALU_GELAP"
        ? "Cahaya kurang"
        : "Cahaya terlalu intens",
    );
  }

  let status, title;
  if (jumlahMasalah === 0) {
    status = "aman";
    title = "Optimal";
  } else if (jumlahMasalah >= 2) {
    status = "bahaya";
    title = "Bahaya";
  } else {
    status = "waspada";
    title = "Peringatan";
  }

  return { status, title, jumlahMasalah, daftarMasalah };
}

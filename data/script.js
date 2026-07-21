// ===== CHART SETUP =====
const ctx = document.getElementById('lineChart').getContext('2d');
const lineChart = new Chart(ctx, {
  type: 'line',
  data: {
    labels: [],
    datasets: [
      {
        label: 'Suhu (°C)',
        data: [],
        borderColor: '#e53935',
        backgroundColor: 'rgba(229,57,53,0.1)',
        tension: 0.4,
        fill: true
      },
      {
        label: 'Kelembapan Udara (%)',
        data: [],
        borderColor: '#1e88e5',
        backgroundColor: 'rgba(30,136,229,0.1)',
        tension: 0.4,
        fill: true
      }
    ]
  },
  options: {
    responsive: true,
    scales: {
      y: { min: 0, max: 100 }
    }
  }
});

// ===== GAUGE SETUP =====
const gauge = new Gauge(document.getElementById('gaugeCanvas')).setOptions({
  angle: -0.2,
  lineWidth: 0.2,
  radiusScale: 0.9,
  pointer: { length: 0.6, strokeWidth: 0.035, color: '#2e7d32' },
  limitMax: false,
  limitMin: false,
  colorStart: '#6fadcf',
  colorStop: '#2e7d32',
  strokeColor: '#e0e0e0',
  generateGradient: true,
  highDpiSupport: true
});
gauge.maxValue = 100;
gauge.setMinValue(0);
gauge.animationSpeed = 32;
gauge.set(0);

// ===== AMBIL DATA DARI ESP32 =====
function ambilData() {
  fetch('/data')
    .then(res => res.json())
    .then(data => {
      // Update kartu
      document.getElementById('suhu').textContent     = data.suhu;
      document.getElementById('humidity').textContent = data.humidity;
      document.getElementById('soil').textContent     = data.soil;

      // Update gauge
      gauge.set(data.soil);
      document.getElementById('gaugeLabel').textContent = data.soil + ' %';

      // Update status relay
      document.getElementById('statusFan').textContent =
        'Status: ' + (data.fan ? '🟢 ON' : '🔴 OFF');
      document.getElementById('statusPompa').textContent =
        'Status: ' + (data.pompa ? '🟢 ON' : '🔴 OFF');

      // Update line chart
      const now = new Date().toLocaleTimeString();
      lineChart.data.labels.push(now);
      lineChart.data.datasets[0].data.push(data.suhu);
      lineChart.data.datasets[1].data.push(data.humidity);

      // Batasi data di chart max 10 titik
      if (lineChart.data.labels.length > 10) {
        lineChart.data.labels.shift();
        lineChart.data.datasets[0].data.shift();
        lineChart.data.datasets[1].data.shift();
      }
      lineChart.update();
    })
    .catch(err => console.log('Error:', err));
}

// ===== KONTROL RELAY =====
function kontrolRelay(device, action) {
  fetch('/' + device + '/' + action)
    .then(res => res.text())
    .then(msg => {
      console.log(msg);
      ambilData(); // refresh data
    });
}

// ===== WAKTU =====
function updateWaktu() {
  document.getElementById('waktu').textContent =
    'Update: ' + new Date().toLocaleString('id-ID');
}

// ===== AUTO REFRESH =====
ambilData();
updateWaktu();
setInterval(() => {
  ambilData();
  updateWaktu();
}, 5000); // refresh tiap 5 detik

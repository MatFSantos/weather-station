const char html_page[] =
"HTTP/1.1 200 OK\r\n"
"Content-Type: text/html\r\n"
"Connection: close\r\n"
"\r\n"
"<!DOCTYPE html>"
"<html lang=\"pt-BR\">"
"<head>"
"<meta charset=\"UTF-8\">"
"<title>Estação Meteorológica</title>"
"<script src=\"https://cdn.jsdelivr.net/npm/chart.js\"></script>"
"<style>"
"body{"
"font-family: sans-serif;"
"font-size: 14px;"
"margin: 0;"
"padding: 10px;"
"display: flex;"
"flex-direction: column;"
"align-items: center;"
"text-align: center;"
"}"
"h1,h2{"
"font-size: 18px;"
"margin: 10px 0;"
"}"
".sensor{"
"margin: 10px 0;"
"}"
".charts{"
"display: flex;"
"flex-wrap: wrap;"
"justify-content: center;"
"gap: 10px;"
"margin-bottom: 20px;"
"}"
".chart-box {"
"width: 300px;"
"}"
"canvas {"
"width: 100%%;"
"height: 150px;"
"}"
"form {"
"display: inline-block;"
"text-align: left;"
"}"
"form label {"
"display: inline-block;"
"width: fit-content;"
"}"
"form input {"
"width: 60px;"
"margin: 2px;"
"}"
"button {"
"margin-top: 8px;"
"}"
"</style>"
"</head>"
"<body>"
"<h1>Estação Meteorológica</h1>"
"<div class=\"sensor\">"
"<div><strong>Temperatura:</strong> <span id=\"temp\">--</span> °C</div>"
"<div><strong>Umidade:</strong> <span id=\"hum\">--</span> %%</div>"
"<div><strong>Pressão:</strong> <span id=\"pres\">--</span> kPa</div>"
"</div>"
"<div class=\"charts\">"
"<div class=\"chart-box\"><canvas id=\"chartTemp\"></canvas></div>"
"<div class=\"chart-box\"><canvas id=\"chartHum\"></canvas></div>"
"<div class=\"chart-box\"><canvas id=\"chartPres\"></canvas></div>"
"</div>"
"<h2>Configurações</h2>"
"<form id=\"configForm\">"
"<div>"
"<label for=\"temp_min\">Temp Min:</label><input type=\"number\" name=\"temp_min\" id=\"temp_min\">"
"<label for=\"temp_max\">Max:</label><input type=\"number\" name=\"temp_max\" id=\"temp_max\">"
"<label for=\"offset_temp\">Offset:</label><input type=\"number\" name=\"offset_temp\" id=\"offset_temp\">"
"</div>"
"<div>"
"<label for=\"hum_min\">Hum Min:</label><input type=\"number\" name=\"hum_min\" id=\"hum_min\">"
"<label for=\"hum_max\">Max:</label><input type=\"number\" name=\"hum_max\" id=\"hum_max\">"
"<label for=\"offset_hum\">Offset:</label><input type=\"number\" name=\"offset_hum\" id=\"offset_hum\">"
"</div>"
"<div>"
"<label for=\"pres_min\">Pres Min:</label><input type=\"number\" name=\"pres_min\" id=\"pres_min\">"
"<label for=\"pres_max\">Max:</label><input type=\"number\" name=\"pres_max\" id=\"pres_max\">"
"<label for=\"offset_pres\">Offset:</label><input type=\"number\" name=\"offset_pres\" id=\"offset_pres\">"
"</div>"
"<button type=\"submit\">Salvar</button>"
"</form>"
"<script>"
"const tempSpan = document.getElementById('temp');"
"const humSpan = document.getElementById('hum');"
"const presSpan = document.getElementById('pres');"
"const tempMin = document.getElementById('temp_min');"
"const tempMax = document.getElementById('temp_max');"
"const offsetTemp = document.getElementById('offset_temp');"
"const humMin = document.getElementById('hum_min');"
"const humMax = document.getElementById('hum_max');"
"const offsetHum = document.getElementById('offset_hum');"
"const presMin = document.getElementById('pres_min');"
"const presMax = document.getElementById('pres_max');"
"const offsetPres = document.getElementById('offset_pres');"
"const maxPoints = 300;"
"const makeChart = (id, label, color) => new Chart(document.getElementById(id), {"
"type: 'line',"
"data: {"
"labels: [],"
"datasets: [{"
"label,"
"data: [],"
"borderColor: color,"
"tension: 0.2,"
"pointRadius: 0"
"}]"
"},"
"options: {"
"animation: false,"
"responsive: true,"
"scales: {"
"x: { display: false },"
"y: { beginAtZero: false }"
"},"
"plugins: {"
"legend: { display: false },"
"title: { display: true, text: label }"
"}"
"}"
"});"
"const chartTemp = makeChart('chartTemp', 'Temperatura (°C)', 'red');"
"const chartHum  = makeChart('chartHum',  'Umidade (%%)', 'blue');"
"const chartPres = makeChart('chartPres', 'Pressão (kPa)', 'green');"
"function updateCharts(temp, hum, pres) {"
"const time = new Date().toLocaleTimeString();"
"const update = (chart, value) => {"
"chart.data.labels.push(time);"
"chart.data.datasets[0].data.push(value);"
"if (chart.data.labels.length > maxPoints) {"
"chart.data.labels.shift();"
"chart.data.datasets[0].data.shift();"
"}"
"chart.update();"
"};"
"update(chartTemp, temp);"
"update(chartHum, hum);"
"update(chartPres, pres);"
"}"
"function fetchData() {"
"fetch('/data')"
".then(res => res.json())"
".then(data => {"
"tempSpan.textContent = data.temp;"
"humSpan.textContent = data.hum;"
"presSpan.textContent = data.pres;"
"updateCharts(data.temp, data.hum, data.pres);"
"})"
".catch(err => console.error('Erro ao buscar dados:', err));"
"}"
"setInterval(fetchData, 2000);"
"document.getElementById('configForm').addEventListener('submit', e => {"
"e.preventDefault();"
"const formData = new FormData(e.target);"
"const config = {};"
"formData.forEach((v, k) => config[k] = parseFloat(v));"
"fetch('/config', {"
"method: 'POST',"
"headers: {'Content-Type': 'application/json'},"
"body: JSON.stringify(config)"
"})"
".then(data => {"
"if (data.ok) {"
"fetch('/config')"
".then(res => res.json())"
".then(data => {"
"tempMin.value = data.min_temp;"
"tempMax.value = data.max_temp;"
"offsetTemp.value = data.offset_temp;"
"humMin.value = data.min_hum;"
"humMax.value = data.max_hum;"
"offsetHum.value = data.offset_hum;"
"presMin.value = data.min_pres;"
"presMax.value = data.max_pres;"
"offsetPres.value = data.offset_pres;"
"})} else {"
"alert('Erro ao salvar configurações!');"
"}"
"});"
"});"
"fetch('/config')"
".then(res => res.json())"
".then(data => {"
"tempMin.value = data.min_temp;"
"tempMax.value = data.max_temp;"
"offsetTemp.value = data.offset_temp;"
"humMin.value = data.min_hum;"
"humMax.value = data.max_hum;"
"offsetHum.value = data.offset_hum;"
"presMin.value = data.min_pres;"
"presMax.value = data.max_pres;"
"offsetPres.value = data.offset_pres;"
"})"
"</script>"
"</body>"
"</html>";

#include "JaamWeb.h"
#include "JaamLed.h"
#include "JaamLogs.h"
#include "JaamUtils.h"
#include <esp_system.h>
#include <ArduinoJson.h>

#ifdef __cplusplus
extern "C" {
#endif
// ESP32 temperature sensor function
uint8_t temprature_sens_read();
#ifdef __cplusplus
}
#endif

extern volatile bool needAdaptAnimationColors;
extern volatile bool needAdaptStripBrightness;
extern volatile bool needAdaptColors;
extern volatile bool needAdaptAnimationBrightness;
extern volatile bool needAdaptAnimationPeriod;
extern volatile bool needAdaptAnimationType;
extern volatile bool needReconnectWebsocket;
extern volatile bool needReconnectMainStrip;
extern volatile bool needReconnectBgStrip;
extern volatile bool needReconnectServiceStrip;
extern volatile bool needUpdateBatteryPin;
extern volatile bool needRecalculateLeds;
extern volatile bool needReconfigureDisplay;
extern volatile bool needUpdateAnimationsMode;
extern volatile bool needAdaptClimate;

extern RegionLedMapEntry                customMap[MAX_REGIONS];


void JaamWeb::setSettings(JaamSettings* settings) {
    this->settings = settings;
}

String JaamWeb::getMeta() {
    return R"HTML(
<meta charset='UTF-8'>
<meta name='viewport' content='width=device-width, initial-scale=1'>
<meta name='mobile-web-app-capable' content='yes' />
<meta name='application-name' content='JAAM' />
<meta name='msapplication-starturl' content='/' />
<meta name='apple-mobile-web-app-capable' content='yes' />
<meta name='apple-mobile-web-app-title' content='JAAM'>
<meta name='apple-mobile-web-app-status-bar-style' content='black-translucent' />
<link rel='shortcut icon' href='favicon.png'>
<link rel='apple-touch-icon' href='apple-touch-icon.png'>
)HTML";
}

String JaamWeb::getStyles() {
    return R"HTML(
<style>
/* Основні CSS змінні для темізації */
:root {
    --bg-color: #f0f0f0;
    --container-bg: #ffffff;
    --text-color: #000000;
    --border-color: #dee2e6;
    --panel-bg: #f8f9fa;
    --alerts-panel-bg: #fff5f5;
    --alerts-panel-border: #fed7d7;
    --input-bg: #ffffff;
    --input-border: #ddd;
    --warning-bg: #fff3cd;
    --warning-border: #ffeaa7;
    --warning-text: #856404;
    --secondary-text: #6c757d;
}

/* Темна тема */
[data-theme='dark'] {
    --bg-color: #1a1a1a;
    --container-bg: #2d2d2d;
    --text-color: #ffffff;
    --border-color: #444444;
    --panel-bg: #3a3a3a;
    --alerts-panel-bg: #4a2c2c;
    --alerts-panel-border: #8b4545;
    --input-bg: #3a3a3a;
    --input-border: #555555;
    --warning-bg: #5a4a2d;
    --warning-border: #8b7355;
    --warning-text: #d4b855;
    --secondary-text: #aaaaaa;
}

body {
    font-family: Arial, sans-serif;
    margin: 20px;
    background-color: var(--bg-color);
    color: var(--text-color);
    transition: background-color 0.3s ease, color 0.3s ease;
}

.container {
    max-width: 600px;
    margin: 0 auto;
    background-color: var(--container-bg);
    padding: 20px;
    border-radius: 10px;
    box-shadow: 0 0 10px rgba(0,0,0,0.3);
    transition: background-color 0.3s ease;
}

.header-container {
    display: flex;
    justify-content: space-between;
    align-items: center;
    margin-bottom: 20px;
}

.header-buttons {
    display: flex;
    gap: 8px;
    align-items: center;
}

h1 {
    margin: 0;
    font-size: 1.5em;
}

/* Кнопки управління */
.control-button {
    background: none;
    border: 1px solid var(--border-color);
    cursor: pointer;
    padding: 8px;
    border-radius: 8px;
    transition: background-color 0.3s ease, border-color 0.3s ease;
    display: flex;
    align-items: center;
    justify-content: center;
}

.control-button:hover {
    background-color: var(--panel-bg);
}

.control-button.active {
    background-color: #007bff;
    border-color: #007bff;
}

.control-button svg {
    width: 18px;
    height: 18px;
    fill: var(--text-color);
    transition: fill 0.3s ease;
}

.control-button.active svg {
    fill: white;
}

/* Перемикач теми - залишаємо для зворотної сумісності */
.theme-toggle {
    background: none;
    border: 1px solid var(--border-color);
    cursor: pointer;
    padding: 8px;
    border-radius: 8px;
    transition: background-color 0.3s ease, border-color 0.3s ease;
}

.theme-toggle:hover {
    background-color: var(--panel-bg);
}

.theme-toggle svg {
    width: 20px;
    height: 20px;
    fill: var(--text-color);
    transition: fill 0.3s ease;
}

.system-panel {
    background: var(--panel-bg);
    border: 1px solid var(--border-color);
    border-radius: 8px;
    padding: 15px;
    margin-bottom: 20px;
    display: flex;
    justify-content: space-between;
    align-items: center;
    flex-wrap: wrap;
    transition: background-color 0.3s ease, border-color 0.3s ease;
}

.alerts-panel {
    background: var(--alerts-panel-bg);
    border: 1px solid var(--alerts-panel-border);
    border-radius: 8px;
    padding: 15px;
    margin-bottom: 20px;
    display: block;
    justify-content: flex-start;
    align-items: center;
    flex-wrap: wrap;
    transition: background-color 0.3s ease, border-color 0.3s ease;
}

.alert-metric {
    display: flex;
    align-items: center;
    margin: 5px 0;
    min-width: 120px;
}

.alerts-loading {
    color: var(--secondary-text);
    font-size: 12px;
}

.alerts-no-alerts {
    color: #28a745;
    font-weight: bold;
    font-size: 12px;
}

.alerts-error {
    color: #dc3545;
    font-size: 12px;
}

.system-metric {
    display: flex;
    align-items: center;
    margin: 5px 0;
    min-width: 120px;
}

.metric-icon {
    width: 16px;
    height: 16px;
    margin-right: 8px;
    fill: currentColor;
}

.metric-label {
    font-size: 12px;
    color: var(--secondary-text);
    margin-right: 5px;
}

.metric-value {
    font-weight: bold;
    font-size: 14px;
    color: var(--text-color);
}

.memory-bar {
    width: 100px;
    height: 8px;
    background: var(--border-color);
    border-radius: 4px;
    overflow: hidden;
    margin-left: 8px;
}

.memory-fill {
    height: 100%;
    transition: width 0.3s ease, background-color 0.3s ease;
}

.slider-container {
    margin: 20px 0;
}

.slider {
    width: 100%;
    height: 25px;
    background: var(--border-color);
    outline: none;
    opacity: 0.7;
    transition: opacity .2s;
}

.slider:hover {
    opacity: 1;
}

.value {
    font-size: 18px;
    margin-right: 10px;
    width: 60px;
    display: inline-block;
    text-align: right;
    color: var(--text-color);
}

.color-picker-container {
    margin: 20px 0;
}

.text-input-container {
    margin: 20px 0;
}

.text-input {
    width: 100%;
    padding: 8px;
    font-size: 16px;
    border: 1px solid var(--input-border);
    border-radius: 4px;
    background-color: var(--input-bg);
    color: var(--text-color);
    box-sizing: border-box;
    transition: background-color 0.3s ease, border-color 0.3s ease, color 0.3s ease;
}

.text-input-container label {
    display: block;
    margin-bottom: 5px;
    font-weight: bold;
    color: var(--text-color);
}

.form-group {
    margin: 20px 0;
}

.form-control {
    width: 100%;
    padding: 8px;
    font-size: 16px;
    border: 1px solid var(--input-border);
    border-radius: 4px;
    background-color: var(--input-bg);
    color: var(--text-color);
    transition: background-color 0.3s ease, border-color 0.3s ease, color 0.3s ease;
}

.form-group label {
    display: block;
    margin-bottom: 5px;
    font-weight: bold;
    color: var(--text-color);
}

.label {
    display: block;
    margin-bottom: 5px;
    font-weight: bold;
    color: var(--text-color);
}

.warning {
    background-color: var(--warning-bg);
    border: 1px solid var(--warning-border);
    color: var(--warning-text);
    padding: 10px;
    border-radius: 4px;
    margin: 10px 0;
    font-size: 14px;
    transition: background-color 0.3s ease, border-color 0.3s ease, color 0.3s ease;
}

.switch-container {
    margin: 20px 0;
    display: flex;
    align-items: center;
}

.switch-container label {
    margin-left: 8px;
    color: var(--text-color);
}

.switch {
    appearance: none;
    width: 50px;
    height: 24px;
    background: var(--border-color);
    border-radius: 12px;
    position: relative;
    cursor: pointer;
    transition: background-color 0.3s ease;
}

.switch:checked {
    background: #007bff;
}

.switch::before {
    content: '';
    position: absolute;
    width: 20px;
    height: 20px;
    border-radius: 50%;
    background: white;
    top: 2px;
    left: 2px;
    transition: transform 0.3s ease;
}

.switch:checked::before {
    transform: translateX(26px);
}

.alerts-details-panel {
    background: var(--alerts-panel-bg);
    border: 1px solid var(--alerts-panel-border);
    border-radius: 8px;
    padding: 15px;
    margin-bottom: 20px;
    display: flex;
    flex-direction: column;
    gap: 0;
    transition: background-color 0.3s ease, border-color 0.3s ease;
}

.alert-detail-row {
    display: flex;
    align-items: center;
    margin: 0 0;
    min-width: 120px;
}

.alert-detail-region {
    font-size: 12px;
    color: var(--secondary-text);
    margin-right: 5px;
    font-weight: bold;
    min-width: 60px;
}

.alert-detail-icon {
    width: 16px;
    height: 16px;
    fill: currentColor;
    margin-right: 8px;
}

/* Navigation menu styles */
.nav-menu {
    background: var(--panel-bg);
    border: 1px solid var(--border-color);
    border-radius: 8px;
    padding: 15px;
    margin-bottom: 20px;
    display: flex;
    flex-wrap: wrap;
    gap: 8px;
    transition: background-color 0.3s ease, border-color 0.3s ease;
}

.nav-item {
    padding: 8px 16px;
    border: 1px solid var(--border-color);
    border-radius: 6px;
    background: var(--container-bg);
    color: var(--text-color);
    cursor: pointer;
    transition: all 0.3s ease;
    font-size: 14px;
    white-space: nowrap;
    user-select: none;
    text-decoration: none;
    display: inline-block;
}

.nav-item:hover {
    background: var(--panel-bg);
    border-color: var(--text-color);
}

.nav-item.active {
    background: #007bff;
    color: white;
    border-color: #007bff;
}

.nav-item.active:hover {
    background: #0056b3;
    border-color: #0056b3;
}

/* Section container styles */
.section-container {
    display: none;
}

.section-container.active {
    display: block;
}

.section-header {
    font-size: 18px;
    font-weight: bold;
    color: var(--text-color);
    margin-bottom: 15px;
    padding-bottom: 8px;
    border-bottom: 2px solid var(--border-color);
}

/* Info panel styles */
.info-panel {
    border-radius: 8px;
    padding: 5px;
    margin: 5px 0;
    display: flex;
    align-items: center;
    transition: background-color 0.3s ease, border-color 0.3s ease;
    border: 1px solid;
}

.info-metric {
    display: flex;
    align-items: center;
    width: 100%;
}

.info-icon {
    width: 18px;
    height: 18px;
    margin-right: 5px;
    flex-shrink: 0;
    fill: currentColor;
}

.info-text {
    font-size: 12px;
    font-weight: 500;
    line-height: 1.4;
}
</style>
)HTML";
}

String JaamWeb::getScripts() {
    // All site JavaScript: theme, system panel, alerts, and dynamic UI rendering
    String html = R"JS(
<script>
// Theme helpers
function detectSystemTheme() {
    return (window.matchMedia && window.matchMedia('(prefers-color-scheme: dark)').matches) ? 'dark' : 'light';
}

function getThemeFromCookie() {
    const cookies = document.cookie.split(';');
    for (let cookie of cookies) {
        const [name, value] = cookie.trim().split('=');
        if (name === 'jaam_theme') return value;
    }
    return null;
}

function setThemeCookie(theme) {
    document.cookie = 'jaam_theme=' + theme + '; max-age=31536000; path=/';
}

function applyTheme(theme) {
    document.documentElement.setAttribute('data-theme', theme);
    setThemeCookie(theme);
}

function toggleTheme() {
    const currentTheme = document.documentElement.getAttribute('data-theme') || 'light';
    const newTheme = currentTheme === 'dark' ? 'light' : 'dark';
    applyTheme(newTheme);
}

function initTheme() {
    const savedTheme = getThemeFromCookie();
    const theme = savedTheme || detectSystemTheme();
    applyTheme(theme);
    if (!savedTheme && window.matchMedia) {
        const mediaQuery = window.matchMedia('(prefers-color-scheme: dark)');
        mediaQuery.addListener(function(e) {
            if (!getThemeFromCookie()) {
                applyTheme(e.matches ? 'dark' : 'light');
            }
        });
    }
}

document.addEventListener('DOMContentLoaded', initTheme);

// System UI helpers
function getMemoryColor(percent) {
    if (percent < 50) return '#28a745';
    if (percent < 75) return '#ffc107';
    return '#dc3545';
}

function formatDuration(seconds) {
    seconds = Math.max(0, Math.floor(seconds || 0));
    
    const days = Math.floor(seconds / 86400); // 86400 seconds in a day
    const remainingAfterDays = seconds % 86400;
    const h = Math.floor(remainingAfterDays / 3600);
    const m = Math.floor((remainingAfterDays % 3600) / 60);
    
    if (days > 0) {
        return days + 'д ' + h + 'г ' + m + 'хв';
    } else {
        return h + 'г ' + m + 'хв';
    }
}

function createMetricContainer() {
    const div = document.createElement('div');
    div.className = 'system-metric';
    return div;
}

function svgNode(svgStr) {
    const span = document.createElement('span');
    span.innerHTML = svgStr;
    const svg = span.firstElementChild;
    return svg ? svg : document.createTextNode('');
}

function addMetric(panel, iconSvg, labelText, valueEl) {
    const wrap = createMetricContainer();
    const icon = svgNode(iconSvg || '');
    if (icon.nodeType !== 3) {
        icon.classList.add('metric-icon');
    }
    
    const lab = document.createElement('span');
    lab.className = 'metric-label';
    lab.textContent = labelText + ':';
    
    const val = document.createElement('span');
    val.className = 'metric-value';
    if (valueEl instanceof HTMLElement) {
        val.appendChild(valueEl);
    } else {
        val.textContent = valueEl;
    }
    
    wrap.appendChild(icon);
    wrap.appendChild(lab);
    wrap.appendChild(val);
    panel.appendChild(wrap);
    return wrap;
}

function renderSystemPanelFromSchema(data) {
    const panel = document.getElementById('systemPanel');
    if (!panel) return;
    
    panel.innerHTML = '';
    const items = (data && data.system) ? data.system : [];
    
    for (const item of items) {
        const type = item[0];
        
        if (type === 'bar') {
            const label = item[2];
            const unit = item[3];
            const iconSvg = item[4];
            const used = item[5];
            const total = item[6];
            const percent = total > 0 ? Math.min(100, Math.max(0, (used / total) * 100)) : 0;
            
            const value = document.createElement('span');
            value.textContent = used + '/' + total + ' ' + unit;
            
            const metric = addMetric(panel, iconSvg, label, value);
            
            const bar = document.createElement('div');
            bar.className = 'memory-bar';
            const fill = document.createElement('div');
            fill.className = 'memory-fill';
            fill.style.width = percent + '%';
            fill.style.backgroundColor = getMemoryColor(percent);
            bar.appendChild(fill);
            metric.appendChild(bar);
            
        } else if (type === 'number') {
            const label = item[2];
            const unit = item[3];
            const iconSvg = item[4];
            const value = item[5];
            const text = (value !== null && value !== undefined) ? 
                        (value + (unit ? (' ' + unit) : '')) : '--';
            addMetric(panel, iconSvg, label, text);
            
        } else if (type === 'time') {
            const label = item[2];
            const iconSvg = item[3];
            const seconds = item[4];
            addMetric(panel, iconSvg, label, formatDuration(seconds));
        }
    }
}

function updateSystemInfo() {
    fetch('/system-info')
        .then(r => r.json())
        .then(data => {
            if (data && Array.isArray(data.system) && data.system.length) {
                renderSystemPanelFromSchema(data);
            } else {
                const panel = document.getElementById('systemPanel');
                if (panel) {
                    panel.innerHTML = '';
                }
            }
        })
        .catch(err => console.error('Error fetching system info:', err));
}

// Alerts panel
function updateAlertsInfo() {
    fetch('/alerts-info')
        .then(r => r.json())
        .then(data => {
            const alertsContent = document.getElementById('alertsContent');
            if (!alertsContent) return;
            
            if (!data.regions || data.regions.length === 0) {
                alertsContent.innerHTML = '<span class="alerts-no-alerts">Немає активних тривог</span>';
                const adp = document.getElementById('alertsDetailsPanel');
                if (adp) adp.innerHTML = '';
                return;
            }
            
            let activeRegions = data.regions.length;
            let totalAlerts = 0;
            data.regions.forEach(region => {
                Object.values(region.alerts).forEach(alert => {
                    if (alert) totalAlerts++;
                });
            });
            
            alertsContent.innerHTML = '<span class="alerts-error">' + 
                                    activeRegions + ' регіонів (' + totalAlerts + ' тривог)</span>';
            
            const alertsDetailsPanel = document.getElementById('alertsDetailsPanel');
            if (!alertsDetailsPanel) return;
            
            let detailsHtml = '';
            const alertTypes = [
                {key: 'air', label: 'Повітряна тривога', 
                 icon: '<svg class="alert-detail-icon" viewBox="0 0 24 24"><path d="M13,14H11V10H13M13,18H11V16H13M1,21H23L12,2L1,21Z"/></svg>'},
                {key: 'artillery', label: 'Артобстріл', 
                 icon: '<svg class="alert-detail-icon" viewBox="0 0 24 24"><path d="M13,14H11V10H13M13,18H11V16H13M1,21H23L12,2L1,21Z"/></svg>'},
                {key: 'urban', label: 'Вуличні бої', 
                 icon: '<svg class="alert-detail-icon" viewBox="0 0 24 24"><path d="M13,14H11V10H13M13,18H11V16H13M1,21H23L12,2L1,21Z"/></svg>'},
                {key: 'chemical', label: 'Хімічна', 
                 icon: '<svg class="alert-detail-icon" viewBox="0 0 24 24"><path d="M13,14H11V10H13M13,18H11V16H13M1,21H23L12,2L1,21Z"/></svg>'},
                {key: 'nuclear', label: 'Ядерна', 
                 icon: '<svg class="alert-detail-icon" viewBox="0 0 24 24"><path d="M13,14H11V10H13M13,18H11V16H13M1,21H23L12,2L1,21Z"/></svg>'},
                {key: 'drones', label: 'Дрони', 
                 icon: '<svg class="alert-detail-icon" viewBox="0 0 32 32"><circle cx="16" cy="16" r="16" fill="#921D14"/><path fill-rule="evenodd" clip-rule="evenodd" d="M16 29.998c7.73 0 14-6.267 14-13.998S23.73 2.002 16 2.002 2 8.269 2 16s6.269 13.998 14 13.998M4.625 16C4.7 14.7 7.5 14.3 10.5 14.3h1l9.4-6 .5-.2h2.9v15.8h-2.9l-.5-.2-9.4-6h-1c-3 0-5.8-.4-5.875-1.7" fill="#fff"/></svg>'},
                {key: 'missiles', label: 'Ракети', 
                 icon: '<svg class="alert-detail-icon" viewBox="0 0 24 24"><path d="M13,14H11V10H13M13,18H11V16H13M1,21H23L12,2L1,21Z"/></svg>'},
                {key: 'kab', label: 'КАБ', 
                 icon: '<svg class="alert-detail-icon" viewBox="0 0 24 24"><path d="M13,14H11V10H13M13,18H11V16H13M1,21H23L12,2L1,21Z"/></svg>'},
                {key: 'ballistic', label: 'Балістична', 
                 icon: '<svg class="alert-detail-icon" viewBox="0 0 24 24"><path d="M13,14H11V10H13M13,18H11V16H13M1,21H23L12,2L1,21Z"/></svg>'},
                {key: 'explosion', label: 'Вибухи', 
                 icon: '<svg class="alert-detail-icon" viewBox="0 0 24 24"><path d="M13,14H11V10H13M13,18H11V16H13M1,21H23L12,2L1,21Z"/></svg>'}
            ];
            
            data.regions.forEach(region => {
                let icons = '';
                alertTypes.forEach(type => {
                    if (region.alerts && region.alerts[type.key]) {
                        icons += '<span title="' + type.label + '">' + type.icon + '</span>';
                    }
                });
                detailsHtml += '<div class="alert-detail-row">' +
                             '<span class="alert-detail-region">' + region.regionName + '</span>' +
                             icons + '</div>';
            });
            
            alertsDetailsPanel.innerHTML = detailsHtml;
        })
        .catch(err => {
            console.error('Error fetching alerts info:', err);
            const ac = document.getElementById('alertsContent');
            if (ac) ac.innerHTML = '<span class="alerts-error">Помилка завантаження</span>';
            const adp = document.getElementById('alertsDetailsPanel');
            if (adp) adp.innerHTML = '';
        });
}

// Settings update helpers
function updateParameter(name, value) {
    const valueElement = document.getElementById(name + 'Value');
    if (valueElement) {
        valueElement.textContent = '[' + value + ']';
    }
    
    fetch('/parameter', {
        method: 'POST',
        headers: {'Content-Type': 'application/x-www-form-urlencoded'},
        body: 'name=' + encodeURIComponent(name) + '&value=' + encodeURIComponent(value)
    })
        .then(r => {
            if (!r.ok) {
                console.error('Error updating parameter:', name, value);
            }
        })
        .catch(e => console.error('Network error:', e));
}

function updateSliderValue(name, value) {
    const valueElement = document.getElementById(name + 'Value');
    if (valueElement) {
        valueElement.textContent = '[' + value + ']';
    }
}

function updateBoolParameter(name, checked) {
    fetch('/parameter', {
        method: 'POST',
        headers: {'Content-Type': 'application/x-www-form-urlencoded'},
        body: 'name=' + encodeURIComponent(name) + '&value=' + (checked ? 1 : 0)
    });
}

function updateColor(name, value) {
    fetch('/color', {
        method: 'POST',
        headers: {'Content-Type': 'application/x-www-form-urlencoded'},
        body: 'name=' + encodeURIComponent(name) + '&value=' + encodeURIComponent(value)
    });
}

function updateTextParameter(name, value) {
    fetch('/text', {
        method: 'POST',
        headers: {'Content-Type': 'application/x-www-form-urlencoded'},
        body: 'name=' + encodeURIComponent(name) + '&value=' + encodeURIComponent(value)
    })
        .then(r => {
            if (!r.ok) {
                console.error('Error updating text parameter:', name, value);
            }
        })
        .catch(e => console.error('Network error:', e));
}

// Dynamic UI rendering (from /ui-schema)
async function fetchSchema() {
    const res = await fetch('/ui-schema');
    return await res.json();
}

function optionEl(id, name, sub) {
    const opt = document.createElement('option');
    opt.value = id;
    opt.textContent = sub ? ('-- ' + name) : name;
    return opt;
}

function labelEl(text) {
    const label = document.createElement('label');
    label.className = 'label';
    label.textContent = text;
    return label;
}

function groupDiv() {
    const div = document.createElement('div');
    div.className = 'form-group';
    return div;
}

function renderControl(ctrl, lists) {
    const type = ctrl[0];
    
    if (type === 'label') {
        return labelEl(ctrl[1]);
    }
    
    if (type === 'info') {
        const [_, text, color, icon, section] = ctrl;
        const div = document.createElement('div');
        div.className = 'info-panel';
        div.style.backgroundColor = color + '20'; // Add transparency
        div.style.borderColor = color;
        div.style.color = color;
        
        const metric = document.createElement('div');
        metric.className = 'info-metric';
        
        // Create SVG icon using the same method as system-metric
        const svgStr = `<svg class='info-icon' viewBox='0 0 24 24'><path d='${icon}'/></svg>`;
        const iconNode = svgNode(svgStr);
        if (iconNode.nodeType !== 3) {
            iconNode.classList.add('info-icon');
        }
        
        const textSpan = document.createElement('span');
        textSpan.className = 'info-text';
        textSpan.textContent = text;
        
        metric.appendChild(iconNode);
        metric.appendChild(textSpan);
        div.appendChild(metric);
        
        return div;
    }
    
    if (type === 'dropdown') {
        const [_, name, label, list, current] = ctrl;
        const div = groupDiv();
        
        const lab = document.createElement('label');
        lab.setAttribute('for', name);
        lab.textContent = label + ':';
        
        const sel = document.createElement('select');
        sel.className = 'form-control';
        sel.id = name;
        sel.name = name;
        
        const opts = lists[list] || [];
        for (const o of opts) {
            const el = optionEl(o[0], o[1], o[2]);
            if (String(o[0]) === String(current)) el.selected = true;
            sel.appendChild(el);
        }
        sel.onchange = (e) => updateParameter(name, e.target.value);
        
        div.appendChild(lab);
        div.appendChild(sel);
        return div;
    }
    
    if (type === 'bool') {
        const [_, name, label, current] = ctrl;
        const div = document.createElement('div');
        div.className = 'switch-container';
        
        const input = document.createElement('input');
        input.type = 'checkbox';
        input.id = name;
        input.className = 'switch';
        input.checked = !!current;
        input.onchange = (e) => updateBoolParameter(name, e.target.checked);
        
        const lab = document.createElement('label');
        lab.setAttribute('for', name);
        lab.textContent = label;
        
        div.appendChild(input);
        div.appendChild(lab);
        return div;
    }
    
    if (type === 'text') {
        const [_, name, label, current, placeholder] = ctrl;
        const div = document.createElement('div');
        div.className = 'text-input-container';
        
        const lab = document.createElement('label');
        lab.setAttribute('for', name);
        lab.textContent = label + ':';
        
        const inp = document.createElement('input');
        inp.type = 'text';
        inp.id = name;
        inp.value = current;
        inp.placeholder = placeholder || '';
        inp.className = 'text-input';
        inp.onchange = (e) => updateTextParameter(name, e.target.value);
        
        div.appendChild(lab);
        div.appendChild(inp);
        return div;
    }
    
    if (type === 'color') {
        const [_, name, label, current] = ctrl;
        const div = document.createElement('div');
        div.className = 'color-picker-container';
        
        const span = document.createElement('span');
        span.className = 'value';
        
        const inp = document.createElement('input');
        inp.type = 'color';
        inp.id = name;
        inp.value = current;
        inp.onchange = (e) => updateColor(name, e.target.value);
        span.appendChild(inp);
        
        const lab = document.createElement('label');
        lab.setAttribute('for', name);
        lab.textContent = label;
        
        div.appendChild(span);
        div.appendChild(lab);
        return div;
    }
    
    if (type === 'slider') {
        const [_, name, label, min, max, step, current] = ctrl;
        const div = document.createElement('div');
        div.className = 'slider-container';
        
        const val = document.createElement('span');
        val.className = 'value';
        val.id = name + 'Value';
        val.textContent = '[' + current + ']';
        
        const lab = document.createElement('label');
        lab.setAttribute('for', name);
        lab.textContent = label + ':';
        
        const rng = document.createElement('input');
        rng.type = 'range';
        rng.min = min;
        rng.max = max;
        rng.step = step;
        rng.value = current;
        rng.className = 'slider';
        rng.id = name;
        rng.oninput = (e) => updateSliderValue(name, e.target.value);
        rng.onchange = (e) => updateParameter(name, e.target.value);
        
        div.appendChild(val);
        div.appendChild(lab);
        div.appendChild(rng);
        return div;
    }
    
    return document.createTextNode('');
}

async function renderUI() {
    try {
        const schema = await fetchSchema();
        const lists = schema.dropdown_lists || {};
        const controls = schema.controls || [];
        const sections = schema.sections || [];
        const root = document.getElementById('uiControls');
        
        if (!root) return;
        
        // Create navigation menu
        const navMenu = createNavigationMenu(sections);
        
        // Create section containers
        const sectionContainers = createSectionContainers(sections);
        
        // Set up the UI structure
        root.innerHTML = navMenu + sectionContainers;
        
        // Group controls by section
        const controlsBySection = {};
        for (const ctrl of controls) {
            const type = ctrl[0];
            let section = 'general'; // Default section
            
            // Extract section based on control type and position
            if (type === 'dropdown' || type === 'bool' || type === 'text' || type === 'color' || type === 'slider') {
                section = ctrl[ctrl.length - 1] || 'general';
            } else if (type === 'label') {
                section = ctrl[2] || 'general';
            } else if (type === 'info') {
                section = ctrl[4] || 'general';
            }
            
            if (!controlsBySection[section]) {
                controlsBySection[section] = [];
            }
            controlsBySection[section].push(ctrl);
        }
        
        // Render controls into their respective sections
        for (const section of sections) {
            const sectionContent = document.getElementById('content-' + section.id);
            if (sectionContent && controlsBySection[section.id]) {
                for (const ctrl of controlsBySection[section.id]) {
                    sectionContent.appendChild(renderControl(ctrl, lists));
                }
            }
        }
        
        // Load and apply saved section
        const savedSection = loadSavedSection(sections);
        switchSection(savedSection);
        
    } catch (e) {
        console.error('UI render error', e);
    }
}

// Section navigation functionality
let currentSection = '';

function switchSection(sectionId) {
    // Hide all sections
    const allSections = document.querySelectorAll('.section-container');
    allSections.forEach(section => section.classList.remove('active'));
    
    // Hide all nav items active state
    const allNavItems = document.querySelectorAll('.nav-item');
    allNavItems.forEach(item => item.classList.remove('active'));
    
    // Show selected section
    const targetSection = document.getElementById('section-' + sectionId);
    if (targetSection) {
        targetSection.classList.add('active');
    }
    
    // Activate corresponding nav item
    const targetNavItem = document.querySelector(`[data-section="${sectionId}"]`);
    if (targetNavItem) {
        targetNavItem.classList.add('active');
    }
    
    currentSection = sectionId;
    
    // Save current section to localStorage
    try {
        localStorage.setItem('jaam-current-section', sectionId);
    } catch (e) {
        console.warn('Unable to save section to localStorage', e);
    }
}

// Panel visibility functionality
let systemPanelVisible = true;
let alertsPanelVisible = true;

function toggleSystemPanel() {
    const panel = document.getElementById('systemPanel');
    const button = document.getElementById('systemPanelToggle');
    
    if (!panel || !button) return;
    
    systemPanelVisible = !systemPanelVisible;
    
    if (systemPanelVisible) {
        panel.style.display = 'flex';
        button.classList.add('active');
    } else {
        panel.style.display = 'none';
        button.classList.remove('active');
    }
    
    // Save state to localStorage
    try {
        localStorage.setItem('jaam-system-panel-visible', systemPanelVisible);
    } catch (e) {
        console.warn('Unable to save system panel state to localStorage', e);
    }
}

function toggleAlertsPanel() {
    const panel = document.getElementById('alertsPanel');
    const button = document.getElementById('alertsPanelToggle');
    
    if (!panel || !button) return;
    
    alertsPanelVisible = !alertsPanelVisible;
    
    if (alertsPanelVisible) {
        panel.style.display = 'block';
        button.classList.add('active');
    } else {
        panel.style.display = 'none';
        button.classList.remove('active');
    }
    
    // Save state to localStorage
    try {
        localStorage.setItem('jaam-alerts-panel-visible', alertsPanelVisible);
    } catch (e) {
        console.warn('Unable to save alerts panel state to localStorage', e);
    }
}

function loadPanelStates() {
    try {
        // Load system panel state
        const systemState = localStorage.getItem('jaam-system-panel-visible');
        if (systemState !== null) {
            systemPanelVisible = systemState === 'true';
        }
        
        // Load alerts panel state
        const alertsState = localStorage.getItem('jaam-alerts-panel-visible');
        if (alertsState !== null) {
            alertsPanelVisible = alertsState === 'true';
        }
    } catch (e) {
        console.warn('Unable to load panel states from localStorage', e);
    }
}

function applyPanelStates() {
    const systemPanel = document.getElementById('systemPanel');
    const systemButton = document.getElementById('systemPanelToggle');
    const alertsPanel = document.getElementById('alertsPanel');
    const alertsButton = document.getElementById('alertsPanelToggle');
    
    // Apply system panel state
    if (systemPanel && systemButton) {
        if (systemPanelVisible) {
            systemPanel.style.display = 'flex';
            systemButton.classList.add('active');
        } else {
            systemPanel.style.display = 'none';
            systemButton.classList.remove('active');
        }
    }
    
    // Apply alerts panel state
    if (alertsPanel && alertsButton) {
        if (alertsPanelVisible) {
            alertsPanel.style.display = 'block';
            alertsButton.classList.add('active');
        } else {
            alertsPanel.style.display = 'none';
            alertsButton.classList.remove('active');
        }
    }
}

function createNavigationMenu(sections) {
    const navHtml = sections.map(section => 
        `<div class="nav-item" data-section="${section.id}" onclick="switchSection('${section.id}')" style="border-left: 3px solid ${section.color}">
            ${section.name}
        </div>`
    ).join('');
    
    return `<div class="nav-menu">${navHtml}</div>`;
}

function createSectionContainers(sections) {
    return sections.map(section => 
        `<div class="section-container" id="section-${section.id}">
            <div class="section-header" style="border-color: ${section.color}">${section.name}</div>
            <div class="section-content" id="content-${section.id}"></div>
        </div>`
    ).join('');
}

function loadSavedSection(sections) {
    try {
        const savedSection = localStorage.getItem('jaam-current-section');
        if (savedSection && sections.find(s => s.id === savedSection)) {
            return savedSection;
        }
    } catch (e) {
        console.warn('Unable to load section from localStorage', e);
    }
    return sections[0]?.id || 'general';
}

// Startup tasks
document.addEventListener('DOMContentLoaded', () => {
    loadPanelStates();
    renderUI();
    applyPanelStates();
    updateSystemInfo();
    updateAlertsInfo();
    setInterval(updateSystemInfo, 5000);
    setInterval(updateAlertsInfo, 10000);
});
</script>
)JS";
    return html;
}


void JaamWeb::handleColorParameter() {
    if (server.hasArg("name") && server.hasArg("value")) {
        setCrossOrigin();
        String name = server.arg("name");
        String value = server.arg("value");
        
        // Use c_str() directly to avoid string copying
        const char* namePtr = name.c_str();
        const char* valuePtr = value.c_str();

        if (name == "color_alert") {
            settings->saveString(COLOR_ALERT, valuePtr);
            LOG.printf("[WEB] Setting color_alert: raw=%s\n", valuePtr);
        }
        if (name == "color_clear") {
            settings->saveString(COLOR_CLEAR, valuePtr);
            LOG.printf("[WEB] Setting color_clear: raw=%s\n", valuePtr);
        }
        if (name == "color_explosion") {
            settings->saveString(COLOR_EXPLOSION, valuePtr);
            LOG.printf("[WEB] Setting color_explosion: raw=%s\n", valuePtr);
        }
        if (name == "color_missiles") {
            settings->saveString(COLOR_MISSILES, valuePtr);
            LOG.printf("[WEB] Setting color_color_missiles: raw=%s\n", valuePtr);
        }
        if (name == "color_drones") {
            settings->saveString(COLOR_DRONES, valuePtr);
            LOG.printf("[WEB] Setting color_drones: raw=%s\n", valuePtr);
        }
        if (name == "color_recon_drones") {
            settings->saveString(COLOR_RECON_DRONES, valuePtr);
            LOG.printf("[WEB] Setting color_recon_drones: raw=%s\n", valuePtr);
        }
        if (name == "color_kab") {
            settings->saveString(COLOR_KABS, valuePtr);
            LOG.printf("[WEB] Setting color_kab: raw=%s\n", valuePtr);
        }
        if (name == "color_ballistic") {
            settings->saveString(COLOR_BALLISTIC, valuePtr);
            LOG.printf("[WEB] Setting color_ballistic: raw=%s\n", valuePtr);
        }
        if (name == "color_home") {
            settings->saveString(COLOR_HOME_DISTRICT, valuePtr);
            LOG.printf("[WEB] Setting color_home: raw=%s\n", valuePtr);
        }
        if (name == "color_bg") {
            settings->saveString(COLOR_BG, valuePtr);
            LOG.printf("[WEB] Setting color_bg: raw=%s\n", valuePtr);
        }
        needAdaptColors = true;
        needAdaptAnimationColors = true;

        server.send(200, "text/plain", "OK");
    } else {
        server.send(400, "text/plain", "Missing parameters");
    }
}

void JaamWeb::handleParameter() {
    if (server.hasArg("name") && server.hasArg("value")) {
        setCrossOrigin();
        String name = server.arg("name");
        String value = server.arg("value");
        
        const char* namePtr = name.c_str();
        const char* valuePtr = value.c_str();
        int intValue = value.toInt();
        float floatValue = value.toFloat();

        if (name == "legacy") {
            settings->saveInt(LEGACY, intValue);
            LOG.printf("[WEB] Setting legacy: %d\n", intValue);
            needRecalculateLeds = true;
            needReconfigureDisplay = true;
        } else if (name == "district_mode_kyiv") {
            settings->saveInt(DISTRICT_MODE_KYIV, intValue);
            LOG.printf("[WEB] Setting district_mode_kyiv: %d\n", intValue);
            needRecalculateLeds = true;
        } else if (name == "district_mode_kharkiv") {
            settings->saveInt(DISTRICT_MODE_KHARKIV, intValue);
            LOG.printf("[WEB] Setting district_mode_kharkiv: %d\n", intValue);
            needRecalculateLeds = true;
        } else if (name == "district_mode_zp") {
            settings->saveInt(DISTRICT_MODE_ZP, intValue);
            LOG.printf("[WEB] Setting district_mode_zp: %d\n", intValue);
            needRecalculateLeds = true;
        } else if (name == "home_district") {
            settings->saveInt(HOME_DISTRICT, intValue);
            LOG.printf("[WEB] Setting home_district: %d\n", intValue);
            needAdaptColors = true;
            needAdaptAnimationColors = true;
        } else if (name == "bg_led_mode") {
            settings->saveInt(BG_LED_MODE, intValue);
            LOG.printf("[WEB] Setting bg_led_mode: %d\n", intValue);
            needAdaptColors = true;
            needAdaptAnimationColors = true;
        } else if (name == "map_mode") {
            settings->saveInt(MAP_MODE, intValue);
            LOG.printf("[WEB] Setting map_mode: %d\n", intValue);
            needAdaptColors = true;
        } else if (name == "brightness") {
            settings->saveInt(BRIGHTNESS, intValue);
            LOG.printf("[WEB] Setting brightness: %d\n", intValue);
            needAdaptStripBrightness = true;
        } else if (name == "brightness_day") {
            settings->saveInt(BRIGHTNESS_DAY, intValue);
            LOG.printf("[WEB] Setting brightness_day: %d\n", intValue);
            needAdaptStripBrightness = true;
            // needAdaptColors = true;
            // needAdaptAnimationColors = true;
        } else if (name == "brightness_night") {
            settings->saveInt(BRIGHTNESS_NIGHT, intValue);
            LOG.printf("[WEB] Setting brightness_night: %d\n", intValue);
            needAdaptStripBrightness = true;
            // needAdaptColors = true;
            // needAdaptAnimationColors = true;
        } else if (name == "brightness_alert") {
            settings->saveInt(BRIGHTNESS_ALERT, intValue);
            LOG.printf("[WEB] Setting brightness_alert: %d\n", intValue);
            needAdaptColors = true;
            needAdaptAnimationBrightness = true;
        } else if (name == "brightness_clear") {
            settings->saveInt(BRIGHTNESS_CLEAR, intValue);
            LOG.printf("[WEB] Setting brightness_clear: %d\n", intValue);
            needAdaptColors = true;
            needAdaptAnimationBrightness = true;
        } else if (name == "brightness_explosion") {
            settings->saveInt(BRIGHTNESS_EXPLOSION, intValue);
            LOG.printf("[WEB] Setting brightness_explosion: %d\n", intValue);
            needAdaptColors = true;
            needAdaptAnimationBrightness = true;
        } else if (name == "brightness_missiles") {
            settings->saveInt(BRIGHTNESS_MISSILES, intValue);
            LOG.printf("[WEB] Setting brightness_missiles: %d\n", intValue);
            needAdaptColors = true;
            needAdaptAnimationBrightness = true;
        } else if (name == "brightness_drones") {
            settings->saveInt(BRIGHTNESS_DRONES, intValue);
            LOG.printf("[WEB] Setting brightness_drones: %d\n", intValue);
            needAdaptColors = true;
            needAdaptAnimationBrightness = true;
        } else if (name == "brightness_recon_drones") {
            settings->saveInt(BRIGHTNESS_RECON_DRONES, intValue);
            LOG.printf("[WEB] Setting brightness_recon_drones: %d\n", intValue  );
            needAdaptColors = true;
            needAdaptAnimationBrightness = true;
        } else if (name == "brightness_kabs") {
            settings->saveInt(BRIGHTNESS_KABS, intValue);
            LOG.printf("[WEB] Setting brightness_kabs: %d\n", intValue);
            needAdaptColors = true;
            needAdaptAnimationBrightness = true;
        } else if (name == "brightness_ballistic") {
            settings->saveInt(BRIGHTNESS_BALLISTIC, intValue);
            LOG.printf("[WEB] Setting brightness_ballistic: %d\n", intValue);
            needAdaptColors = true;
            needAdaptAnimationBrightness = true;
        } else if (name == "brightness_home_district") {
            settings->saveInt(BRIGHTNESS_HOME_DISTRICT, intValue);
            LOG.printf("[WEB] Setting brightness_home_district: %d\n", intValue);
            needAdaptColors = true; 
            needAdaptAnimationBrightness = true;
        } else if (name == "brightness_bg") {
            settings->saveInt(BRIGHTNESS_BG, intValue);
            LOG.printf("[WEB] Setting brightness_bg: %d\n", intValue);
            needAdaptColors = true; 
            needAdaptAnimationBrightness = true;
        } else if (name == "brightness_service") {
            settings->saveInt(BRIGHTNESS_SERVICE, intValue);
            LOG.printf("[WEB] Setting brightness_service: %d\n", intValue);
            needAdaptColors = true; 
        } else if (name == "brightness_animation_end") {
            settings->saveInt(BRIGHTNESS_ANIMATION_END, intValue);
            LOG.printf("[WEB] Setting brightness_animation_end: %d\n", intValue);
            needAdaptAnimationBrightness = true;
        } else if (name == "main_led_color_format") {
            settings->saveInt(MAIN_LED_COLOR_FORMAT, intValue);
            LOG.printf("[WEB] Setting main_led_color_format: %d\n", intValue);
            needReconnectMainStrip = true;
        } else if (name == "main_led_frequency") {
            settings->saveInt(MAIN_LED_FREQUENCY, intValue);
            LOG.printf("[WEB] Setting main_led_frequency: %d\n", intValue);
            needReconnectMainStrip = true;
        } else if (name == "bg_led_color_format") {
            settings->saveInt(BG_LED_COLOR_FORMAT, intValue);
            LOG.printf("[WEB] Setting bg_led_color_format: %d\n", intValue);
            needReconnectBgStrip = true;
        } else if (name == "bg_led_frequency") {
            settings->saveInt(BG_LED_FREQUENCY, intValue);
            LOG.printf("[WEB] Setting bg_led_frequency: %d\n", intValue);
            needReconnectBgStrip = true;
        } else if (name == "service_led_color_format") {
            settings->saveInt(SERVICE_LED_COLOR_FORMAT, intValue);
            LOG.printf("[WEB] Setting service_led_color_format: %d\n", intValue);
            needReconnectServiceStrip = true;
        } else if (name == "service_led_frequency") {
            settings->saveInt(SERVICE_LED_FREQUENCY, intValue);
            LOG.printf("[WEB] Setting service_led_frequency: %d\n", intValue);
            needReconnectServiceStrip = true;
        } else if (name == "display_model") {
            settings->saveInt(DISPLAY_MODEL, intValue);
            LOG.printf("[WEB] Setting display_model: %d\n", intValue);
            needReconfigureDisplay = true;
        } else if (name == "display_height") {
            settings->saveInt(DISPLAY_HEIGHT, intValue);
            LOG.printf("[WEB] Setting display_height: %d\n", intValue);
            needReconfigureDisplay = true;
        } else if (name == "display_rotation") {
            settings->saveInt(DISPLAY_ROTATION, intValue);
            LOG.printf("[WEB] Setting display_rotation: %d\n", intValue);
            needReconfigureDisplay = true;
        } else if (name == "invert_display") {
            bool boolValue = intValue != 0;
            settings->saveBool(INVERT_DISPLAY, boolValue);
            LOG.printf("[WEB] Setting invert_display: %d\n", boolValue);
            needReconfigureDisplay = true;
        } else if (name == "enable_kabs") {
            bool boolValue = intValue != 0;
            settings->saveBool(ENABLE_KABS, boolValue);
            LOG.printf("[WEB] Setting enable_kabs: %d\n", boolValue);
            needAdaptColors = true;
        } else if (name == "enable_missiles") {
            bool boolValue = intValue != 0;
            settings->saveBool(ENABLE_MISSILES, boolValue);
            LOG.printf("[WEB] Setting enable_missiles: %d\n", boolValue);
            needAdaptColors = true;
        } else if (name == "enable_drones") {
            bool boolValue = intValue != 0;
            settings->saveBool(ENABLE_DRONES, boolValue);
            LOG.printf("[WEB] Setting enable_drones: %d\n", boolValue);
            needAdaptColors = true;
        } else if (name == "enable_recon_drones") {
            bool boolValue = intValue != 0;
            settings->saveBool(ENABLE_RECON_DRONES, boolValue);
            LOG.printf("[WEB] Setting enable_recon_drones: %d\n", boolValue);
            needAdaptColors = true;
        } else if (name == "enable_ballistic") {
            bool boolValue = intValue != 0;
            settings->saveBool(ENABLE_BALLISTIC, boolValue);
            LOG.printf("[WEB] Setting enable_ballistic: %d\n", boolValue);
            needAdaptColors = true;
        } else if (name == "enable_explosions") {
            bool boolValue = intValue != 0;
            settings->saveBool(ENABLE_EXPLOSIONS, boolValue);
            LOG.printf("[WEB] Setting enable_explosions: %d\n", boolValue);
            needAdaptColors = true;
        } else if (name == "enable_sync_animations") {
            bool boolValue = intValue != 0;
            settings->saveBool(ENABLE_SYNC_ANIMATIONS, boolValue);
            LOG.printf("[WEB] Setting enable_sync_animations: %d\n", boolValue);
            needUpdateAnimationsMode = true;
        } else if (name == "brightness_mode") {
            settings->saveInt(BRIGHTNESS_MODE, intValue);
            LOG.printf("[WEB] Setting brightness_mode: %d\n", intValue);
               } else if (name == "day_start") {
            settings->saveInt(DAY_START, intValue);
            LOG.printf("[WEB] Setting day_start: %d\n", intValue);
        } else if (name == "night_start") {
            settings->saveInt(NIGHT_START, intValue);
            LOG.printf("[WEB] Setting night_start: %d\n", intValue);
        } else if (name == "alert_on_time") {
            settings->saveInt(ALERT_ON_TIME, intValue);
            LOG.printf("[WEB] Setting alert_on_time: %d\n", intValue);
        } else if (name == "alert_off_time") {
            settings->saveInt(ALERT_OFF_TIME, intValue);
            LOG.printf("[WEB] Setting alert_off_time: %d\n", intValue);
        } else if (name == "drone_time") {
            settings->saveInt(DRONE_TIME, intValue);
            LOG.printf("[WEB] Setting drone_time: %d\n", intValue);
        } else if (name == "recon_drone_time") {
            settings->saveInt(RECON_DRONE_TIME, intValue);
            LOG.printf("[WEB] Setting recon_drone_time: %d\n", intValue);
        } else if (name == "missile_time") {
            settings->saveInt(MISSILE_TIME, intValue);
            LOG.printf("[WEB] Setting missile_time: %d\n", intValue);
        } else if (name == "kab_time") {
            settings->saveInt(KAB_TIME, intValue);
            LOG.printf("[WEB] Setting kab_time: %d\n", intValue);
        } else if (name == "ballistic_time") {
            settings->saveInt(BALLISTIC_TIME, intValue);
            LOG.printf("[WEB] Setting ballistic_time: %d\n", intValue);
        } else if (name == "explosion_time") {
            settings->saveInt(EXPLOSION_TIME, intValue);
            LOG.printf("[WEB] Setting explosion_time: %d\n", intValue);
        } else if (name == "alert_on_cycle") {
            settings->saveInt(ANIMATION_ALERT_ON_CYCLE_TIME, intValue);
            LOG.printf("[WEB] Setting alert_on_cycle: %d\n", intValue);
            needAdaptAnimationPeriod = true;
        } else if (name == "alert_off_cycle") {
            settings->saveInt(ANIMATION_ALERT_OFF_CYCLE_TIME, intValue);
            LOG.printf("[WEB] Setting alert_off_cycle: %d\n", intValue);
            needAdaptAnimationPeriod = true;
        } else if (name == "drone_cycle") {
            settings->saveInt(ANIMATION_DRONE_CYCLE_TIME, intValue);
            LOG.printf("[WEB] Setting drone_cycle: %d\n", intValue);
            needAdaptAnimationPeriod = true;
        } else if (name == "recon_drone_cycle") {
            settings->saveInt(ANIMATION_RECON_DRONE_CYCLE_TIME, intValue);
            LOG.printf("[WEB] Setting recon_drone_cycle: %d\n", intValue);
            needAdaptAnimationPeriod = true;
        } else if (name == "missile_cycle") {
            settings->saveInt(ANIMATION_MISSILE_CYCLE_TIME, intValue);
            LOG.printf("[WEB] Setting missile_cycle: %d\n", intValue);
            needAdaptAnimationPeriod = true;
        } else if (name == "kab_cycle") {
            settings->saveInt(ANIMATION_KAB_CYCLE_TIME, intValue);
            LOG.printf("[WEB] Setting kab_cycle: %d\n", intValue);
            needAdaptAnimationPeriod = true;
        } else if (name == "ballistic_cycle") {
            settings->saveInt(ANIMATION_BALLISTIC_CYCLE_TIME, intValue);
            LOG.printf("[WEB] Setting ballistic_cycle: %d\n", intValue);
            needAdaptAnimationPeriod = true;
        } else if (name == "explosion_cycle") {
            settings->saveInt(ANIMATION_EXPLOSION_CYCLE_TIME, intValue);
            LOG.printf("[WEB] Setting explosion_cycle: %d\n", intValue);
            needAdaptAnimationPeriod = true;
        } else if (name == "alert_on_animation") {
            settings->saveInt(ANIMATION_ALERT_ON_TYPE, intValue);
            LOG.printf("[WEB] Setting alert_on_animation: %d\n", intValue);
            needAdaptAnimationType = true;
        } else if (name == "alert_off_animation") {
            settings->saveInt(ANIMATION_ALERT_OFF_TYPE, intValue);
            LOG.printf("[WEB] Setting alert_off_animation: %d\n", intValue);
            needAdaptAnimationType = true;
        } else if (name == "drone_animation") {
            settings->saveInt(ANIMATION_DRONE_TYPE, intValue);
            LOG.printf("[WEB] Setting drone_animation: %d\n", intValue);
            needAdaptAnimationType = true;
        } else if (name == "recon_drone_animation") {
            settings->saveInt(ANIMATION_RECON_DRONE_TYPE, intValue);
            LOG.printf("[WEB] Setting recon_drone_animation: %d\n", intValue);
            needAdaptAnimationType = true;
        } else if (name == "missile_animation") {
            settings->saveInt(ANIMATION_MISSILE_TYPE, intValue);
            LOG.printf("[WEB] Setting missile_animation: %d\n", intValue);
            needAdaptAnimationType = true;
        } else if (name == "kab_animation") {
            settings->saveInt(ANIMATION_KAB_TYPE, intValue);
            LOG.printf("[WEB] Setting kab_animation: %d\n", intValue);
            needAdaptAnimationType = true;
        } else if (name == "ballistic_animation") {
            settings->saveInt(ANIMATION_BALLISTIC_TYPE, intValue);
            LOG.printf("[WEB] Setting ballistic_animation: %d\n", intValue);
            needAdaptAnimationType = true;
        } else if (name == "explosion_animation") {
            settings->saveInt(ANIMATION_EXPLOSION_TYPE, intValue);
            LOG.printf("[WEB] Setting explosion_animation: %d\n", intValue);
            needAdaptAnimationType = true;
        } else if (name == "enable_battery") {
            bool boolValue = intValue != 0;
            settings->saveBool(ENABLE_BATTERY_MONITORING, boolValue);
            needUpdateBatteryPin = true;
            LOG.printf("[WEB] Set enable_battery: %d\n", intValue);
        } else if (name == "kyiv_led") {
            bool boolValue = intValue != 0;
            settings->saveBool(KYIV_LED, boolValue);
            needRecalculateLeds = true;
            LOG.printf("[WEB] Set enable_battery: %d\n", intValue);
        } else if (name == "weather_min_temp") {
            settings->saveInt(WEATHER_MIN_TEMP, intValue);
            needAdaptColors = true;
            LOG.printf("[WEB] Set weather_min_temp: %d\n", intValue);
        } else if (name == "weather_max_temp") {
            settings->saveInt(WEATHER_MAX_TEMP, intValue);
            needAdaptColors = true;
            LOG.printf("[WEB] Set weather_max_temp: %d\n", intValue);
        } else if (name == "temp_correction") {
            settings->saveFloat(TEMP_CORRECTION, floatValue);
            needAdaptClimate = true;
            LOG.printf("[WEB] Set temp_correction: %.2f\n", floatValue);
        } else if (name == "hum_correction") {
            settings->saveFloat(HUM_CORRECTION, floatValue);
            needAdaptClimate = true;
            LOG.printf("[WEB] Set hum_correction: %.2f\n", floatValue);
        } else if (name == "pressure_correction") {
            settings->saveFloat(PRESSURE_CORRECTION, floatValue);
            needAdaptClimate = true;
            LOG.printf("[WEB] Set pressure_correction: %.2f\n", floatValue);
        }

        server.send(200, "text/plain", "OK");
    } else {
        server.send(400, "text/plain", "Missing parameters");
    }
}

void JaamWeb::handleTextParameter() {
    if (server.hasArg("name") && server.hasArg("value")) {
        setCrossOrigin();
        String name = server.arg("name");
        String value = server.arg("value");
        
        // Use c_str() directly to avoid string copying
        const char* namePtr = name.c_str();
        const char* valuePtr = value.c_str();

        if (name == "device_name") {
            settings->saveString(DEVICE_NAME, valuePtr);
            LOG.printf("[WEB] Setting device_name: %s\n", valuePtr);
        } else if (name == "device_description") {
            settings->saveString(DEVICE_DESCRIPTION, valuePtr);
            LOG.printf("[WEB] Setting device_description: %s\n", valuePtr);
        } else if (name == "broadcast_name") {
            settings->saveString(BROADCAST_NAME, valuePtr);
            LOG.printf("[WEB] Setting broadcast_name: %s\n", valuePtr);
        } else if (name == "ws_server_host") {
            settings->saveString(WS_SERVER_HOST, valuePtr);
            needReconnectWebsocket = true;
            LOG.printf("[WEB] Setting ws_server_host: %s\n", valuePtr);
        } else if (name == "ws_server_port") {
            settings->saveInt(WS_SERVER_PORT, value.toInt());
            needReconnectWebsocket = true;
            LOG.printf("[WEB] Setting ws_server_port: %s\n", valuePtr);
        } else if (name == "ntp_host") {
            settings->saveString(NTP_HOST, valuePtr);
            LOG.printf("[WEB] Setting ntp_host: %s\n", valuePtr);
        } else if (name == "ha_mqtt_user") {
            settings->saveString(HA_MQTT_USER, valuePtr);
            LOG.printf("[WEB] Setting ha_mqtt_user: %s\n", valuePtr);
        } else if (name == "ha_mqtt_password") {
            settings->saveString(HA_MQTT_PASSWORD, valuePtr);
            LOG.printf("[WEB] Setting ha_mqtt_password: %s\n", valuePtr);
        } else if (name == "ha_broker_address") {
            settings->saveString(HA_BROKER_ADDRESS, valuePtr);
            LOG.printf("[WEB] Setting ha_broker_address: %s\n", valuePtr);
        } else if (name == "main_led_pin") {
            settings->saveInt(MAIN_LED_PIN, value.toInt());
            LOG.printf("[WEB] Setting main_led_pin: %s\n", valuePtr);
            needReconnectMainStrip = true;
        } else if (name == "bg_led_pin") {
            settings->saveInt(BG_LED_PIN, value.toInt());
            LOG.printf("[WEB] Setting bg_led_pin: %s\n", valuePtr);
            needReconnectBgStrip = true;
        } else if (name == "bg_led_count") {
            settings->saveInt(BG_LED_COUNT, value.toInt());
            LOG.printf("[WEB] Setting bg_led_count: %s\n", valuePtr);
            needReconnectBgStrip = true;
        } else if (name == "service_led_pin") {
            settings->saveInt(SERVICE_LED_PIN, value.toInt());
            LOG.printf("[WEB] Setting service_led_pin: %s\n", valuePtr);
            needReconnectServiceStrip = true;
        } else if (name == "battery_pin") {
            settings->saveInt(BATTERY_PIN, value.toInt());
            needUpdateBatteryPin = true;
            LOG.printf("[WEB] Set battery_pin: %d\n", valuePtr);
        }
        server.send(200, "text/plain", "OK");
    } else {
        server.send(400, "text/plain", "Missing parameters");
    }
}

void JaamWeb::setCrossOrigin() {
    server.sendHeader(F("Access-Control-Allow-Origin"), F("*"));
    server.sendHeader(F("Access-Control-Max-Age"), F("600"));
    server.sendHeader(F("Access-Control-Allow-Methods"), F("PUT,POST,GET,OPTIONS"));
    server.sendHeader(F("Access-Control-Allow-Headers"), F("Content-Type,Authorization"));
}

void JaamWeb::sendCrossOriginHeader(){
    LOG.printf("[WEB] sendCORSHeader");
    setCrossOrigin();
    server.send(204);
}

void JaamWeb::handleNotFound() {
    LOG.printf("[WEB] Not found: %s\n", server.uri().c_str());
    server.send(404, "text/plain", "Not found");
}

void JaamWeb::begin(Adafruit_NeoPixel* strip_main, Adafruit_NeoPixel* strip_bg, Adafruit_NeoPixel* strip_service) {
    this->strip_main = strip_main;
    this->strip_bg = strip_bg;
    this->strip_service = strip_service;


    // Налаштування веб-сервера
    //server.enableCORS();
    server.on("/", HTTP_GET, [this]() { this->handleUiPage(); });
    server.on("/", HTTP_OPTIONS, [this]() { this->sendCrossOriginHeader(); });
    server.on("/map-editor", HTTP_GET, [this]() { this->handleMapEditor(); });
    server.on("/save-map", HTTP_POST, [this]() { this->handleSaveMap(); });
    //server.on("/parameter", HTTP_GET, [this]() { this->handleParameter(); });
    server.on("/parameter", HTTP_POST, [this]() { this->handleParameter(); });
    server.on("/parameter", HTTP_OPTIONS, [this]() { this->sendCrossOriginHeader(); });
    //server.on("/color", HTTP_GET, [this]() { this->handleColorParameter(); });
    server.on("/color", HTTP_POST, [this]() { this->handleColorParameter(); });
    server.on("/color", HTTP_OPTIONS, [this]() { this->sendCrossOriginHeader(); });
    //server.on("/text", HTTP_GET, [this]() { this->handleTextParameter(); });
    server.on("/text", HTTP_POST, [this]() { this->handleTextParameter(); });
    server.on("/text", HTTP_OPTIONS, [this]() { this->sendCrossOriginHeader(); });
    server.on("/system-info", HTTP_GET, [this]() { this->handleSystemInfo(); });
    server.on("/system-info", HTTP_OPTIONS, [this]() { this->sendCrossOriginHeader(); });
    server.on("/alerts-info", HTTP_GET, [this]() { this->handleAlertsInfo(); });
    server.on("/alerts-info", HTTP_OPTIONS, [this]() { this->sendCrossOriginHeader(); });
    server.on("/ui-schema", HTTP_GET, [this]() { this->handleUiSchema(); });
    server.on("/ui-schema", HTTP_OPTIONS, [this]() { this->sendCrossOriginHeader(); });
    // Dynamic UI page that renders based on /ui-schema

    server.on("/favicon.png", HTTP_GET, [this]() { server.send(204); });
    server.onNotFound([this]() { this->handleNotFound(); });

    server.begin();
}

void JaamWeb::handleClient() {
    if (!wifiConnected) {
        return;
    }
    server.handleClient();
}

void JaamWeb::handleSystemInfo() {
    setCrossOrigin();
    String response = getSystemInfoJson();
    server.send(200, "application/json", response);
}

void JaamWeb::handleAlertsInfo() {
    setCrossOrigin();
    String response = getAlertsJson();
    server.send(200, "application/json", response);
}

void JaamWeb::handleUiPage() {
        // Serve a dynamic UI page that mirrors getHtmlTemplate design but renders from /ui-schema
        String html;
        html.reserve(12000);

        // Head start
        html += R"HTML(
<!DOCTYPE html>
<html>
<head>
<title>)HTML";
        String deviceName = settings->getString(DEVICE_NAME);
        if (deviceName.isEmpty()) deviceName = "JAAM";
        html += deviceName;
        html += R"HTML(</title>
)HTML";
        // Inject meta/styles/scripts from existing helpers
        html += getMeta();
        html += getStyles();
        html += getScripts(); // Reuse system theme and utils scripts

        // Body start
        html += R"HTML(
</head>
<body>
    <div class='container'>
        <div class='header-container'>
            <h1>)HTML";
        String deviceDesc = settings->getString(DEVICE_DESCRIPTION);
        if (deviceDesc.isEmpty()) deviceDesc = "JAAM LED Control";
        html += deviceDesc;
        html += R"HTML(</h1>
            <div class='header-buttons'>
                <button class='control-button' id='systemPanelToggle' onclick='toggleSystemPanel()' title='Показати/сховати системну панель'>
                    <svg viewBox='0 0 24 24'>
                        <path d='M13,9V7H11V9H13M13,17V11H11V17H13M12,2A10,10 0 0,0 2,12A10,10 0 0,0 12,22A10,10 0 0,0 22,12A10,10 0 0,0 12,2Z'/>
                    </svg>
                </button>
                <button class='control-button' id='alertsPanelToggle' onclick='toggleAlertsPanel()' title='Показати/сховати панель тривог'>
                    <svg viewBox='0 0 24 24'>
                        <path d='M13,14H11V10H13M13,18H11V16H13M1,21H23L12,2L1,21Z'/>
                    </svg>
                </button>
                <button class='control-button theme-toggle' onclick='toggleTheme()' title='Перемкнути тему'>
                    <svg viewBox='0 0 24 24'>
                        <path d='M12,18C11.11,18 10.26,17.8 9.5,17.46C11.56,16.06 13,13.72 13,11A6.8,6.8 0 0,0 9.5,4.54C10.26,4.2 11.11,4 12,4A8,8 0 0,1 20,12A8,8 0 0,1 12,20M12,2A10,10 0 0,0 2,12A10,10 0 0,0 12,22A10,10 0 0,0 22,12A10,10 0 0,0 12,2Z'/>
                    </svg>
                </button>
            </div>
        </div>

        <!-- System panel (rendered dynamically from /system-info schema) -->
        <div class='system-panel' id='systemPanel'></div>

        <!-- Alerts panel (content filled by updateAlertsInfo) -->
        <div class='alerts-panel' id='alertsPanel'>
            <div class='alert-metric'>
                <svg class='metric-icon' viewBox='0 0 24 24'>
                    <path d='M13,14H11V10H13M13,18H11V16H13M1,21H23L12,2L1,21Z'/>
                </svg>
                <span class='metric-label'>Активні тривоги:</span>
                <span class='metric-value' id='alertsContent'>
                    <span class='alerts-loading'>Завантаження...</span>
                </span>
            </div>
            <div id='alertsDetailsPanel'></div>
        </div>

        <!-- Navigation and controls -->
        <div id='uiControls'></div>
    </div>
</body>
</html>
)HTML";
    server.send(200, "text/html", html);
}

// Helper to push dropdown options to a JsonArray as compact lists: [id, name, sub]
static void appendOptionsList(JsonArray arr, SettingListItem items[], int itemCount) {
    for (int i = 0; i < itemCount; ++i) {
        if (items[i].ignore) continue;
        JsonArray opt = arr.add<JsonArray>();
        opt.add(items[i].id);
        opt.add(items[i].name);
        opt.add(items[i].sub ? 1 : 0);
    }
}

void JaamWeb::handleUiSchema() {
    setCrossOrigin();

    JsonDocument doc;
    // Top-level models glossary (compact field lists) as a clean JSON literal
    {
        static const char modelsJson[] PROGMEM = R"JSON(
        {
          "dropdown": ["name", "label", "list", "current", "section"],
          "bool":     ["name", "label", "current", "section"],
          "text":     ["name", "label", "current", "placeholder", "section"],
          "color":    ["name", "label", "current", "section"],
          "slider":   ["name", "label", "min", "max", "step", "current", "section"],
          "label":    ["label", "section"],
          "info":     ["text", "color", "icon", "section"],
          "option":   ["id", "name", "sub"]
        }
        )JSON";

        JsonDocument modelsDoc;
        DeserializationError err = deserializeJson(modelsDoc, modelsJson);
        if (err) {
            LOG.printf("[WEB] Failed to parse models JSON: %s\n", err.c_str());
            server.send(500, "application/json", "{\"error\":\"Internal server error\"}");
            return;
        }
        doc["models"].set(modelsDoc.as<JsonObject>());
    }

    // Sections definition
    {
        static const char sectionsJson[] PROGMEM = R"JSON(
        [
          {"id": "general", "name": "Загальні", "color": "#007bff"},
          {"id": "display", "name": "Дисплей", "color": "#28a745"},
          {"id": "network", "name": "Мережа", "color": "#17a2b8"},
          {"id": "hardware", "name": "Апаратне забезпечення", "color": "#6f42c1"},
          {"id": "climate", "name": "Клімат", "color": "#34f396"},
          {"id": "animations", "name": "Анімації", "color": "#fd7e14"},
          {"id": "brightness", "name": "Яскравість", "color": "#ffc107"},
          {"id": "alerts", "name": "Тривоги", "color": "#6c757d"}
        ]
        )JSON";

        JsonDocument sectionsDoc;
        DeserializationError err = deserializeJson(sectionsDoc, sectionsJson);
        if (err) {
            LOG.printf("[WEB] Failed to parse sections JSON: %s\n", err.c_str());
            server.send(500, "application/json", "{\"error\":\"Internal server error\"}");
            return;
        }
        doc["sections"].set(sectionsDoc.as<JsonArray>());
    }

    // Top-level dropdown option lists to be referenced by name from controls
    {
        JsonObject dropdownLists = doc["dropdown_lists"].to<JsonObject>();
        {
            JsonArray arr = dropdownLists["legacy"].to<JsonArray>();
            appendOptionsList(arr, LEGACY_OPTIONS, LEGACY_OPTIONS_COUNT);
        }
        {
            JsonArray arr = dropdownLists["display_model"].to<JsonArray>();
            appendOptionsList(arr, DISPLAY_TYPES, DISPLAY_TYPES_COUNT);
        }
        {
            JsonArray arr = dropdownLists["display_height"].to<JsonArray>();
            appendOptionsList(arr, DISPLAY_HEIGHTS, DISPLAY_HEIGHT_COUNT);
        }
        {
            JsonArray arr = dropdownLists["display_rotation"].to<JsonArray>();
            appendOptionsList(arr, DISPLAY_ROTATIONS, DISPLAY_ROTATION_COUNT);
        }
        {
            JsonArray arr = dropdownLists["districts"].to<JsonArray>();
            appendOptionsList(arr, DISTRICTS, MAX_REGIONS);
        }
        {
            JsonArray arr = dropdownLists["bg_led_mode"].to<JsonArray>();
            appendOptionsList(arr, BG_LED_MODES, BG_LED_MODES_COUNT);
        }
        {
            JsonArray arr = dropdownLists["map_mode"].to<JsonArray>();
            appendOptionsList(arr, MAP_MODES, MAP_MODES_COUNT);
        }
        {
            JsonArray arr = dropdownLists["led_color_formats"].to<JsonArray>();
            appendOptionsList(arr, LED_COLOR_FORMATS, LED_COLOR_FORMATS_COUNT);
        }
        {
            JsonArray arr = dropdownLists["led_frequencies"].to<JsonArray>();
            appendOptionsList(arr, LED_FREQUENCIES, LED_FREQUENCIES_COUNT);
        }
        {
            JsonArray arr = dropdownLists["animation_types"].to<JsonArray>();
            appendOptionsList(arr, ANIMATION_TYPES, ANIMATION_TYPES_COUNT);
        }
        {
            JsonArray arr = dropdownLists["auto_brightness_modes"].to<JsonArray>();
            appendOptionsList(arr, AUTO_BRIGHTNESS_MODES, AUTO_BRIGHTNESS_OPTIONS_COUNT);
        }
    }

    JsonArray controls = doc["controls"].to<JsonArray>();

    // Helper to add a dropdown control referencing a named list and reading current from settings key
    auto addDropdown = [&](const char* section, const char* name, const char* label, const char* listId, Type key){
        JsonArray c = controls.add<JsonArray>();
        c.add("dropdown"); c.add(name); c.add(label); c.add(listId); c.add(settings->getInt(key)); c.add(section);
    };

    // Helper to add a label/section header
    auto addLabel = [&](const char* section, const char* text){
        JsonArray group = controls.add<JsonArray>();
        group.add("label"); group.add(text); group.add(section);
    };

    // Helper to add a boolean control
    auto addBool = [&](const char* section, const char* name, const char* label, Type key){
        JsonArray c = controls.add<JsonArray>();
        c.add("bool"); c.add(name); c.add(label); c.add(settings->getBool(key)); c.add(section);
    };

    auto addText = [&](const char* section, const char* name, const char* label, const String& value, const char* placeholder){
        JsonArray c = controls.add<JsonArray>();
        c.add("text"); c.add(name); c.add(label); c.add(value); c.add(placeholder); c.add(section);
    };

    auto addSlider = [&](const char* section, const char* name, const char* label, float minv, float maxv, float step, float current){
        JsonArray c = controls.add<JsonArray>();
        c.add("slider"); c.add(name); c.add(label); c.add(minv); c.add(maxv); c.add(step); c.add(current); c.add(section);
    };

    auto addColor = [&](const char* section, const char* name, const char* label, Type key){
        JsonArray c = controls.add<JsonArray>();
        c.add("color"); c.add(name); c.add(label); c.add(String(settings->getString(key))); c.add(section);
    };

    // Helper to add an info panel
    auto addInfo = [&](const char* section, const char* text, const char* color, const char* icon){
        JsonArray c = controls.add<JsonArray>();
        c.add("info"); c.add(text); c.add(color); c.add(icon); c.add(section);
    };

    // Helper to add different types of info panels
    auto addInfoSuccess = [&](const char* section, const char* text){
        addInfo(section, text, "#28a745", "M21,7L9,19L3.5,13.5L4.91,12.09L9,16.17L19.59,5.59L21,7Z");
    };
    auto addInfoWarning = [&](const char* section, const char* text){
        addInfo(section, text, "#ffc107", "M13,14H11V10H13M13,18H11V16H13M1,21H23L12,2L1,21Z");
    };
    auto addInfoError = [&](const char* section, const char* text){
        addInfo(section, text, "#dc3545", "M13,14H11V10H13M13,18H11V16H13M12,2C6.47,2 2,6.5 2,12A10,10 0 0,0 12,22A10,10 0 0,0 22,12A10,10 0 0,0 12,2Z");
    };

    // Загальні налаштування
    addInfo("general", "Оберіть режим прошивки відповідно до вашої версії пристрою", "#007bff", "M13,9H11V7H13M13,17H11V11H13M12,2A10,10 0 0,0 2,12A10,10 0 0,0 12,22A10,10 0 0,0 22,12A10,10 0 0,0 12,2Z");  
    addDropdown("general", "legacy", "Режим прошивки", "legacy", LEGACY);
    addBool("general", "kyiv_led", "Київ як окремий LED", KYIV_LED);
    addDropdown("general", "home_district", "Домашній регіон", "districts", HOME_DISTRICT);
    addDropdown("general", "bg_led_mode", "Режим фонової підствітки", "bg_led_mode", BG_LED_MODE);
    addDropdown("general", "map_mode", "Режим мапи", "map_mode", MAP_MODE);
    addText("general", "device_name", "Назва пристрою", String(settings->getString(DEVICE_NAME)), "JAAM");
    addText("general", "device_description", "Опис пристрою", String(settings->getString(DEVICE_DESCRIPTION)), "JAAM Informer");
    addText("general", "broadcast_name", "Ім'я в мережі", String(settings->getString(BROADCAST_NAME)), "jaam");

    // Display settings
    addInfo("display", "Налаштуйте параметри дисплея та візуального відображення мапи", "#28a745", "M4,6H20V16H4M20,18A2,2 0 0,0 22,16V6C22,4.89 21.1,4 20,4H4C2.89,4 2,4.89 2,6V16A2,2 0 0,0 4,18H10V20H8V22H16V20H14V18H20Z");
    addDropdown("display", "display_model", "Тип дисплея", "display_model", DISPLAY_MODEL);
    addDropdown("display", "display_height", "Висота дисплея", "display_height", DISPLAY_HEIGHT);
    addDropdown("display", "display_rotation", "Поворот дисплея", "display_rotation", DISPLAY_ROTATION);
    addBool("display", "invert_display", "Інвертувати дисплей", INVERT_DISPLAY);

    // Мережеві налаштування
    addInfo("network", "Налаштуйте підключення до серверів та мережевих сервісів", "#17a2b8", "M17,3A2,2 0 0,1 19,5V15A2,2 0 0,1 17,17H13V19H14A1,1 0 0,1 15,20H22V22H15A1,1 0 0,1 14,21H10A1,1 0 0,1 9,22H2V20H9A1,1 0 0,1 10,19H11V17H7C5.89,17 5,16.1 5,15V5A2,2 0 0,1 7,3H17Z");
    addText("network", "ws_server_host", "Сервер WebSocket", String(settings->getString(WS_SERVER_HOST)), "ws.jaam.net.ua");
    addText("network", "ws_server_port", "Порт WebSocket", String(settings->getInt(WS_SERVER_PORT)), "80");
    addText("network", "ntp_host", "NTP сервер", String(settings->getString(NTP_HOST)), "time.google.com");

    // Home Assistant
    addLabel("network", "Home Assistant");
    addText("network", "ha_mqtt_user", "MQTT користувач", String(settings->getString(HA_MQTT_USER)), "");
    addText("network", "ha_mqtt_password", "MQTT пароль", String(settings->getString(HA_MQTT_PASSWORD)), "");
    addText("network", "ha_broker_address", "Адреса брокера", String(settings->getString(HA_BROKER_ADDRESS)), "");
    addInfoSuccess("network", "З'єднання встановлено. Перевірте налаштування при проблемах зі з'єднанням.");

    // Піни LED стрічок
    addInfo("hardware", "Конфігурація апаратних пінів та параметрів LED стрічок", "#6f42c1", "M9,7H11V17H9V19H15V17H13V7H15V5H9V7M12,2A10,10 0 0,0 2,12A10,10 0 0,0 12,22A10,10 0 0,0 22,12A10,10 0 0,0 12,2Z");
    addText("hardware", "main_led_pin", "Основна стрічка (пін)", String(settings->getInt(MAIN_LED_PIN)), "13");
    addDropdown("hardware", "main_led_color_format", "Основна стрічка (формат кольору)", "led_color_formats", MAIN_LED_COLOR_FORMAT);
    addDropdown("hardware", "main_led_frequency", "Основна стрічка (частота)", "led_frequencies", MAIN_LED_FREQUENCY);
    addText("hardware", "bg_led_pin", "Фонова стрічка (пін)", String(settings->getInt(BG_LED_PIN)), "-1");
    addText("hardware", "bg_led_count", "Фонова стрічка (кількість)", String(settings->getInt(BG_LED_COUNT)), "0");
    addDropdown("hardware", "bg_led_color_format", "Фонова стрічка (формат кольору)", "led_color_formats", BG_LED_COLOR_FORMAT);
    addDropdown("hardware", "bg_led_frequency", "Фонова стрічка (частота)", "led_frequencies", BG_LED_FREQUENCY);
    addText("hardware", "service_led_pin", "Сервісна стрічка (пін)", String(settings->getInt(SERVICE_LED_PIN)), "-1");
    addDropdown("hardware", "service_led_color_format", "Сервісна стрічка (формат кольору)", "led_color_formats", SERVICE_LED_COLOR_FORMAT);
    addDropdown("hardware", "service_led_frequency", "Сервісна стрічка (частота)", "led_frequencies", SERVICE_LED_FREQUENCY);
    addInfoError("hardware", "Увага: неправильна конфігурація пінів може призвести до пошкодження пристрою!");
    addLabel("hardware", "Батарея>");
    addBool("hardware", "enable_battery", "Моніторинг батареї", ENABLE_BATTERY_MONITORING);
    addText("hardware", "battery_pin", "ADC пін батареї", String(settings->getInt(BATTERY_PIN)), "-1");

    // Налаштування погоди / температури — sliders
    addInfo("climate", "Налаштування погодних переметрів та кліматичних сенсорів", "#34f396", "M12,2A10,10 0 0,1 22,12A10,10 0 0,1 12,22A10,10 0 0,1 2,12A10,10 0 0,1 12,2M12,4A8,8 0 0,0 4,12A8,8 0 0,0 12,20A8,8 0 0,0 20,12A8,8 0 0,0 12,4M11,17H13V11H11V17M11,9H13V7H11V9Z");
    addLabel("climate", "Налаштування погоди");
    addSlider("climate", "weather_min_temp", "Мінімальна температура (°C)", -40, 40, 1, settings->getInt(WEATHER_MIN_TEMP));
    addSlider("climate", "weather_max_temp", "Максимальна температура (°C)", -40, 40, 1, settings->getInt(WEATHER_MAX_TEMP));

    addLabel("climate", "Налаштування температури");
    addSlider("climate", "temp_correction", "Корегування температури (°C)", -10.0f, 10.0f, 0.1f, settings->getFloat(TEMP_CORRECTION));
    addSlider("climate", "hum_correction", "Корегування вологості (%)", -20.0f, 20.0f, 0.5f, settings->getFloat(HUM_CORRECTION));
    addSlider("climate", "pressure_correction", "Корегування атмосферного тиску (мм.рт.ст.)", -50.0f, 50.0f, 1.0f, settings->getFloat(PRESSURE_CORRECTION));

    // Налаштування анімацій
    addInfo("animations", "Оберіть типи і налаштування анімацій для різних видів тривог та подій", "#fd7e14", "M12,2A10,10 0 0,1 22,12A10,10 0 0,1 12,22A10,10 0 0,1 2,12A10,10 0 0,1 12,2M12,4A8,8 0 0,0 4,12A8,8 0 0,0 12,20A8,8 0 0,0 20,12A8,8 0 0,0 12,4M11,17H13V11H11V17M11,9H13V7H11V9Z");
    addBool("animations", "enable_sync_animations", "Синхронні анімації", ENABLE_SYNC_ANIMATIONS);
    addDropdown("animations", "alert_on_animation", "Початок тривог", "animation_types", ANIMATION_ALERT_ON_TYPE);
    addDropdown("animations", "alert_off_animation", "Відбій тривог", "animation_types", ANIMATION_ALERT_OFF_TYPE);
    addDropdown("animations", "drone_animation", "Загроза ударних БПЛА", "animation_types", ANIMATION_DRONE_TYPE);
    addDropdown("animations", "recon_drone_animation", "Розвідувальні БПЛА", "animation_types", ANIMATION_RECON_DRONE_TYPE);
    addDropdown("animations", "missile_animation", "Загроза ракет", "animation_types", ANIMATION_MISSILE_TYPE);
    addDropdown("animations", "kab_animation", "Загроза КАБ", "animation_types", ANIMATION_KAB_TYPE);
    addDropdown("animations", "ballistic_animation", "Загроза балістичних ракет", "animation_types", ANIMATION_BALLISTIC_TYPE);
    addDropdown("animations", "explosion_animation", "Вибухи", "animation_types", ANIMATION_EXPLOSION_TYPE);

    // Таймінги (секунди)
    addLabel("animations", "Налаштування таймінгів (в секундах)");
    addInfoWarning("animations", "Увага: занадто малі значення таймінгів можуть призвести до частого миготіння");
    addSlider("animations", "alert_on_time", "Початок тривог", 5, 600, 5, settings->getInt(ALERT_ON_TIME));
    addSlider("animations", "alert_off_time", "Відбій тривог", 5, 600, 5, settings->getInt(ALERT_OFF_TIME));
    addSlider("animations", "drone_time", "Загроза ударних БПЛА", 5, 600, 5, settings->getInt(DRONE_TIME));
    addSlider("animations", "recon_drone_time", "Розвідувальні БПЛА", 5, 600, 5, settings->getInt(RECON_DRONE_TIME));
    addSlider("animations", "missile_time", "Загроза ракет", 5, 600, 5, settings->getInt(MISSILE_TIME));
    addSlider("animations", "kab_time", "Загроза КАБ", 5, 600, 5, settings->getInt(KAB_TIME));
    addSlider("animations", "ballistic_time", "Загроза балістичних ракет", 5, 600, 5, settings->getInt(BALLISTIC_TIME));
    addSlider("animations", "explosion_time", "Вибухи", 5, 600, 5, settings->getInt(EXPLOSION_TIME));

    // Цикли (мс)
    addLabel("animations", "Налаштування цикла (в мілісекундах)");
    addSlider("animations", "alert_on_cycle", "Початок тривог", 300, 5000, 100, settings->getInt(ANIMATION_ALERT_ON_CYCLE_TIME));
    addSlider("animations", "alert_off_cycle", "Відбій тривог", 300, 5000, 100, settings->getInt(ANIMATION_ALERT_OFF_CYCLE_TIME));
    addSlider("animations", "drone_cycle", "Загроза ударних БПЛА", 300, 5000, 100, settings->getInt(ANIMATION_DRONE_CYCLE_TIME));
    addSlider("animations", "recon_drone_cycle", "Розвідувальні БПЛА", 300, 5000, 100, settings->getInt(ANIMATION_RECON_DRONE_CYCLE_TIME));
    addSlider("animations", "missile_cycle", "Загроза ракет", 300, 5000, 100, settings->getInt(ANIMATION_MISSILE_CYCLE_TIME));
    addSlider("animations", "kab_cycle", "Загроза КАБ", 300, 5000, 100, settings->getInt(ANIMATION_KAB_CYCLE_TIME));
    addSlider("animations", "ballistic_cycle", "Загроза балістичних ракет", 300, 5000, 100, settings->getInt(ANIMATION_BALLISTIC_CYCLE_TIME));
    addSlider("animations", "explosion_cycle", "Вибухи", 300, 5000, 100, settings->getInt(ANIMATION_EXPLOSION_CYCLE_TIME));

    // Кольори
    addLabel("animations", "Налаштування кольорів");
    addInfo("animations", "Налаштуйте кольорову схему для різних типів тривог та елементів", "#dc3545", "M17.5,12A1.5,1.5 0 0,1 16,10.5A1.5,1.5 0 0,1 17.5,9A1.5,1.5 0 0,1 19,10.5A1.5,1.5 0 0,1 17.5,12M9,11A3,3 0 0,1 12,8A3,3 0 0,1 15,11A3,3 0 0,1 12,14A3,3 0 0,1 9,11M12,2A10,10 0 0,1 22,12A10,10 0 0,1 12,22C6.47,22 2,17.5 2,12A10,10 0 0,1 12,2Z");
    addColor("animations", "color_alert", "Тривога", COLOR_ALERT);
    addColor("animations", "color_clear", "Відбій", COLOR_CLEAR);
    addColor("animations", "color_explosion", "Вибухи", COLOR_EXPLOSION);
    addColor("animations", "color_missiles", "Ракети", COLOR_MISSILES);
    addColor("animations", "color_drones", "Ударні БПЛА", COLOR_DRONES);
    addColor("animations", "color_recon_drones", "Розвідувальні БПЛА", COLOR_RECON_DRONES);
    addColor("animations", "color_kab", "КАБ", COLOR_KABS);
    addColor("animations", "color_ballistic", "Балістичні ракети", COLOR_BALLISTIC);
    addColor("animations", "color_home", "Домашній регіон", COLOR_HOME_DISTRICT);
    addColor("animations", "color_bg", "Задня підсвітка", COLOR_BG);

    // Яскравість
    addInfo("brightness", "Контроль яскравості LED стрічок для різних режимів та часу доби", "#ffc107", "M12,8A4,4 0 0,0 8,12A4,4 0 0,0 12,16A4,4 0 0,0 16,12A4,4 0 0,0 12,8M12,18A6,6 0 0,1 6,12A6,6 0 0,1 12,6A6,6 0 0,1 18,12A6,6 0 0,1 12,18M20,8.69V4H15.31L12,0.69L8.69,4H4V8.69L0.69,12L4,15.31V20H8.69L12,23.31L15.31,20H20V15.31L23.31,12L20,8.69Z");
    addDropdown("brightness", "brightness_mode", "Режим яскравості", "auto_brightness_modes", BRIGHTNESS_MODE);
    addSlider("brightness", "day_start", "Початок дня", 0, 24, 1, settings->getInt(DAY_START));
    addSlider("brightness", "night_start", "Початок ночі", 0, 24, 1, settings->getInt(NIGHT_START));
    addSlider("brightness", "brightness", "Загальна", 0, 100, 1, settings->getInt(BRIGHTNESS));
    addSlider("brightness", "brightness_day", "День", 0, 100, 1, settings->getInt(BRIGHTNESS_DAY));
    addSlider("brightness", "brightness_night", "Ніч", 0, 100, 1, settings->getInt(BRIGHTNESS_NIGHT));
    addSlider("brightness", "brightness_alert", "Тривога", 0, 100, 1, settings->getInt(BRIGHTNESS_ALERT));
    addSlider("brightness", "brightness_clear", "Без тривоги", 0, 100, 1, settings->getInt(BRIGHTNESS_CLEAR));
    addSlider("brightness", "brightness_explosion", "Вибухи", 0, 100, 1, settings->getInt(BRIGHTNESS_EXPLOSION));
    addSlider("brightness", "brightness_missiles", "Крилаті та авіаційні ракети", 0, 100, 1, settings->getInt(BRIGHTNESS_MISSILES));
    addSlider("brightness", "brightness_drones", "Ударні БПЛА", 0, 100, 1, settings->getInt(BRIGHTNESS_DRONES));
    addSlider("brightness", "brightness_recon_drones", "Розвідувальні БПЛА", 0, 100, 1, settings->getInt(BRIGHTNESS_RECON_DRONES));
    addSlider("brightness", "brightness_kabs", "КАБ", 0, 100, 1, settings->getInt(BRIGHTNESS_KABS));
    addSlider("brightness", "brightness_ballistic", "Балістичні ракети", 0, 100, 1, settings->getInt(BRIGHTNESS_BALLISTIC));
    addSlider("brightness", "brightness_home_district", "Домашній регіон", 0, 100, 1, settings->getInt(BRIGHTNESS_HOME_DISTRICT));
    addSlider("brightness", "brightness_bg", "Фонова стрічка", 0, 100, 1, settings->getInt(BRIGHTNESS_BG));
    addSlider("brightness", "brightness_service", "Сервісні діоди", 0, 100, 1, settings->getInt(BRIGHTNESS_SERVICE));
    addSlider("brightness", "brightness_animation_end", "Кінцева яскравість анімацій", 0, 100, 1, settings->getInt(BRIGHTNESS_ANIMATION_END));

    // Налаштування тривог
    addInfo("alerts", "Увімкніть або вимкніть відображення різних типів тривог", "#6c757d", "M13,14H11V10H13M13,18H11V16H13M1,21H23L12,2L1,21Z");
    addBool("alerts", "enable_kabs", "Загроза КАБ", ENABLE_KABS);
    addBool("alerts", "enable_missiles", "Загроза крилатих та авіаційних ракет", ENABLE_MISSILES);
    addBool("alerts", "enable_drones", "Загроза ударних БПЛА", ENABLE_DRONES);
    addBool("alerts", "enable_recon_drones", "Розвідувальні БПЛА", ENABLE_RECON_DRONES);
    addBool("alerts", "enable_ballistic", "Загроза балістичних ракет", ENABLE_BALLISTIC);
    addBool("alerts", "enable_explosions", "Вибухи", ENABLE_EXPLOSIONS);

    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
}

void JaamWeb::handleSaveMap() {

    for (int i = 0; i < server.args(); ++i) {
        String argName = server.argName(i);
        if (argName.startsWith("region_")) {
            uint16_t region_id = argName.substring(7).toInt();
            String value = server.arg(i);
            value.trim();

            int map_idx = -1;
            for(int j = 0; j < MAX_REGIONS; ++j) {
                if (DISTRICTS[j].id == region_id) {
                    map_idx = j;
                    break;
                }
            }

            if (map_idx != -1) {
                customMap[map_idx].region_id = region_id;
                uint8_t count = 0;
                
                int last_comma = -1;
                for (int k = 0; k < value.length(); ++k) {
                    if (value.charAt(k) == ',') {
                        String num_str = value.substring(last_comma + 1, k);
                        num_str.trim();
                        if (num_str.length() > 0 && count < MAX_LEDS_PER_REGION) {
                            customMap[map_idx].led_positions[count++] = num_str.toInt();
                        }
                        last_comma = k;
                    }
                }
                String num_str = value.substring(last_comma + 1);
                num_str.trim();
                if (num_str.length() > 0 && count < MAX_LEDS_PER_REGION) {
                    customMap[map_idx].led_positions[count++] = num_str.toInt();
                }
                customMap[map_idx].led_count = count;
            }
        }
    }

    if (storage.saveCustomMap(customMap)) {
        //generateCustomRegionMap();
        needRecalculateLeds = true;
        LOG.println("[WEB] Custom map saved successfully.");
        server.sendHeader("Location", "/map-editor", true);
        server.send(303);
    } else {
        LOG.println("[WEB] Custom map saving error.");
        server.send(500, "text/plain", "Custom map error");
    }
}

void JaamWeb::handleMapEditor() {
    String html = "<!DOCTYPE html><html>";
    html += "<head>";
    String deviceName = settings->getString(DEVICE_NAME);
    if (deviceName.isEmpty()) deviceName = "JAAM";
    html += "<title>" + deviceName + "</title>";
    html += getMeta();
    html += getStyles();
    html += getScripts();
    html += "</head><body>";

    html += "<div class='container'>";
    html += "<div class='header-container'>";
    html += "<h1>Редактор власної карти LED</h1>";
    html += "<button class='theme-toggle' onclick='toggleTheme()' title='Перемкнути тему'><svg viewBox='0 0 24 24'><path d='M12,18C11.11,18 10.26,17.8 9.5,17.46C11.56,16.06 13,13.72 13,11A6.8,6.8 0 0,0 9.5,4.54C10.26,4.2 11.11,4 12,4A8,8 0 0,1 20,12A8,8 0 0,1 12,20M12,2A10,10 0 0,0 2,12A10,10 0 0,0 12,22A10,10 0 0,0 22,12A10,10 0 0,0 12,2Z'/></svg></button>";
    html += "</div>";
    
    // Навігація та перемикач теми
    html += "<a href='/' style='display: block; margin-bottom: 20px; color: var(--text-color); text-decoration: none;'>&larr; На головну</a>";

    html += "<form action='/save-map' method='post'>";

    storage.loadCustomMap(customMap);

    for (int i = 0; i < MAX_REGIONS; ++i) {
        if ((DISTRICTS[i].id == 0 && i > 0)) {
            continue;
        }
        String leds_str = "";
        for (int j = 0; j < MAX_REGIONS; ++j) {
            if (customMap[j].region_id == DISTRICTS[i].id) {
                for (int k = 0; k < customMap[j].led_count; ++k) {
                    leds_str += String(customMap[j].led_positions[k]);
                    if (k < customMap[j].led_count - 1) leds_str += ", ";
                }
                break;
            }
        }
        
        String displayName = DISTRICTS[i].sub ? "&nbsp;&nbsp;&nbsp;&nbsp;" + String(DISTRICTS[i].name) : "<b>" + String(DISTRICTS[i].name) + "</b>";
        
        // Використання структури та класів з головної сторінки
        html += "<div class='text-input-container'>";
        html += "<label for='region_" + String(DISTRICTS[i].id) + "'>" + displayName + "</label>";
        html += "<input type='text' id='region_" + String(DISTRICTS[i].id) + "' name='region_" + String(DISTRICTS[i].id) + "' value='" + leds_str + "' placeholder='номери LED, через кому' class='text-input'>";
        html += "</div>";
    }

    html += "<button type='submit' class='btn-submit'>Зберегти Карту</button></form></div></body></html>";
    server.send(200, "text/html", html);
}
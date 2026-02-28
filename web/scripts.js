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
        
        if (type === 'text') {
            const label = item[2];
            const iconSvg = item[3];
            const value = item[4];
            const text = (value !== null && value !== undefined) ? value : '--';
            addMetric(panel, iconSvg, label, text);
            
        } else if (type === 'bar') {
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
    // Only fetch if system panel is visible
    if (!systemPanelVisible) return;
    
    fetch('/system-info')
        .then(r => r.json())
        .then(data => {
            if (data && Array.isArray(data.system) && data.system.length) {
                renderSystemPanelFromSchema(data);
            } else {
                const panel = document.getElementById('systemPanel');
                if (panel) {
                    panel.innerHTML = '<span class="system-no-data">Системна інформація недоступна</span>';
                }
            }
        })
        .catch(err => console.error('Error fetching system info:', err));
}

// Alerts panel
function updateAlertsInfo() {
    // Only fetch if alerts panel is visible
    if (!alertsPanelVisible) return;
    
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
            
            const alertTypes = [
                {key: 'air', label: 'Повітряна тривога', 
                 icon: '<svg class="alert-detail-icon" viewBox="0 0 38 38" fill="none"><defs><clipPath id="clip0_1253_836"><rect width="24" height="24" fill="white" transform="translate(7 7)"/></clipPath></defs><circle cx="19" cy="19" r="19" fill="#D55B50"/><g clip-path="url(#clip0_1253_836)"><path fill-rule="evenodd" clip-rule="evenodd" d="M23.1813 12.3365L23.4098 8.798L24.6992 7.50708L25.8692 9.64465L23.1813 12.3365ZM10.161 23.9349C10.1088 23.9926 10.0586 24.0535 10.0106 24.1177C8.82995 25.6952 8.22721 27.0553 7.92452 28.0985C7.54354 29.4114 8.58945 30.4574 9.90208 30.0752C10.9451 29.7716 12.3049 29.167 13.8817 27.9826C13.9451 27.9349 14.0055 27.8851 14.0627 27.8333C14.69 27.3603 15.3494 26.7923 16.0747 26.0659L11.935 21.9264C11.2079 22.6546 10.6339 23.3054 10.161 23.9349ZM29.2034 14.5938L25.6678 14.8229L28.356 12.1295L30.4931 13.3026L29.2034 14.5938ZM19.9385 13.9233L12.4869 21.3745L16.6267 25.514L24.0782 18.0629L19.9385 13.9233ZM23.249 12.9607C23.2588 12.9496 23.269 12.9387 23.2796 12.9281L26.3258 9.8821C26.6087 9.5992 27.0674 9.5992 27.3503 9.8821L28.1197 10.6514C28.4026 10.9343 28.4026 11.393 28.1197 11.6759L25.0735 14.7219C25.0629 14.7325 25.052 14.7427 25.041 14.7525C25.514 15.6383 25.3771 16.7641 24.6302 17.5109L20.4904 13.3714C21.2373 12.6246 22.3632 12.4877 23.249 12.9607Z" fill="#141921"/></g></svg>'},
                {key: 'artillery', label: 'Артобстріл', 
                 icon: '<svg class="alert-detail-icon" viewBox="0 0 38 38" fill="none"><circle cx="19" cy="19" r="19" fill="#FF9900"/><g clip-path="url(#clip0_1253_7001)"><path fill-rule="evenodd" clip-rule="evenodd" d="M25.0462 12.2486L25.474 10.1808L27.4499 7.67603L30.3242 9.94179L28.3486 12.4468L26.4368 13.3448C26.3353 13.3925 26.2463 13.463 26.1769 13.551L22.2811 18.4896L21.0101 17.4876L24.9061 12.549C24.9753 12.461 25.0234 12.358 25.0461 12.2483L25.0462 12.2486ZM24.8029 30.3239C26.6678 30.3239 28.1799 28.8123 28.1799 26.9481C28.1799 25.0838 26.6678 23.5722 24.8029 23.5722C22.938 23.5722 21.4258 25.0838 21.4258 26.9481C21.4258 28.8123 22.938 30.3239 24.8029 30.3239ZM24.8028 28.2502C25.5222 28.2502 26.1054 27.6671 26.1054 26.948C26.1054 26.229 25.5222 25.6459 24.8028 25.6459C24.0835 25.6459 23.5003 26.229 23.5003 26.948C23.5003 27.6671 24.0835 28.2502 24.8028 28.2502ZM18.6621 17.31L17.1939 19.1711L16.0821 18.2947L15.4273 18.3721L9.9556 25.3084H8.35168C7.97866 25.3084 7.67627 25.6107 7.67627 25.9836V28.8772C7.67627 29.25 7.97866 29.5523 8.35168 29.5523H8.63925L8.04223 30.1482C7.98138 30.209 8.02441 30.3129 8.11041 30.3129H10.6289C10.6544 30.3129 10.6789 30.3028 10.697 30.2847L11.431 29.5523L21.0572 29.5523C20.4738 28.8442 20.1233 27.937 20.1233 26.9481C20.1233 26.3651 20.2451 25.8106 20.4646 25.3084H18.5939L22.5104 20.3436L22.433 19.6891L19.3168 17.2326L18.6621 17.31Z" fill="#141921"/></g><defs><clipPath id="clip0_1253_7001"><rect width="24" height="24" fill="white" transform="translate(7 7)"/></clipPath></defs></svg>'},
                {key: 'urban', label: 'Вуличні бої', 
                 icon: '<svg class="alert-detail-icon" viewBox="0 0 38 38" fill="none"><circle cx="19" cy="19" r="19" fill="#00B280"/><path d="M18.8736 20.3452L19.194 20.0271L16.0897 16.9456L12.6987 20.3114L13.449 22.2628L8.521 27.1546L11.8841 30.493L14.2836 25.502L16.153 28.8246L17.6686 27.3202L16.5333 25.2533C17.091 25.1168 17.5314 24.8912 17.8476 24.5773C17.9702 24.4557 18.074 24.3212 18.1596 24.1731C18.4189 23.7242 18.4485 23.247 18.414 22.8833C20.3494 23.8122 21.9053 23.6488 21.9863 23.6398L22.1862 23.6161L22.6184 21.3369L22.2609 21.364C20.5725 21.4931 19.333 20.6976 18.8735 20.3454L18.8736 20.3452ZM17.6835 23.9027C17.4384 24.3253 16.9582 24.6112 16.26 24.7553L15.6142 23.5806L15.9105 23.2865C16.1545 23.5591 16.5677 23.8105 17.1945 23.7765L17.164 23.2326C16.6867 23.2581 16.4293 23.065 16.2981 22.9024L17.1042 22.1022C17.3385 22.2699 17.5707 22.4226 17.7994 22.5562C17.8612 22.7746 17.9866 23.377 17.6835 23.9027Z" fill="#141921"/><path d="M27.1501 8.66255L27.5382 9.04786L26.5679 10.0111L25.1674 8.62085L24.5226 11.2709L19.3882 16.3679L18.6122 15.5976L17.0598 17.1386L19.5823 19.6425L21.1346 18.1015L20.5526 17.5237L25.7921 12.3227L25.4041 11.9376L27.9266 9.4335L28.3147 9.81867L29.4787 8.66288L28.3144 7.50708L27.1505 8.66273L27.1501 8.66255ZM26.1799 10.3965L25.0201 11.5478L25.4713 9.69314L26.1799 10.3965Z" fill="#141921"/></svg>'},
                {key: 'chemical', label: 'Хімічна', 
                 icon: '<svg class="alert-detail-icon" viewBox="0 0 38 38" fill="none"><circle cx="19" cy="19" r="19" fill="#D080FF"/><path fill-rule="evenodd" clip-rule="evenodd" d="M22.6397 10.6752C22.6397 12.3733 21.4992 13.8024 19.9495 14.2241L19.9495 15.3719C22.296 15.8238 24.0704 17.9196 24.0704 20.4366C24.0704 21.023 23.9741 21.5866 23.7967 22.1118L24.7908 22.6913C25.9273 21.547 27.7232 21.2642 29.1795 22.1132C30.9204 23.1281 31.5168 25.3758 30.5118 27.1336C29.5067 28.8914 27.2807 29.4937 25.5399 28.4788C24.0835 27.6298 23.4281 25.9179 23.8413 24.3519L22.8597 23.7797C21.9297 24.8884 20.5456 25.5915 19 25.5915C17.4544 25.5915 16.0703 24.8884 15.1403 23.7797L14.1587 24.3519C14.5719 25.9179 13.9165 27.6298 12.4601 28.4788C10.7193 29.4937 8.49329 28.8914 7.48823 27.1336C6.48317 25.3758 7.07962 23.1281 8.82044 22.1132C10.2768 21.2642 12.0727 21.547 13.2092 22.6913L14.2033 22.1118C14.0259 21.5866 13.9296 21.023 13.9296 20.4366C13.9296 17.9196 15.704 15.8239 18.0505 15.3719L18.0505 14.224C16.5009 13.8023 15.3604 12.3732 15.3604 10.6752C15.3604 8.64543 16.9899 7 19.0001 7C21.0102 7 22.6397 8.64544 22.6397 10.6752ZM21.3298 19.0642C20.5869 17.765 18.9416 17.3198 17.6549 18.0699C16.3682 18.82 15.9274 20.4814 16.6702 21.7806C17.4131 23.0799 19.0584 23.5251 20.3451 22.7749C21.6318 22.0248 22.0727 20.3635 21.3298 19.0642ZM22.5493 20.4366C22.5493 22.4435 20.9602 24.0704 19 24.0704C17.0398 24.0704 15.4507 22.4435 15.4507 20.4366C15.4507 18.4297 17.0398 16.8028 19 16.8028C20.9602 16.8028 22.5493 18.4297 22.5493 20.4366Z" fill="#141921"/></svg>'},
                {key: 'nuclear', label: 'Ядерна', 
                 icon: '<svg class="alert-detail-icon" viewBox="0 0 38 38" fill="none"><circle cx="19" cy="19" r="19" fill="#E6CD40"/><path d="M25 8.35217C26.8242 9.41741 28.3391 10.9496 29.3923 12.7946C30.4455 14.6397 31 16.7326 31 18.8631L23.171 18.8631C23.171 18.1226 22.9783 17.3951 22.6122 16.7538C22.2461 16.1125 21.7196 15.5799 21.0855 15.2096L25 8.35217Z" fill="#141921"/><path d="M25 29.374C23.1758 30.4393 21.1064 31.0001 19 31.0001C16.8936 31.0001 14.8242 30.4393 13 29.374L16.9145 22.5165C17.5486 22.8868 18.2678 23.0817 19 23.0817C19.7322 23.0817 20.4514 22.8868 21.0855 22.5165L25 29.374Z" fill="#141921"/><path d="M7 18.8631C7 16.7326 7.55448 14.6397 8.60769 12.7946C9.66091 10.9496 11.1758 9.41741 13 8.35217L16.9145 15.2096C16.2804 15.5799 15.7539 16.1125 15.3878 16.7538C15.0217 17.3951 14.829 18.1226 14.829 18.8631L7 18.8631Z" fill="#141921"/><path d="M21.7781 18.8631C21.7781 20.4149 20.5343 21.6728 19 21.6728C17.4657 21.6728 16.222 20.4149 16.222 18.8631C16.222 17.3113 17.4657 16.0533 19 16.0533C20.5343 16.0533 21.7781 17.3113 21.7781 18.8631Z" fill="#141921"/></svg>'},
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
            
            // Clear existing content
            alertsDetailsPanel.innerHTML = '';
            
            // Build alert rows using DOM APIs
            data.regions.forEach(region => {
                const row = document.createElement('div');
                row.className = 'alert-detail-row';
                
                const regionSpan = document.createElement('span');
                regionSpan.className = 'alert-detail-region';
                regionSpan.textContent = region.regionName;
                row.appendChild(regionSpan);
                
                alertTypes.forEach(type => {
                    if (region.alerts && region.alerts[type.key]) {
                        const iconSpan = document.createElement('span');
                        iconSpan.setAttribute('title', type.label);
                        // Icons are from static alertTypes array, safe to use innerHTML
                        iconSpan.innerHTML = type.icon;
                        row.appendChild(iconSpan);
                    }
                });
                
                alertsDetailsPanel.appendChild(row);
            });
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
                return r.json().then(data => {
                    if (data && data.error) {
                        console.error('Server error:', data.error);
                    } else {
                        console.error('Помилка оновлення параметра');
                    }
                    throw new Error('Server error');
                }).catch(jsonErr => {
                    // Якщо не JSON відповідь
                    if (jsonErr.message !== 'Server error') {
                        console.error('Помилка оновлення параметра');
                    }
                    throw jsonErr;
                });
            }
        })
        .catch(e => {
            if (e.message !== 'Server error') {
                console.error('Network error:', e);
            }
        });
}

// Dynamic UI rendering (from /ui-schema)
async function fetchSchema() {
    const [models, sections, dropdownLists, controls] = await Promise.all([
        fetch('/ui-schema/models').then(r => r.json()),
        fetch('/ui-schema/sections').then(r => r.json()),
        fetch('/ui-schema/dropdown_lists').then(r => r.json()),
        fetch('/ui-schema/controls').then(r => r.json())
    ]);
    
    return {
        models: models.models,
        sections: sections.sections,
        dropdown_lists: dropdownLists.dropdown_lists,
        controls: controls.controls
    };
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
        const [_, text, section, visibility] = ctrl;
        const el = labelEl(text);
        if (visibility && visibility.trim() !== '') {
            el.setAttribute('data-visibility', visibility);
            updateElementVisibility(el);
        }
        return el;
    }
    
    if (type === 'info') {
        const [_, text, color, icon, section, visibility] = ctrl;
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
        
        if (visibility && visibility.trim() !== '') {
            div.setAttribute('data-visibility', visibility);
            updateElementVisibility(div);
        }
        
        return div;
    }
    
    if (type === 'dropdown') {
        const [_, name, label, list, current, section, visibility] = ctrl;
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
        
        if (visibility && visibility.trim() !== '') {
            div.setAttribute('data-visibility', visibility);
            updateElementVisibility(div);
        }
        
        return div;
    }
    
    if (type === 'dropdown_confirm') {
        const [_, name, label, list, current, section, visibility] = ctrl;
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
        
        // Create confirm button
        const confirmBtn = document.createElement('button');
        confirmBtn.type = 'button';
        confirmBtn.className = 'form-button confirm-button';
        confirmBtn.textContent = 'Підтвердити';
        confirmBtn.style.marginTop = '8px';

        confirmBtn.onclick = () => {
            updateParameter(name, sel.value);
        };
        
        div.appendChild(lab);
        div.appendChild(sel);
        div.appendChild(confirmBtn);
        
        if (visibility && visibility.trim() !== '') {
            div.setAttribute('data-visibility', visibility);
            updateElementVisibility(div);
        }
        
        return div;
    }
    
    if (type === 'bool') {
        const [_, name, label, current, section, visibility] = ctrl;
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
        
        if (visibility && visibility.trim() !== '') {
            div.setAttribute('data-visibility', visibility);
            updateElementVisibility(div);
        }
        
        return div;
    }
    
    if (type === 'text') {
        const [_, name, label, current, placeholder, section, visibility] = ctrl;
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
        
        if (visibility && visibility.trim() !== '') {
            div.setAttribute('data-visibility', visibility);
            updateElementVisibility(div);
        }
        
        return div;
    }
    
    if (type === 'color') {
        const [_, name, label, current, section, visibility] = ctrl;
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
        
        if (visibility && visibility.trim() !== '') {
            div.setAttribute('data-visibility', visibility);
            updateElementVisibility(div);
        }
        
        return div;
    }
    
    if (type === 'slider') {
        const [_, name, label, min, max, step, current, section, visibility] = ctrl;
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
        
        if (visibility && visibility.trim() !== '') {
            div.setAttribute('data-visibility', visibility);
            updateElementVisibility(div);
        }
        
        return div;
    }
    
    if (type === 'button') {
        const [_, name, label, color, url, section, visibility] = ctrl;
        const div = document.createElement('div');
        div.className = 'button-container';
        
        const btn = document.createElement('button');
        btn.type = 'button';
        btn.className = 'form-button';
        btn.textContent = label;
        btn.style.backgroundColor = color;
        btn.style.borderColor = color;
        btn.onclick = () => window.open(url, '_blank');
        
        div.appendChild(btn);
        
        if (visibility && visibility.trim() !== '') {
            div.setAttribute('data-visibility', visibility);
            updateElementVisibility(div);
        }
        
        return div;
    }
    
    return document.createTextNode('');
}

// Visibility management for conditional showing/hiding of elements
function updateElementVisibility(element) {
    const visibility = element.getAttribute('data-visibility');
    if (!visibility) return;
    
    const conditions = visibility.split(',').map(c => c.trim());
    let shouldShow = true;
    
    for (const condition of conditions) {
        if (!evaluateCondition(condition)) {
            shouldShow = false;
            break;
        }
    }
    
    if (shouldShow) {
        element.classList.remove('hidden');
    } else {
        element.classList.add('hidden');
    }
}

function evaluateCondition(condition) {
    // Format: "hardware!=0" means hardware != 0
    const match = condition.match(/(\w+)(!=|==)(\d+)/);
    if (!match) return true;
    
    const [_, field, operator, value] = match;
    const hwSelect = document.getElementById(field);
    if (!hwSelect) return true;
    
    // For checkboxes, convert checked state to "1" or "0" for comparison
    let currentValue;
    if (hwSelect.type === 'checkbox') {
        currentValue = hwSelect.checked ? '1' : '0';
    } else {
        currentValue = hwSelect.value;
    }
    
    if (operator === '!=') {
        return String(currentValue) !== String(value);
    } else if (operator === '==') {
        return String(currentValue) === String(value);
    }
    
    return true;
}

function setupVisibilityListeners() {
    // Get all elements that have data-visibility or are select/input elements that affect visibility
    const triggerElements = document.querySelectorAll('select, input[type="checkbox"]');
    
    triggerElements.forEach(el => {
        el.addEventListener('change', () => {
            updateAllVisibilities();
        });
    });
    
    // Also listen to input events for text fields
    const textInputs = document.querySelectorAll('input[type="text"]');
    textInputs.forEach(el => {
        el.addEventListener('input', () => {
            updateAllVisibilities();
        });
    });
}

function updateAllVisibilities() {
    const elements = document.querySelectorAll('[data-visibility]');
    elements.forEach(el => updateElementVisibility(el));
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
        root.innerHTML = '';
        root.appendChild(navMenu);
        root.appendChild(sectionContainers);
        
        // Group controls by section
        const controlsBySection = {};
        for (const ctrl of controls) {
            const type = ctrl[0];
            let section = 'general'; // Default section
            
            // Extract section based on control type and position
            // All controls have visibility as last parameter, section is always second-to-last
            section = ctrl[ctrl.length - 2] || 'general';
            
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
        
        // Setup visibility listeners and update initial state
        setupVisibilityListeners();
        updateAllVisibilities();
        
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
        // Immediately fetch data when panel is expanded
        updateSystemInfo();
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
        // Immediately fetch data when panel is expanded
        updateAlertsInfo();
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
    const navMenu = document.createElement('div');
    navMenu.className = 'nav-menu';
    
    sections.forEach(section => {
        const navItem = document.createElement('div');
        navItem.className = 'nav-item';
        navItem.setAttribute('data-section', section.id);
        navItem.textContent = section.name;
        navItem.style.borderLeft = '3px solid ' + (section.color || '#ccc');
        
        // Use addEventListener instead of inline onclick
        navItem.addEventListener('click', function() {
            switchSection(section.id);
        });
        
        navMenu.appendChild(navItem);
    });
    
    return navMenu;
}

function createSectionContainers(sections) {
    const fragment = document.createDocumentFragment();
    
    sections.forEach(section => {
        const container = document.createElement('div');
        container.className = 'section-container';
        container.id = 'section-' + section.id;
        
        const header = document.createElement('div');
        header.className = 'section-header';
        header.textContent = section.name;
        header.style.borderColor = section.color || '#ccc';
        
        const content = document.createElement('div');
        content.className = 'section-content';
        content.id = 'content-' + section.id;
        
        container.appendChild(header);
        container.appendChild(content);
        fragment.appendChild(container);
    });
    
    return fragment;
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
    // Only run UI rendering and polling on the main UI page
    const uiControls = document.getElementById('uiControls');
    if (!uiControls) {
        // Editor pages don't have uiControls - skip polling
        return;
    }
    
    loadPanelStates();
    renderUI();
    applyPanelStates();
    updateSystemInfo();
    updateAlertsInfo();
    setInterval(updateSystemInfo, 5000);
    setInterval(updateAlertsInfo, 10000);
});

// Map Editor functionality

var brightnessData = {};

function loadMapData() {
    fetch('/map-data')
        .then(response => {
            if (!response.ok) {
                throw new Error(`HTTP error! status: ${response.status}`);
            }
            return response.json();
        })
        .then(data => {
            brightnessData = {
                hardware: data.hardware,
                brightness_max: data.brightness_max,
                brightness_default_pct: data.brightness_default_pct,
                brightness_absolute_pct: data.brightness_absolute_pct
            };
            renderMapEditor(data.regions);
            renderBrightnessSection();
        })
        .catch(err => {
            console.error('Error fetching map data:', err);
            document.getElementById('mapContent').innerHTML = 
                '<div style="color: var(--error-color); text-align: center; padding: 20px;">Помилка завантаження даних карти</div>';
        });
}

function renderMapEditor(regions) {
    const container = document.getElementById('mapContent');
    // Clear loading message
    container.innerHTML = '';
    
    const form = document.createElement('form');
    form.id = 'mapForm';
    form.action = '/save-map';
    form.method = 'post';
    
    regions.forEach(region => {
        const div = document.createElement('div');
        div.className = 'text-input-container';
        
        const label = document.createElement('label');
        label.setAttribute('for', 'region_' + region.id);
        label.textContent = region.name;
        
        // Apply styling via CSS classes instead of HTML injection
        if (region.sub) {
            label.classList.add('sub-region');
        } else {
            label.classList.add('root-region');
        }
        
        const input = document.createElement('input');
        input.type = 'text';
        input.id = 'region_' + region.id;
        input.name = 'region_' + region.id;
        input.value = region.leds_string || '';
        input.placeholder = 'номери LED, через кому';
        input.className = 'text-input';
        
        div.appendChild(label);
        div.appendChild(input);
        form.appendChild(div);
    });
    
    container.appendChild(form);
    
    // Enable save button
    const saveBtn = document.getElementById('saveBtn');
    if (saveBtn) {
        saveBtn.disabled = false;
        saveBtn.textContent = 'Зберегти карту';
    }
}

function saveMap() {
    const form = document.getElementById('mapForm');
    if (form) {
        const saveBtn = document.getElementById('saveBtn');
        if (saveBtn) {
            saveBtn.disabled = true;
            saveBtn.textContent = 'Збереження...';
        }
        form.submit();
    }
}

async function exportMap() {
    try {
        const response = await fetch('/map-data');
        if (!response.ok) {
            throw new Error(`HTTP error! status: ${response.status}`);
        }
        const data = await response.json();
        
        // Create a clean export object with only necessary fields
        const exportData = {
            regions: data.regions.map(region => ({
                id: region.id,
                name: region.name,
                leds: region.leds || []
            }))
        };
        
        const json_str = JSON.stringify(exportData, null, 2);
        const fileName = 'jaam_map_' + new Date().toISOString().slice(0, 10) + '.json';
        
        // Try modern File System Access API
        if ('showSaveFilePicker' in window) {
            const fileHandle = await window.showSaveFilePicker({
                suggestedName: fileName,
                types: [{
                    description: 'JSON файл',
                    accept: { 'application/json': ['.json'] }
                }]
            });
            const writable = await fileHandle.createWritable();
            await writable.write(json_str);
            await writable.close();
        } else {
            // Fallback to traditional download method
            const blob = new Blob([json_str], { type: 'application/json' });
            const url = URL.createObjectURL(blob);
            const link = document.createElement('a');
            link.href = url;
            link.download = fileName;
            link.click();
            URL.revokeObjectURL(url);
        }
    } catch (err) {
        console.error('Error exporting map:', err);
    }
}

async function importMapFromFile(event) {
    const file = event.target.files[0];
    if (!file) return;
    
    const reader = new FileReader();
    reader.onload = async (e) => {
        try {
            const importData = JSON.parse(e.target.result);
            
            if (!importData.regions || !Array.isArray(importData.regions)) {
                alert('Невірний формат файлу. Очікується структура з полем "regions".');
                return;
            }
            
            // Fetch current region structure from server
            const response = await fetch('/map-data');
            if (!response.ok) {
                throw new Error(`HTTP error! status: ${response.status}`);
            }
            const currentData = await response.json();
            
            if (!currentData.regions || !Array.isArray(currentData.regions)) {
                alert('Помилка завантаження поточної структури регіонів.');
                return;
            }
            
            // Create a map of imported LEDs by region ID for quick lookup
            const importedLedsMap = new Map();
            importData.regions.forEach(region => {
                // Validate region ID is a number
                if (typeof region.id !== 'number') {
                    return;
                }
                
                // Validate leds is an array
                if (!Array.isArray(region.leds)) {
                    return;
                }
                
                // Validate all LED entries are numbers
                const validLeds = region.leds.filter(led => typeof led === 'number');
                
                importedLedsMap.set(region.id, validLeds);
            });
            
            // Merge: use current structure but replace LEDs from imported file
            const mergedRegions = currentData.regions.map(region => ({
                id: region.id,
                name: region.name,
                sub: region.sub,
                leds_string: (importedLedsMap.get(region.id) || []).join(', ')
            }));
            
            renderMapEditor(mergedRegions);
            alert('Карта успішно завантажена з файлу. Натисніть "Зберегти карту" для застосування змін.');
        } catch (err) {
            alert('Помилка при завантаженні файлу: ' + err.message);
            console.error('Error parsing import file:', err);
        }
    };
    reader.readAsText(file);
    
    // Reset file input
    event.target.value = '';
}

function renderBrightnessSection() {
    var container = document.getElementById('brightnessSection');
    if (!container) return;

    // Only show for custom mapping hardware (type 5)
    if (brightnessData.hardware !== 5) {
        container.style.display = 'none';
        return;
    }

    container.style.display = 'block';

    var defaultPct = brightnessData.brightness_default_pct || 28;
    var absPct = brightnessData.brightness_absolute_pct || 100;
    var currentPct = brightnessData.brightness_max || 0;
    var effectivePct = currentPct > 0 ? currentPct : defaultPct;

    // Clamp to valid range
    if (effectivePct < defaultPct) effectivePct = defaultPct;
    if (effectivePct > absPct) effectivePct = absPct;

    var isCustomized = currentPct > 0;

    container.innerHTML =
        '<h3>Максимальна яскравість LED</h3>' +
        '<div class="brightness-warning">' +
            '<strong>⚠️ Увага!</strong> Збільшення максимальної яскравості може призвести до пошкодження ' +
            'плати через високий струм та перегрів. При високій яскравості пристрій може сильно нагріватися, ' +
            'особливо при великій кількості LED. Нагрівання є основним обмежуючим фактором і може призвести ' +
            'до виходу компонентів з ладу. Змінюйте це значення лише якщо ваше обладнання має достатньо ' +
            'потужне живлення, відповідну проводку та належне охолодження. ' +
            'Ви берете на себе всю відповідальність за можливі пошкодження.' +
        '</div>' +
        '<div class="brightness-info">' +
            'Стандартне значення: <strong>' + defaultPct + '</strong> з ' + absPct +
        '</div>' +
        '<div class="brightness-accept">' +
            '<label>' +
                '<input type="checkbox" id="brightnessRiskAccept"' + (isCustomized ? ' checked' : '') + ' onchange="toggleBrightnessControls()">' +
                ' Я розумію ризики та приймаю відповідальність за можливі пошкодження обладнання' +
            '</label>' +
        '</div>' +
        '<div class="brightness-controls" id="brightnessControls" style="display:' + (isCustomized ? 'block' : 'none') + ';">' +
            '<label for="brightnessMaxSlider">Максимальна яскравість: <span id="brightnessMaxPct">' + effectivePct + '</span> з ' + absPct + '</label>' +
            '<input type="range" id="brightnessMaxSlider" min="' + defaultPct + '" max="' + absPct + '" value="' + effectivePct + '" aria-label="Максимальна яскравість" oninput="updateBrightnessDisplay(this.value)">' +
            '<button class="brightness-save-btn" id="brightnessSaveBtn" onclick="saveBrightnessMax()">Зберегти яскравість</button>' +
            '<button class="brightness-reset-btn" id="brightnessResetBtn" onclick="resetBrightnessMax()">Скинути до стандартного</button>' +
        '</div>';
}

function toggleBrightnessControls() {
    var checkbox = document.getElementById('brightnessRiskAccept');
    var controls = document.getElementById('brightnessControls');
    if (!checkbox || !controls) return;

    if (checkbox.checked) {
        controls.style.display = 'block';
    } else {
        controls.style.display = 'none';
    }
}

function updateBrightnessDisplay(value) {
    var pctEl = document.getElementById('brightnessMaxPct');
    if (pctEl) pctEl.textContent = value;
}

function saveBrightnessMax() {
    var slider = document.getElementById('brightnessMaxSlider');
    var btn = document.getElementById('brightnessSaveBtn');
    if (!slider || !btn) return;

    var value = parseInt(slider.value, 10);
    btn.disabled = true;
    btn.textContent = 'Збереження...';

    fetch('/parameter', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: 'name=brightness_max&value=' + value
    })
    .then(function(response) {
        if (!response.ok) throw new Error('Save failed');
        btn.textContent = 'Збережено!';
        brightnessData.brightness_max = value;
        setTimeout(function() {
            btn.disabled = false;
            btn.textContent = 'Зберегти яскравість';
        }, 2000);
    })
    .catch(function(err) {
        console.error('Error saving brightness:', err);
        btn.textContent = 'Помилка!';
        setTimeout(function() {
            btn.disabled = false;
            btn.textContent = 'Зберегти яскравість';
        }, 2000);
    });
}

function resetBrightnessMax() {
    var btn = document.getElementById('brightnessResetBtn');
    if (!btn) return;

    btn.disabled = true;
    btn.textContent = 'Скидання...';

    fetch('/parameter', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: 'name=brightness_max&value=0'
    })
    .then(function(response) {
        if (!response.ok) throw new Error('Reset failed');
        brightnessData.brightness_max = 0;
        renderBrightnessSection();
    })
    .catch(function(err) {
        console.error('Error resetting brightness:', err);
        btn.textContent = 'Помилка!';
        setTimeout(function() {
            btn.disabled = false;
            btn.textContent = 'Скинути до стандартного';
        }, 2000);
    });
}

// Load map data when page loads
document.addEventListener('DOMContentLoaded', () => {
    loadMapData();
    
    // Show import/export buttons once map is loaded
    const exportBtn = document.getElementById('exportBtn');
    const importBtn = document.getElementById('importBtn');
    if (exportBtn) exportBtn.style.display = 'block';
    if (importBtn) importBtn.style.display = 'block';
});

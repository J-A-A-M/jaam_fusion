// Map Editor functionality

function loadMapData() {
    fetch('/map-data')
        .then(response => response.json())
        .then(data => {
            renderMapEditor(data.regions);
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
        const data = await response.json();
        
        // Create a clean export object with only necessary fields
        const exportData = {
            regions: data.regions.map(region => ({
                id: region.id,
                name: region.name,
                sub: region.sub,
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

function importMapFromFile(event) {
    const file = event.target.files[0];
    if (!file) return;
    
    const reader = new FileReader();
    reader.onload = (e) => {
        try {
            const importData = JSON.parse(e.target.result);
            
            if (!importData.regions || !Array.isArray(importData.regions)) {
                alert('Невірний формат файлу. Очікується структура з полем "regions".');
                return;
            }
            
            // Convert leds array to leds_string format
            const regions = importData.regions.map(region => ({
                ...region,
                leds_string: (region.leds || []).join(', ')
            }));
            
            renderMapEditor(regions);
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

// Load map data when page loads
document.addEventListener('DOMContentLoaded', () => {
    loadMapData();
    
    // Show import/export buttons once map is loaded
    const exportBtn = document.getElementById('exportBtn');
    const importBtn = document.getElementById('importBtn');
    if (exportBtn) exportBtn.style.display = 'block';
    if (importBtn) importBtn.style.display = 'block';
});

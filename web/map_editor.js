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
        
        const displayName = region.sub ? 
            '&nbsp;&nbsp;&nbsp;&nbsp;' + region.name : 
            '<b>' + region.name + '</b>';
        label.innerHTML = displayName;
        
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

// Load map data when page loads
document.addEventListener('DOMContentLoaded', () => {
    loadMapData();
});

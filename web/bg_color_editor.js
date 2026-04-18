// Background Color Editor functionality

function loadColorData() {
    fetch('/bg-colors-data')
        .then(response => response.json())
        .then(data => {
            renderColorEditor(data);
        })
        .catch(err => {
            console.error('Error fetching color data:', err);
            document.getElementById('colorContent').innerHTML = 
                '<div style="color: var(--error-color); text-align: center; padding: 20px;">Помилка завантаження даних кольорів</div>';
        });
}

function renderColorEditor(data) {
    const container = document.getElementById('colorContent');
    // Clear loading message
    container.innerHTML = '';
    
    if (data.count <= 0) {
        container.innerHTML = '<div style="text-align: center; padding: 20px; color: var(--secondary-text);">Кількість LED фонової підсвітки не налаштована. Будь ласка, спочатку налаштуйте кількість LED в розділі "Апаратне забезпечення".</div>';
        return;
    }
    
    const form = document.createElement('form');
    form.id = 'colorForm';
    form.action = '/save-bg-colors';
    form.method = 'post';
    
    const info = document.createElement('div');
    const infoParagraph = document.createElement('p');
    infoParagraph.textContent = 'Налаштування індивідуальних кольорів для ' + data.count + ' LED фонової підсвітки. Чорний колір означає вимкнений LED.';
    infoParagraph.style.marginBottom = '20px';
    infoParagraph.style.color = 'var(--text-color)';
    info.appendChild(infoParagraph);
    form.appendChild(info);
    
    const grid = document.createElement('div');
    grid.className = 'led-color-grid';
    
    data.colors.forEach(colorData => {
        const item = document.createElement('div');
        item.className = 'led-color-item';
        
        const label = document.createElement('label');
        label.textContent = 'LED ' + colorData.led;
        label.setAttribute('for', 'color_' + colorData.led);
        
        const picker = document.createElement('input');
        picker.type = 'color';
        picker.id = 'color_' + colorData.led;
        picker.name = 'color_' + colorData.led;
        
        // Normalize and validate color value
        let colorValue = colorData.color;
        if (colorValue == null) {
            colorValue = '000000';
        } else {
            colorValue = String(colorValue).trim();
            // Strip leading "0x" if present
            if (colorValue.toLowerCase().startsWith('0x')) {
                colorValue = colorValue.substring(2);
            }
            // Remove any non-hex characters
            colorValue = colorValue.replace(/[^0-9a-fA-F]/g, '');
            // Pad to 6 characters or fall back to default
            if (colorValue.length > 0) {
                colorValue = colorValue.padStart(6, '0').substring(0, 6);
            } else {
                colorValue = '000000';
            }
        }
        picker.value = '#' + colorValue;
        picker.className = 'led-color-picker';
        
        item.appendChild(label);
        item.appendChild(picker);
        grid.appendChild(item);
    });
    
    form.appendChild(grid);
    container.appendChild(form);
    
    // Enable save button
    const saveBtn = document.getElementById('saveBtn');
    if (saveBtn) {
        saveBtn.disabled = false;
        saveBtn.textContent = 'Зберегти кольори';
    }
}

function saveColors() {
    const form = document.getElementById('colorForm');
    if (form) {
        const saveBtn = document.getElementById('saveBtn');
        if (saveBtn) {
            saveBtn.disabled = true;
            saveBtn.textContent = 'Збереження...';
        }
        form.submit();
    }
}

// Load color data when page loads
document.addEventListener('DOMContentLoaded', () => {
    loadColorData();
});

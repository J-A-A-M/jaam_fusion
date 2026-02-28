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
    
    if (data.count === 0) {
        container.innerHTML = '<div style="text-align: center; padding: 20px; color: var(--secondary-text);">Кількість задніх LED не налаштована. Будь ласка, спочатку налаштуйте кількість LED в розділі "Налаштування".</div>';
        return;
    }
    
    const form = document.createElement('form');
    form.id = 'colorForm';
    form.action = '/save-bg-colors';
    form.method = 'post';
    
    const info = document.createElement('div');
    info.innerHTML = '<p style="margin-bottom: 20px; color: var(--text-color);">Налаштування індивідуальних кольорів для ' + data.count + ' задніх LED. Чорний колір означає відсутність підсвітки.</p>';
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
        picker.value = '#' + colorData.color.padStart(6, '0');
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

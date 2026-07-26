interface ModeMetadata {
    id: number;
    name: string;
    usesColor: boolean;
    usesText: boolean;
    usesSpeed: boolean;
    usesRotation: boolean;
}

interface SettingsResponse {
    data?: {
        state?: boolean;
        mode?: string;
        color?: string;
        text?: string;
        brightness?: number;
        speed?: number;
        rotation?: number;
        states?: string;
        modes?: ModeMetadata[];
    };
}

let relayInput: HTMLInputElement;
let modeSelect: HTMLSelectElement;
let options: string[] = [];
let brightnessInput: HTMLInputElement;
let speedInput: HTMLInputElement;
let colorInput: HTMLInputElement;
let colorSettingRow: HTMLElement | null;
let textInput: HTMLInputElement;
let textSettingRow: HTMLElement | null;
let rotationSelect: HTMLSelectElement;
let settingsLoaded = false;
let saveTimer: number | undefined;
let modeMetadata: ModeMetadata[] = [];

document.addEventListener('DOMContentLoaded', () => {
    relayInput = document.getElementById('stateToggleid') as HTMLInputElement;
    modeSelect = document.getElementById('selectModeid') as HTMLSelectElement;
    brightnessInput = document.getElementById('brightnessid') as HTMLInputElement;
    speedInput = document.getElementById('speedid') as HTMLInputElement;
    colorInput = document.getElementById('colorid') as HTMLInputElement;
    colorSettingRow = document.getElementById('colorSettingRow');
    textInput = document.getElementById('textid') as HTMLInputElement;
    textSettingRow = document.getElementById('textSettingRow');
    rotationSelect = document.getElementById('rotationid') as HTMLSelectElement;

    bindAutoApply();
    getSettingsJSON();
});

function bindAutoApply(): void {
    relayInput.onchange = () => saveSettings();
    modeSelect.onchange = () => {
        updateSelectedPreview(modeSelect.value);
        updateVisibleParameters(modeSelect.value);
        saveSettings();
    };
    rotationSelect.onchange = () => saveSettings();
    colorInput.oninput = () => scheduleSave();
    colorInput.onchange = () => saveSettings();
    textInput.oninput = () => scheduleSave();
    textInput.onchange = () => saveSettings();
    brightnessInput.oninput = () => scheduleSave();
    brightnessInput.onchange = () => saveSettings();
    speedInput.oninput = () => scheduleSave();
    speedInput.onchange = () => saveSettings();
}

function scheduleSave(): void {
    clearTimeout(saveTimer);
    saveTimer = window.setTimeout(saveSettings, 250);
}

function saveSettings(): void {
    if (!settingsLoaded || !options.length) return;
    clearTimeout(saveTimer);

    const selectedMode = getModeInfo(modeSelect.value);
    const fallbackModeIndex = options.indexOf(modeSelect.value.trim());
    const mode = selectedMode ? selectedMode.id : (fallbackModeIndex >= 0 ? fallbackModeIndex : 0);
    const color = hexToRgb(colorInput.value).split(',');

    window.LampApp.request('settings', {
        method: 'POST',
        body: {
            state: relayInput.checked ? 1 : 0,
            mode,
            r: color[0],
            g: color[1],
            b: color[2],
            brightness: brightnessInput.value,
            rotation: rotationSelect.value || 270,
            speed: speedInput.value,
            text: textInput.value || ''
        },
        onSuccess: () => {}
    });
}

function parseSettingsJSON(jsonstring: string): void {
    const obj = JSON.parse(jsonstring) as SettingsResponse;
    relayInput.checked = !!obj.data?.state;
    modeSelect.value = obj.data?.mode || '';
    colorInput.value = rgbToHex(obj.data?.color) || '#035F59';
    textInput.value = obj.data?.text || '';
    brightnessInput.value = String(obj.data?.brightness ?? 50);
    speedInput.value = String(obj.data?.speed ?? 50);
    rotationSelect.value = String(obj.data?.rotation ?? 270);

    modeMetadata = Array.isArray(obj.data?.modes) ? obj.data?.modes || [] : [];
    options = modeMetadata.length ? modeMetadata.map((mode) => mode.name) : (obj.data?.states?.split(',') || []);

    const modeHint = document.getElementById('modeHint');
    if (modeHint && options.length) modeHint.textContent = options.join(', ');

    modeSelect.innerHTML = '';
    options.forEach((opt) => {
        const el = document.createElement('option');
        el.textContent = opt;
        el.value = opt;
        modeSelect.appendChild(el);
    });

    if (obj.data?.mode) modeSelect.value = obj.data.mode;
    fillModePreviews(options, modeSelect.value);
    updateVisibleParameters(modeSelect.value);
    settingsLoaded = true;
}

function getModeInfo(mode: string): ModeMetadata | undefined {
    const normalizedMode = (mode || '').toLowerCase();
    return modeMetadata.find((item) => (item.name || '').toLowerCase() === normalizedMode);
}

function usesColor(mode: string): boolean {
    const info = getModeInfo(mode);
    return info ? !!info.usesColor : ['color', 'parts', 'text'].includes((mode || '').toLowerCase());
}

function usesText(mode: string): boolean {
    const info = getModeInfo(mode);
    return info ? !!info.usesText : (mode || '').toLowerCase() === 'text';
}

function usesSpeed(mode: string): boolean {
    const info = getModeInfo(mode);
    return !info || !!info.usesSpeed;
}

function usesRotation(mode: string): boolean {
    const info = getModeInfo(mode);
    return !info || !!info.usesRotation;
}

function updateVisibleParameters(mode: string): void {
    if (colorSettingRow) colorSettingRow.style.display = usesColor(mode) ? '' : 'none';
    if (textSettingRow) textSettingRow.style.display = usesText(mode) ? '' : 'none';
    const speedRow = speedInput.closest('.row') as HTMLElement | null;
    if (speedRow) speedRow.style.display = usesSpeed(mode) ? '' : 'none';
    const rotationRow = rotationSelect.closest('.row') as HTMLElement | null;
    if (rotationRow) rotationRow.style.display = usesRotation(mode) ? '' : 'none';
}

function modeClass(mode: string): string {
    return `mode-${mode.toLowerCase().replace(/[^a-z0-9]+/g, '-')}`;
}

function fillModePreviews(modes: string[], selectedMode: string): void {
    const grid = document.getElementById('modePreviewGrid');
    if (!grid || !modes.length) return;
    grid.innerHTML = '';

    modes.forEach((mode) => {
        const button = document.createElement('button');
        button.type = 'button';
        button.className = `mode-preview ${modeClass(mode)}`;
        button.dataset.mode = mode;
        button.onclick = () => {
            modeSelect.value = mode;
            updateSelectedPreview(mode);
            updateVisibleParameters(mode);
            saveSettings();
        };

        const sample = document.createElement('span');
        sample.className = 'mode-preview-sample';
        for (let i = 0; i < 16; i++) {
            sample.appendChild(document.createElement('i'));
        }

        const label = document.createElement('span');
        label.className = 'mode-preview-label';
        label.textContent = mode;

        button.appendChild(sample);
        button.appendChild(label);
        grid.appendChild(button);
    });

    updateSelectedPreview(selectedMode);
}

function updateSelectedPreview(selectedMode: string): void {
    document.querySelectorAll<HTMLElement>('.mode-preview').forEach((preview) => {
        preview.classList.toggle('selected', preview.dataset.mode === selectedMode);
    });
}

function getSettingsJSON(): void {
    window.LampApp.request('settings', {
        method: 'GET',
        onSuccess: (settings) => {
            try {
                parseSettingsJSON(settings);
            } catch (error) {
                console.error(error);
            }
        }
    });
}

function hexToRgb(hex: string): string {
    const result = /^#?([a-f\d]{2})([a-f\d]{2})([a-f\d]{2})$/i.exec(hex);
    return result ? `${parseInt(result[1], 16)},${parseInt(result[2], 16)},${parseInt(result[3], 16)}` : '0,0,0';
}

function componentToHex(c: number): string {
    const hex = c.toString(16);
    return hex.length === 1 ? `0${hex}` : hex;
}

function rgbToHex(rgb?: string): string | null {
    if (!rgb) return null;
    const [r, g, b] = rgb.split(',').map((x) => Number(x));
    return `#${componentToHex(r)}${componentToHex(g)}${componentToHex(b)}`;
}

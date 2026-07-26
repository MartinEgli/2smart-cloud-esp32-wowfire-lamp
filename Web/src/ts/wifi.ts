interface WifiStation {
    encType?: string;
    signal?: string;
}

let connectedWifi = '';
let refreshButton: HTMLInputElement;
let wifiTable: HTMLTableElement;

document.addEventListener('DOMContentLoaded', () => {
    refreshButton = document.getElementById('refreshbtn') as HTMLInputElement;
    wifiTable = document.getElementById('myTable') as HTMLTableElement;
    refreshButton.onclick = refresh;
    getConnectedWifi();
    getWifi();
    window.setTimeout(getWifi, 3000);
});

function refresh(): void {
    refreshButton.disabled = true;
    clearTable();
    getWifi();
}

function clearTable(): void {
    for (let i = wifiTable.rows.length; i > 1; i--) wifiTable.deleteRow(i - 1);
}

function getWifi(): void {
    clearTable();
    try {
        window.LampApp.request('scan/v2', {
            method: 'GET',
            onSuccess: (stations) => {
                getConnectedWifi();
                try {
                    if (stations) tableFill(Object.entries(JSON.parse(stations) as Record<string, WifiStation>));
                } catch (error) {
                    console.error(error);
                }
            }
        });
    } catch (error) {
        console.error(error);
    }
}

function getConnectedWifi(): void {
    try {
        window.LampApp.request('connectedwifi', {
            method: 'GET',
            onSuccess: (wifi) => {
                connectedWifi = wifi;
            }
        });
    } catch (error) {
        console.error(error);
    }
}

function tableFill(stations: Array<[string, WifiStation]>): void {
    if (!stations.length) return;

    stations.forEach(([ssid, station]) => {
        const row = wifiTable.insertRow(-1);
        if (connectedWifi === ssid) row.classList.add('connectedWifi');
        const ssidCell = row.insertCell(0);
        const securedCell = row.insertCell(1);
        const signalCell = row.insertCell(2);

        ssidCell.innerHTML = ssid;
        securedCell.dataset.open = station?.encType === '0' ? 'true' : 'false';
        securedCell.innerHTML = securedCell.dataset.open === 'true'
            ? window.LampApp.t('wifi.open')
            : window.LampApp.t('wifi.secured');
        signalCell.innerHTML = `${station?.signal}%`;
    });

    addRowHandlers();
}

function addRowHandlers(): void {
    for (let i = 1; i < wifiTable.rows.length; i++) {
        const currentRow = wifiTable.rows[i];
        currentRow.onclick = () => {
            const ssid = currentRow.getElementsByTagName('td')[0].innerHTML;
            const securedCell = currentRow.getElementsByTagName('td')[1];
            let pass = '';

            if (securedCell.dataset.open !== 'true') {
                const promptedPassword = prompt(window.LampApp.t('alerts.inputPassword', { ssid }));
                if (promptedPassword == null) return;
                pass = promptedPassword;
            }

            window.LampApp.request('setwifi', { method: 'POST', body: { ssid, pass } });
        };
    }
}


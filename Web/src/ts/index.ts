interface CredentialsInputs {
    mail: HTMLInputElement;
    token: HTMLInputElement;
    hostname: HTMLInputElement;
    brokerport: HTMLInputElement;
    product: HTMLInputElement;
    device: HTMLInputElement;
}

let inputs: CredentialsInputs;

document.addEventListener('DOMContentLoaded', () => {
    const saveButton = document.getElementById('btn_save') as HTMLInputElement | null;
    if (saveButton) saveButton.onclick = save;

    inputs = {
        mail: document.getElementById('mailid') as HTMLInputElement,
        token: document.getElementById('tokenid') as HTMLInputElement,
        hostname: document.getElementById('hostnameid') as HTMLInputElement,
        brokerport: document.getElementById('brokerportid') as HTMLInputElement,
        product: document.getElementById('productid') as HTMLInputElement,
        device: document.getElementById('deviceid') as HTMLInputElement
    };

    Array.from(document.getElementsByClassName('accordion')).forEach((accordion) => {
        accordion.addEventListener('click', function(this: HTMLElement) {
            this.classList.toggle('active');
            const panel = this.nextElementSibling as HTMLElement | null;
            if (!panel) return false;
            panel.style.maxHeight = panel.style.maxHeight ? '' : `${panel.scrollHeight}px`;
            return false;
        });
    });
});

function save(): void {
    try {
        if (!confirm(window.LampApp.t('alerts.applyChanges'))) return;
        const credentials = getCredentials();
        window.LampApp.request('setcredentials', {
            method: 'POST',
            body: {
                mail: credentials.mail,
                token: credentials.token,
                hostname: credentials.hostname,
                brokerPort: credentials.brokerport,
                productId: credentials.productid,
                deviceId: credentials.deviceid
            }
        });
    } catch (error) {
        console.error(error);
        alert(error instanceof Error ? error.message : String(error));
    }
}

function getCredentials() {
    return {
        mail: inputs.mail.value,
        token: inputs.token.value,
        hostname: inputs.hostname.value,
        brokerport: inputs.brokerport.value,
        productid: inputs.product.value,
        deviceid: inputs.device.value
    };
}


const maxFileSize = 2048 * 1024;
const maxFileSizeLabel = '2048KB';
const availableFirmwareTypes = [
    'application/mac-binary',
    'application/macbinary',
    'application/octet-stream',
    'application/x-binary',
    'application/x-macbinary'
];

document.addEventListener('DOMContentLoaded', () => {
    bindButton('pwchange', pwchange);
    bindButton('reset', reset);
    bindButton('reboot', reboot);
    bindButton('addUser', addUser);
    getUsers();

    const submitButton = document.getElementById('submitbtn') as HTMLButtonElement;
    submitButton.disabled = true;
    submitButton.classList.add('disabledBtn');

    const fileInput = document.getElementById('newfile') as HTMLInputElement;
    fileInput.onchange = upload;

    const form = document.querySelector('form') as HTMLFormElement | null;
    form?.addEventListener('submit', (event) => {
        event.preventDefault();
        const formData = new FormData(form);
        fetch(form.action, { method: 'POST', body: formData })
            .then((response) => {
                if (response.status === 200) alert(window.LampApp.t('alerts.firmwareUploaded'));
                else alert(`${window.LampApp.t('alerts.error')}\n${response.statusText}`);
            });
    });
});

function bindButton(id: string, handler: () => void): void {
    const button = document.getElementById(id);
    if (button) button.onclick = handler;
}

function pwchange(): void {
    const newPassword = prompt(window.LampApp.t('alerts.newPassword'));
    if (!newPassword) return;
    window.LampApp.request('newauthpass', { method: 'POST', body: { newpass: newPassword } });
}

function reset(): void {
    if (!confirm(window.LampApp.t('alerts.resetConfirm'))) return;
    window.LampApp.request('resetdefault', {
        method: 'GET',
        onSuccess: () => alert(window.LampApp.t('alerts.resetSuccess'))
    });
}

function reboot(): void {
    if (!confirm(window.LampApp.t('alerts.rebootConfirm'))) return;
    window.LampApp.request('reboot', {
        method: 'GET',
        onSuccess: () => alert(window.LampApp.t('alerts.rebooting'))
    });
}

function upload(): void {
    const submitButton = document.getElementById('submitbtn') as HTMLButtonElement;
    const fileInput = document.getElementById('newfile') as HTMLInputElement;
    const fileLabel = document.getElementById('file-label');
    const files = fileInput.files;

    submitButton.disabled = true;
    submitButton.classList.add('disabledBtn');

    if (!files || files.length === 0) {
        alert(window.LampApp.t('alerts.fileRequired'));
    } else if (files[0].size > maxFileSize) {
        alert(window.LampApp.t('alerts.fileSize', { size: maxFileSizeLabel }));
    } else if (!availableFirmwareTypes.includes(files[0].type)) {
        alert(window.LampApp.t('alerts.fileBin'));
    } else {
        submitButton.disabled = false;
        submitButton.classList.remove('disabledBtn');
        if (fileLabel) fileLabel.innerHTML = files[0].name;
    }
}

function getUsers(): void {
    window.LampApp.request('auth/users', {
        method: 'GET',
        onSuccess: (response) => {
            try {
                fillUsersTable((JSON.parse(response).users || []) as string[]);
            } catch (error) {
                console.error(error);
            }
        }
    });
}

function fillUsersTable(users: string[]): void {
    const table = document.getElementById('usersTable') as HTMLTableElement;
    for (let i = table.rows.length; i > 1; i--) table.deleteRow(i - 1);

    users.forEach((username) => {
        const row = table.insertRow(-1);
        row.insertCell(0).innerHTML = username;
        const actionCell = row.insertCell(1);
        const button = document.createElement('a');
        button.href = '#';
        button.className = 'btn btn-warning system-btn';
        button.innerHTML = window.LampApp.t('actions.delete');
        button.onclick = () => deleteUser(username);
        actionCell.appendChild(button);
    });
}

function addUser(): void {
    const usernameInput = document.getElementById('newUsername') as HTMLInputElement;
    const passwordInput = document.getElementById('newUserPassword') as HTMLInputElement;
    const username = usernameInput.value.trim();
    const password = passwordInput.value;

    window.LampApp.request('auth/users', {
        method: 'POST',
        body: { username, password },
        onSuccess: () => {
            usernameInput.value = '';
            passwordInput.value = '';
            getUsers();
        }
    });
}

function deleteUser(username: string): void {
    window.LampApp.request('auth/users/delete', {
        method: 'POST',
        body: { username },
        onSuccess: getUsers
    });
}


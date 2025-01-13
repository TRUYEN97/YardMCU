async function fetchClients() {
    const response = await fetch('/clients');
    const clients = await response.json();
    const clientList = document.getElementById('client-list');
    clientList.innerHTML = '';
    clients.forEach(client => {
        const li = document.createElement('li');
        li.textContent = client;
        clientList.appendChild(li);
    });
}

document.getElementById('disconnect-form').addEventListener('submit', async (event) => {
    event.preventDefault();
    const username = document.getElementById('disconnect-username').value;
    const response = await fetch('/disconnect', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ username })
    });
    const result = await response.json();
    alert(result.status === 'success' ? 'Client disconnected' : result.message);
    fetchClients();
});

document.getElementById('update-form').addEventListener('submit', async (event) => {
    event.preventDefault();
    const maxClients = parseInt(document.getElementById('max-clients').value, 10);
    const response = await fetch('/update_max_clients', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ max_clients: maxClients })
    });
    const result = await response.json();
    alert(result.status === 'success' ? 'Max clients updated' : result.message);
});

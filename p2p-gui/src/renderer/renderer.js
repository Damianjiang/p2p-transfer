let connected = false;
let mode = 'host';

function init() {
    setupTabs();
    setupActions();
    setupDragDrop();
    setupChat();
}

function setupTabs() {
    document.querySelectorAll('.tab').forEach(tab => {
        tab.addEventListener('click', () => {
            document.querySelectorAll('.tab').forEach(t => t.classList.remove('active'));
            tab.classList.add('active');
            mode = tab.dataset.tab;
            
            document.querySelector('.host-only').style.display = mode === 'host' ? 'block' : 'none';
            document.querySelector('.connect-only').style.display = mode === 'connect' ? 'block' : 'none';
            document.getElementById('actionBtn').textContent = mode === 'host' ? 'Start' : 'Connect';
        });
    });
}

function setupActions() {
    const btn = document.getElementById('actionBtn');
    btn.addEventListener('click', handleAction);
}

async function handleAction() {
    const username = document.getElementById('username').value.trim();
    const password = document.getElementById('password').value.trim();
    
    if (!username || !password) {
        return;
    }
    
    if (connected) {
        await window.p2pAPI.stopP2P();
        connected = false;
        updateUI();
        return;
    }
    
    const config = { password, username };
    
    if (mode === 'host') {
        config.port = parseInt(document.getElementById('port').value) || 8888;
        const result = await window.p2pAPI.startServer(config);
        if (result.success) {
            connected = true;
            updateUI();
        }
    } else {
        config.ip = document.getElementById('address').value.trim();
        config.port = parseInt(document.getElementById('port').value) || 8888;
        if (!config.ip) return;
        
        const result = await window.p2pAPI.connectClient(config);
        if (result.success) {
            connected = true;
            updateUI();
        }
    }
}

function updateUI() {
    const btn = document.getElementById('actionBtn');
    const chatInput = document.getElementById('chatInput');
    const sendBtn = document.getElementById('sendBtn');
    
    if (connected) {
        btn.textContent = 'Disconnect';
        chatInput.disabled = false;
        sendBtn.disabled = false;
    } else {
        btn.textContent = mode === 'host' ? 'Start' : 'Connect';
        chatInput.disabled = true;
        sendBtn.disabled = true;
    }
}

function setupDragDrop() {
    const zone = document.getElementById('dropZone');
    
    zone.addEventListener('dragover', e => {
        e.preventDefault();
        zone.classList.add('drag-over');
    });
    
    zone.addEventListener('dragleave', () => {
        zone.classList.remove('drag-over');
    });
    
    zone.addEventListener('drop', e => {
        e.preventDefault();
        zone.classList.remove('drag-over');
        
        if (!connected) return;
        
        const files = Array.from(e.dataTransfer.files).map(f => f.path).filter(Boolean);
        if (files.length > 0) {
            files.forEach(file => {
                window.p2pAPI.sendCommand('send ' + file);
            });
        }
    });
    
    zone.addEventListener('click', async () => {
        if (!connected) return;
        const files = await window.p2pAPI.selectFile();
        if (files && files.length > 0) {
            files.forEach(file => {
                window.p2pAPI.sendCommand('send ' + file);
            });
        }
    });
}

function setupChat() {
    const input = document.getElementById('chatInput');
    const btn = document.getElementById('sendBtn');
    
    btn.addEventListener('click', sendMessage);
    input.addEventListener('keypress', e => {
        if (e.key === 'Enter') sendMessage();
    });
    
    window.p2pAPI.onP2POutput(data => {
        if (data.msg && data.msg.includes('[Peer]:')) {
            const msg = data.msg.replace(/\[.*?\]:\s*/, '').trim();
            if (msg) addMessage(msg, 'received');
        }
    });
    
    window.p2pAPI.onP2PStatus(data => {
        if (!data.connected) {
            connected = false;
            updateUI();
        }
    });
}

function sendMessage() {
    const input = document.getElementById('chatInput');
    const msg = input.value.trim();
    if (!msg) return;
    
    window.p2pAPI.sendCommand('msg ' + msg);
    addMessage(msg, 'sent');
    input.value = '';
}

function addMessage(text, type) {
    const container = document.getElementById('chatMessages');
    const msg = document.createElement('div');
    msg.className = 'message ' + type;
    msg.innerHTML = '<div class="message-bubble">' + escapeHtml(text) + '</div>';
    container.appendChild(msg);
    container.scrollTop = container.scrollHeight;
}

function escapeHtml(text) {
    const div = document.createElement('div');
    div.textContent = text;
    return div.innerHTML;
}

document.addEventListener('DOMContentLoaded', init);

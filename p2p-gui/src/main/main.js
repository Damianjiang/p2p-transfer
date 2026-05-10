const { app, BrowserWindow, ipcMain, dialog } = require('electron');
const path = require('path');
const { spawn } = require('child_process');
const fs = require('fs');
const os = require('os');

let mainWindow = null;
let p2pProcess = null;

function createWindow() {
  if (mainWindow && !mainWindow.isDestroyed()) {
    mainWindow.focus();
    return;
  }

  mainWindow = new BrowserWindow({
    width: 960,
    height: 680,
    minWidth: 800,
    minHeight: 560,
    webPreferences: {
      preload: path.join(__dirname, '../preload/preload.js'),
      contextIsolation: true,
      nodeIntegration: false,
      sandbox: true
    },
    backgroundColor: '#ffffff',
    show: false
  });

  mainWindow.loadFile(path.join(__dirname, '../renderer/index.html'));

  mainWindow.once('ready-to-show', () => {
    mainWindow.show();
  });

  mainWindow.on('closed', () => {
    mainWindow = null;
  });
}

function getP2PBinaryPath() {
  const platform = process.platform;
  let binaryName = platform === 'win32' ? 'p2p_transfer.exe' : 'p2p_transfer';
  
  const possiblePaths = [
    path.join(__dirname, '../../../p2p_transfer', binaryName),
    path.join(app.getPath('exe'), '../Resources/bin', platform, binaryName),
    path.join(process.resourcesPath || '', 'bin', platform, binaryName)
  ];

  for (const p of possiblePaths) {
    if (fs.existsSync(p)) {
      return p;
    }
  }

  return path.join(__dirname, '../../../p2p_transfer', binaryName);
}

function startP2PServer(port, password, username) {
  return new Promise((resolve, reject) => {
    if (p2pProcess) {
      reject(new Error('P2P already running'));
      return;
    }

    const binaryPath = getP2PBinaryPath();
    
    if (!fs.existsSync(binaryPath)) {
      reject(new Error('P2P binary not found'));
      return;
    }

    try {
      p2pProcess = spawn(binaryPath, [
        '-l', String(port),
        '-P', password,
        '-u', username
      ], {
        stdio: ['pipe', 'pipe', 'pipe'],
        detached: false
      });
    } catch (err) {
      reject(err);
      return;
    }

    let ready = false;

    p2pProcess.stdout.on('data', (data) => {
      const output = data.toString();
      if (!ready && output.includes('[+]')) {
        ready = true;
        resolve({ success: true, mode: 'host' });
      }
      sendToRenderer('log', { type: 'info', msg: output });
    });

    p2pProcess.stderr.on('data', (data) => {
      sendToRenderer('log', { type: 'error', msg: data.toString() });
    });

    p2pProcess.on('error', (err) => {
      p2pProcess = null;
      reject(err);
    });

    p2pProcess.on('close', () => {
      p2pProcess = null;
      sendToRenderer('status', { connected: false });
    });

    setTimeout(() => {
      if (!ready) {
        ready = true;
        resolve({ success: true, mode: 'host' });
      }
    }, 1500);
  });
}

function connectToPeer(ip, port, password, username) {
  return new Promise((resolve, reject) => {
    if (p2pProcess) {
      reject(new Error('P2P already running'));
      return;
    }

    const binaryPath = getP2PBinaryPath();
    
    if (!fs.existsSync(binaryPath)) {
      reject(new Error('P2P binary not found'));
      return;
    }

    try {
      p2pProcess = spawn(binaryPath, [
        '-c', ip,
        '-p', String(port),
        '-P', password,
        '-u', username
      ], {
        stdio: ['pipe', 'pipe', 'pipe'],
        detached: false
      });
    } catch (err) {
      reject(err);
      return;
    }

    let ready = false;

    p2pProcess.stdout.on('data', (data) => {
      const output = data.toString();
      if (!ready && output.includes('[+]')) {
        ready = true;
        resolve({ success: true, mode: 'client' });
      }
      sendToRenderer('log', { type: 'info', msg: output });
    });

    p2pProcess.stderr.on('data', (data) => {
      sendToRenderer('log', { type: 'error', msg: data.toString() });
    });

    p2pProcess.on('error', (err) => {
      p2pProcess = null;
      reject(err);
    });

    p2pProcess.on('close', () => {
      p2pProcess = null;
      sendToRenderer('status', { connected: false });
    });

    setTimeout(() => {
      if (!ready) {
        ready = true;
        resolve({ success: true, mode: 'client' });
      }
    }, 2000);
  });
}

function sendToRenderer(channel, data) {
  if (mainWindow && !mainWindow.isDestroyed()) {
    mainWindow.webContents.send(channel, data);
  }
}

function stopP2P() {
  if (p2pProcess) {
    p2pProcess.kill('SIGTERM');
    p2pProcess = null;
    return true;
  }
  return false;
}

function selectFiles() {
  return new Promise((resolve) => {
    if (!mainWindow || mainWindow.isDestroyed()) {
      resolve([]);
      return;
    }

    dialog.showOpenDialog(mainWindow, {
      properties: ['openFile', 'multiSelections'],
      filters: [{ name: 'All Files', extensions: ['*'] }]
    }).then(result => {
      resolve(result.canceled ? [] : result.filePaths);
    }).catch(() => {
      resolve([]);
    });
  });
}

function getSystemInfo() {
  return {
    platform: process.platform,
    arch: process.arch,
    hostname: os.hostname(),
    version: app.getVersion()
  };
}

ipcMain.handle('start-server', async (event, { port, password, username }) => {
  try {
    if (!port || !password || !username) {
      return { success: false, error: 'Missing parameters' };
    }
    return await startP2PServer(port, password, username);
  } catch (error) {
    return { success: false, error: error.message };
  }
});

ipcMain.handle('connect-client', async (event, { ip, port, password, username }) => {
  try {
    if (!ip || !port || !password || !username) {
      return { success: false, error: 'Missing parameters' };
    }
    return await connectToPeer(ip, port, password, username);
  } catch (error) {
    return { success: false, error: error.message };
  }
});

ipcMain.handle('stop-p2p', () => {
  return stopP2P();
});

ipcMain.handle('select-file', selectFiles);

ipcMain.handle('get-system-info', getSystemInfo);

ipcMain.on('send-command', (event, command) => {
  if (p2pProcess && p2pProcess.stdin) {
    p2pProcess.stdin.write(command + '\n');
  }
});

app.whenReady().then(() => {
  createWindow();

  app.on('activate', () => {
    if (BrowserWindow.getAllWindows().length === 0) {
      createWindow();
    }
  });
});

app.on('window-all-closed', () => {
  stopP2P();
  if (process.platform !== 'darwin') {
    app.quit();
  }
});

app.on('before-quit', () => {
  stopP2P();
});

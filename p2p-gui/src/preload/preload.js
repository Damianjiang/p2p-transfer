const { contextBridge, ipcRenderer } = require('electron');

let p2pOutputCallback = null;
let p2pStatusCallback = null;

contextBridge.exposeInMainWorld('p2pAPI', {
  startServer: (config) => {
    if (!config || typeof config.port !== 'number' || !config.password || !config.username) {
      return Promise.resolve({ success: false, error: 'Invalid config' });
    }
    return ipcRenderer.invoke('start-server', config);
  },

  connectClient: (config) => {
    if (!config || !config.ip || typeof config.port !== 'number' || !config.password || !config.username) {
      return Promise.resolve({ success: false, error: 'Invalid config' });
    }
    return ipcRenderer.invoke('connect-client', config);
  },

  stopP2P: () => ipcRenderer.invoke('stop-p2p'),

  selectFile: () => ipcRenderer.invoke('select-file'),

  getSystemInfo: () => ipcRenderer.invoke('get-system-info'),

  onP2POutput: (callback) => {
    if (typeof callback === 'function') {
      p2pOutputCallback = callback;
      ipcRenderer.on('log', (event, data) => {
        if (p2pOutputCallback) {
          p2pOutputCallback(data);
        }
      });
    }
  },

  onP2PStatus: (callback) => {
    if (typeof callback === 'function') {
      p2pStatusCallback = callback;
      ipcRenderer.on('status', (event, data) => {
        if (p2pStatusCallback) {
          p2pStatusCallback(data);
        }
      });
    }
  },

  removeAllListeners: (channel) => {
    ipcRenderer.removeAllListeners(channel);
  }
});

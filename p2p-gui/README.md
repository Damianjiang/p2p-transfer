# P2P Transfer

Simple peer-to-peer file transfer.

## Download

Get the latest release from [GitHub Releases](https://github.com/Damianjiang/p2p-transfer/releases).

## Build from Source

```bash
# Clone
git clone https://github.com/Damianjiang/p2p-transfer.git
cd p2p-transfer

# Install dependencies
cd p2p-gui
npm install

# Run in dev mode
npm start

# Build for current platform
npm run build
```

## Features

- Direct P2P connection
- File transfer
- Instant messaging
- Password protection
- Cross-platform (macOS, Windows, Linux)

## Usage

### Host Mode
1. Enter username and password
2. Set port (default: 8888)
3. Click "Start"
4. Share address and password with peer

### Connect Mode
1. Enter username and password
2. Enter peer's address and port
3. Click "Connect"

### Transfer Files
- Drag and drop files onto the drop zone
- Or click the zone to select files

### Send Messages
- Type in the message input
- Press Enter or click Send

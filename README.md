# P2P Transfer - Direct Peer-to-Peer File Transfer

A simple, serverless P2P file transfer application written in C.

## Features

- Direct peer-to-peer connection (no server required)
- NAT traversal support for connections behind routers
- Password-protected rooms
- File transfer with progress display
- Text messaging between peers
- Cross-platform (Linux, macOS, Windows with WSL)

## Building

```bash
cd p2p_transfer
make
```

## Usage

### Host Mode (Listen for connections)

```bash
./p2p_transfer -l 8888 -P mypassword
```

Options:
- `-l 8888` - Listen on port 8888
- `-P mypassword` - Room password

### Client Mode (Connect to host)

```bash
./p2p_transfer -c 192.168.1.100 -p 8888 -P mypassword
```

Options:
- `-c 192.168.1.100` - Host IP address
- `-p 8888` - Host port
- `-P mypassword` - Room password

## Commands

Once connected, use these commands:

```
send <file>     - Send a file to the peer
msg <text>      - Send a text message
info            - Show connection information
discover        - Discover peer network info
help            - Show available commands
quit            - Exit
```

## Examples

### Local Network Transfer

**Host:**
```bash
./p2p_transfer -l 8888 -P secret123
```

**Client:**
```bash
./p2p_transfer -c 192.168.1.100 -p 8888 -P secret123
```

### Internet Transfer (Port Forwarding Required)

**Host (behind router):**
1. Configure port forwarding on your router for port 8888
2. Find your public IP (whatismyip.com)
3. Run: `./p2p_transfer -l 8888 -P secret123`

**Client:**
```bash
./p2p_transfer -c <public_ip> -p 8888 -P secret123
```

### With VPN

If both peers use the same VPN (WireGuard, OpenVPN, etc.), use the VPN IP addresses:

```bash
./p2p_transfer -l 8888 -P secret123  # On VPN host
./p2p_transfer -c 10.0.0.1 -p 8888 -P secret123  # On VPN client
```

## NAT Traversal

The program supports basic NAT traversal strategies:

1. **Direct Connection** - Both peers on public IP or same LAN
2. **Port Forwarding** - Host configures router to forward ports
3. **VPN/Proxy** - Both peers on same private network (VPN)

For best results with strict NATs:
- Use port forwarding on the host's router
- Or use a VPN service to create a virtual LAN

## Security

- Password authentication between peers
- No data encryption (use VPN for security)
- Password is sent in plain text

## Requirements

- GCC or Clang compiler
- POSIX threads (pthread)
- Linux, macOS, or Windows with WSL/cygwin

## Troubleshooting

### Connection Refused
- Check firewall settings
- Verify port is correct
- Ensure host is running and reachable

### Connection Timeout
- Check network connectivity
- Verify NAT/firewall allows outbound connections
- Try with VPN if behind strict NAT

### File Transfer Fails
- Check disk space on receiver
- Verify file permissions
- Try with smaller files first

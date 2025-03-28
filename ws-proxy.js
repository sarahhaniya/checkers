const WebSocket = require('ws');
const net = require('net');

const WS_PORT = 3000; // Browser will connect to this
const TCP_HOST = 'localhost';
const TCP_PORT = 8080; // Your C++ server

const wss = new WebSocket.Server({ port: WS_PORT });

console.log(`WebSocket proxy listening on ws://localhost:${WS_PORT}`);

wss.on('connection', (ws) => {
  console.log('WebSocket client connected');

  // Create a TCP socket to the C++ server
  const tcpSocket = net.createConnection({ host: TCP_HOST, port: TCP_PORT }, () => {
    console.log('Connected to TCP backend');
  });

  // When TCP server sends data, forward to WebSocket client
  tcpSocket.on('data', (data) => {
    ws.send(data.toString());
  });

  // When WebSocket client sends message, forward to TCP server
  ws.on('message', (message) => {
    tcpSocket.write(message + '\n'); 
  });

  // Handle disconnects
  ws.on('close', () => {
    console.log('WebSocket client disconnected');
    tcpSocket.end();
  });

  tcpSocket.on('close', () => {
    console.log('TCP server disconnected');
    ws.close();
  });

  tcpSocket.on('error', (err) => {
    console.error('TCP error:', err.message);
    ws.close();
  });

  ws.on('error', (err) => {
    console.error('WebSocket error:', err.message);
    tcpSocket.end();
  });
});

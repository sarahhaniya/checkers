import { useEffect, useRef, useState, useCallback } from "react";

const useWebSocket = (url, onMessageCallback) => {
  const socketRef = useRef(null);
  const [status, setStatus] = useState('disconnected');

  const sendMessage = useCallback((message) => {
    if (socketRef.current && socketRef.current.readyState === WebSocket.OPEN) {
      socketRef.current.send(message);
    } else {
      console.error('WebSocket is not open');
    }
  }, []);

  useEffect(() => {
    // Create WebSocket connection
    const ws = new WebSocket(url.replace('http', 'ws'));

    ws.onopen = () => {
      setStatus('connected');
      console.log('WebSocket Connected');
    };

    ws.onmessage = (event) => {
      try {
        const data = JSON.parse(event.data);
        
        // Call the callback with parsed data
        if (onMessageCallback) {
          onMessageCallback(data);
        }
      } catch (error) {
        console.error('Error parsing message:', error);
      }
    };

    ws.onclose = () => {
      setStatus('disconnected');
      console.log('WebSocket Disconnected');
    };

    ws.onerror = (error) => {
      setStatus('error');
      console.error('WebSocket Error:', error);
    };

    socketRef.current = ws;

    // Cleanup on unmount
    return () => {
      ws.close();
    };
  }, [url, onMessageCallback]);

  return { sendMessage, status };
};

export default useWebSocket;
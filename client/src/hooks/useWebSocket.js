import { useEffect, useRef, useState } from "react";

const useWebSocket = (url, onMessage, player, gameId) => {
  const socketRef = useRef(null);
  const [connectionStatus, setConnectionStatus] = useState("disconnected");
  const reconnectTimeoutRef = useRef(null);

  useEffect(() => {
    // Clear any existing reconnect timeout if component re-renders
    if (reconnectTimeoutRef.current) {
      clearTimeout(reconnectTimeoutRef.current);
    }

    const connectWebSocket = () => {
      // Make sure we're using the correct URL
      socketRef.current = new WebSocket(url);
      setConnectionStatus("connecting");

      socketRef.current.onopen = () => {
        console.log("Connected to server");
        setConnectionStatus("connected");
         // Rejoin game automatically if player is still in state
  if (gameId && player) {
    socketRef.current.send(`join ${gameId}`);
    console.log(`[WebSocket] Rejoining game ${gameId} as ${player}`);
  }
      };

      socketRef.current.onmessage = (event) => {
        console.log("RAW WebSocket message from server:", event.data);
        onMessage(event.data);
      };

      socketRef.current.onclose = (event) => {
        console.log("Disconnected from server", event.code, event.reason);
        setConnectionStatus("disconnected");

        // Auto reconnect after 3 seconds if not intentionally closed
        if (!event.wasClean) {
          console.log("Attempting to reconnect in 3 seconds...");
          reconnectTimeoutRef.current = setTimeout(() => {
            connectWebSocket();
          }, 3000);
        }
      };

      socketRef.current.onerror = (error) => {
        console.error("WebSocket error:", error);
        setConnectionStatus("error");
      };
    };
    connectWebSocket();

    return () => {
      if (socketRef.current) {
        socketRef.current.close();
      }
      if (reconnectTimeoutRef.current) {
        clearTimeout(reconnectTimeoutRef.current);
      }
    };
  }, [url, onMessage]);

  const sendMessage = (msg) => {
    if (socketRef.current?.readyState === WebSocket.OPEN) {
      console.log("[WebSocket] Sending:", msg);  
      socketRef.current.send(msg);
      return true;
    } else {
      console.warn("WebSocket not ready. Status:", connectionStatus);
      return false;
    }
  };

  return { sendMessage, status: connectionStatus };
};

export default useWebSocket;

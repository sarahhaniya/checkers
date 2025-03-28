import { useEffect, useRef } from "react";

const useWebSocket = (onMessage) => {
  const socketRef = useRef(null);

  useEffect(() => {
    socketRef.current = new WebSocket("ws://localhost:3000");

    socketRef.current.onopen = () => {
      console.log("Connected to server");
    };

    socketRef.current.onmessage = (event) => {
      console.log("Received:", event.data);
      onMessage(event.data);
    };

    socketRef.current.onclose = () => {
      console.log("Disconnected from server");
    };

    return () => {
      socketRef.current.close();
    };
  }, [onMessage]);

  const sendMessage = (msg) => {
    if (socketRef.current?.readyState === WebSocket.OPEN) {
      socketRef.current.send(msg);
    } else {
      console.warn("WebSocket not ready");
    }
  };

  return sendMessage;
};

export default useWebSocket;

import { useState, useCallback } from "react";

const useGameState = () => {
  const [player, setPlayer] = useState(null);
  const [gameId, setGameId] = useState(null);
  const [board, setBoard] = useState(
    Array(8)
      .fill()
      .map(() => Array(8).fill(""))
  );
  const [messages, setMessages] = useState([]);
  const [currentPlayer, setCurrentPlayer] = useState(null);
  const [gameStatus, setGameStatus] = useState("waiting");

  const addMessage = (message) => {
    setMessages((prev) => [...prev, message]);
  };

  const updateFromServer = useCallback((data) => {
    console.log("Received server data:", data);

    try {
      switch (data.type) {
        case "login_success":
          setPlayer(data.username);
          addMessage(`Logged in as ${data.username}`);
          break;

        case "register_success":
          addMessage("Registration successful");
          break;

        case "game_created":
          setGameId(data.gameId);
          addMessage(`Game created. Invite code: ${data.gameCode}`);
          setGameStatus("waiting");
          break;

        case "game_joined":
          setGameId(data.gameId);
          addMessage(`Joined game ${data.gameId}`);
          setGameStatus("playing");
          break;

        case "move_success":
          addMessage(`Move made from ${data.from} to ${data.to}`);
          // Update board state if needed
          break;

        case "error":
          addMessage(`Error: ${data.message}`);
          break;

        default:
          console.log('Unhandled message type:', data);
          addMessage(`Received: ${JSON.stringify(data)}`);
      }
    } catch (err) {
      console.error("Failed to process server message:", err);
      addMessage(`Error processing message: ${err.message}`);
    }
  }, []);

  const makeMove = useCallback((fromX, fromY, toX, toY) => {
    // This method now returns a string compatible with the backend's MOVE command
    return `move ${fromX} ${fromY} ${toX} ${toY}`;
  }, []);

  return {
    player,
    setPlayer,
    gameId,
    setGameId,
    board,
    setBoard,
    currentPlayer,
    setCurrentPlayer,
    gameStatus,
    setGameStatus,
    messages,
    updateFromServer,
    makeMove,
    addMessage,
  };
};

export default useGameState;
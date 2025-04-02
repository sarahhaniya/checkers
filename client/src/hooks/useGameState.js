import { useState, useCallback } from "react";

const useGameState = () => {
  const [player, setPlayer] = useState("");
  const [gameId, setGameId] = useState(null);
  const [board, setBoard] = useState(
    Array(8)
      .fill()
      .map(() => Array(8).fill(""))
  );
  const [messages, setMessages] = useState([]);
  const [currentPlayer, setCurrentPlayer] = useState("");
  const [gameStatus, setGameStatus] = useState("waiting");

  const updateFromServer = useCallback((data) => {
    try {
      const response = JSON.parse(data);

      // Handle different message types
      switch (response.type) {
        case "login_success":
          setMessages((prev) => [...prev, `Logged in as ${response.username}`]);
          break;

        case "game_created":
          setGameId(response.gameId);
          setMessages((prev) => [
            ...prev,
            `Game created with ID: ${response.gameId}`,
          ]);
          break;

        case "game_joined":
          setGameId(response.gameId);
          if (response.board) {
            setBoard(response.board);
          }
          setGameStatus("playing");
          setMessages((prev) => [
            ...prev,
            `Joined game with ID: ${response.gameId}`,
          ]);
          break;

        case "game_update":
          setBoard(response.board);
          setCurrentPlayer(response.currentPlayer);
          if (response.winner) {
            setGameStatus("finished");
            setMessages((prev) => [
              ...prev,
              `Game over! Winner: ${response.winner}`,
            ]);
          }
          break;

        case "error":
          setMessages((prev) => [...prev, `Error: ${response.message}`]);
          break;

        default:
          setMessages((prev) => [...prev, `Received: ${data}`]);
      }
    } catch (err) {
      console.error("Failed to parse server message:", data);
      setMessages((prev) => [...prev, `Received: ${data}`]);
    }
  }, []);

  const makeMove = useCallback(
    (fromX, fromY, toX, toY) => {
      return {
        type: "MOVE",
        payload: { fromX, fromY, toX, toY, gameId },
      };
    },
    [gameId]
  );

  return {
    player,
    setPlayer,
    gameId,
    setGameId,
    board,
    currentPlayer,
    gameStatus,
    messages,
    updateFromServer,
    makeMove,
  };
};

export default useGameState;

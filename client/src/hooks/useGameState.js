import { useState, useCallback } from "react";

const useGameState = () => {
  const [player, setPlayer] = useState("");
  const [gameId, setGameId] = useState("");
  const [player1Id, setPlayer1Id] = useState("");
  const [player2Id, setPlayer2Id] = useState("");
  const [board, setBoard] = useState(
    Array(8)
      .fill()
      .map(() => Array(8).fill(""))
  );

  const formatBoard = (serverBoard) => {
    return serverBoard.map(row =>
      row.map(cell => {
        if (!cell) return null;
        const pieceChar = cell.isWhite ? 'r' : 'b';
        return cell.isKing ? pieceChar.toUpperCase() : pieceChar;
      })
    );
  };

  const [messages, setMessages] = useState([]);
  const [currentPlayer, setCurrentPlayer] = useState("");
  const [gameStatus, setGameStatus] = useState("waiting");

  const updateFromServer = useCallback((data) => {

    console.log("[Frontend] Raw data received:", data);

    try {
      if (!data.trim()) return; // ✅ Ignore blank messages from server

      const response = JSON.parse(data);

      // Handle different message types
      switch (response.type) {
        case "login_success":
          setMessages((prev) => [...prev, `Logged in as ${response.username}`]);
          setPlayer(response.username);
          break;

        case "game_created":
          setGameStatus("waiting");
          setCurrentPlayer("");
          setGameId(response.gameCode);
          setMessages((prev) => [
            ...prev,
            `Game created with ID: ${response.gameCode}`,
          ]);
          break;

        case "game_joined": {
          const { gameInfo } = response;
          setPlayer1Id(gameInfo.player1Id);
          setPlayer2Id(gameInfo.player2Id);

          const turnUsername =
            gameInfo.currentTurn === "Player1" ? gameInfo.player1Id : gameInfo.player2Id;

          if (Array.isArray(response.board)) {
            setBoard(formatBoard(response.board));
          } else {
            console.warn("Invalid board format received in MoveResult:", response.board);
          }

          setGameStatus("playing");
          setCurrentPlayer(turnUsername);
          setMessages((prev) => [
            ...prev,
            `Joined game with ID: ${response.gameId}`,
          ]);
          break;
        }

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

        case "MoveResult":
          const boardPayload = response.board;

          if (boardPayload && Array.isArray(boardPayload)) {
            setBoard(formatBoard(boardPayload));
          } else if (boardPayload && Array.isArray(boardPayload.board)) {
            setBoard(formatBoard(boardPayload.board));
          } else {
            console.warn("Unexpected board structure in MoveResult:", boardPayload);
          }

          const gameInfo = boardPayload?.gameInfo;
          if (gameInfo) {
            setPlayer1Id(gameInfo.player1Id);
            setPlayer2Id(gameInfo.player2Id);
            const turnUsername = gameInfo.currentTurn === "Player1" ? gameInfo.player1Id : gameInfo.player2Id;
            setCurrentPlayer(turnUsername);
          }

          setMessages((prev) => [
            ...prev,
            response.success
              ? `Move successful: (${response.from.join(",")}) → (${response.to.join(",")})`
              : `Invalid move attempt from (${response.from.join(",")}) to (${response.to.join(",")})`,
          ]);
          break;

        case "error":
          setMessages((prev) => [...prev, `Error: ${response.message}`]);
          alert(`Error: ${response.message}`);
          break;

        default:

        if (data.includes("WINS!")) {
            setGameStatus("finished");

            const winnerMatch = data.match(/(WHITE|BLACK) WINS!/);
            const winnerColor = winnerMatch ? winnerMatch[1] : "Unknown";

            setMessages((prev) => [...prev, `Game over! ${data.trim()}`]);
          } else {
            setMessages((prev) => [...prev, `Received: ${data}`]);
          }
      }
    } catch (err) {
      // Not a JSON message (likely plain text)
      if (data.includes("WINS!")) {
        setGameStatus("finished");

        const winnerMatch = data.match(/(WHITE|BLACK) WINS!/);
        const winnerColor = winnerMatch ? winnerMatch[1] : "Unknown";

        setMessages((prev) => [...prev, `Game over! ${data.trim()}`]);
      } else {
        console.error("Failed to parse server message:", err);
        setMessages((prev) => [...prev, `Received: ${data}`]);
      }
    }
  }, []);

  const makeMove = useCallback(
    (fromX, fromY, toX, toY, sendMessage) => {
      const moveCommand = `move ${fromX} ${fromY} ${toX} ${toY}`;
      sendMessage(moveCommand);
    },
    []
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
    player1Id,
    player2Id,
  };
};

export default useGameState;
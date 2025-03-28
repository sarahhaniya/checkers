import { useState } from "react";

const useGameState = () => {
  const [messages, setMessages] = useState([]);
  const [player, setPlayer] = useState(null);
  const [gameId, setGameId] = useState(null);

  const updateFromServer = (data) => {
    setMessages((prev) => [...prev, data]);
  };

  return {
    messages,
    player,
    gameId,
    setPlayer,
    setGameId,
    updateFromServer,
  };
};

export default useGameState;

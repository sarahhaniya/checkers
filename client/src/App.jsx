import React, { useState } from "react";
import useWebSocket from "./hooks/useWebSocket";
import useGameState from "./hooks/useGameState";

function App() {
  const {
    messages,
    setPlayer,
    setGameId,
    updateFromServer,
  } = useGameState();

  const sendMessage = useWebSocket(updateFromServer);

  const [username, setUsername] = useState("");
  const [gameToJoin, setGameToJoin] = useState("");

  const handleLogin = () => {
    if (username.trim()) {
      sendMessage(`login ${username}`);
      setPlayer(username);
    }
  };

  const handleCreateGame = () => {
    sendMessage("create");
  };

  const handleJoinGame = () => {
    if (gameToJoin.trim()) {
      sendMessage(`join ${gameToJoin}`);
      setGameId(gameToJoin);
    }
  };

  return (
    <div style={{ padding: "2rem", fontFamily: "Arial" }}>
      <h1>🕹️ Checkers Game</h1>

      <div style={{ marginBottom: "1rem" }}>
        <h2>Login</h2>
        <input
          type="text"
          placeholder="Enter username"
          value={username}
          onChange={(e) => setUsername(e.target.value)}
        />
        <button onClick={handleLogin}>Login</button>
      </div>

      <div style={{ marginBottom: "1rem" }}>
        <h2>Create Game</h2>
        <button onClick={handleCreateGame}>Create New Game</button>
      </div>

      <div style={{ marginBottom: "1rem" }}>
        <h2>Join Game</h2>
        <input
          type="text"
          placeholder="Enter Game ID"
          value={gameToJoin}
          onChange={(e) => setGameToJoin(e.target.value)}
        />
        <button onClick={handleJoinGame}>Join Game</button>
      </div>

      <div>
        <h2>Server Messages</h2>
        <pre
          style={{
            background: "#f4f4f4",
            padding: "1rem",
            maxHeight: "300px",
            overflowY: "auto",
            border: "1px solid #ccc",
          }}
        >
          {messages.join("\n")}
        </pre>
      </div>
    </div>
  );
}

export default App;

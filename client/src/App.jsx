import React, { useState, useCallback } from "react";
import useWebSocket from "./hooks/useWebSocket";
import useGameState from "./hooks/useGameState";

function App() {
  const {
    messages,
    setPlayer,
    setGameId,
    updateFromServer,
  } = useGameState();

  const memoizedUpdateFromServer = useCallback(updateFromServer, []);
  const sendMessage = useWebSocket(memoizedUpdateFromServer);

  const [view, setView] = useState("login"); // 'login' or 'register'

  // Login state
  const [loginUsername, setLoginUsername] = useState("");
  const [loginPassword, setLoginPassword] = useState("");

  // Register state
  const [regEmail, setRegEmail] = useState("");
  const [regUsername, setRegUsername] = useState("");
  const [regPassword, setRegPassword] = useState("");

  const handleLogin = () => {
    if (loginUsername && loginPassword) {
      sendMessage(`login ${loginUsername} ${loginPassword}`);
      setPlayer(loginUsername);
    }
  };

  const handleRegister = () => {
    if (regEmail && regUsername && regPassword) {
      sendMessage(`register ${regEmail} ${regUsername} ${regPassword}`);
    }
  };

  const handleCreateGame = () => {
    sendMessage("create");
  };

  const handleJoinGame = (id) => {
    sendMessage(`join ${id}`);
    setGameId(id);
  };

  return (
    <div style={{ padding: "2rem", fontFamily: "Arial" }}>
      <h1>🕹️ Checkers Game</h1>

      {/* TAB SWITCHER */}
      <div style={{ marginBottom: "1rem" }}>
        <button onClick={() => setView("login")} disabled={view === "login"}>
          Login
        </button>
        <button onClick={() => setView("register")} disabled={view === "register"}>
          Create Account
        </button>
      </div>

      {/* LOGIN FORM */}
      {view === "login" && (
        <div style={{ marginBottom: "2rem" }}>
          <h2>Login</h2>
          <input
            type="text"
            placeholder="Username"
            value={loginUsername}
            onChange={(e) => setLoginUsername(e.target.value)}
          />
          <input
            type="password"
            placeholder="Password"
            value={loginPassword}
            onChange={(e) => setLoginPassword(e.target.value)}
          />
          <button onClick={handleLogin}>Login</button>
        </div>
      )}

      {/* REGISTER FORM */}
      {view === "register" && (
        <div style={{ marginBottom: "2rem" }}>
          <h2>Create Account</h2>
          <input
            type="email"
            placeholder="Email"
            value={regEmail}
            onChange={(e) => setRegEmail(e.target.value)}
          />
          <input
            type="text"
            placeholder="Username"
            value={regUsername}
            onChange={(e) => setRegUsername(e.target.value)}
          />
          <input
            type="password"
            placeholder="Password"
            value={regPassword}
            onChange={(e) => setRegPassword(e.target.value)}
          />
          <button onClick={handleRegister}>Register</button>
        </div>
      )}

      {/* GAME CREATION / JOINING */}
      <div style={{ marginBottom: "1rem" }}>
        <h2>Create Game</h2>
        <button onClick={handleCreateGame}>Create New Game</button>
      </div>

      <div style={{ marginBottom: "1rem" }}>
        <h2>Join Game</h2>
        <input
          type="text"
          placeholder="Enter Game ID"
          onChange={(e) => handleJoinGame(e.target.value)}
        />
      </div>
    </div>
  );
}

export default App;

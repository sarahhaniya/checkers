import React, { useState, useEffect, useCallback } from "react";
import useWebSocket from "./hooks/useWebSocket";
import useGameState from "./hooks/useGameState";
import Board from "./components/Board";

function App() {
  const {
    player,
    setPlayer,
    gameId,
    setGameId,
    board,
    currentPlayer,
    gameStatus,
    messages,
    updateFromServer,
    makeMove
  } = useGameState();

  const memoizedUpdateFromServer = useCallback(updateFromServer, [updateFromServer]);
  const { sendMessage, status } = useWebSocket("http://localhost:8080", memoizedUpdateFromServer);

  const [view, setView] = useState("login"); // 'login', 'register', 'lobby', 'game'
  const [gameIdInput, setGameIdInput] = useState("");

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

  const handleJoinGame = () => {
    if (gameIdInput) {
      sendMessage(`join ${gameIdInput}`);
      setGameId(gameIdInput);
    }
  };

  const handleMakeMove = (fromX, fromY, toX, toY) => {
    sendMessage(`move ${fromX} ${fromY} ${toX} ${toY}`);
  };

    // Handle successful login/game join
    useEffect(() => {
      if (player && !gameId) {
        setView("lobby");
      } else if (gameId) {
        setView("game");
      }
    }, [player, gameId]);

  return (
    <div style={{ padding: "2rem", fontFamily: "Arial", maxWidth: "800px", margin: "0 auto" }}>
      <h1>🕹️ Checkers Game</h1>

      {/* LOGIN/REGISTER VIEW */}
      {(view === "login" || view === "register") && (
        <>
          {/* TAB SWITCHER */}
          <div style={{ marginBottom: "1rem" }}>
            <button 
              onClick={() => setView("login")} 
              style={{
                padding: "0.5rem 1rem",
                marginRight: "0.5rem",
                backgroundColor: view === "login" ? "indianred" : "#f1f1f1"
              }}
            >
              Login
            </button>
            <button 
              onClick={() => setView("register")} 
              style={{
                padding: "0.5rem 1rem",
                backgroundColor: view === "register" ? "indianred" : "#f1f1f1" 
              }}
            >
              Create Account
            </button>
          </div>

          {/* LOGIN FORM */}
          {view === "login" && (
            <div style={{ marginBottom: "2rem", display: "flex", flexDirection: "column", gap: "1rem" }}>
              <h2>Login</h2>
              <input
                type="text"
                placeholder="Username"
                value={loginUsername}
                onChange={(e) => setLoginUsername(e.target.value)}
                style={{ padding: "0.5rem" }}
              />
              <input
                type="password"
                placeholder="Password"
                value={loginPassword}
                onChange={(e) => setLoginPassword(e.target.value)}
                style={{ padding: "0.5rem" }}
              />
              <button 
                onClick={handleLogin}
                style={{ padding: "0.5rem", backgroundColor: "indianred", color: "white" }}
              >
                Login
              </button>
            </div>
          )}

          {/* REGISTER FORM */}
          {view === "register" && (
            <div style={{ marginBottom: "2rem", display: "flex", flexDirection: "column", gap: "1rem" }}>
              <h2>Create Account</h2>
              <input
                type="email"
                placeholder="Email"
                value={regEmail}
                onChange={(e) => setRegEmail(e.target.value)}
                style={{ padding: "0.5rem" }}
              />
              <input
                type="text"
                placeholder="Username"
                value={regUsername}
                onChange={(e) => setRegUsername(e.target.value)}
                style={{ padding: "0.5rem" }}
              />
              <input
                type="password"
                placeholder="Password"
                value={regPassword}
                onChange={(e) => setRegPassword(e.target.value)}
                style={{ padding: "0.5rem" }}
              />
              <button 
                onClick={handleRegister}
                style={{ padding: "0.5rem", backgroundColor: "indianred", color: "white" }}
              >
                Register
              </button>
            </div>
          )}
        </>
      )}

      {/* LOBBY VIEW */}
      {view === "lobby" && (
        <div>
          <h2>Welcome, {player}!</h2>
          
          <div style={{ marginBottom: "1rem", padding: "1rem", backgroundColor: "#f1f1f1", borderRadius: "4px" }}>
            <h3>Create Game</h3>
            <button 
              onClick={handleCreateGame}
              style={{ padding: "0.5rem 1rem", backgroundColor: "indianred", color: "white" }}
            >
              Create New Game
            </button>
          </div>

          <div style={{ marginBottom: "1rem", padding: "1rem", backgroundColor: "#f1f1f1", borderRadius: "4px" }}>
            <h3>Join Game</h3>
            <div style={{ display: "flex", gap: "0.5rem" }}>
              <input
                type="text"
                placeholder="Enter Game ID"
                value={gameIdInput}
                onChange={(e) => setGameIdInput(e.target.value)}
                style={{ padding: "0.5rem", flexGrow: 1 }}
              />
              <button 
                onClick={handleJoinGame}
                style={{ padding: "0.5rem 1rem", backgroundColor: "indianred", color: "white" }}
              >
                Join
              </button>
            </div>
          </div>
        </div>
      )}

      {/* GAME VIEW */}
      {view === "game" && (
        <div>
          <div style={{ display: "flex", justifyContent: "space-between", alignItems: "center", marginBottom: "1rem" }}>
            <h2>Game Invite Code: {gameId}</h2>
            <div>
              {gameStatus === "playing" ? (
                <span style={{ fontWeight: "bold" }}>
                  Current turn: {currentPlayer === player ? "Your turn" : `${currentPlayer}'s turn`}
                </span>
              ) : (
                <span>Game completed</span>
              )}
            </div>
          </div>
          
          <Board 
            board={board} 
            currentPlayer={currentPlayer}
            isMyTurn={currentPlayer === player}
            onMove={handleMakeMove}
          />
          
          <button 
            onClick={() => { setGameId(null); setView("lobby"); }}
            style={{ marginTop: "1rem", padding: "0.5rem 1rem" }}
          >
            Return to Lobby
          </button>
        </div>
      )}

      {/* MESSAGE LOG */}
      <div style={{ marginTop: "2rem", backgroundColor: "#f1f1f1", padding: "1rem", borderRadius: "4px" }}>
        <h3>Messages</h3>
        <div style={{ maxHeight: "200px", overflowY: "auto" }}>
          {messages.map((msg, i) => (
            <div key={i} style={{ marginBottom: "0.5rem" }}>{msg}</div>
          ))}
        </div>
      </div>
    </div>
  );
}

export default App;

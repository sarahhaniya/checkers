import React, { useState, useEffect, useCallback, useRef } from "react";
import useWebSocket from "./hooks/useWebSocket";
import useGameState from "./hooks/useGameState";
import Board from "./components/Board"; // <-- Make sure this is the correct path
import "./App.css"; // For styling

const initialBoard = [
  ['b', null, 'b', null, 'b', null, 'b', null],
  [null, 'b', null, 'b', null, 'b', null, 'b'],
  ['b', null, 'b', null, 'b', null, 'b', null],
  [null, null, null, null, null, null, null, null],
  [null, null, null, null, null, null, null, null],
  [null, 'r', null, 'r', null, 'r', null, 'r'],
  ['r', null, 'r', null, 'r', null, 'r', null],
  [null, 'r', null, 'r', null, 'r', null, 'r'],
];

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
    makeMove,
    player1Id,
    player2Id,
    wins,
    losses
  } = useGameState();

  const [loginPassword, setLoginPassword] = useState("");
  const memoizedUpdateFromServer = useCallback(updateFromServer, [updateFromServer]);
  const { sendMessage, status } = useWebSocket("ws://localhost:8080", memoizedUpdateFromServer, player, loginPassword, gameId );

  const [view, setView] = useState("login");
  const [gameIdInput, setGameIdInput] = useState("");

  const [loginUsername, setLoginUsername] = useState("");

  const [regEmail, setRegEmail] = useState("");
  const [regUsername, setRegUsername] = useState("");
  const [regPassword, setRegPassword] = useState("");

  const [showGameOverPopup, setShowGameOverPopup] = useState(false);

  const handleLogin = () => {
    if (loginUsername && loginPassword) {
      sendMessage(`login ${loginUsername} ${loginPassword}`);
    }
  };

  const handleRegister = () => {
    const emailRegex = /^[^\s@]+@[^\s@]+\.[^\s@]+$/;

    if (!emailRegex.test(regEmail)) {
      alert("Please enter a valid email address (e.g. name@email.com)");
      return;
    }

    if (regEmail && regUsername && regPassword) {
      sendMessage(`register ${regEmail} ${regUsername} ${regPassword}`);
    }
   setRegEmail("")
   setRegUsername("")
   setRegPassword("")

  };

  const handleCreateGame = () => sendMessage("create");

  const handleJoinGame = () => {
    if (gameIdInput) {
      console.log("[JOIN] Player:", player, "| Game ID:", gameIdInput);
      setGameId(gameIdInput);
      sendMessage(`join ${gameIdInput}`);
    }
  };

  // Function to handle piece movement
  const handleMakeMove = (fromX, fromY, toX, toY) => {
    makeMove(fromX, fromY, toX, toY, sendMessage);
    if(gameStatus === "finished"){
      setShowGameOverPopup(true)

    }
  };

  function Header() {
    if (gameStatus === "playing") {
      return(
      <span style={{ fontWeight: "bold" }}>
      Current turn: {currentPlayer === player ? "Your turn" : `${currentPlayer}'s turn`}
    </span>
      )
     }
    else if(gameStatus === "finished"){
     return <span>Game completed</span>
    }
  }

  useEffect(() => {
    if (player && gameId) {
      setView("game"); // always show the game view if player is in a game
    } else if (player && !gameId) {
      setView("lobby");
    }
  }, [player, gameId]);

  const styles = {
    wrapper: {
      background: "linear-gradient(135deg, #0f0f0f, #1c1c1c)",
      backgroundSize: "200% 200%",
      animation: "gradientMove 10s ease infinite",
      minHeight: "100vh",
      width: "100vw",
      color: "#f0f0f0",
      display: "flex",
      flexDirection: "column",
      justifyContent: "center",
      alignItems: "center",
      fontFamily: "'Press Start 2P', cursive",
      padding: "1rem",
    },
    card: {
      backgroundColor: "#2a2a2a",
      padding: "2rem",
      borderRadius: "1.5rem",
      boxShadow: "0 0 30px rgba(255, 0, 0, 0.2)",
      width: "100%",
      maxWidth: "800px",
      minHeight: "90vh",
      display: "flex",
      flexDirection: "column",
      justifyContent: "space-between",
      border: "2px solid #e53935",
    },
    title: {
      textAlign: "center",
      fontSize: "2.2rem",
      marginBottom: "2rem",
      color: "#ff5252",
      textShadow: "0 0 5px #e53935",
    },
    input: {
      width: "45%",
      padding: "0.75rem",
      marginBottom: "1rem",
      backgroundColor: "#1e1e1e",
      color: "#f8f8f8",
      border: "2px solid #444",
      borderRadius: "10px",
      fontSize: "0.85rem",
      marginLeft: "auto",
      marginRight: "auto",
    },
    button: {
      width: "30%",
      padding: "0.8rem",
      backgroundColor: "#e53935",
      border: "2px solid #ff6b6b",
      borderRadius: "10px",
      color: "#fff",
      fontWeight: "bold",
      cursor: "pointer",
      transition: "0.3s ease",
      marginBottom: "1rem",
      fontSize: "0.9rem",
      marginLeft: "auto",
      marginRight: "auto",
    },
    toggleBtn: (active) => ({
      padding: "0.7rem 1.5rem",
      backgroundColor: active ? "#e53935" : "#333",
      color: "#fff",
      border: "2px solid #444",
      borderRadius: "10px",
      cursor: "pointer",
      marginRight: "0.5rem",
      fontSize: "1rem",
      boxShadow: active ? "0 0 10px #ff4444" : "none",
    }),
    messageBox: {
      backgroundColor: "#1e1e1e",
      padding: "1rem",
      borderRadius: "1rem",
      marginTop: "2rem",
      maxHeight: "150px",
      overflowY: "auto",
      border: "1px solid #444",
    },
  };

  return (
    <div style={styles.wrapper}>
      <div style={styles.card}>
        <h1 style={styles.title}>Checkers</h1>

        {/* TAB SWITCHER */}
        <div style={{ textAlign: "center", marginBottom: "1rem" }}>
          <button style={styles.toggleBtn(view === "login")} onClick={() => setView("login")}>
            Login
          </button>
          <button style={styles.toggleBtn(view === "register")} onClick={() => setView("register")}>
            Register
          </button>
        </div>

        {/* LOGIN FORM */}
        {view === "login" && (
          <>
            <input
              style={styles.input}
              placeholder="Username"
              value={loginUsername}
              onChange={(e) => setLoginUsername(e.target.value)}
            />
            <input
              type="password"
              style={styles.input}
              placeholder="Password"
              value={loginPassword}
              onChange={(e) => setLoginPassword(e.target.value)}
            />
            <button style={styles.button} onClick={handleLogin}>
              Login
            </button>
          </>
        )}

        {/* REGISTER FORM */}
        {view === "register" && (
          <>
            <input
              style={styles.input}
              placeholder="Email"
              value={regEmail}
              onChange={(e) => setRegEmail(e.target.value)}
            />
            <input
              style={styles.input}
              placeholder="Username"
              value={regUsername}
              onChange={(e) => setRegUsername(e.target.value)}
            />
            <input
              type="password"
              style={styles.input}
              placeholder="Password"
              value={regPassword}
              onChange={(e) => setRegPassword(e.target.value)}
            />
            <button style={styles.button} onClick={handleRegister}>
              Register
            </button>
          </>
        )}

        {/* LOBBY */}
        {view === "lobby" && (
          <>
            <h2 style={{ marginBottom: "1rem" }}>Welcome, {player}!</h2>
            <p style={{ textAlign: "center" }}>Wins: {wins} | Losses: {losses}</p>
            <button style={styles.button} onClick={handleCreateGame}>
              Create Game
            </button>
            <input
              style={styles.input}
              placeholder="Enter Game ID"
              value={gameIdInput}
              onChange={(e) => setGameIdInput(e.target.value)}
            />
            <button style={styles.button} onClick={handleJoinGame}>
              Join Game
            </button>
          </>
        )}

        {/* GAME VIEW */}
        {view === "game" && (
          <div>
            <div style={{ display: "flex", justifyContent: "space-between", alignItems: "center", marginBottom: "1rem" }}>
              <h2>Game Invite Code: {gameId}</h2>
              <div>
            <Header></Header>
              </div>
            </div>
            <Board
              board={board}
              currentPlayer={currentPlayer}
              isMyTurn={currentPlayer === player && gameStatus === "playing"}
              onMove={handleMakeMove}
              player={player}
              player1Id={player1Id}
              player2Id={player2Id}
              isFlipped={player === player1Id} 

            />
            <button
              style={{ ...styles.button, backgroundColor: "#555" }}
              onClick={() => {
                setGameId(null);
                setView("lobby");

                if (loginUsername && loginPassword) {
                  sendMessage(`login ${loginUsername} ${loginPassword}`);
                }            
              }}
            >
              Return to Lobby
            </button>
          </div>
        )}
     {/* Winner Display */}
     {gameStatus === "finished" && showGameOverPopup && (
  <div style={{
    position: "fixed",
    top: 0,
    left: 0,
    width: "100vw",
    height: "100vh",
    backgroundColor: "rgba(0, 0, 0, 0.85)",
    display: "flex",
    justifyContent: "center",
    alignItems: "center",
    zIndex: 9999
  }}>
    <div style={{
      background: "#222",
      padding: "2rem 3rem",
      borderRadius: "1rem",
      border: "2px solid limegreen",
      color: "limegreen",
      textAlign: "center",
      fontSize: "1.3rem",
      fontWeight: "bold",
      boxShadow: "0 0 30px limegreen"
    }}>
      <h2>🎉 Game Over</h2>
      <p style={{ marginBottom: "1rem" }}>
        {
          messages.find((msg) => msg.includes("WINS!")) ||
          messages.find((msg) => msg.toLowerCase().includes("winner")) ||
          "The game has ended."
        }
      </p>
      <button
  onClick={() => setShowGameOverPopup(false)}
  style={{
    padding: "0.7rem 1.5rem",
    backgroundColor: "#e53935",
    border: "2px solid #ff6b6b",
    borderRadius: "10px",
    color: "#fff",
    cursor: "pointer",
    fontSize: "1rem"
  }}
>
  Close
</button>
    </div>
  </div>
)}
        {/* MESSAGES */}
        <div style={styles.messageBox}>
          <h3 style={{ marginBottom: "0.5rem" }}>Messages</h3>
          {messages.map((msg, i) => (
            <div key={i} style={{ fontSize: "0.9rem", marginBottom: "0.3rem" }}>
              {msg}
            </div>
          ))}
        </div>
      </div>
    </div>
  );
}

export default App;

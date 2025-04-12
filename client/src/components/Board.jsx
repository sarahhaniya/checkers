// components/Board.jsx
import React, { useState } from "react";
import './CheckersBoard.css';

const Board = ({ 
  board, 
  currentPlayer, 
  player, 
  isMyTurn, 
  onMove, 
  player1Id, 
  player2Id,
  isFlipped   
}) => {
  const [selectedPiece, setSelectedPiece] = useState(null);

  // Determine piece color
  const getPieceColor = (piece) => {
    if (piece === 'r' || piece === 'R') return 'red';
    if (piece === 'b' || piece === 'B') return 'black';
    return null;
  };

  // Is the piece a king?
  const isKing = (piece) => piece === 'R' || piece === 'B';

  // Determine the player's color
  const playerColor = player === player1Id ? 'red' : 'black';

  const handleSquareClick = (x, y) => {
    if (!isMyTurn) return;

    const piece = board[y][x];
    const pieceColor = getPieceColor(piece);

    // No selection yet — try to select a piece
    if (!selectedPiece && pieceColor) {
      if (pieceColor === playerColor) {
        setSelectedPiece({ x, y });
      }
    }
    // Already selected something
    else if (selectedPiece) {
      // Deselect if clicked the same square
      if (selectedPiece.x === x && selectedPiece.y === y) {
        setSelectedPiece(null);
      }
      // Try to move if clicked on empty
      else if (!pieceColor) {
        onMove(selectedPiece.x, selectedPiece.y, x, y);
        setSelectedPiece(null);
      }
      // Change selection if clicked own piece
      else if (pieceColor === playerColor) {
        setSelectedPiece({ x, y });
      }
    }
  };

  const renderSquare = (x, y) => {
    const isBlackSquare = (x + y) % 2 === 1;
    const piece = board?.[y]?.[x] ?? null;  
    const pieceColor = getPieceColor(piece);
    const isSelected = selectedPiece?.x === x && selectedPiece?.y === y;

    return (
      <div
        key={`${x}-${y}`}
        onClick={() => handleSquareClick(x, y)}
        style={{
          width: "50px",
          height: "50px",
          backgroundColor: isBlackSquare ? "black" : "indianred",
          display: "flex",
          justifyContent: "center",
          alignItems: "center",
          cursor: isMyTurn ? "pointer" : "default",
          border: isSelected ? "2px solid yellow" : "none",
          boxSizing: "border-box"
        }}
      >
        {pieceColor && (
          <div className={`piece ${pieceColor}`}>
            {isKing(piece) && <div className="king">K</div>}
          </div>
        )}
      </div>
    );
  };

  return (
    <div style={{ display: "inline-block", border: "2px solid #333" }}>
    {(isFlipped ? [...Array(8).keys()].reverse() : [...Array(8).keys()]).map((y) => (
      <div key={y} style={{ display: "flex" }}>
        {(isFlipped ? [...Array(8).keys()].reverse() : [...Array(8).keys()]).map((x) =>
          renderSquare(x, y)
        )}
      </div>
    ))}
  </div>
);
};

export default Board;

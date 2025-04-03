// components/CheckersBoard.js
import React, { useState } from "react";

const Board = ({ board, currentPlayer, isMyTurn, onMove }) => {
  const [selectedPiece, setSelectedPiece] = useState(null);
  
  // Helper to determine piece color
  const getPieceColor = (piece) => {
    if (piece === 'r' || piece === 'R') return 'red';
    if (piece === 'b' || piece === 'B') return 'black';
    return null;
  };
  
  // Check if piece is king
  const isKing = (piece) => {
    return piece === 'R' || piece === 'B';
  };
  
  // Handle square click
  const handleSquareClick = (x, y) => {
    if (!isMyTurn) return;
    
    const piece = board[y][x];
    const pieceColor = getPieceColor(piece);
    
    // If no piece is selected and clicking on own piece, select it
    if (!selectedPiece && pieceColor) {
      // Only select pieces of current player's color
      const playerColor = currentPlayer.toLowerCase() === 'red' ? 'red' : 'black';
      if (pieceColor === playerColor) {
        setSelectedPiece({ x, y });
      }
    } 
    // If a piece is already selected
    else if (selectedPiece) {
      // If clicking on same piece, deselect it
      if (selectedPiece.x === x && selectedPiece.y === y) {
        setSelectedPiece(null);
      } 
      // If clicking on an empty square, try to move
      else if (!pieceColor) {
        onMove(selectedPiece.x, selectedPiece.y, x, y);
        setSelectedPiece(null);
      }
      // If clicking on another piece of same color, select that piece instead
      else if (getPieceColor(piece) === getPieceColor(board[selectedPiece.y][selectedPiece.x])) {
        setSelectedPiece({ x, y });
      }
    }
  };
  
  const renderSquare = (x, y) => {
    const isBlackSquare = (x + y) % 2 === 1;
    const piece = board[y][x];
    const pieceColor = getPieceColor(piece);
    
    // Determine if this square is selected
    const isSelected = selectedPiece && selectedPiece.x === x && selectedPiece.y === y;
    
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
          <div style={{
            width: "40px",
            height: "40px",
            borderRadius: "50%",
            backgroundColor: pieceColor,
            border: "2px solid white",
            display: "flex",
            justifyContent: "center",
            alignItems: "center"
          }}>
            {isKing(piece) && (
              <div style={{
                fontSize: "20px",
                color: "gold",
                fontWeight: "bold"
              }}>
                K
              </div>
            )}
          </div>
        )}
      </div>
    );
  };
  
  return (
    <div style={{ display: "inline-block", border: "2px solid #333" }}>
      {Array(8).fill().map((_, y) => (
        <div key={y} style={{ display: "flex" }}>
          {Array(8).fill().map((_, x) => renderSquare(x, y))}
        </div>
      ))}
    </div>
  );
};

export default Board;
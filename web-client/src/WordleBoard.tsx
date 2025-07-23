import React from 'react';

export type Feedback = 0 | 1 | 2; // 0: gray, 1: yellow, 2: green

interface WordleBoardProps {
  guesses: string[];
  feedback: Feedback[][];
  wordLength: number;
  maxGuesses?: number;
  input?: string;
  exploreMode?: boolean;
  exploreCellStates?: number[][];
  onExploreCellClick?: (rowIdx: number, colIdx: number) => void;
}

const getCellStyle = (fb: Feedback): React.CSSProperties => {
  switch (fb) {
    case 2: return { background: '#6aaa64', color: 'white' }; // green
    case 1: return { background: '#c9b458', color: 'white' }; // yellow
    case 0: default: return { background: '#787c7e', color: 'white' }; // gray
  }
};

const getExploreCellStyle = (state: number): React.CSSProperties => {
  switch (state) {
    case 2: return { background: '#6aaa64', color: 'white' }; // green
    case 1: return { background: '#c9b458', color: 'white' }; // orange/yellow
    case 0: default: return { background: '#d3d6da', color: 'black' }; // normal
  }
};

export const WordleBoard: React.FC<WordleBoardProps> = ({ guesses, feedback, wordLength, maxGuesses = 6, input, exploreMode, exploreCellStates, onExploreCellClick }) => {
  const rows: React.ReactNode[] = [];
  let inputShown = false;
  for (let i = 0; i < maxGuesses; ++i) {
    let guess = guesses[i] || '';
    let fb = (i < feedback.length) ? feedback[i] : undefined;
    let isInputRow = false;
    // Only show input in the first empty row, and only if the game is not over
    if (!guess && input && !inputShown && guesses.length < maxGuesses) {
      guess = input;
      fb = undefined;
      inputShown = true;
      isInputRow = true;
    }
    const cells: React.ReactNode[] = [];
    for (let j = 0; j < wordLength; ++j) {
      const letter = guess[j] || '';
      let style: React.CSSProperties;
      if (exploreMode && exploreCellStates && exploreCellStates[i] && exploreCellStates[i][j] !== undefined) {
        style = getExploreCellStyle(exploreCellStates[i][j]);
      } else {
        style = (fb && fb[j] !== undefined && !isInputRow)
          ? getCellStyle(fb[j])
          : { background: '#d3d6da', color: 'black' };
      }
      cells.push(
        <td
          key={j}
          style={{ ...style, width: 40, height: 40, textAlign: 'center', fontSize: 24, border: '2px solid #888', cursor: exploreMode ? 'pointer' : 'default' }}
          onClick={exploreMode && onExploreCellClick ? () => onExploreCellClick(i, j) : undefined}
        >
          {letter.toUpperCase()}
        </td>
      );
    }
    rows.push(<tr key={i}>{cells}</tr>);
  }
  return (
    <table style={{ borderCollapse: 'collapse', margin: '20px auto' }}>
      <tbody>{rows}</tbody>
    </table>
  );
}; 
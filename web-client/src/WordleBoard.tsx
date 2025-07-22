import React from 'react';

export type Feedback = 0 | 1 | 2; // 0: gray, 1: yellow, 2: green

interface WordleBoardProps {
  guesses: string[];
  feedback: Feedback[][];
  wordLength: number;
  maxGuesses?: number;
  input?: string;
}

const getCellStyle = (fb: Feedback): React.CSSProperties => {
  switch (fb) {
    case 2: return { background: '#6aaa64', color: 'white' }; // green
    case 1: return { background: '#c9b458', color: 'white' }; // yellow
    case 0: default: return { background: '#787c7e', color: 'white' }; // gray
  }
};

export const WordleBoard: React.FC<WordleBoardProps> = ({ guesses, feedback, wordLength, maxGuesses = 6, input }) => {
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
      // Only use feedback style if this is not the input row and feedback exists
      const style = (fb && fb[j] !== undefined && !isInputRow)
        ? getCellStyle(fb[j])
        : { background: '#d3d6da', color: 'black' };
      // Debug log for diagnosis
      console.log(`Row ${i}, Col ${j}, isInputRow: ${isInputRow}, fb[j]:`, fb ? fb[j] : undefined, 'style:', style);
      cells.push(
        <td key={j} style={{ ...style, width: 40, height: 40, textAlign: 'center', fontSize: 24, border: '2px solid #888' }}>
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
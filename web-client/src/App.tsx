import React, { useEffect, useRef, useState, ReactNode } from 'react';
import './App.css';
import { WordleBoard, Feedback } from './WordleBoard';
import { startGame, submitGuess, getStatus, bestWords, reveal, explore } from './api';

const QWERTY_ROWS = [
  ['Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P'],
  ['A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L'],
  ['Z', 'X', 'C', 'V', 'B', 'N', 'M'],
];

function getEliminatedLetters(guesses: string[], feedback: Feedback[][]): Set<string> {
  // A letter is eliminated if it only ever received 0 feedback in all guesses
  const eliminated = new Set<string>();
  const seen = new Set<string>();
  for (let i = 0; i < guesses.length; ++i) {
    const guess = guesses[i];
    const fb = feedback[i] || [];
    for (let j = 0; j < guess.length; ++j) {
      const letter = guess[j]?.toUpperCase();
      if (!letter || seen.has(letter)) continue;
      seen.add(letter);
      // If this letter never got a 1 or 2 in any position in any guess, eliminate it
      let found = false;
      for (let k = 0; k < guesses.length; ++k) {
        const g = guesses[k];
        const f = feedback[k] || [];
        for (let l = 0; l < g.length; ++l) {
          if (g[l]?.toUpperCase() === letter && f[l] > 0) {
            found = true;
            break;
          }
        }
        if (found) break;
      }
      if (!found) eliminated.add(letter);
    }
  }
  return eliminated;
}

const QwertyKeyboard: React.FC<{ guesses: string[]; feedback: Feedback[][]; onKeyClick: (letter: string) => void }> = ({ guesses, feedback, onKeyClick }) => {
  const eliminated = getEliminatedLetters(guesses, feedback);
  return (
    <div style={{ display: 'flex', flexDirection: 'column', alignItems: 'flex-start', marginRight: 24, marginTop: 56 }}>
      {QWERTY_ROWS.map((row, i) => (
        <div
          key={i}
          style={{
            display: 'flex',
            flexDirection: 'row',
            marginBottom: 2,
            paddingLeft: i === 2 ? 36 : 0,
          }}
        >
          {row.map((letter) => (
            <div
              key={letter}
              onClick={() => !eliminated.has(letter) && onKeyClick(letter)}
              style={{
                width: 28,
                height: 38,
                borderRadius: 4,
                background: eliminated.has(letter) ? '#787c7e' : '#d3d6da',
                color: eliminated.has(letter) ? 'white' : 'black',
                display: 'flex',
                alignItems: 'center',
                justifyContent: 'center',
                fontWeight: 'bold',
                fontSize: 18,
                border: '1px solid #888',
                userSelect: 'none',
                cursor: eliminated.has(letter) ? 'default' : 'pointer',
                margin: 1,
              }}
            >
              {letter}
            </div>
          ))}
        </div>
      ))}
    </div>
  );
};

function App() {
  const [gameId, setGameId] = useState<string | null>(null);
  const [guesses, setGuesses] = useState<string[]>([]);
  const [feedback, setFeedback] = useState<Feedback[][]>([]);
  const [wordLength, setWordLength] = useState<number>(5);
  const [input, setInput] = useState('');
  const [won, setWon] = useState(false);
  const [lost, setLost] = useState(false);
  const [error, setError] = useState<ReactNode>(null);
  const [theWord, setTheWord] = useState<string | null>(null);
  const [showRemaining, setShowRemaining] = useState(false);
  const [showRemainingWords, setShowRemainingWords] = useState(false);
  const [remainingWords, setRemainingWords] = useState<string[]>([]);
  const [exploreMode, setExploreMode] = useState(false);
  const [exploreCellStates, setExploreCellStates] = useState<number[][]>([]);
  const [remaining, setRemaining] = useState<number | null>(null);
  const [showBest, setShowBest] = useState(false);
  const [bestList, setBestList] = useState<string[]>([]);
  const inputRef = useRef<HTMLInputElement>(null);

  useEffect(() => {
    (async () => {
      try {
        const res = await startGame();
        setGameId(res.game_id);
        setWordLength(res.length);
        setGuesses([]);
        setFeedback([]);
        setInput('');
        setWon(false);
        setLost(false);
        setError(null);
        setRemaining(null);
      } catch (e) {
        setError('Failed to start game');
      }
    })();
  }, []);

  useEffect(() => {
    if (showBest && gameId) {
      bestWords(gameId).then(setBestList).catch(() => setBestList([]));
    } else {
      setBestList([]);
    }
  }, [showBest, gameId, guesses.length]);

  useEffect(() => {
    if (inputRef.current) inputRef.current.focus();
  }, [guesses.length, won, lost]);

  // Reset explore cell states on new game, guess, or when explore mode is turned off
  useEffect(() => {
    setExploreCellStates([]);
  }, [gameId, guesses.length, exploreMode]);

  // Handler to cycle cell state in explore mode
  const handleExploreCellClick = (rowIdx: number, colIdx: number) => {
    if (!exploreMode) return;
    setExploreCellStates(prev => {
      const newStates = prev.map(arr => arr.slice());
      // Ensure the row exists
      while (newStates.length <= rowIdx) newStates.push([]);
      // Ensure the col exists
      while (newStates[rowIdx].length <= colIdx) newStates[rowIdx].push(0);
      // Cycle: 0 (normal) -> 1 (orange) -> 2 (green) -> 0
      newStates[rowIdx][colIdx] = ((newStates[rowIdx][colIdx] || 0) + 1) % 3;
      return newStates;
    });
  };

  // Add a click handler to the board area to refocus input
  const handleBoardClick = () => {
    if (inputRef.current) inputRef.current.focus();
  };

  const handleInput = (e: React.ChangeEvent<HTMLInputElement>) => {
    setInput(e.target.value.toUpperCase());
    setError(null);
  };

  const handleSubmit = async (e?: React.FormEvent) => {
    if (e) e.preventDefault();
    if (!gameId || input.length !== wordLength || won || lost) return;
    try {
      let res: any;
      setError(null);
      if (exploreMode) {
        let exploreState = (exploreCellStates[guesses.length] || []).slice(0, wordLength);
        while (exploreState.length < wordLength) exploreState.push(0);
        res = await explore(gameId, input, exploreState);
      } else {
        res = await submitGuess(gameId, input);
      }
      setGuesses(res.guesses);
      setFeedback(prev => {
        let newFeedback = res.feedback;
        if (newFeedback.length > 0 && typeof newFeedback[0] === 'number') {
          newFeedback = [newFeedback];
        }
        return [...prev, ...newFeedback.map((row: any) => row.map(Number))];
      });
      setInput('');
      setWon(res.won);
      setLost(res.lost);
      if (res.lost && res.the_word) setTheWord(res.the_word);
      if (typeof res.remaining === 'number') setRemaining(res.remaining);
      if (Array.isArray(res.remaining_words)) setRemainingWords(res.remaining_words);
    } catch (err: any) {
      setInput('');
      let msg = '';
      if (err instanceof Error && err.message) {
        msg = err.message;
      } else if (typeof err === 'string') {
        msg = err;
      } else if (err && typeof err === 'object' && 'message' in err) {
        msg = String(err.message);
      }
      if (msg.toLowerCase().includes('invalid word')) {
        setError(<span style={{ fontSize: '0.7em' }}>Not a valid word: <b>{input}</b></span>);
      } else {
        setError('Failed to submit guess');
      }
    }
  };

  const handleRestart = async () => {
    try {
      const res = await startGame();
      setGameId(res.game_id);
      setWordLength(res.length);
      setGuesses([]);
      setFeedback([]);
      setInput('');
      setWon(false);
      setLost(false);
      setTheWord(null);
      setError(null);
      setRemaining(null);
    } catch (e) {
      setError('Failed to restart game');
    }
  };

  const handleReveal = async () => {
    if (!gameId) return;
    if (exploreMode) {
      // In explore mode, only allow reveal if exactly one remaining word
      if (remaining === 1 && remainingWords.length > 0) {
        setTheWord(remainingWords[0]);
      }
      return;
    }
    try {
      const word = await reveal(gameId);
      setTheWord(word);
    } catch (e) {
      setError('Failed to reveal word');
    }
  };

  const handleKeyClick = (letter: string) => {
    if (input.length < wordLength && !won && !lost) {
      setInput(input + letter);
    }
  };

  // Button style: half the previous size
  const buttonStyle = {
    marginTop: 5,
    marginRight: 5,
    fontSize: '0.45em',
    padding: '1px 5px',
    minWidth: 0,
    display: 'inline-block',
    verticalAlign: 'middle',
  };

  return (
    <div className="App">
      <header className="App-header">
        <h1>Wordle Web Client</h1>
        <div style={{ display: 'flex', flexDirection: 'row', alignItems: 'flex-start', width: '100%', justifyContent: 'center' }}>
          {/* QWERTY keyboard on the left */}
          <QwertyKeyboard guesses={guesses} feedback={feedback} onKeyClick={handleKeyClick} />
          <div style={{ display: 'flex', flexDirection: 'row', alignItems: 'flex-start' }}>
            <div style={{ flex: '0 1 auto', alignSelf: 'flex-start' }} onClick={handleBoardClick}>
              <WordleBoard
                guesses={guesses}
                feedback={feedback}
                wordLength={wordLength}
                input={input}
                exploreMode={exploreMode}
                exploreCellStates={exploreCellStates}
                onExploreCellClick={handleExploreCellClick}
              />
            </div>
            <div style={{ flex: '0 0 auto', marginLeft: 24, marginTop: 0, alignSelf: 'flex-start', textAlign: 'left' }}>
              <div style={{ display: 'flex', flexDirection: 'column', alignItems: 'flex-start' }}>
                <label style={{ fontSize: '0.4em', marginRight: 8, marginBottom: 4 }}>
                  <input
                    type="checkbox"
                    checked={exploreMode}
                    onChange={e => setExploreMode(e.target.checked)}
                    style={{ marginRight: 4 }}
                  />
                  Explore mode
                </label>
                <label style={{ fontSize: '0.4em', marginRight: 8, marginBottom: 4 }}>
                  <input
                    type="checkbox"
                    checked={showRemaining}
                    onChange={e => setShowRemaining(e.target.checked)}
                    style={{ marginRight: 4 }}
                  />
                  Show remaining count
                </label>
                {showRemaining && typeof remaining === 'number' && (
                  <div style={{ color: 'deepskyblue', fontWeight: 'bold', fontSize: '0.4em', marginTop: 6, marginBottom: 4 }}>
                    Remaining: {remaining}
                  </div>
                )}
                <label style={{ fontSize: '0.4em', marginRight: 8, marginBottom: 4 }}>
                  <input
                    type="checkbox"
                    checked={showRemainingWords}
                    onChange={e => setShowRemainingWords(e.target.checked)}
                    style={{ marginRight: 4 }}
                  />
                  Show remaining words
                </label>
                {showRemainingWords && remainingWords.length > 0 && (
                  <div style={{
                    display: 'grid',
                    gridTemplateColumns: '1fr 1fr 1fr',
                    gap: '2px 12px',
                    fontSize: '0.4em',
                    color: 'deepskyblue',
                    margin: '8px 0 0 0',
                    padding: 0,
                    textAlign: 'left',
                    listStyle: 'none',
                    width: '100%',
                    maxWidth: 240,
                  }}>
                    {remainingWords.map((word, i) => (
                      <div key={i} style={{ padding: '0 2px' }}>{word}</div>
                    ))}
                  </div>
                )}
                <label style={{ fontSize: '0.4em', marginRight: 8, marginBottom: 4 }}>
                  <input
                    type="checkbox"
                    checked={showBest}
                    onChange={e => setShowBest(e.target.checked)}
                    style={{ marginRight: 4 }}
                  />
                  Show best words
                </label>
                {showBest && bestList.length > 0 && (typeof remaining !== 'number' || remaining > 1) && (
                  <ul style={{ fontSize: '0.4em', color: 'gold', margin: '8px 0 0 0', padding: 0, listStyle: 'none', textAlign: 'left' }}>
                    {bestList.map((word, i) => (
                      <li key={i}>{word}</li>
                    ))}
                  </ul>
                )}
              </div>
            </div>
          </div>
        </div>
        {won && <div style={{ color: 'lightgreen', fontWeight: 'bold' }}>You won!</div>}
        {lost && <div style={{ color: 'salmon', fontWeight: 'bold' }}>You lost!</div>}
        {theWord && <div style={{ color: 'orange', fontWeight: 'bold' }}>The word was: {theWord}</div>}
        {error && <div style={{ color: 'red' }}>{error}</div>}
        {/* Remove the visible input and word length display. Only keep the hidden input for keyboard capture. */}
        <div style={{ height: 0, overflow: 'hidden' }}>
          <input
            ref={inputRef}
            type="text"
            value={input}
            onChange={handleInput}
            maxLength={wordLength}
            disabled={won || lost}
            autoFocus
            tabIndex={-1}
            style={{ position: 'absolute', left: 0, top: 0, width: 1, height: 1, opacity: 0, zIndex: -1 }}
            onKeyDown={e => {
              if (e.key === 'Enter') {
                e.preventDefault();
                handleSubmit();
              }
            }}
          />
        </div>
        <div style={{ display: 'flex', flexDirection: 'row', marginTop: 8 }}>
          <button
            type="button"
            onClick={handleSubmit}
            style={buttonStyle}
            disabled={input.length !== wordLength || won || lost}
          >
            Submit
          </button>
          <button
            type="button"
            onClick={handleRestart}
            style={buttonStyle}
          >
            Restart
          </button>
          <button
            type="button"
            onClick={handleReveal}
            style={buttonStyle}
            disabled={
              exploreMode
                ? (remaining !== 1 || !!theWord)
                : (won || lost || !!theWord)
            }
          >
            Reveal
          </button>
        </div>
      </header>
    </div>
  );
}

export default App; 
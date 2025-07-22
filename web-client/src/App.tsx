import React, { useEffect, useRef, useState } from 'react';
import './App.css';
import { WordleBoard, Feedback } from './WordleBoard';
import { startGame, submitGuess, getStatus, bestWords } from './api';

function App() {
  const [gameId, setGameId] = useState<string | null>(null);
  const [guesses, setGuesses] = useState<string[]>([]);
  const [feedback, setFeedback] = useState<Feedback[][]>([]);
  const [wordLength, setWordLength] = useState<number>(5);
  const [input, setInput] = useState('');
  const [won, setWon] = useState(false);
  const [lost, setLost] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [theWord, setTheWord] = useState<string | null>(null);
  const [showRemaining, setShowRemaining] = useState(false);
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

  // Add a click handler to the board area to refocus input
  const handleBoardClick = () => {
    if (inputRef.current) inputRef.current.focus();
  };

  const handleInput = (e: React.ChangeEvent<HTMLInputElement>) => {
    setInput(e.target.value.toUpperCase());
  };

  const handleSubmit = async (e?: React.FormEvent) => {
    if (e) e.preventDefault();
    if (!gameId || input.length !== wordLength || won || lost) return;
    try {
      const res = await submitGuess(gameId, input);
      console.log("submitGuess response (full):", JSON.stringify(res, null, 2));
      console.log("submitGuess response:", res);
      setGuesses(res.guesses);
      // Patch: append new feedback to feedbacks array
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
    } catch (err) {
      // Optionally handle error
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

  // Add logs before rendering WordleBoard
  console.log("App.tsx: guesses", guesses);
  console.log("App.tsx: feedback", feedback);

  return (
    <div className="App">
      <header className="App-header">
        <h1>Wordle Web Client</h1>
        <div style={{ display: 'flex', flexDirection: 'row', alignItems: 'flex-start', width: '100%', justifyContent: 'center' }}>
          <div style={{ flex: '0 1 auto' }} onClick={handleBoardClick}>
            <WordleBoard guesses={guesses} feedback={feedback} wordLength={wordLength} input={input} />
          </div>
          <div style={{ flex: '0 0 auto', marginLeft: 24, marginTop: 8, alignSelf: 'flex-start', textAlign: 'left' }}>
            <div style={{ display: 'flex', flexDirection: 'column', alignItems: 'flex-start' }}>
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
        {won && <div style={{ color: 'lightgreen', fontWeight: 'bold' }}>You won!</div>}
        {lost && <div style={{ color: 'salmon', fontWeight: 'bold' }}>You lost!</div>}
        {lost && theWord && <div style={{ color: 'orange', fontWeight: 'bold' }}>The word was: {theWord}</div>}
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
        </div>
        {error && <div style={{ color: 'red' }}>{error}</div>}
      </header>
    </div>
  );
}

export default App; 
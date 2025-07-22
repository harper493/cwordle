export interface StartGameResponse {
  game_id: string;
  length: number;
}

export interface GuessResponse {
  feedback: number[];
  won: boolean;
  lost: boolean;
  guesses: string[];
  the_word?: string;
  remaining?: number;
}

export interface StatusResponse {
  guesses: string[];
  won: boolean;
  lost: boolean;
  length: number;
  answer?: string;
  the_word?: string;
}

const API_URL = 'http://localhost:18080';

export async function startGame(): Promise<StartGameResponse> {
  const res = await fetch(`${API_URL}/start`, { method: 'POST' });
  if (!res.ok) throw new Error('Failed to start game');
  return res.json();
}

export async function submitGuess(gameId: string, guess: string): Promise<any> {
  const res = await fetch(`${API_URL}/guess`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ game_id: gameId, guess }),
  });
  if (!res.ok) throw new Error('Failed to submit guess');
  const data = await res.json();
  return data;
}

export async function getStatus(gameId: string): Promise<StatusResponse> {
  const res = await fetch(`${API_URL}/status?game_id=${encodeURIComponent(gameId)}`);
  if (!res.ok) throw new Error('Failed to get status');
  return res.json();
}

export async function bestWords(gameId: string): Promise<string[]> {
  const res = await fetch(`${API_URL}/best`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ game_id: gameId }),
  });
  if (!res.ok) throw new Error('Failed to get best words');
  const data = await res.json();
  return data.best || [];
}

export async function reveal(gameId: string): Promise<string> {
  const res = await fetch(`${API_URL}/reveal`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ game_id: gameId }),
  });
  if (!res.ok) throw new Error('Failed to reveal word');
  const data = await res.json();
  return data.word || data.the_word || data.answer;
} 
import uvicorn
import mido
from mido import Message, MidiFile, MidiTrack, bpm2tempo, merge_tracks
from pydantic import BaseModel
from fastapi import FastAPI, File, UploadFile, Form
from fastapi.middleware.cors import CORSMiddleware
from typing import List, Literal
from io import BytesIO

app = FastAPI()

origins = [
    "http://localhost:3000",
]



app.add_middleware(
    CORSMiddleware,
    allow_origins=origins,
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)



class guitarEvent(BaseModel):
    time: int
    string: int
    fret: int
class GuitarMidiEvents(BaseModel):
    title: str
    artist: str
    genre: str
    duration_formatted: str 
    events: List[guitarEvent] # C-style array string representation probs will change
#CORS middleware

@app.get("/test-cors")
async def test_cors():
    return {"message": "CORS test"}


@app.post("/upload-midi", response_model=GuitarMidiEvents)
async def upload_midi(midi_file: UploadFile = File(...), title: str = Form(...), artist: str = Form(...), genre: str = Form(...)):
    contents = await midi_file.read()
    ir = process_midi_to_guitar_from_midi(contents)
    return {
        "title": title,
        "artist": artist,
        "genre": genre,
        "duration_formatted": ir["duration"],
        "events" : ir["events"]
    }




def extract_global_meta_messages(mid):
    """Gather all tempo/key/time meta messages from all tracks."""
    meta_track = MidiTrack()
    for track in mid.tracks:
        for msg in track:
            if msg.is_meta and msg.type in ['set_tempo', 'time_signature', 'key_signature']:
                meta_track.append(msg.copy(time=msg.time))
    return meta_track


def is_melodic_track(track):
    """Return True if track contains note messages and is not percussion."""
    for msg in track:
        if msg.type in ['note_on', 'note_off']:
            if hasattr(msg, 'channel') and msg.channel == 9:
                return False  # Percussion
            return True
    return False  # No notes at all

def strip_non_melodic_and_preserve_tempo(input_bytes: bytes) -> bytes:
    mid = MidiFile(file=BytesIO(input_bytes))  # ← Parse the bytes here
    new_mid = MidiFile(ticks_per_beat=mid.ticks_per_beat)

    meta_track = extract_global_meta_messages(mid)
    new_mid.tracks.append(meta_track)

    melodic_tracks = [track for track in mid.tracks if is_melodic_track(track)]
    new_mid.tracks.extend(melodic_tracks)

    out_bytes_io = BytesIO()
    new_mid.save(file=out_bytes_io)
    return out_bytes_io.getvalue()


def process_midi_to_guitar_from_midi(midi_data, max_frets=10):
    stripped_bytes = strip_non_melodic_and_preserve_tempo(midi_data)
    mid = MidiFile(file=BytesIO(stripped_bytes))  # Reload cleaned version
    PPQ = mid.ticks_per_beat
    DEFAULT_TEMPO = 500000  # µs per quarter note

    STRING_OPEN_NOTES = {
        6: 40,  # E2
        5: 45,  # A2
        4: 50,  # D3
        3: 55,  # G3
        2: 59,  # B3
        1: 64   # E4
    }

    # Store active notes per string
    active_notes = {s: None for s in STRING_OPEN_NOTES}
    events = []
    tempo = DEFAULT_TEMPO
    current_time = 0.0

    # Merge tracks to handle absolute timing correctly
    merged = merge_tracks(mid.tracks)

    for msg in merged:
        current_time += mido.tick2second(msg.time, PPQ, tempo)

        if msg.type == 'set_tempo':
            tempo = msg.tempo

        elif msg.type == 'note_on' and msg.velocity > 0 and msg.channel != 9:
            note = msg.note
            note_played = False

            for s in sorted(STRING_OPEN_NOTES.keys(), reverse=True):
                open_note = STRING_OPEN_NOTES[s]
                fret = note - open_note
                if 0 <= fret <= max_frets:
                    if active_notes[s] is None:
                        event_time_ms = round(current_time * 1000)
                        events.append({ "time": event_time_ms, "string": s, "fret": fret })
                        active_notes[s] = note
                        note_played = True
                        break
            # If not played, silently skip

        elif (msg.type == 'note_off' or (msg.type == 'note_on' and msg.velocity == 0)) and msg.channel != 9:
            note = msg.note
            for s in active_notes:
                if active_notes[s] == note:
                    event_time_ms = round(current_time * 1000)
                    events.append({ "time": event_time_ms, "string": s, "fret": -1 })
                    active_notes[s] = None
                    break

    # Sort events just in case
    events.sort(key=lambda e: e["time"])
    duration_ms = events[-1]["time"] if events else 0
    duration_minutes, duration_seconds = divmod(duration_ms // 1000, 60)
    duration_formatted = f"{duration_minutes}:{duration_seconds:02}"

    return { "duration": duration_formatted, "events": events }
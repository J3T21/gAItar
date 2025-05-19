import uvicorn
import mido
import torch
import pickle
import uuid
from io import BytesIO
from mido import MidiFile, MidiTrack, merge_tracks
from typing import List
from fastapi import FastAPI, File, UploadFile, Form, HTTPException
from fastapi.middleware.cors import CORSMiddleware
from fastapi.responses import FileResponse
from pydantic import BaseModel
from transformers import T5Tokenizer
from huggingface_hub import hf_hub_download
from model.transformer_model import Transformer
from miditok import TokenizerConfig

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
class PromptRequest(BaseModel):
    prompt: str

# ---------- Load Model + Tokenizer ----------
repo_id = "amaai-lab/text2midi"
model_path = hf_hub_download(repo_id=repo_id, filename="pytorch_model.bin")
tokenizer_path = hf_hub_download(repo_id=repo_id, filename="vocab_remi.pkl")

device = 'cuda' if torch.cuda.is_available() else 'mps' if torch.backends.mps.is_available() else 'cpu'
print(f"Using device: {device}")

with open(tokenizer_path, "rb") as f:
    r_tokenizer = pickle.load(f)

default_config = TokenizerConfig()
for attr in vars(default_config):
    if not hasattr(r_tokenizer.config, attr):
        setattr(r_tokenizer.config, attr, getattr(default_config, attr))

vocab_size = len(r_tokenizer)
model = Transformer(vocab_size, 768, 8, 2048, 18, 1024, False, 8, device=device)
model.load_state_dict(torch.load(model_path, map_location=device))
model.eval()

tokenizer = T5Tokenizer.from_pretrained("google/flan-t5-base")
# Patch config if missing fields

print("Model and tokenizer loaded.")



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

@app.post("/generate-midi/")
def generate_midi(request: PromptRequest):
    prompt = request.prompt
    print(f"Generating MIDI for: {prompt}")

    try:
        inputs = tokenizer(prompt, return_tensors='pt', padding=True, truncation=True)
        input_ids = torch.nn.utils.rnn.pad_sequence(inputs.input_ids, batch_first=True, padding_value=0).to(device)
        attention_mask = torch.nn.utils.rnn.pad_sequence(inputs.attention_mask, batch_first=True, padding_value=0).to(device)

        output = model.generate(input_ids, attention_mask, max_len=500, temperature=1.0)
        output_list = output[0].tolist()

        generated_midi = r_tokenizer.decode(output_list)
        print("Generated MIDI object:", generated_midi)
        print("Type of generated_midi:", type(generated_midi))

        filename = f"output_{uuid.uuid4().hex[:8]}.mid"
        generated_midi.dump_midi(filename)



        return FileResponse(filename, media_type="audio/midi", filename=filename)

    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))





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
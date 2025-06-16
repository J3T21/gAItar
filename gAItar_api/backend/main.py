import uvicorn
import struct
import mido
import torch
import pickle
import uuid
import tempfile
import os
from io import BytesIO
from mido import MidiFile, MidiTrack, merge_tracks
from typing import List
from fastapi import FastAPI, File, UploadFile, Form, HTTPException, BackgroundTasks
from fastapi.middleware.cors import CORSMiddleware
from fastapi.responses import Response, FileResponse
from pydantic import BaseModel
from transformers import T5Tokenizer
from huggingface_hub import hf_hub_download
from model.transformer_model import Transformer
from miditok import TokenizerConfig
from basic_pitch.inference import predict
from basic_pitch import ICASSP_2022_MODEL_PATH
import pretty_midi
import tensorflow as tf

gpus = tf.config.experimental.list_physical_devices('GPU')
if gpus:
    try:
        for gpu in gpus:
            tf.config.experimental.set_memory_growth(gpu, True)
    except RuntimeError as e:
        print(f"GPU configuration error: {e}")

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
    ir = process_midi_to_guitar_from_midi(contents, max_frets=12)
    return {
        "title": title,
        "artist": artist,
        "genre": genre,
        "duration_formatted": ir["duration"],
        "events" : ir["events"]
    }



@app.post("/generate-midi/")
def generate_midi(request: PromptRequest, background_tasks: BackgroundTasks):
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

        # Schedule file cleanup after response is sent
        background_tasks.add_task(cleanup_file, filename)

        return FileResponse(
            filename, 
            media_type="audio/midi", 
            filename=filename,
            headers={"Content-Disposition": f"attachment; filename={filename}"}
        )

    except Exception as e:
        # Clean up file if it was created but an error occurred
        if 'filename' in locals() and os.path.exists(filename):
            os.unlink(filename)
        raise HTTPException(status_code=500, detail=str(e))

def cleanup_file(filename: str):
    """Background task to clean up generated files"""
    try:
        if os.path.exists(filename):
            os.unlink(filename)
            print(f"Cleaned up file: {filename}")
    except Exception as e:
        print(f"Error cleaning up file {filename}: {e}")


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

@app.post("/convert-audio-to-midi/")
async def convert_audio_to_midi(background_tasks: BackgroundTasks, audio_file: UploadFile = File(...)):
    """Convert audio file (MP3, WAV, etc.) to MIDI using basic_pitch with TensorFlow."""
    
    # Support multiple audio formats
    supported_extensions = ['.mp3', '.wav', '.m4a', '.flac', '.ogg', '.webm']
    file_extension = None
    
    for ext in supported_extensions:
        if audio_file.filename.lower().endswith(ext):
            file_extension = ext
            break
    
    if not file_extension:
        raise HTTPException(
            status_code=400, 
            detail=f"File must be one of: {', '.join(supported_extensions)}"
        )
    
    try:
        # Read the uploaded audio file
        audio_contents = await audio_file.read()
        
        # Create a temporary file to store the audio
        with tempfile.NamedTemporaryFile(delete=False, suffix=file_extension) as temp_audio:
            temp_audio.write(audio_contents)
            temp_audio_path = temp_audio.name
        
        # Use basic_pitch with TensorFlow runtime (don't specify model_or_model_path to use default TF model)
        print(f"Converting audio file using TensorFlow runtime: {temp_audio_path}")
        model_output, midi_data, note_events = predict(temp_audio_path)
        print("Audio to MIDI conversion completed with TensorFlow")
        
        # Generate output filename
        output_filename = f"converted_tf_{uuid.uuid4().hex[:8]}.mid"
        
        # Save the MIDI file
        midi_data.write(output_filename)
        print(f"MIDI file saved: {output_filename}")
        
        # Clean up temporary audio file immediately
        os.unlink(temp_audio_path)
        
        # Schedule MIDI file cleanup after response is sent
        background_tasks.add_task(cleanup_file, output_filename)
        
        # Return the MIDI file
        return FileResponse(
            output_filename, 
            media_type="audio/midi", 
            filename=output_filename,
            headers={"Content-Disposition": f"attachment; filename={output_filename}"}
        )
        
    except Exception as e:
        print(f"Error in convert_audio_to_midi: {str(e)}")
        # Clean up temporary files if they exist
        if 'temp_audio_path' in locals() and os.path.exists(temp_audio_path):
            os.unlink(temp_audio_path)
        if 'output_filename' in locals() and os.path.exists(output_filename):
            os.unlink(output_filename)
        raise HTTPException(status_code=500, detail=f"Error converting audio to MIDI: {str(e)}")


def serialize_guitar_events_micro(events_data):
    """Ultra-compact binary format optimized for Adafruit Grand Central"""
    events = events_data["events"]
    
    # Header: total_duration_ms (4 bytes) + event_count (2 bytes) = 6 bytes
    # Fix: Use "duration" instead of "duration_formatted" to match the return from process_midi_to_guitar_from_midi
    duration_key = "duration_formatted" if "duration_formatted" in events_data else "duration"
    duration_parts = events_data[duration_key].split(":")
    total_duration_ms = (int(duration_parts[0]) * 60 + int(duration_parts[1])) * 1000
    
    # Pack header: duration (4 bytes) + event count (2 bytes)
    binary_data = struct.pack('>IH', total_duration_ms, len(events))
    
    # Each event: time_ms (4 bytes) + packed_string_fret (1 byte) = 5 bytes per event
    for event in events:
        time_ms = event["time"]
        string = event["string"]  # 1-6
        fret = 31 if event["fret"] == -1 else event["fret"]  # 0-30 or 31 for fret-off
        
        # Pack string (3 bits: 0-7) + fret (5 bits: 0-31)
        # String 1-6 becomes 0-5 in 3 bits, fret 0-30 or 31 in 5 bits
        packed_byte = (string << 5) | fret
        
        # 5 bytes per event: 4 bytes time + 1 byte string_fret
        binary_data += struct.pack('>IB', time_ms, packed_byte)
    
    return binary_data

@app.post("/upload-midi-binary")
async def upload_midi_binary(midi_file: UploadFile = File(...), title: str = Form(...), artist: str = Form(...), genre: str = Form(...)):
    """Upload MIDI file and return binary format optimized for microcontroller"""
    contents = await midi_file.read()
    ir = process_midi_to_guitar_from_midi(contents, max_frets=12)
    
    binary_data = serialize_guitar_events_micro(ir)
    
    return Response(
        content=binary_data,
        media_type="application/octet-stream",
        headers={
            "Content-Disposition": f"attachment; filename=guitar_events.bin",
            "X-Title": title,
            "X-Artist": artist,
            "X-Genre": genre,
            "X-Duration": ir["duration"],
            "X-Size": str(len(binary_data)),
            "X-Event-Count": str(len(ir["events"]))
        }
    )
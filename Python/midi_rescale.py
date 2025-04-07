import mido
import struct
from mido import Message, MidiFile, MidiTrack, bpm2tempo
import serial
import struct
import time
import pyperclip
def extract_global_meta_messages(mid):
    """Gather all tempo/key/time meta messages from all tracks."""
    meta_track = mido.MidiTrack()

    for track in mid.tracks:
        time_counter = 0
        for msg in track:
            time_counter += msg.time
            if msg.is_meta and msg.type in ['set_tempo', 'time_signature', 'key_signature']:
                meta_copy = msg.copy(time=msg.time)
                meta_track.append(meta_copy)

    return meta_track

def is_melodic_track(track):
    """Return True if track contains note messages and is not percussion."""
    for msg in track:
        if msg.type in ['note_on', 'note_off']:
            if hasattr(msg, 'channel') and msg.channel == 9:
                return False  # Percussion track
            return True
    return False  # No notes at all

def strip_non_melodic_and_preserve_tempo(input_file, output_file):
    mid = mido.MidiFile(input_file)
    new_mid = mido.MidiFile(ticks_per_beat=mid.ticks_per_beat)

    # Extract and add global meta messages
    meta_track = extract_global_meta_messages(mid)
    new_mid.tracks.append(meta_track)

    # Add only melodic tracks
    melodic_tracks = [track for track in mid.tracks if is_melodic_track(track)]
    new_mid.tracks.extend(melodic_tracks)

    new_mid.save(output_file)
    print(f"🎵 Saved: {output_file} (melodic-only with preserved tempo changes)")

def process_midi_to_guitar_from_midi(file_path, max_frets=12):
    mid = mido.MidiFile(file_path)
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

    # Store active notes per string to ensure a string isn't used for multiple notes simultaneously
    active_notes = {s: None for s in STRING_OPEN_NOTES}

    absolute_time = 0.0  # Start time for the first event
    events = []
    tempo = DEFAULT_TEMPO

    for track in mid.tracks:
        time = 0.0
        local_tempo = tempo

        for msg in track:
            time += mido.tick2second(msg.time, PPQ, local_tempo)

            if msg.type == 'set_tempo':
                local_tempo = msg.tempo

            elif msg.type == 'note_on' and msg.velocity > 0 and msg.channel != 9:
                note = msg.note
                note_played = False  # Flag to track if the note was successfully played

                for s in sorted(STRING_OPEN_NOTES.keys(), reverse=True):  # Start with lower strings first
                    open_note = STRING_OPEN_NOTES[s]
                    fret = note - open_note
                    if 0 <= fret <= max_frets:
                        # Check if the string is already in use (active note is mapped to it)
                        if active_notes[s] is None:
                            event_time_ms = round((absolute_time + time) * 1000)  # Absolute time in ms
                            events.append((event_time_ms, s, fret, 'on'))
                            active_notes[s] = note  # Mark the string as in use by this note
                            note_played = True
                            break  # Note played successfully, stop searching

                if not note_played:
                    # Skip the note if no string was available
                    continue

            elif (msg.type == 'note_off' or (msg.type == 'note_on' and msg.velocity == 0)) and msg.channel != 9:
                note = msg.note
                for s in active_notes:
                    if active_notes[s] == note:  # Find the string associated with the note
                        event_time_ms = round((absolute_time + time) * 1000)  # Absolute time in ms
                        events.append((event_time_ms, s, -1, 'off'))  # Note-off event
                        active_notes[s] = None  # Release the string after note-off
                        break

    # Sort events by absolute time
    events.sort()
    output = []
    for t, s, f, status in events:
        output.append((t, s, f, status))

    # Format the output as a C-style array
    event_string = "const int events[][3] = {\n"
    for absolute_time, string, fret, status in output:
        event_string += f"    {{{absolute_time}, {string}, {fret}}},\n"
    event_string = event_string.rstrip(",\n")  # Remove last comma
    event_string += "\n};"
    pyperclip.copy(event_string)
    return event_string



def write_guitar_events_to_binary(events, output_path):
    """
    Write (delta_ms, string, fret) guitar events to a compact binary file.
    Each event is 4 bytes: uint16 (delta), uint8 (string), uint8 (fret)
    """
    with open(output_path, "wb") as f:
        for delta_ms, string, fret in events:
            # Pack as little-endian: H (uint16), B (uint8), B (uint8)
            packed = struct.pack('<HBB', delta_ms, string, fret)
            f.write(packed)

    print(f"📦 Binary event file written to {output_path}")


def send_guitar_events_over_serial(events, port='COM6', baudrate=9600):
    """
    Send (delta_ms, string, fret) events over a serial port in binary.
    """
    with serial.Serial(port, baudrate, timeout=1) as ser:
        print(f"📡 Sending to {port} at {baudrate} baud")

        for delta_ms, string, fret in events:
            # Pack into 4 bytes: uint16 (delta), uint8 (string), uint8 (fret)
            packet = struct.pack('<HBB', delta_ms, string, fret)
            ser.write(packet)  # Send the event

            # Wait for the microcontroller to acknowledge it is ready
            ack = ser.read(3)  # Wait for 3 bytes (e.g., 'READY' message or simple ack)
            if ack != b'ACK':  # Check if the response is 'ACK'
                time.sleep(0.001)  # Small delay in seconds to avoid overwhelming the serial buffer

        print("✅ All events sent.")

def guitar_commands_to_midi(guitar_commands, output_file, tempo_bpm=120, default_duration_ms=300):
    """
    Convert guitar (delta_ms, string, fret) commands back into a MIDI file for verification.
    
    Args:
        guitar_commands: List of tuples in the form (delta_ms, string, fret).
        output_file: Path to save the generated MIDI file.
        tempo_bpm: Tempo of the MIDI file in beats per minute (default 120 BPM).
        default_duration_ms: Default duration for the note in milliseconds.
    """
    # Define the open string notes for standard guitar tuning (MIDI numbers)
    STRING_OPEN_NOTES = {
        1: 64,  # High E (E4)
        2: 59,  # B (B3)
        3: 55,  # G (G3)
        4: 50,  # D (D3)
        5: 45,  # A (A2)
        6: 40   # Low E (E2)
    }
    
    # Convert BPM to tempo in microseconds per beat
    tempo = bpm2tempo(tempo_bpm)
    
    # Create a new MIDI file with the given tempo
    mid = MidiFile(ticks_per_beat=480)  # Using 480 ticks per beat (standard PPQ)
    track = MidiTrack()
    mid.tracks.append(track)
    
    # Set the tempo meta message
    track.append(mido.MetaMessage('set_tempo', tempo=tempo))
    
    # Start from tick 0
    current_tick = 0
    
    # Process the guitar commands and convert to MIDI notes
    for delta_ms, string, fret in guitar_commands:
        # Convert delta time from ms to ticks
        delta_seconds = delta_ms / 1000
        delta_ticks = round(mido.second2tick(delta_seconds, 480, tempo))
        current_tick += delta_ticks
        
        # Calculate the MIDI note number
        note = STRING_OPEN_NOTES[string] + fret  # Add fret to open string note
        
        # Set the note duration in ticks (default duration)
        duration_ticks = round(mido.second2tick(default_duration_ms / 1000, 480, tempo))
        
        # Add the note-on and note-off events
        track.append(Message('note_on', note=note, velocity=64, time=current_tick))
        track.append(Message('note_off', note=note, velocity=64, time=duration_ticks))
        
        # Reset current tick after note-off
        current_tick = 0
    
    # Save the MIDI file to the specified output path
    mid.save(output_file)
    print(f"✅ MIDI file saved to {output_file}")



if __name__ == "__main__":
    input_file = "midi_tracks/Bach_P.mid"
    output_file = "midi_tracks/processed.mid"
    strip_non_melodic_and_preserve_tempo(input_file, output_file)

    guitar_commands = process_midi_to_guitar_from_midi(output_file, max_frets=10)
    print("Guitar commands:", guitar_commands)


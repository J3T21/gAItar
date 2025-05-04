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


def process_midi_to_guitar_from_midi(midi_data, max_frets=12):
    stripped_bytes = strip_non_melodic_and_preserve_tempo(midi_data)
    mid = MidiFile(file=BytesIO(stripped_bytes))  # ← Reload cleaned version
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
    duration_ms = events[-1][0] if events else 0  # Get the duration of the last event
    duration_minutes, duration_seconds = divmod(duration_ms // 1000, 60)
    duration_formatted = f"{duration_minutes}:{duration_seconds:02}"
    output = []
    for t, s, f, status in events:
        output.append((t, s, f, status))

    # Format the output as a C-style array
    event_string = "const int events[][3] = {\n"
    for absolute_time, string, fret, status in output:
        event_string += f"    {{{absolute_time}, {string}, {fret}}},\n"
    event_string = event_string.rstrip(",\n")  # Remove last comma
    event_string += "\n};"
    return {"duration" : duration_formatted, "events": event_string}
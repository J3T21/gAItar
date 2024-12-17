import mido



def print_midi_file(file_path):
    '''few notes about midi:
    - midi files are composed of tracks
    - each track is composed of messages
    - each message is composed of a type and a value
    - the type can be note_on, note_off, control_change, etc
    - time is mesured in pulses per quarter note (ppqn)'''
    mid = mido.MidiFile(file_path)
    with open("midi_messages_nor.txt", "w") as f:
        for i, track in enumerate(mid.tracks):
            f.write(f"Track {i}: {track.name}\n")
            for msg in track:
                f.write(f"{msg}\n")
    # for i, track in enumerate(mid.tracks):
    #     print(f"Track {i}: {track.name}")
    #     for msg in track:
    #         if msg.is_meta:
    #             print(msg)
    # #mid.print_tracks()
    # for i, track in enumerate(mid.tracks):
    #     for msg in track:
    #         if msg.type == 'program_change':
    #             print(f"Track {i} Channel {msg.channel} Instrument {msg.program}")
    new_con = mido.MidiFile()
    for i, track in enumerate(mid.tracks):
        new_mid = mido.MidiFile()
        new_file_path = f"track_{i}.mid"
        new_con.tracks.append(track)
        new_mid.tracks.append(track)
        new_mid.save(new_file_path)
        print(f"Saved track {i} as {new_file_path}")
    new_con.save("test1.mid")
    for msg in new_con:
        if msg.type == 'program_change':
            print(f"Track {i} Channel {msg.channel} Instrument {msg.program}")
        #print(msg)
if __name__ == '__main__':
    print_midi_file('midi_tracks/LED ZEPPELIN.Stairway to heaven K.mid')
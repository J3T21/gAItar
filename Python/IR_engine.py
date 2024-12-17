import mido
Recuerdos= mido.MidiFile('midi_tracks/Tarrega_Recuerdos_de_la_Alhambra.mid',clip=True)
stairway = mido.MidiFile('midi_tracks/LED ZEPPELIN.Stairway to heaven K.mid',clip=True)
norwegian= mido.MidiFile('midi_tracks/Norwegian-Wood.mid',clip=True)
gran= mido.MidiFile('midi_tracks/Tarrega_Gran_Vals.mid',clip=True)


def main():
    '''3 types of midi files: 0 (single track), 1 (synchronous), 2(asynchronous) '''
    #print(norwegian)
    for track in stairway.tracks:
        for msg in track:
            if msg.type == 'program_change':
                print(f"Track {track} Channel {msg.channel} Instrument {msg.program}")





if __name__ == '__main__':
    main()
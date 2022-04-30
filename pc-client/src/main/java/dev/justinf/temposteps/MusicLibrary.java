package dev.justinf.temposteps;

import javazoom.jl.player.Player;

import java.io.File;
import java.io.FileInputStream;
import java.util.ArrayList;
import java.util.List;
import java.util.Scanner;

public class MusicLibrary {

    private final TempoStepsApp app;
    private final List<Song> songs;
    private Thread playerThread;
    
    public MusicLibrary(TempoStepsApp app) {
        this.app = app;
        songs = new ArrayList<>();
    }

    public int importSongs(String filePath) {
        try {
            File f = new File(filePath);
            Scanner in = new Scanner(f);
            while (in.hasNextLine()) {
                String line = in.nextLine();
                String[] parts = line.split("\t");
                songs.add(new Song(parts[0], parts[1], Integer.parseInt(parts[2]), parts[3]));
            }
            return songs.size();
        } catch (Exception e) {
            e.printStackTrace();
            return -1;
        }
    }

    public void play(float bpm){
        if (songs.isEmpty()) return;
        float lowestDiff = Float.MAX_VALUE;
        Song toPlay = null;
        for (Song s : songs) {
            if (s.diff(bpm) < lowestDiff) {
                toPlay = s;
                lowestDiff = s.diff(bpm);
            }
        }

        // Sanity check
        if (toPlay == null) return;

        try {
            playerThread.stop(); // evil
        } catch (Exception ignored) { }

        Song finalToPlay = toPlay;
        playerThread = new Thread(() -> {
            try {
                FileInputStream fis = new FileInputStream(finalToPlay.getPath());
                Player mp3Player = new Player(fis);
                app.log("Now playing: " + finalToPlay.getTitle() + " - " + finalToPlay.getArtist() + " (" + finalToPlay.getBpm() + "BPM)");
                mp3Player.play();
            } catch (Exception e) {
                e.printStackTrace();
            }
        });
        playerThread.start();
    }
}
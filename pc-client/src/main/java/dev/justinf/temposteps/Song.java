package dev.justinf.temposteps;

public class Song {

    private final String title;
    private final String artist;
    private final int bpm;
    private final String path;

    public Song(String title, String artist, int bpm, String path) {
        this.title = title;
        this.artist = artist;
        this.bpm = bpm;
        this.path = path;
    }

    public float diff(float bpm) {
        return Math.abs(bpm - this.bpm);
    }

    public String getTitle() {
        return title;
    }

    public String getArtist() {
        return artist;
    }

    public int getBpm() {
        return bpm;
    }

    public String getPath() {
        return path;
    }
}
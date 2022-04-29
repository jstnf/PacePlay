package dev.justinf.temposteps;

import javax.swing.*;

public class TempoStepsApp {

    private static final String LIBRARY_PATH = "./library.tsv";

    private final SerialTerminal arduino;
    private final InfoFrame window;
    private final MusicLibrary musicLibrary;
    private Thread connectionThread;

    public TempoStepsApp() {
        arduino = new SerialTerminal(this);
        window = new InfoFrame(this);
        musicLibrary = new MusicLibrary(this);
        connectionThread = null;
    }

    public void start() {
        musicLibrary.importSongs(LIBRARY_PATH); // can change

        Thread updateThread = new Thread(() -> {
            while (true) {
                window.update();
                Thread.yield();
            }
        });

        updateThread.start();
        testConnection();

        JFrame frame = new JFrame("TempoSteps");
        frame.setContentPane(window.getMainPanel());
        frame.setDefaultCloseOperation(WindowConstants.EXIT_ON_CLOSE);
        frame.pack();
        frame.setResizable(false);
        frame.setLocationRelativeTo(null);
        frame.setVisible(true);
    }

    public void log(Object o) {
        System.out.println(o.toString());
        window.getConsoleTextArea().append("\n" + o.toString());
    }

    public void queueData(String data) {
        Thread t = new Thread(() -> {
            processData(data);
        });
        t.start();
    }

    public void testConnection() {
        if (connectionThread != null && connectionThread.isAlive()) connectionThread.interrupt();
        connectionThread = new Thread(() -> {
            window.atomicallySetConnectionResult((arduino.establishConnection()));
        });
        connectionThread.start();
    }

    public void queueSongPlay(float bpm) {
        // We can't make this blocking, or we'll never get out of processData
        log("Attempting to play song that matches " + bpm + "BPM!");
        musicLibrary.play(bpm);
    }

    // This should be thread-safe :)
    private String buffer;

    public synchronized void processData(String s) {
        for (char c : s.toCharArray()) {
            switch (c) {
                case 's':
                    // Reset the buffer
                    buffer = "";
                    break;
                case 'e':
                    // Test if the current buffer is a valid float, if so pass it off to the app
                    try {
                        queueSongPlay(Float.parseFloat(buffer));
                    } catch (NumberFormatException ignored) { }
                    buffer = "";
                    break;
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
                case '.':
                    buffer = buffer.concat(String.valueOf(c));
                    break;
            }
        }
    }

    public SerialTerminal getArduino() {
        return arduino;
    }
}
package dev.justinf.temposteps;

public class TempoStepsApp {

    private final SerialTerminal arduino;

    public TempoStepsApp() {
        arduino = new SerialTerminal(this);
    }

    public void start() {
        arduino.setPort("COM15");
        arduino.establishConnection();
    }

    public void log(Object o) {
        System.out.println(o.toString());
    }

    public void queueData(String data) {
        Thread t = new Thread(() -> {
            processData(data);
        });
        t.start();
    }

    public void queueSongPlay(float bpm) {
        // We can't make this blocking, or we'll never get out of processData
        System.out.println(bpm);
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
}
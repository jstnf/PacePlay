package dev.justinf.temposteps;

public class TempoStepsApp {

    private final SerialTerminal arduino;

    public TempoStepsApp() {
        arduino = new SerialTerminal(this);
    }

    public void start() {

    }

    public void log(Object o) {
        System.out.println(o.toString());
    }
}
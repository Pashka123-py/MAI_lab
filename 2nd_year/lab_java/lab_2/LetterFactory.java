public class LetterFactory implements PostalFactory {
    @Override
    public Packaging createPackaging() {
        return new Envelope();
    }

    @Override
    public Documentation createDocumentation() {
        return new Stamp();
    }
}

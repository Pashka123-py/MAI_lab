public class ParcelFactory implements PostalFactory {
    @Override
    public Packaging createPackaging() {
        return new CardboardBox();
    }

    @Override
    public Documentation createDocumentation() {
        return new Invoice();
    }
}
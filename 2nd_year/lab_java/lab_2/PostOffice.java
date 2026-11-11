public class PostOffice {
    private Packaging packaging;
    private Documentation documentation;

    public PostOffice(PostalFactory factory) {
        this.packaging = factory.createPackaging();
        this.documentation = factory.createDocumentation();
    }

    public void processItem(PostalItem item) {
        System.out.println("\n--- Обработка: " + item.toString() + " ---");
        item.register();
        packaging.pack();
        documentation.printDocs();
        item.calculateShippingCost();
        item.printLabel();
        System.out.println("----------------------------------------");
    }
}
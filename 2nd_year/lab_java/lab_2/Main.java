public class Main {
    public static void main(String[] args) {
        PostalItem letter = new PostalItem("RU123456789", 0.05, "Москва");
        PostalItem parcel = new PostalItem("RU987654321", 5.5, "Санкт-Петербург");

        PostOffice letterOffice = new PostOffice(new LetterFactory());
        letterOffice.processItem(letter);

        PostOffice parcelOffice = new PostOffice(new ParcelFactory());
        parcelOffice.processItem(parcel);
    }
}
public class PostalItem {
    private String trackingNumber;
    private double weight;
    private String destination;

    public PostalItem(String trackingNumber, double weight, String destination) {
        this.trackingNumber = trackingNumber;
        this.weight = weight;
        this.destination = destination;
    }

    public String getTrackingNumber() { return trackingNumber; }
    public void setTrackingNumber(String trackingNumber) { this.trackingNumber = trackingNumber; }
    public double getWeight() { return weight; }
    public void setWeight(double weight) { this.weight = weight; }
    public String getDestination() { return destination; }
    public void setDestination(String destination) { this.destination = destination; }

    public void register() {
        System.out.println("Отправление " + trackingNumber + " зарегистрировано.");
    }

    public void calculateShippingCost() {
        System.out.println("Стоимость пересылки в " + destination + ": " + (weight * 100) + " руб.");
    }

    public void printLabel() {
        System.out.println("Печать этикетки для " + trackingNumber);
    }

    @Override
    public String toString() {
        return "PostalItem [" + trackingNumber + ", " + weight + "kg, to " + destination + "]";
    }
}

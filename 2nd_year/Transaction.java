public class Transaction extends BankOperation {
    // 3 собственных атрибута класса
    private double amount;
    private String currency;
    private String description;

    public Transaction(String operationId, String date, String status,
                       double amount, String currency, String description) {
        super(operationId, date, status);
        this.amount = amount;
        this.currency = currency;
        this.description = description;
    }

    // Геттеры и сеттеры
    public double getAmount() {
        return amount;
    }

    public void setAmount(double amount) {
        this.amount = amount;
    }

    public String getCurrency() {
        return currency;
    }

    public void setCurrency(String currency) {
        this.currency = currency;
    }

    public String getDescription() {
        return description;
    }

    public void setDescription(String description) {
        this.description = description;
    }

    // 3 собственных метода класса
    public void printReceipt() {
        System.out.println("Чек: " + amount + " " + currency +
                " | " + description + " | Статус: " + getStatus());
    }

    public double applyCommission(double percent) {
        double commission = amount * percent / 100.0;
        System.out.println("Комиссия: " + commission + " " + currency);
        return commission;
    }

    public boolean isSuccessful() {
        return "DONE".equalsIgnoreCase(getStatus());
    }
}
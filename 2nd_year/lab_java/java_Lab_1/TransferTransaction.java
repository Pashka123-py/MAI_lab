public class TransferTransaction extends Transaction {
    // 3 собственных атрибута класса
    private String senderAccount;
    private String receiverAccount;
    private double exchangeRate;

    public TransferTransaction(String operationId, String date, String status,
                               double amount, String currency, String description,
                               String senderAccount, String receiverAccount, double exchangeRate) {
        super(operationId, date, status, amount, currency, description);
        this.senderAccount = senderAccount;
        this.receiverAccount = receiverAccount;
        this.exchangeRate = exchangeRate;
    }

    // Геттеры и сеттеры
    public String getSenderAccount() {
        return senderAccount;
    }

    public void setSenderAccount(String senderAccount) {
        this.senderAccount = senderAccount;
    }

    public String getReceiverAccount() {
        return receiverAccount;
    }

    public void setReceiverAccount(String receiverAccount) {
        this.receiverAccount = receiverAccount;
    }

    public double getExchangeRate() {
        return exchangeRate;
    }

    public void setExchangeRate(double exchangeRate) {
        this.exchangeRate = exchangeRate;
    }

    // 3 собственных метода класса
    public double getConvertedAmount() {
        return getAmount() * exchangeRate;
    }

    public void printTransferDetails() {
        System.out.println("Перевод: " + senderAccount + " -> " + receiverAccount +
                " | Сумма: " + getAmount() + " " + getCurrency() +
                " | Конвертация: " + getConvertedAmount());
    }

    public boolean isSameCurrencyTransfer() {
        return Math.abs(exchangeRate - 1.0) < 1e-9;
    }
}

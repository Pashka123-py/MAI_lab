public class Main {
    public static void main(String[] args) {
        TransferTransaction transfer = new TransferTransaction(
                "TXN-001", "2025-01-15", "PENDING",
                1000.0, "USD", "Оплата за услуги",
                "ACC-111", "ACC-222", 1.05
        );

        // Методы родителя (BankOperation)
        transfer.startOperation();
        transfer.completeOperation();

        // Методы Transaction
        transfer.printReceipt();
        double commission = transfer.applyCommission(2.0);
        System.out.println("Успешна ли транзакция: " + transfer.isSuccessful());

        // Методы TransferTransaction
        transfer.printTransferDetails();
        System.out.println("Конвертированная сумма: " + transfer.getConvertedAmount());
        System.out.println("Одна ли валюта: " + transfer.isSameCurrencyTransfer());

        // Работа сеттеров
        transfer.setStatus("DONE");
        transfer.setAmount(1500.0);
        transfer.setExchangeRate(1.0);

        System.out.println("После изменений:");
        transfer.printTransferDetails();
        System.out.println("Одна ли валюта: " + transfer.isSameCurrencyTransfer());
    }
}
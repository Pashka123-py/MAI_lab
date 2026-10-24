public class BankOperation {
    // 3 собственных атрибута класса
    private String operationId;
    private String date;
    private String status; // "PENDING", "DONE", "FAILED"

    public BankOperation(String operationId, String date, String status) {
        this.operationId = operationId;
        this.date = date;
        this.status = status;
    }

    // Геттеры и сеттеры
    public String getOperationId() {
        return operationId;
    }

    public void setOperationId(String operationId) {
        this.operationId = operationId;
    }

    public String getDate() {
        return date;
    }

    public void setDate(String date) {
        this.date = date;
    }

    public String getStatus() {
        return status;
    }

    public void setStatus(String status) {
        this.status = status;
    }

    // 3 собственных метода класса
    public void startOperation() {
        this.status = "PENDING";
        System.out.println("Операция " + operationId + " начата.");
    }

    public void completeOperation() {
        this.status = "DONE";
        System.out.println("Операция " + operationId + " завершена.");
    }

    public void cancelOperation() {
        this.status = "FAILED";
        System.out.println("Операция " + operationId + " отменена.");
    }
}
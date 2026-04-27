#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

// Types of tree nodes
typedef enum {
    NODE_NUMBER,
    NODE_UNARY_MINUS,
    NODE_BINARY_OP
} NodeType;

// Binary operations
typedef enum {
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV
} BinOp;

// Expression tree node
typedef struct ExprNode {
    NodeType type;
    union {
        double number;          // for NODE_NUMBER
        BinOp bin_op;           // for NODE_BINARY_OP
    } data;
    struct ExprNode *left;      // left operand (for binary and unary minus)
    struct ExprNode *right;     // right operand (only for binary)
} ExprNode;

// Global string for parsing
const char *input_str;
int pos;

// Function prototypes
ExprNode* parse_expression();
ExprNode* parse_term();
ExprNode* parse_factor();
ExprNode* parse_number();
void skip_spaces();
ExprNode* create_number_node(double val);
ExprNode* create_unary_minus_node(ExprNode* operand);
ExprNode* create_binary_node(BinOp op, ExprNode* left, ExprNode* right);
ExprNode* transform_tree(ExprNode* node);
void print_tree(ExprNode* node);
void print_tree_structure(ExprNode* node, int level);
void free_tree(ExprNode* node);

// Helper functions
void skip_spaces() {
    while (input_str[pos] == ' ') pos++;
}

ExprNode* create_number_node(double val) {
    ExprNode* node = (ExprNode*)malloc(sizeof(ExprNode));
    node->type = NODE_NUMBER;
    node->data.number = val;
    node->left = node->right = NULL;
    return node;
}

ExprNode* create_unary_minus_node(ExprNode* operand) {
    ExprNode* node = (ExprNode*)malloc(sizeof(ExprNode));
    node->type = NODE_UNARY_MINUS;
    node->left = operand;
    node->right = NULL;
    return node;
}

ExprNode* create_binary_node(BinOp op, ExprNode* left, ExprNode* right) {
    ExprNode* node = (ExprNode*)malloc(sizeof(ExprNode));
    node->type = NODE_BINARY_OP;
    node->data.bin_op = op;
    node->left = left;
    node->right = right;
    return node;
}

// Recursive descent parser
ExprNode* parse_factor() {
    skip_spaces();
    ExprNode* node = NULL;
    
    if (input_str[pos] == '(') {
        pos++; // skip '('
        node = parse_expression();
        skip_spaces();
        if (input_str[pos] == ')') pos++;
        else printf("Error: expected ')'\n");
    }
    else if (input_str[pos] == '-') {
        // Unary minus
        pos++;
        ExprNode* operand = parse_factor();
        node = create_unary_minus_node(operand);
    }
    else if (isdigit(input_str[pos]) || input_str[pos] == '.') {
        node = parse_number();
    }
    else {
        printf("Error: unexpected character '%c'\n", input_str[pos]);
        exit(1);
    }
    return node;
}

ExprNode* parse_number() {
    double val = 0.0;
    int int_part = 0;
    double frac_part = 0.0;
    int frac_len = 0;
    
    // Integer part
    while (isdigit(input_str[pos])) {
        int_part = int_part * 10 + (input_str[pos] - '0');
        pos++;
    }
    val = int_part;
    
    // Fractional part
    if (input_str[pos] == '.') {
        pos++;
        while (isdigit(input_str[pos])) {
            frac_part = frac_part * 10 + (input_str[pos] - '0');
            frac_len++;
            pos++;
        }
        while (frac_len-- > 0) frac_part /= 10.0;
        val += frac_part;
    }
    return create_number_node(val);
}

ExprNode* parse_term() {
    ExprNode* node = parse_factor();
    skip_spaces();
    while (input_str[pos] == '*' || input_str[pos] == '/') {
        char op = input_str[pos];
        pos++;
        ExprNode* right = parse_factor();
        BinOp bin_op = (op == '*') ? OP_MUL : OP_DIV;
        node = create_binary_node(bin_op, node, right);
        skip_spaces();
    }
    return node;
}

ExprNode* parse_expression() {
    ExprNode* node = parse_term();
    skip_spaces();
    while (input_str[pos] == '+' || input_str[pos] == '-') {
        char op = input_str[pos];
        pos++;
        ExprNode* right = parse_term();
        BinOp bin_op = (op == '+') ? OP_ADD : OP_SUB;
        node = create_binary_node(bin_op, node, right);
        skip_spaces();
    }
    return node;
}

// Transformation: a - b -> a + (-b)
ExprNode* transform_tree(ExprNode* node) {
    if (node == NULL) return NULL;
    
    // First transform subtrees (post-order traversal)
    node->left = transform_tree(node->left);
    node->right = transform_tree(node->right);
    
    // If node is subtraction, replace with addition and unary minus
    if (node->type == NODE_BINARY_OP && node->data.bin_op == OP_SUB) {
        // Create unary minus for the right subtree
        ExprNode* unary_minus = create_unary_minus_node(node->right);
        // Create new addition node
        ExprNode* new_node = create_binary_node(OP_ADD, node->left, unary_minus);
        // Free the old node (but not its children, they are reused)
        free(node);
        return new_node;
    }
    
    return node;
}

// Print tree in infix form
void print_tree(ExprNode* node) {
    if (node == NULL) return;
    
    switch (node->type) {
        case NODE_NUMBER:
            if (node->data.number == (int)node->data.number)
                printf("%d", (int)node->data.number);
            else
                printf("%g", node->data.number);
            break;
            
        case NODE_UNARY_MINUS:
            printf("(-");
            print_tree(node->left);
            printf(")");
            break;
            
        case NODE_BINARY_OP:
            printf("(");
            print_tree(node->left);
            switch (node->data.bin_op) {
                case OP_ADD: printf("+"); break;
                case OP_SUB: printf("-"); break;
                case OP_MUL: printf("*"); break;
                case OP_DIV: printf("/"); break;
            }
            print_tree(node->right);
            printf(")");
            break;
    }
}

// Print tree structure (for debugging)
void print_tree_structure(ExprNode* node, int level) {
    if (node == NULL) return;
    
    for (int i = 0; i < level; i++) printf("  ");
    
    switch (node->type) {
        case NODE_NUMBER:
            if (node->data.number == (int)node->data.number)
                printf("NUMBER: %d\n", (int)node->data.number);
            else
                printf("NUMBER: %g\n", node->data.number);
            break;
        case NODE_UNARY_MINUS:
            printf("UNARY MINUS\n");
            print_tree_structure(node->left, level + 1);
            break;
        case NODE_BINARY_OP:
            switch (node->data.bin_op) {
                case OP_ADD: printf("ADDITION (+)\n"); break;
                case OP_SUB: printf("SUBTRACTION (-)\n"); break;
                case OP_MUL: printf("MULTIPLICATION (*)\n"); break;
                case OP_DIV: printf("DIVISION (/)\n"); break;
            }
            print_tree_structure(node->left, level + 1);
            print_tree_structure(node->right, level + 1);
            break;
    }
}

void free_tree(ExprNode* node) {
    if (node == NULL) return;
    free_tree(node->left);
    free_tree(node->right);
    free(node);
}

int main() {
    char buffer[256];
    
    printf("=========================================================\n");
    printf("Expression transformation: a - b*c  ->  a + (-(b*c))\n");
    printf("To exit, enter an empty line or 'quit'\n");
    printf("=========================================================\n\n");
    
    while (1) {
        printf("Enter expression: ");
        fgets(buffer, sizeof(buffer), stdin);
        
        // Remove newline character
        buffer[strcspn(buffer, "\n")] = '\0';
        
        if (strlen(buffer) == 0 || strcmp(buffer, "quit") == 0) {
            printf("Exit.\n");
            break;
        }
        
        input_str = buffer;
        pos = 0;
        
        // Parse expression
        ExprNode* root = parse_expression();
        
        printf("\n--- Original expression ---\n");
        printf("Text form: ");
        print_tree(root);
        printf("\n\nTree structure:\n");
        print_tree_structure(root, 0);
        
        // Transform
        ExprNode* transformed = transform_tree(root);
        
        printf("\n--- After transformation ---\n");
        printf("Text form: ");
        print_tree(transformed);
        printf("\n\nTree structure:\n");
        print_tree_structure(transformed, 0);
        printf("\n----------------------------------------\n\n");
        
        free_tree(transformed);
    }
    
    return 0;
}
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

typedef struct Node
{
    int value;
    struct Node* left;
    struct Node* right;
} Node;

void append(Node* root, int value) {
    Node* current_node = root;
    while (1) {
        if (value < current_node->value) {
            if (current_node->left == NULL) {
                Node* new_node = (Node*)malloc(sizeof(Node));
                new_node->value = value;
                new_node->right = NULL;
                new_node->left = NULL;

                current_node->left = new_node;
                break;

            } else {
                current_node = current_node->left;
            }  
        } else if (value > current_node->value) {

            if (current_node->right == NULL) {
                Node* new_node = (Node*)malloc(sizeof(Node));
                new_node->value = value;
                new_node->right = NULL;
                new_node->left = NULL;

                current_node->right = new_node;
                break;

            } else {
                current_node = current_node->right;
            }
        } else {
            break;
        }
    }
}

void print_tree(Node* root, int deep) {
    if (root == NULL) {
        printf("%*s[]\n", deep, "");
        return;
    } else {
        printf("%*d\n", deep, root->value);
        print_tree(root->left, deep + 1);
        print_tree(root->right, deep + 1);
    }
}

bool delete_node(Node* root, int value, bool delete) {
    if (root == NULL) {
        return delete;
    } else {
        if (root->value == value || delete == true) {
            delete_node(root->left, value, true);
            delete_node(root->right, value, true);
            root->left = NULL; 
            root->right = NULL;
            memset(root, 0, sizeof(int));
            free(root);
            return true;

        } else {
            bool delete_left = delete_node(root->left, value, false);
            bool delete_right = delete_node(root->right, value, false);
            if (delete_left == true) {
                root->left = NULL;
            }
            if (delete_right == true) {
                root->right = NULL;
            }
            return false;
        }
        
    }
}


int get_min_deep(Node* root) {
    if (root == NULL) {
        return 1;
    } else {
        int left_deep = get_min_deep(root->left);
        int right_deep = get_min_deep(root->right);
        if (left_deep <= right_deep) {
            printf("left %d deep %d\n" , root->value, left_deep);
            return left_deep + 1;
        } else {
            printf("right %d deep %d\n" , root->value, left_deep);
            return right_deep + 1;
        }
    }
}

    
int main() {
    int root_value;
    printf("Введите значение для корня дерева: ");
    scanf("%d", &root_value);
    
    Node* root = (Node*)malloc(sizeof(Node));
    root->value = root_value;
    root->left = NULL;
    root->right = NULL;
    
    int choice, value;
    
    while (1) {
        printf("\n--- Меню ---\n");
        printf("1 - добавить значение в дерево\n");
        printf("2 - удалить значение из дерева (кроме корневого)\n");
        printf("3 - распечатать дерево\n");
        printf("4 - определить глубину минимальной вершины\n");
        printf("0 - выход\n");
        printf("Ваш выбор: ");
        scanf("%d", &choice);
        
        switch (choice) {
            case 1:
                printf("Введите значение для добавления: ");
                scanf("%d", &value);
                append(root, value);
                printf("Значение %d добавлено в дерево.\n", value);
                break;
                
            case 2:
                printf("Введите значение для удаления (кроме корневого %d): ", root->value);
                scanf("%d", &value);
                if (value == root->value) {
                    printf("Ошибка: Нельзя удалить корневое значение %d!\n", root->value);
                } else {
                    delete_node(root, value, false);
                    printf("Значение %d удалено из дерева (если существовало).\n", value);
                }
                break;
                
            case 3:
                printf("\nДерево:\n");
                print_tree(root, 0);
                break;
                
            case 4:
                {
                    int min_depth = get_min_deep(root);
                    printf("Минимальная глубина дерева: %d\n", min_depth);
                }
                break;
                
            case 0:
                printf("До свидания!\n");
                // Освобождаем память перед выходом
                delete_node(root, root->value, true);
                return 0;
                
            default:
                printf("Неверный выбор! Пожалуйста, выберите 0-4.\n");
                break;
        }
    }
    
    return 0;
}

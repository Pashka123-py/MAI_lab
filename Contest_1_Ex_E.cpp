#include <iostream>
#include <vector>
using namespace std;
 
int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    
    int n;
    cin >> n;
    
    vector<long long> a(n);
    for (int i = 0; i < n; i++) {
        cin >> a[i];
    }
    
    vector<int> local_min, local_max;
    
    // Поиск локальных экстремумов
    for (int i = 0; i < n; i++) {
        bool is_min = true;
        bool is_max = true;
        
        if (i > 0) {
            if (a[i] > a[i-1]) is_min = false;
            if (a[i] < a[i-1]) is_max = false;
        }
        
        if (i < n - 1) {
            if (a[i] > a[i+1]) is_min = false;
            if (a[i] < a[i+1]) is_max = false;
        }
        
        if (is_min) local_min.push_back(i + 1);
        if (is_max) local_max.push_back(i + 1);
    }
    
    // Поиск глобального минимума и максимума
    int global_min_pos = 1, global_max_pos = 1;
    for (int i = 1; i < n; i++) {
        if (a[i] < a[global_min_pos - 1]) {
            global_min_pos = i + 1;
        }
        if (a[i] > a[global_max_pos - 1]) {
            global_max_pos = i + 1;
        }
    }
    
    cout << local_min.size();
    for (int pos : local_min) {
        cout << ' ' << pos;
    }
    cout << '\n';
    
    cout << local_max.size();
    for (int pos : local_max) {
        cout << ' ' << pos;
    }
    cout << '\n';
    
    cout << global_min_pos << ' ' << global_max_pos << '\n';
    
    return 0;
}

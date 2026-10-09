#include <bits/stdc++.h>
using namespace std;
 
int main() {
    int n;
    long long fi;
    cin >> n >> fi;
    
    vector<pair<int, int>> a(n);
    for (int i = 0; i < n; i++) {
        int x;
        cin >> x;
        a[i] = {x, i + 1};
    }
    
    sort(a.begin(), a.end());
    long long sum = fi;
    vector<int> order;
    
    for (int i = 0; i < n; i++) {
        if (a[i].first < sum) {
            sum += a[i].first;
            order.push_back(a[i].second);
        } else {
            cout << "NO" << endl;
            return 0;
        }
    }
    
    cout << "YES" << endl;
    for (int num : order) {  
        cout << num << " ";
    }
    cout << endl;
    
    return 0;
}
